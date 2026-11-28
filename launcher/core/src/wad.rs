//! The game's WAD compression (LZO-like, in 0x2000-byte blocks), for the shared maps a player adds
//! ([`crate::maps`]): a map names the level files it changes by the player's own disc, and the
//! level's core and gameplay files are WAD streams, packed again here.
//!
//! [`decode`] reads a stream as `extractor/formats.py` `wad` does; [`compress`] writes one byte
//! for byte as OpenRAC Editor's `wadpack.py` does, so the file a map makes is the file its maker
//! made (the map holds its SHA-256).

/// A stream's header: "WAD", then the stream's size at +3.
pub const HEADER: usize = 16;
const BLOCK: usize = 0x2000;
const SKIP: [u8; 3] = [0x12, 0, 0]; // distance 0, count 2: skip to the next block
const ROOM: usize = 12; // unused bytes kept after a block's skip
const CHAIN: usize = 24; // candidates tried per position
const MAX_LITERALS: usize = 273; // one literal packet: 18 + 255

/// The bytes the WAD stream at the start of `data` holds, at most `limit` of them.
pub fn decode(data: &[u8], limit: usize) -> Result<Vec<u8>, String> {
    let bad = |what: &str| Err(format!("a WAD stream is damaged ({what})"));
    if data.len() < HEADER || &data[..3] != b"WAD" {
        return bad("no WAD header");
    }
    let end = u32::from_le_bytes([data[3], data[4], data[5], data[6]]) as usize;
    if end < HEADER || end > data.len() {
        return bad("its size");
    }
    let mut pos = HEADER;
    let mut out: Vec<u8> = Vec::new();
    let byte = |pos: &mut usize| -> Result<usize, String> {
        if *pos >= end {
            return Err("a WAD stream is damaged (a packet is cut short)".into());
        }
        *pos += 1;
        Ok(data[*pos - 1] as usize)
    };
    let literal = |pos: &mut usize, out: &mut Vec<u8>, count: usize| -> Result<(), String> {
        if *pos + count > end || out.len() + count > limit {
            return Err("a WAD stream is damaged (a literal run is too long)".into());
        }
        out.extend_from_slice(&data[*pos..*pos + count]);
        *pos += count;
        Ok(())
    };
    if pos < end && data[pos] > 0x11 {
        let n = byte(&mut pos)? - 0x11;
        literal(&mut pos, &mut out, n)?;
    }
    let mut needs_match = !out.is_empty();
    while pos < end {
        let tag = byte(&mut pos)?;
        let (distance, count);
        if tag < 0x10 {
            if needs_match {
                return bad("two literal runs");
            }
            let n = if tag != 0 { tag + 3 } else { byte(&mut pos)? + 18 };
            literal(&mut pos, &mut out, n)?;
            needs_match = true;
            continue;
        }
        if tag >= 0x40 {
            distance = 1 + ((tag >> 2) & 7) + byte(&mut pos)? * 8;
            count = (tag >> 5) + 1;
        } else if tag >= 0x20 {
            let c = if tag & 31 != 0 { tag & 31 } else { byte(&mut pos)? + 31 };
            count = c + 2;
            let (lo, hi) = (byte(&mut pos)?, byte(&mut pos)?);
            distance = 1 + (lo >> 2) + hi * 64;
        } else {
            let mut c = if tag & 7 != 0 { tag & 7 } else { byte(&mut pos)? + 7 };
            let (lo, hi) = (byte(&mut pos)?, byte(&mut pos)?);
            let mut d = (tag & 8) * 2048 + (lo >> 2) + hi * 64;
            if d != 0 {
                d += 0x4000;
                c += 2;
            } else if c == 1 {
                c = 0;
            } else {
                pos = HEADER + (pos - HEADER).div_ceil(0x2000) * 0x2000;
                if pos > end {
                    return bad("a block skip");
                }
                needs_match = false;
                continue;
            }
            distance = d;
            count = c;
        }
        if distance > out.len() || out.len() + count > limit {
            return bad("a match");
        }
        for _ in 0..count {
            let b = out[out.len() - distance];
            out.push(b);
        }
        let trailing = (data[pos - 2] & 3) as usize;
        literal(&mut pos, &mut out, trailing)?;
        needs_match = trailing != 0;
    }
    Ok(out)
}

