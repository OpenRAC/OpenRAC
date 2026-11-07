//! iOS handoff paths around the desktop save archive format. UIKit owns Files and sharing.
use super::{text, value, Runtime};
use openrac_launcher_core::{catalog, save_transfer as transfer, saves};
use serde_json::{json, Value};
use std::path::{Path, PathBuf};

fn serial(runtime: &Runtime, args: &Value) -> Result<String, String> {
    let requested = text(args, "serial")?;
    catalog::load(runtime.config.root()?)?
        .games
        .into_iter()
        .flat_map(|g| g.versions)
        .find(|v| v.serial.eq_ignore_ascii_case(requested))
        .map(|v| v.serial)
        .ok_or_else(|| "Choose a known game's save card".into())
}

fn handoff(runtime: &Runtime, area: &str, path: &Path) -> Result<PathBuf, String> {
    let root = runtime.home.join("save-transfers").join(area).canonicalize().map_err(|e| e.to_string())?;
    let path = path.canonicalize().map_err(|e| e.to_string())?;
    if path == root || !path.starts_with(root) {
        return Err("Choose the save file in Files first".into());
    }
    Ok(path)
}

pub(super) fn command(runtime: &Runtime, name: &str, args: &Value) -> Result<Value, String> {
    let serial = serial(runtime, args)?;
    let card = runtime.home.join("memcard").join(&serial);
    match name {
        "export_saves" => {
            let slot = if args["slot"].is_null() {
                None
            } else {
                Some(
                    args["slot"]
                        .as_u64()
                        .and_then(|n| usize::try_from(n).ok())
                        .filter(|n| *n < usize::MAX)
                        .ok_or("Invalid save slot")?,
                )
            };
            let dir = handoff(runtime, "exports", Path::new(text(args, "directory")?))?;
            let dest = dir.join(transfer::export_name(&serial, slot));
            let manifest = transfer::export_card(&card, &serial, slot, &runtime.bundle.version, &dest)?;
            value(transfer::ExportedSaves {
                saved: dest.to_string_lossy().into_owned(),
                slots: manifest.slots.into_iter().filter(|s| !s.empty).collect(),
            })
        }
        "preview_save_import" | "import_saves" => {
            let source = handoff(runtime, "imports", Path::new(text(args, "source")?))?;
            if name == "preview_save_import" {
                value(transfer::preview_into(&card, &serial, &source)?)
            } else {
                let choice = serde_json::from_value(args["choice"].clone()).map_err(|e| e.to_string())?;
                value(transfer::import_into(&card, &serial, &source, &choice)?)
            }
        }
        "restore_backup" => {
            saves::restore_backup(&serial, text(args, "backupName")?)?;
            Ok(json!(null))
        }
        _ => Err("Unknown save transfer command".into()),
    }
}
