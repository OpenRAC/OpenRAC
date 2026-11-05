//! Prerequisites for an installed launcher's local game build. Checks never install anything.
use std::process::Command;

use crate::actions::Plan;

pub fn player_build(plan: &Plan) -> bool {
    plan.args.first().is_some_and(|arg| arg == "tools/build_game.py")
}

fn read_check(output: std::process::Output) -> Result<Option<String>, String> {
    if !output.status.success() {
        return Err(format!("Could not check the build tools: {}", String::from_utf8_lossy(&output.stderr).trim()));
    }
    let message: String =
        serde_json::from_slice(&output.stdout).map_err(|e| format!("Invalid build tools check: {e}"))?;
    Ok((!message.is_empty()).then_some(message))
}

pub fn check(plan: &Plan) -> Result<Option<String>, String> {
    if !cfg!(windows) || !player_build(plan) {
        return Ok(None);
    }
    let mut command = Command::new(&plan.program);
    command.args(["tools/build_tools.py", "--check"]).current_dir(&plan.cwd);
    #[cfg(windows)]
    {
        use std::os::windows::process::CommandExt;
        command.creation_flags(0x08000000);
    }
    read_check(command.output().map_err(|e| format!("Could not check the build tools: {e}"))?)
}

/// Only the installed player's build accepts the explicit installation consent.
pub fn allow_install(plan: &mut Plan) -> Result<(), String> {
    if !player_build(plan) {
        return Err("This action does not install build tools".into());
    }
    plan.args.push("--install-tools".into());
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn plan(script: &str) -> Plan {
        Plan {
            title: "Build".into(),
            program: "python".into(),
            args: vec![script.into(), "rac2-ntsc".into()],
            cwd: ".".into(),
            detached: false,
            env: vec![],
            output: None,
        }
    }

    #[test]
    fn consent_is_only_forwarded_to_the_player_build() {
        let mut build = plan("tools/build_game.py");
        allow_install(&mut build).unwrap();
        assert_eq!(build.args.last().unwrap(), "--install-tools");
        for script in ["tools/build_port.py", "tools/extractor.py", "other.py"] {
            let mut other = plan(script);
            assert!(allow_install(&mut other).is_err());
            assert_eq!(other.args.len(), 2);
            assert_eq!(check(&other).unwrap(), None);
        }
    }
}