/// (literal count, match length, match distance) triples covering `data`; the last has length 0.
/// wadpack keeps every earlier position of each 3-byte sequence and tries the last [`CHAIN`],
/// newest first: here that is a linked list per sequence (`head`, then `prev`), in the same order.
fn tokens(data: &[u8]) -> Vec<(usize, usize, usize)> {
    const NONE: u32 = u32::MAX;
    let n = data.len();
    let mut out = Vec::new();
    let (mut i, mut start) = (0usize, 0usize);
    let mut head = vec![NONE; 1 << 24];
    let mut prev = vec![NONE; n];
    let key = |p: usize| ((data[p] as usize) << 16) | ((data[p + 1] as usize) << 8) | data[p + 2] as usize;
    while i + 3 <= n {
        let (mut best, mut at) = (0usize, 0usize);
        let limit = (n - i).min(264);
        let mut j = head[key(i)];
        let mut tried = 0;
        while j != NONE && tried < CHAIN {
            let jj = j as usize;
            tried += 1;
            if i - jj > 0xBFFF {
                break;
            }
            // wadpack compares one byte at `best` first (a slice: past the end compares equal).
            if data.get(jj + best) == data.get(i + best) {
                let mut m = 3;
                while m < limit && data[jj + m] == data[i + m] {
                    m += 1;
                }
                if m > best {
                    best = m;
                    at = jj;
                    if m == limit {
                        break;
                    }
                }
            }
            j = prev[jj];
        }
        if best >= 3 && (best > 3 || i - at <= 0x800) {
            out.push((i - start, best, i - at));
            #[allow(clippy::needless_range_loop)] // p is both the key's place and its link
            for p in i..(i + best).min(n.saturating_sub(2)) {
                let k = key(p);
                prev[p] = head[k];
                head[k] = p as u32;
            }
            i += best;
            start = i;
        } else {
            let k = key(i);
            prev[i] = head[k];
            head[k] = i as u32;
            i += 1;
        }
    }
    out.push((n - start, 0, 0));
    out
}

/// The packets of one match (a long one is split); each ends with the byte whose low bits take
/// trailing literals.
fn match_packets(length: usize, distance: usize) -> Vec<Vec<u8>> {
    let mut packets: Vec<Vec<u8>> = Vec::new();
    let mut length = length;
    while length > 0 {
        let mut take = length;
        if distance <= 0x800 && take <= 8 {
            packets.push(vec![(((take - 1) << 5) | (((distance - 1) & 7) << 2)) as u8, ((distance - 1) >> 3) as u8]);
        } else if distance <= 0x4000 {
            take = take.min(31 + 255 + 2);
            let mut head =
                if take - 2 <= 31 { vec![(0x20 | (take - 2)) as u8] } else { vec![0x20, (take - 2 - 31) as u8] };
            let d = distance - 1;
            head.extend_from_slice(&[((d & 63) << 2) as u8, (d >> 6) as u8]);
            packets.push(head);
        } else {
            take = take.min(7 + 255 + 2);
            let d = distance - 0x4000;
            let tag = 0x10 | ((d >> 11) & 8);
            let mut head =
                if take - 2 <= 7 { vec![(tag | (take - 2)) as u8] } else { vec![tag as u8, (take - 2 - 7) as u8] };
            head.extend_from_slice(&[((d & 63) << 2) as u8, ((d >> 6) & 0xFF) as u8]);
            packets.push(head);
        }
        // never leave a remainder too short to be a match of its own
        let rest = length - take;
        if rest > 0 && rest < 3 {
            take -= 3 - rest;
            packets.pop();
            packets.extend(match_packets(take, distance));
        }
        length -= take;
    }
    packets
}

struct Writer {
    out: Vec<u8>,
    trail: Option<usize>, // the byte whose low two bits may still take literals
    after_literals: bool,
}

impl Writer {
    fn room(&self) -> usize {
        BLOCK - (self.out.len() - HEADER) % BLOCK
    }

    fn skip(&mut self) {
        self.out.extend_from_slice(&SKIP);
        let pad = (BLOCK - (self.out.len() - HEADER) % BLOCK) % BLOCK;
        self.out.resize(self.out.len() + pad, 0);
        self.trail = None;
        self.after_literals = false;
    }

    /// Starts a new block unless `size` more bytes leave room for the skip and its margin.
    fn fit(&mut self, size: usize) {
        if size + SKIP.len() + ROOM > self.room() {
            self.skip();
        }
    }

