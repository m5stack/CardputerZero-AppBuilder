use anyhow::Result;
use std::process::{Command, Stdio};

pub fn run() -> Result<()> {
    let checks = build_checks();
    print_table(&checks);

    let missing_required = checks
        .iter()
        .filter(|c| c.required && matches!(c.status, Status::Missing))
        .count();

    if missing_required > 0 {
        println!();
        println!(
            "{} required dependency(ies) missing. See hints above, then re-run `czdev doctor`.",
            missing_required
        );
        std::process::exit(1);
    }

    println!();
    println!("All required dependencies present.");
    Ok(())
}

#[derive(Clone)]
struct Check {
    name: &'static str,
    required: bool,
    status: Status,
    hint: String,
}

#[derive(Clone)]
enum Status {
    Ok(String),    // stores version string
    Missing,
    Warn(String),  // present but with a caveat
}

fn build_checks() -> Vec<Check> {
    let mut out = Vec::new();

    // C/C++ compiler
    let cc = if cfg!(target_os = "windows") { "gcc" } else { "cc" };
    out.push(check_command(
        "C compiler",
        cc,
        &["--version"],
        true,
        install_hint_compiler(),
    ));

    // cmake
    out.push(check_command(
        "cmake",
        "cmake",
        &["--version"],
        true,
        install_hint("cmake", "cmake", "mingw-w64-x86_64-cmake"),
    ));

    // pkg-config (unix; on windows only a warning if missing)
    out.push(check_command(
        "pkg-config",
        "pkg-config",
        &["--version"],
        !cfg!(target_os = "windows"),
        install_hint("pkg-config", "pkg-config", "mingw-w64-x86_64-pkgconf"),
    ));

    // SDL2 via pkg-config; fallback to plain probe on platforms without it.
    out.push(check_pkg(
        "SDL2",
        "sdl2",
        true,
        install_hint("sdl2", "libsdl2-dev", "mingw-w64-x86_64-SDL2"),
    ));
    out.push(check_pkg(
        "SDL2_image",
        "SDL2_image",
        true,
        install_hint("sdl2_image", "libsdl2-image-dev", "mingw-w64-x86_64-SDL2_image"),
    ));
    out.push(check_pkg(
        "SDL2_mixer",
        "SDL2_mixer",
        false,
        install_hint("sdl2_mixer", "libsdl2-mixer-dev", "mingw-w64-x86_64-SDL2_mixer"),
    ));

    // Freetype — required on mac/Linux, skipped on Windows (see DESKTOP_DEV.md §4).
    out.push(check_pkg(
        "freetype",
        "freetype2",
        !cfg!(target_os = "windows"),
        install_hint("freetype", "libfreetype-dev", "(Windows: disabled by design)"),
    ));

    // Git (needed for submodule + CI parity).
    out.push(check_command(
        "git",
        "git",
        &["--version"],
        true,
        install_hint("git", "git", "mingw-w64-x86_64-git"),
    ));

    out
}

fn check_command(name: &'static str, prog: &str, args: &[&str], required: bool, hint: String) -> Check {
    match run_capture(prog, args) {
        Some(out) => Check {
            name,
            required,
            status: Status::Ok(first_line(&out)),
            hint,
        },
        None => Check {
            name,
            required,
            status: Status::Missing,
            hint,
        },
    }
}

fn check_pkg(name: &'static str, mod_name: &str, required: bool, hint: String) -> Check {
    // Prefer pkg-config; if pkg-config itself is missing, fall back to a soft warn
    // so we don't double-report (pkg-config row already flags it).
    if run_capture("pkg-config", &["--version"]).is_none() {
        return Check {
            name,
            required,
            status: Status::Warn("pkg-config unavailable; cannot probe".into()),
            hint,
        };
    }
    match run_capture("pkg-config", &["--modversion", mod_name]) {
        Some(v) => Check {
            name,
            required,
            status: Status::Ok(first_line(&v)),
            hint,
        },
        None => Check {
            name,
            required,
            status: Status::Missing,
            hint,
        },
    }
}

fn run_capture(prog: &str, args: &[&str]) -> Option<String> {
    let out = Command::new(prog)
        .args(args)
        .stdin(Stdio::null())
        .stderr(Stdio::null())
        .output()
        .ok()?;
    if !out.status.success() {
        return None;
    }
    Some(String::from_utf8_lossy(&out.stdout).trim().to_string())
}

fn first_line(s: &str) -> String {
    s.lines().next().unwrap_or(s).trim().to_string()
}

fn install_hint(brew: &str, apt: &str, msys2: &str) -> String {
    if cfg!(target_os = "macos") {
        format!("brew install {}", brew)
    } else if cfg!(target_os = "linux") {
        format!("sudo apt install -y {}", apt)
    } else if cfg!(target_os = "windows") {
        format!("pacman -S {} (MSYS2 MINGW64)", msys2)
    } else {
        String::from("(unsupported platform)")
    }
}

fn install_hint_compiler() -> String {
    if cfg!(target_os = "macos") {
        "xcode-select --install".into()
    } else if cfg!(target_os = "linux") {
        "sudo apt install -y build-essential".into()
    } else if cfg!(target_os = "windows") {
        "pacman -S mingw-w64-x86_64-gcc (MSYS2 MINGW64)".into()
    } else {
        "(install a C/C++ toolchain)".into()
    }
}

fn print_table(checks: &[Check]) {
    // Column widths
    let w_name = checks.iter().map(|c| c.name.len()).max().unwrap_or(8).max(10);
    let w_status = 9; // OK / MISSING / WARN
    let w_detail = checks
        .iter()
        .map(|c| detail_for(c).len())
        .max()
        .unwrap_or(20)
        .max(20);

    print_row("Check", "Status", "Detail", w_name, w_status, w_detail);
    print_sep(w_name, w_status, w_detail);
    for c in checks {
        let (status, detail) = match &c.status {
            Status::Ok(v) => ("OK", v.as_str()),
            Status::Missing => ("MISSING", ""),
            Status::Warn(m) => ("WARN", m.as_str()),
        };
        let detail_owned = if matches!(c.status, Status::Missing) {
            c.hint.clone()
        } else {
            detail.to_string()
        };
        print_row(
            c.name,
            status,
            &detail_owned,
            w_name,
            w_status,
            w_detail,
        );
    }
}

fn detail_for(c: &Check) -> String {
    match &c.status {
        Status::Ok(v) => v.clone(),
        Status::Missing => c.hint.clone(),
        Status::Warn(m) => m.clone(),
    }
}

fn print_row(a: &str, b: &str, c: &str, wa: usize, wb: usize, wc: usize) {
    println!(
        "  {:<wa$}  {:<wb$}  {:<wc$}",
        a,
        b,
        c,
        wa = wa,
        wb = wb,
        wc = wc,
    );
}

fn print_sep(wa: usize, wb: usize, wc: usize) {
    println!(
        "  {:-<wa$}  {:-<wb$}  {:-<wc$}",
        "", "", "", wa = wa, wb = wb, wc = wc,
    );
}
