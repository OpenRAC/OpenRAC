//! Discord Rich Presence integration via local IPC socket.
//!
//! Connects to Discord's local Unix domain socket (`discord-ipc-0` .. `9`) on Linux/macOS
//! and updates activity when idle in launcher or playing a game.

use std::io::{Read, Write};
use std::path::PathBuf;
use std::time::{SystemTime, UNIX_EPOCH};

use serde::{Deserialize, Serialize};

pub const DEFAULT_CLIENT_ID: &str = "1558096324602363955";

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
#[serde(rename_all = "camelCase")]
pub struct Activity {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub details: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub state: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub timestamps: Option<ActivityTimestamps>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub assets: Option<ActivityAssets>,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
#[serde(rename_all = "camelCase")]
pub struct ActivityTimestamps {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub start: Option<u64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub end: Option<u64>,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
#[serde(rename_all = "camelCase")]
pub struct ActivityAssets {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub large_image: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub large_text: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub small_image: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub small_text: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "kind", rename_all = "camelCase")]
pub enum DiscordStatus {
    Idle,
    ViewingGame {
        title: String,
        region: String,
        progress_pct: Option<f64>,
        game_id: String,
    },
    PlayingGame {
        title: String,
        region: String,
        game_id: String,
        start_time: Option<u64>,
    },
}

pub struct DiscordIpc {
    client_id: String,
    #[cfg(unix)]
    stream: Option<std::os::unix::net::UnixStream>,
    seq: u64,
}

impl DiscordIpc {
    pub fn new(client_id: impl Into<String>) -> Self {
        Self {
            client_id: client_id.into(),
            #[cfg(unix)]
            stream: None,
            seq: 0,
        }
    }

    pub fn set_client_id(&mut self, id: impl Into<String>) {
        let new_id = id.into();
        if self.client_id != new_id {
            self.client_id = new_id;
            self.disconnect();
        }
    }

    pub fn disconnect(&mut self) {
        #[cfg(unix)]
        {
            self.stream = None;
        }
    }

    #[cfg(unix)]
    fn connect(&mut self) -> Result<&mut std::os::unix::net::UnixStream, String> {
        if self.stream.is_some() {
            return Ok(self.stream.as_mut().unwrap());
        }

        let socket_paths = find_socket_paths();
        for path in socket_paths {
            if let Ok(mut stream) = std::os::unix::net::UnixStream::connect(&path) {
                let _ = stream.set_read_timeout(Some(std::time::Duration::from_millis(250)));
                let _ = stream.set_write_timeout(Some(std::time::Duration::from_millis(250)));

                // Handshake (Opcode 0): {"v": 1, "client_id": "..."}
                let payload = serde_json::json!({
                    "v": 1,
                    "client_id": self.client_id
                })
                .to_string();

                if send_packet(&mut stream, 0, &payload).is_ok() {
                    // Try reading handshake response packet
                    match read_packet(&mut stream) {
                        Ok((opcode, data)) => {
                            if opcode == 1 {
                                self.stream = Some(stream);
                                return Ok(self.stream.as_mut().unwrap());
                            } else if opcode == 2 {
                                if let Ok(err) = std::str::from_utf8(&data) {
                                    eprintln!("[discord-rpc] Handshake error: {err}");
                                }
                            }
                        }
                        Err(e) => {
                            eprintln!("[discord-rpc] read_packet error on {}: {e}", path.display());
                        }
                    }
                }
            }
        }

        Err("Discord IPC socket not available".to_string())
    }

    pub fn update_activity(&mut self, activity: Option<&Activity>) -> Result<(), String> {
        #[cfg(unix)]
        {
            self.seq += 1;
            let nonce = self.seq.to_string();
            let pid = std::process::id();

            let payload = serde_json::json!({
                "cmd": "SET_ACTIVITY",
                "args": {
                    "pid": pid,
                    "activity": activity
                },
                "nonce": nonce
            })
            .to_string();

            // Try sending to current or newly connected stream
            let mut failed = false;
            match self.connect() {
                Ok(stream) => {
                    if send_packet(stream, 1, &payload).is_err() {
                        failed = true;
                    } else {
                        // Drain response non-blockingly
                        let _ = read_packet(stream);
                    }
                }
                Err(e) => return Err(e),
            }

            if failed {
                self.disconnect();
                // Attempt one reconnect
                if let Ok(stream) = self.connect() {
                    let _ = send_packet(stream, 1, &payload);
                    let _ = read_packet(stream);
                }
            }
            Ok(())
        }
        #[cfg(not(unix))]
        {
            let _ = activity;
            Ok(())
        }
    }