    fn literals(&mut self, mut data: &[u8]) {
        while !data.is_empty() {
            if data.len() <= 3 && self.trail.is_some() && !self.after_literals {
                self.fit(data.len());
                if let Some(trail) = self.trail {
                    self.out[trail] |= data.len() as u8;
                    self.out.extend_from_slice(data);
                    self.after_literals = true;
                    self.trail = None;
                    return;
                }
                continue;
            }
            if self.after_literals || data.len() <= 3 {
                // a match that copies nothing: it ends a literal run, and carries up to three literals
                let take = if data.len() <= 3 { data.len() } else { 0 };
                self.fit(3 + take);
                self.out.extend_from_slice(&[0x11, take as u8, 0]);
                self.out.extend_from_slice(&data[..take]);
                self.after_literals = take != 0;
                self.trail = None;
                data = &data[take..];
                continue;
            }
            let mut take = data.len().min(MAX_LITERALS);
            if take + 2 + SKIP.len() + ROOM > self.room() {
                let room = self.room();
                if room < 2 + SKIP.len() + ROOM + 4 {
                    self.skip();
                    continue;
                }
                take = room - 2 - SKIP.len() - ROOM;
            }
            let left = data.len() - take;
            if (1..=3).contains(&left) && take > 7 {
                take -= 4 - left; // leave a run long enough for a packet of its own
            }
            if take <= 18 {
                self.out.push((take - 3) as u8);
            } else {
                self.out.extend_from_slice(&[0, (take - 18) as u8]);
            }
            self.out.extend_from_slice(&data[..take]);
            self.after_literals = true;
            self.trail = None;
            data = &data[take..];
        }
    }

    fn matched(&mut self, length: usize, distance: usize) {
        for packet in match_packets(length, distance) {
            self.fit(packet.len());
            self.trail = Some(self.out.len() + packet.len() - 2);
            self.out.extend_from_slice(&packet);
            self.after_literals = false;
        }
    }
}

/// `data` as a WAD stream. `header` is the 16-byte header of the stream being replaced (its bytes
/// after the size are kept); without one, a header with only the magic and the size.
pub fn compress(data: &[u8], header: Option<&[u8]>) -> Result<Vec<u8>, String> {
    let head: Vec<u8> = match header {
        Some(h) => h[..h.len().min(HEADER)].to_vec(),
        None => {
            let mut h = b"WAD".to_vec();
            h.resize(HEADER, 0);
            h
        }
    };
    if head.len() != HEADER || &head[..3] != b"WAD" {
        return Err("expected a 16-byte WAD header".into());
    }
    let mut w = Writer { out: head, trail: None, after_literals: false };
    let mut pos = 0;
    for (literal, length, distance) in tokens(data) {
        if literal > 0 {
            w.literals(&data[pos..pos + literal]);
            pos += literal;
        }
        if length > 0 {
            w.matched(length, distance);
            pos += length;
        }
    }
    let size = (w.out.len() as u32).to_le_bytes();
    w.out[3..7].copy_from_slice(&size);
    Ok(w.out)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn sample() -> Vec<u8> {
        // Text-like runs, repeats near and far, and noise: every packet kind.
        let mut data = Vec::new();
        let mut x: u32 = 12345;
        for i in 0..200_000u32 {
            x = x.wrapping_mul(1_103_515_245).wrapping_add(12345);
            let b = match i % 7000 {
                0..=2999 => (i % 251) as u8,
                3000..=4999 => (x >> 24) as u8,
                _ => data[(i as usize) - 2900],
            };
            data.push(b);
        }
        data
    }

    #[test]
    fn a_stream_decodes_to_what_was_packed() {
        let data = sample();
        let packed = compress(&data, None).unwrap();
        assert!(packed.len() < data.len());
        assert_eq!(decode(&packed, 64 << 20).unwrap(), data);
    }

    #[test]
    fn the_stream_is_the_editors_byte_for_byte() {
        // wadpack.compress(sample()) in OpenRAC Editor (openrac/tools/wadpack.py): its SHA-256.
        use sha2::{Digest, Sha256};
        let packed = compress(&sample(), None).unwrap();
        let digest: String = Sha256::digest(&packed).iter().map(|b| format!("{b:02x}")).collect();
        assert_eq!(digest, WADPACK_SAMPLE_SHA256);
    }

    const WADPACK_SAMPLE_SHA256: &str = "2bf87d34d8dc71fbcf3e4ad77120cc81eda28c3783fac189a8ab49b68f257783";

    #[test]
    fn a_header_keeps_its_bytes_after_the_size() {
        let mut header = b"WAD".to_vec();
        header.resize(HEADER, 7);
        let packed = compress(b"hello hello hello hello", Some(&header)).unwrap();
        assert_eq!(&packed[7..16], &header[7..16]);
        assert_eq!(decode(&packed, 1024).unwrap(), b"hello hello hello hello");
    }

    #[test]
    fn a_damaged_stream_is_refused() {
        assert!(decode(b"WAX", 10).is_err());
        let mut packed = compress(&sample(), None).unwrap();
        packed[3] = 0xFF;
        assert!(decode(&packed, 64 << 20).is_err());
    }
}
