// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/iso9660.rs
// (tests) and crates/rc-formats/src/sha1.rs (tests): ISC License, Copyright (c) 2026 ReRAC
// contributors.
//
// The ISO 9660 reader on synthetic 2048- and 2352-byte images, SYSTEM.CNF,
// ELF segment reads, and the hashes on their standard vectors.

#include "assets/disc/iso9660.h"

#include "assets/disc/boot.h"
#include "assets/disc/checksum.h"
#include "tests/check.h"
#include "tests/disc/synthetic_disc.h"

using namespace openrac::assets;
using namespace openrac::assets::disc;
using namespace openrac::test;

namespace {

std::vector<u8> elf_bytes() {
    std::vector<u8> elf(3000);
    elf[0] = 0x7f;
    elf[1] = 'E';
    elf[2] = 'L';
    elf[3] = 'F';
    for (std::size_t i = 4; i < elf.size(); ++i) {
        elf[i] = static_cast<u8>(i * 7);
    }
    return elf;
}

std::vector<u8> mini_iso() {
    return make_iso(
        32,
        {{"DATA/a.bin", bytes_of("hello")}, {"SCUS_971.99", elf_bytes()}, {"SYSTEM.CNF", system_cnf("SCUS_971.99")}}
    );
}

void check_image(const IsoImage& iso) {
    const std::vector<u8> elf = elf_bytes();
    CHECK(iso.volume_id() == "RATCHETANDCLANK");
    CHECK(iso.volume_sectors() == 32);
    std::vector<std::string> paths;
    for (const IsoEntry& e : iso.entries()) {
        paths.push_back(e.path);
    }
    CHECK((paths == std::vector<std::string>{"/DATA", "/DATA/A.BIN", "/SCUS_971.99", "/SYSTEM.CNF"}));
    const IsoEntry* boot = iso.find("/scus_971.99;1");
    CHECK(boot && iso.read_file(*boot) == elf);
    const IsoEntry* a = iso.find("data/a.bin");
    CHECK(a && iso.read_file(*a) == bytes_of("hello"));
    CHECK(iso.find("/DATA") && iso.find("/DATA")->is_directory);
    CHECK(iso.find("/NOPE") == nullptr);
    // An unaligned read across a sector boundary.
    if (boot) {
        const auto part = iso.read_bytes(u64{boot->lba} * 2048 + 2040, 20);
        CHECK(std::equal(part.begin(), part.end(), elf.begin() + 2040));
    }
    bool threw = false;
    try {
        iso.read_sectors(iso.sector_count(), 1);
    } catch (const AssetError&) {
        threw = true;
    }
    CHECK(threw);
}

void iso_images() {
    const std::vector<u8> img = mini_iso();
    const IsoImage plain(image_in_memory(img));
    CHECK(plain.raw_sector_size() == 2048);
    CHECK(plain.sector_count() == 32);
    check_image(plain);

    const IsoImage raw(image_in_memory(to_raw(img)));
    CHECK(raw.raw_sector_size() == 2352);
    CHECK(raw.sector_count() == 32);
    check_image(raw);

    for (const std::size_t size : {std::size_t{32 * 2048}, std::size_t{100}}) {
        bool threw = false;
        try {
            IsoImage bad(image_in_memory(std::vector<u8>(size, 0)));
        } catch (const AssetError&) {
            threw = true;
        }
        CHECK(threw);
    }
    bool io = false;
    try {
        IsoImage::open("/nonexistent/disc.iso");
    } catch (const IoError&) {
        io = true;
    }
    CHECK(io);
}

void system_cnf_lines() {
    const std::string cnf = "BOOT2 = cdrom0:\\SCUS_971.99;1\r\nVER = 1.00\r\nVMODE = NTSC\r\n\r\n";
    CHECK(cnf_value(cnf, "BOOT2") == "cdrom0:\\SCUS_971.99");
    CHECK(cnf_value(cnf, "VER") == "1.00");
    CHECK(cnf_value(cnf, "VMODE") == "NTSC");
    CHECK(!cnf_value(cnf, "NOPE"));
    const auto parsed = SystemCnf::parse(cnf);
    CHECK(parsed && parsed->serial == "SCUS_971.99" && parsed->boot == "/SCUS_971.99");
    CHECK(!SystemCnf::parse("VER = 1.00\n"));
}

void elf_segments() {
    // An ELF32 with one PT_LOAD of 0x40 file bytes at 0x100000, file offset 0x80.
    ByteWriter w;
    w.resize(0xc0);
    w.put_at<u32>(0, 0x464c457f);
    w.put_at<u32>(0x1c, 0x34);
    w.put_at<u16>(0x2a, 0x20);
    w.put_at<u16>(0x2c, 1);
    w.put_at<u32>(0x34, 1);
    w.put_at<u32>(0x38, 0x80);
    w.put_at<u32>(0x3c, 0x100000);
    w.put_at<u32>(0x44, 0x40);
    w.put_at<u32>(0x90, 0xdeadbeef);
    CHECK(elf_read(w.bytes(), 0x100010, 4).u32_at(0) == 0xdeadbeef);
    bool threw = false;
    try {
        elf_read(w.bytes(), 0x100030, 0x20);
    } catch (const AssetError&) {
        threw = true;
    }
    CHECK(threw);
}

std::string sha1_hex(std::string_view s) {
    return to_hex(sha1({reinterpret_cast<const u8*>(s.data()), s.size()}));
}

void hashes() {
    // FIPS 180-2 appendix A and the NIST short-message vectors.
    CHECK(sha1_hex("") == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    CHECK(sha1_hex("abc") == "a9993e364706816aba3e25717850c26c9cd0d89d");
    CHECK(sha1_hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") == "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    CHECK(sha1_hex("The quick brown fox jumps over the lazy dog") == "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
    const std::vector<u8> million(1000000, 'a');
    CHECK(to_hex(sha1(million)) == "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
    CHECK(to_hex(sha256({})) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(
        to_hex(sha256(bytes_of("abc"))) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
    );
    CHECK(
        to_hex(sha256(bytes_of("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")))
        == "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"
    );
    // Incremental updates agree with one-shot hashing across the padding boundaries.
    std::vector<u8> data(1000);
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<u8>(i * 31 + 7);
    }
    for (const std::size_t len : {0, 1, 55, 56, 57, 63, 64, 65, 119, 120, 127, 128, 129, 1000}) {
        const auto whole = sha1(std::span<const u8>(data).first(len));
        for (const std::size_t split : {1, 3, 17, 63, 64, 65, 200}) {
            Sha1 s;
            for (std::size_t at = 0; at < len; at += split) {
                s.update(std::span<const u8>(data).subspan(at, std::min(split, len - at)));
            }
            CHECK(s.finish() == whole);
        }
    }
    CHECK(parse_sha1("A9993E364706816ABA3E25717850C26C9CD0D89D") == sha1(bytes_of("abc")));
    CHECK(!parse_sha1("xyz"));
}

}  // namespace

int main() {
    iso_images();
    system_cnf_lines();
    elf_segments();
    hashes();
    return openrac::test::result();
}
