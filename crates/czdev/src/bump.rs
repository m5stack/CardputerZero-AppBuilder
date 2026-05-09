use anyhow::{anyhow, Context, Result};
use std::path::{Path, PathBuf};
use std::process::Command;

const PACKAGES_INDEX_URL: &str =
    "https://cardputerzero.github.io/packages/dists/stable/main/binary-arm64/Packages";

pub fn run(deb: Option<&Path>) -> Result<()> {
    let deb_path = resolve_deb(deb)?;

    let package = dpkg_field(&deb_path, "Package")?;
    let current_ver = dpkg_field(&deb_path, "Version")?;

    println!("Package: {package}");
    println!("Current version in deb: {current_ver}");

    // Fetch latest published version
    let latest = fetch_latest_version(&package)?;
    let next = match &latest {
        Some(v) => {
            println!("Latest published version: {v}");
            bump_patch(v)
        }
        None => {
            println!("No published version found (new package)");
            bump_patch(&current_ver)
        }
    };

    println!();
    println!("Next version: {next}");
    println!();
    println!("To rebuild with this version, update your package control file:");
    println!("  Version: {next}");
    println!();
    println!("Or if using CMake/packaging scripts, set:");
    println!("  PKG_VERSION={next}");

    Ok(())
}

fn resolve_deb(deb: Option<&Path>) -> Result<PathBuf> {
    if let Some(p) = deb {
        if !p.is_file() {
            return Err(anyhow!("file not found: {}", p.display()));
        }
        return Ok(p.to_path_buf());
    }
    let build_dir = Path::new("build");
    if build_dir.is_dir() {
        let mut debs: Vec<_> = std::fs::read_dir(build_dir)?
            .filter_map(|e| e.ok())
            .filter(|e| e.path().extension().map(|x| x == "deb").unwrap_or(false))
            .collect();
        if debs.len() == 1 {
            return Ok(debs.remove(0).path());
        }
        if debs.len() > 1 {
            return Err(anyhow!(
                "multiple .deb files in build/. Specify one with --deb <path>"
            ));
        }
    }
    Err(anyhow!("no .deb file found. Specify with --deb <path>"))
}

fn dpkg_field(deb: &Path, field: &str) -> Result<String> {
    let output = Command::new("dpkg-deb")
        .args(["-f", &deb.to_string_lossy(), field])
        .output()
        .with_context(|| format!("dpkg-deb -f {field}"))?;
    Ok(String::from_utf8_lossy(&output.stdout).trim().to_string())
}

fn fetch_latest_version(package: &str) -> Result<Option<String>> {
    let resp = reqwest::blocking::get(PACKAGES_INDEX_URL);
    let content = match resp {
        Ok(r) if r.status().is_success() => r.text().unwrap_or_default(),
        _ => return Ok(None),
    };

    let mut in_package = false;
    let mut latest: Option<String> = None;

    for line in content.lines() {
        if line.starts_with("Package: ") {
            in_package = line.trim_start_matches("Package: ") == package;
        }
        if in_package && line.starts_with("Version: ") {
            let ver = line.trim_start_matches("Version: ").to_string();
            match &latest {
                Some(existing) if compare_versions(&ver, existing) == std::cmp::Ordering::Greater => {
                    latest = Some(ver);
                }
                None => latest = Some(ver),
                _ => {}
            }
        }
        if line.is_empty() {
            in_package = false;
        }
    }
    Ok(latest)
}

fn bump_patch(version: &str) -> String {
    // Handle versions like "1.0.0", "1.0", "0.2.0-1~lofibox23"
    // Strip debian revision (everything after first '-')
    let base = version.split('-').next().unwrap_or(version);
    let parts: Vec<&str> = base.split('.').collect();

    match parts.len() {
        1 => format!("{}.0.1", parts[0]),
        2 => format!("{}.{}.1", parts[0], parts[1]),
        _ => {
            let patch: u64 = parts[2].parse().unwrap_or(0);
            format!("{}.{}.{}", parts[0], parts[1], patch + 1)
        }
    }
}

fn compare_versions(a: &str, b: &str) -> std::cmp::Ordering {
    let parse = |v: &str| -> Vec<u64> {
        v.split(|c: char| c == '.' || c == '-' || c == '~')
            .filter_map(|s| s.parse::<u64>().ok())
            .collect()
    };
    parse(a).cmp(&parse(b))
}