    pub fn set_status(&mut self, status: &DiscordStatus, app_start: u64) -> Result<(), String> {
        let activity = match status {
            DiscordStatus::Idle => Activity {
                details: Some("In Launcher".into()),
                state: Some("Browsing Library".into()),
                timestamps: Some(ActivityTimestamps {
                    start: Some(app_start),
                    end: None,
                }),
                assets: Some(ActivityAssets {
                    large_image: Some("openrac".into()),
                    large_text: Some("OpenRAC Launcher".into()),
                    small_image: None,
                    small_text: None,
                }),
            },
            DiscordStatus::ViewingGame {
                title,
                region,
                progress_pct,
                game_id,
            } => {
                let state_str = match progress_pct {
                    Some(p) => format!("{region} · {p:.1}% matched"),
                    None => region.clone(),
                };
                Activity {
                    details: Some(format!("Viewing {title}")),
                    state: Some(state_str),
                    timestamps: Some(ActivityTimestamps {
                        start: Some(app_start),
                        end: None,
                    }),
                    assets: Some(ActivityAssets {
                        large_image: Some(game_id.clone()),
                        large_text: Some(title.clone()),
                        small_image: Some("openrac".into()),
                        small_text: Some("OpenRAC".into()),
                    }),
                }
            }
            DiscordStatus::PlayingGame {
                title,
                region,
                game_id,
                start_time,
            } => {
                let current_sec = now_sec();
                Activity {
                    details: Some(format!("Playing {title}")),
                    state: Some(format!("In Game ({region})")),
                    timestamps: Some(ActivityTimestamps {
                        start: Some(start_time.unwrap_or(current_sec)),
                        end: None,
                    }),
                    assets: Some(ActivityAssets {
                        large_image: Some(game_id.clone()),
                        large_text: Some(title.clone()),
                        small_image: Some("openrac".into()),
                        small_text: Some("OpenRAC".into()),
                    }),
                }
            }
        };

        self.update_activity(Some(&activity))
    }

    pub fn clear(&mut self) -> Result<(), String> {
        self.update_activity(None)
    }
}

pub fn now_sec() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs()
}

#[cfg(unix)]
fn find_socket_paths() -> Vec<PathBuf> {
    let mut candidates = Vec::new();

    let flatpak_subdirs = [
        ".flatpak/com.discordapp.Discord/xdg-run",
        "app/dev.vencord.Vesktop",
        "app/io.github.spacingbat3.webcord",
        "app/com.discordapp.DiscordCanary",
        "app/com.discordapp.Discord",
    ];

    // Check XDG_RUNTIME_DIR or /run/user/<uid>
    if let Ok(runtime_dir) = std::env::var("XDG_RUNTIME_DIR") {
        let base = PathBuf::from(&runtime_dir);
        for i in 0..10 {
            candidates.push(base.join(format!("discord-ipc-{i}")));
        }
        for sub in &flatpak_subdirs {
            for i in 0..10 {
                candidates.push(base.join(sub).join(format!("discord-ipc-{i}")));
            }
        }
    }

    if let Ok(uid) = std::env::var("UID") {
        for sub in &flatpak_subdirs {
            for i in 0..10 {
                candidates.push(PathBuf::from(format!("/run/user/{uid}/{sub}/discord-ipc-{i}")));
            }
        }
        for i in 0..10 {
            candidates.push(PathBuf::from(format!("/run/user/{uid}/discord-ipc-{i}")));
        }
    }

    // Fallback: /tmp
    for i in 0..10 {
        candidates.push(PathBuf::from(format!("/tmp/discord-ipc-{i}")));
    }

    candidates
}

#[cfg(unix)]
fn send_packet<W: Write>(writer: &mut W, opcode: u32, payload: &str) -> std::io::Result<()> {
    let bytes = payload.as_bytes();
    let len = bytes.len() as u32;

    let mut buf = Vec::with_capacity(8 + bytes.len());
    buf.extend_from_slice(&opcode.to_le_bytes());
    buf.extend_from_slice(&len.to_le_bytes());
    buf.extend_from_slice(bytes);

    writer.write_all(&buf)?;
    writer.flush()?;
    Ok(())
}

#[cfg(unix)]
fn read_packet<R: Read>(reader: &mut R) -> std::io::Result<(u32, Vec<u8>)> {
    let mut header = [0u8; 8];
    reader.read_exact(&mut header)?;
    let opcode = u32::from_le_bytes(header[0..4].try_into().unwrap());
    let len = u32::from_le_bytes(header[4..8].try_into().unwrap()) as usize;

    let mut buf = vec![0u8; len];
    reader.read_exact(&mut buf)?;
    Ok((opcode, buf))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn serializes_activity_cleanly() {
        let act = Activity {
            details: Some("Playing Ratchet & Clank".into()),
            state: Some("In Game (PAL)".into()),
            timestamps: Some(ActivityTimestamps {
                start: Some(1700000000),
                end: None,
            }),
            assets: Some(ActivityAssets {
                large_image: Some("rac1".into()),
                large_text: Some("Ratchet & Clank".into()),
                small_image: Some("openrac".into()),
                small_text: Some("OpenRAC".into()),
            }),
        };

        let json = serde_json::to_string(&act).unwrap();
        assert!(json.contains("Playing Ratchet & Clank"));
        assert!(json.contains("largeImage"));
    }

    #[test]
    fn handles_unconnected_ipc_gracefully() {
        let mut ipc = DiscordIpc::new(DEFAULT_CLIENT_ID);
        // Does not panic or crash when Discord is offline
        let _ = ipc.set_status(&DiscordStatus::Idle, now_sec());
        let _ = ipc.clear();
    }
}
