//! `czdev build` — run CMake to produce the app's shared library.

use crate::manifest::Manifest;
use crate::paths;
use anyhow::{anyhow, Context, Result};
use std::path::{Path, PathBuf};
use std::process::Command;

pub struct BuildOutput {
    pub lib_path: PathBuf,
}

pub fn build_app(app_dir: &Path) -> Result<BuildOutput> {
    let manifest = Manifest::load(app_dir)?;

    if manifest.runtime == "legacy-deb-only" {
        return Err(anyhow!(
            "{}: runtime=legacy-deb-only is not runnable under czdev; \
             use the .deb pipeline instead",
            manifest.display_name()
        ));
    }

    let build_dir = manifest.dir.join(".czdev/build");
    std::fs::create_dir_all(&build_dir)
        .with_context(|| format!("creating {}", build_dir.display()))?;

    println!("=> cmake configure [{}]", manifest.display_name());
    let status = Command::new("cmake")
        .args([
            "-S", manifest.dir.to_str().unwrap(),
            "-B", build_dir.to_str().unwrap(),
            "-G", "Unix Makefiles",
            "-DCMAKE_BUILD_TYPE=Release",
        ])
        .status()
        .context("running cmake configure")?;
    if !status.success() {
        return Err(anyhow!("cmake configure failed for {}", manifest.display_name()));
    }

    println!("=> cmake build [{}]", manifest.display_name());
    let status = Command::new("cmake")
        .args(["--build", build_dir.to_str().unwrap(), "-j"])
        .status()
        .context("running cmake build")?;
    if !status.success() {
        return Err(anyhow!("cmake build failed for {}", manifest.display_name()));
    }

    let lib_name = paths::app_lib_name(&manifest.bin_name);
    let candidates = [
        build_dir.join(&lib_name),
        build_dir.join("Release").join(&lib_name),
    ];
    let lib_path = candidates
        .into_iter()
        .find(|p| p.is_file())
        .ok_or_else(|| anyhow!("built library {lib_name} not found under {}", build_dir.display()))?;

    println!("   → {}", lib_path.display());
    Ok(BuildOutput { lib_path })
}
