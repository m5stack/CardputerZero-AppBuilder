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

    if manifest.runtime == "prebuilt-emulator-app" {
        // The app is built by the emulator's own CMake (e.g. APPLaunch via
        // the UserDemo submodule). Ensure the emulator is built, then just
        // locate the staged library under emulator/build/apps/.
        let repo_root = paths::repo_root(app_dir)?;
        let emulator_build = paths::emulator_build(&repo_root);
        let lib_name = paths::app_lib_name(&manifest.bin_name);
        let lib_path = emulator_build.join("apps").join(&lib_name);
        if !lib_path.is_file() {
            return Err(anyhow!(
                "{}: prebuilt library {lib_name} not found at {}\n\
                 Run `czdev run` once on any lvgl-dlopen example to build the emulator,\n\
                 or rebuild it manually: cmake --build {}",
                manifest.display_name(),
                lib_path.display(),
                emulator_build.display(),
            ));
        }
        println!("   → {} (prebuilt by emulator)", lib_path.display());
        return Ok(BuildOutput { lib_path });
    }

    let build_dir = manifest.dir.join(".czdev/build");
    std::fs::create_dir_all(&build_dir)
        .with_context(|| format!("creating {}", build_dir.display()))?;

    println!("=> cmake configure [{}]", manifest.display_name());
    let mut cfg = Command::new("cmake");
    cfg.args([
        "-S", manifest.dir.to_str().unwrap(),
        "-B", build_dir.to_str().unwrap(),
        "-G", "Unix Makefiles",
        "-DCMAKE_BUILD_TYPE=Release",
    ]);
    // Expand manifest cmake_args, substituting ${CZ_SDK_DIR} / ${CZ_LVGL_DIR}
    // so manifests don't hardcode absolute paths.
    let repo_root = paths::repo_root(app_dir)?;
    let sdk = repo_root.join("sdk");
    let lvgl = repo_root.join("emulator/lib/lvgl");
    for a in &manifest.cmake_args {
        let a = a
            .replace("${CZ_SDK_DIR}", sdk.to_str().unwrap())
            .replace("${CZ_LVGL_DIR}", lvgl.to_str().unwrap());
        cfg.arg(a);
    }
    let status = cfg.status().context("running cmake configure")?;
    if !status.success() {
        return Err(anyhow!("cmake configure failed for {}", manifest.display_name()));
    }

    println!("=> cmake build [{}]", manifest.display_name());
    let target = manifest.cmake_target.clone().unwrap_or_else(|| manifest.bin_name.clone());
    let status = Command::new("cmake")
        .args(["--build", build_dir.to_str().unwrap(), "-j",
               "--target", &target])
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
