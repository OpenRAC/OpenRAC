// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//! Isolated mod stacks for the lighting baker and readiness checks.

use std::path::{Path, PathBuf};

pub struct BakeMods(tempfile::TempDir);

impl BakeMods {
    pub fn path(&self) -> PathBuf {
        self.0.path().join("mods")
    }
}

/// Assemble the same enabled packs and packed levels as Play, into an isolated snapshot.
pub fn assemble_bake_mods(
    config: &crate::config::Config,
    key: &str,
    prepare_inputs: bool,
) -> Result<Option<BakeMods>, String> {
    use crate::actions::{Inputs, Scope};

    if !config.play_with_mods {
        return Ok(None);
    }
    let root = config.root()?;
    let scope = Scope::Version(key.to_string());
    let actions = crate::actions::load(root)?;
    // Some versions can assemble editor packs but Play still launches their disc levels.
    if actions.find(&scope, "play-edited").is_none() || actions.find(&scope, "overrides-stack").is_none() {
        return Ok(None);
    }
    let (args, _) = crate::library::filled(config, key, "overrides-stack")?;
    if !args.windows(2).any(|pair| matches!(pair[0].as_str(), "--into" | "--packed") && root.join(&pair[1]).is_dir()) {
        return Ok(None);
    }
    let mut plan = crate::library::plan(config, &scope, "overrides-stack", &Inputs::default())?;
    let out = plan
        .args
        .iter()
        .position(|arg| arg == "--out")
        .and_then(|i| plan.args.get_mut(i + 1))
        .ok_or("The mod assembly action has no output folder")?;
    // Keep the snapshot on the data volume so the stack can link large assets.
    let out_path = plan.cwd.join(out.as_str());
    let data = out_path.parent().and_then(Path::parent).ok_or("Invalid mod assembly folder")?;
    let snapshot = tempfile::Builder::new()
        .prefix(".lighting-mods-")
        .tempdir_in(data)
        .map_err(|e| format!("Cannot stage mods for lighting: {e}"))?;
    let snapshot = BakeMods(snapshot);
    *out = snapshot.path().to_string_lossy().into_owned();
    if prepare_inputs {
        let game = key.split('/').next().unwrap_or(key);
        plan.args.extend([
            "--lighting-disc".into(),
            data.join("iso_data").join(game).join("disc.iso").to_string_lossy().into_owned(),
        ]);
    }
    let mut command = crate::detect::quiet(&plan.program);
    command.args(&plan.args).current_dir(&plan.cwd);
    crate::jobs::environment(&mut command, Some(root));
    command.envs(plan.env);
    crate::detect::run(command, std::time::Duration::from_secs(120))
        .map_err(|e| format!("Cannot prepare mods for lighting: {e}"))?;
    Ok(Some(snapshot))
}
