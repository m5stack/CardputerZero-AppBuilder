//! `czdev run` — build the emulator if needed, then load the app into it.

use crate::build::build_app;
use crate::paths;
use anyhow::{anyhow, Context, Result};
use std::path::Path;
use std::process::Command;

pub fn run_app(app_dir: &Path) -> Result<()> {
    let repo_root = paths::repo_root(app_dir)?;
    let emulator_dir = paths::emulator_dir(&repo_root);
    let emulator_build = paths::emulator_build(&repo_root);
    let emulator_bin = paths::emulator_binary(&repo_root);

    if !emulator_dir.join("CMakeLists.txt").is_file() {
        return Err(anyhow!(
            "emulator submodule not checked out at {}. Run:\n  \
             git submodule update --init --recursive",
            emulator_dir.display()
        ));
    }

    if !emulator_bin.is_file() {
        println!("=> emulator not built yet; configuring + building now");
        std::fs::create_dir_all(&emulator_build)?;
        let status = Command::new("cmake")
            .args([
                "-S", emulator_dir.to_str().unwrap(),
                "-B", emulator_build.to_str().unwrap(),
                "-G", "Unix Makefiles",
                "-DCMAKE_BUILD_TYPE=Release",
            ])
            .status()
            .context("running cmake configure (emulator)")?;
        if !status.success() {
            return Err(anyhow!("emulator cmake configure failed"));
        }
        let status = Command::new("cmake")
            .args(["--build", emulator_build.to_str().unwrap(), "-j"])
            .status()
            .context("running cmake build (emulator)")?;
        if !status.success() {
            return Err(anyhow!("emulator cmake build failed"));
        }
    }

    let out = build_app(app_dir)?;
    let staged = emulator_build.join("apps").join(
        out.lib_path.file_name().unwrap(),
    );
    std::fs::create_dir_all(staged.parent().unwrap())?;
    std::fs::copy(&out.lib_path, &staged)
        .with_context(|| format!("staging {} → {}", out.lib_path.display(), staged.display()))?;

    println!("=> launching emulator: {} {}", emulator_bin.display(), staged.display());
    let status = Command::new(&emulator_bin)
        .current_dir(&emulator_build)
        .arg(staged.strip_prefix(&emulator_build).unwrap_or(&staged))
        .status()
        .context("launching emulator")?;
    if !status.success() {
        return Err(anyhow!("emulator exited with {status}"));
    }
    Ok(())
}
