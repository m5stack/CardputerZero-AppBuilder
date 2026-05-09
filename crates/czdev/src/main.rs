mod auth;
mod build;
mod bump;
mod deploy;
mod doctor;
mod github;
mod list;
mod manifest;
mod paths;
mod publish;
mod run;
mod unpublish;
mod watch;

use anyhow::Result;
use clap::{Parser, Subcommand};
use std::path::PathBuf;

/// CardputerZero desktop dev CLI.
///
/// Build, run and deploy LVGL apps for M5 CardputerZero from macOS / Linux /
/// Windows without the physical device. See docs/DESKTOP_DEV.md.
#[derive(Parser, Debug)]
#[command(name = "czdev", version, about, long_about = None)]
struct Cli {
    #[command(subcommand)]
    command: Command,
}

#[derive(Subcommand, Debug)]
enum Command {
    /// Check desktop build dependencies (SDL2, cmake, toolchain).
    Doctor,

    /// List LVGL apps discovered under a directory.
    List {
        #[arg(default_value = ".")]
        path: PathBuf,
    },

    /// Build an app's shared library for the host platform.
    Build {
        #[arg(default_value = ".")]
        path: PathBuf,
    },

    /// Build the app, launch the emulator, and load it.
    Run {
        #[arg(default_value = ".")]
        path: PathBuf,
    },

    /// Rebuild + restart the emulator whenever app sources change.
    Watch {
        #[arg(default_value = ".")]
        path: PathBuf,
    },

    /// Deploy a pre-built .deb to the device over SSH.
    Deploy {
        #[arg(default_value = ".")]
        path: PathBuf,
        /// Target host, e.g. pi@192.168.50.150
        #[arg(long)]
        host: Option<String>,
        /// Path to the .deb file (built by CI or locally).
        #[arg(long)]
        deb: Option<PathBuf>,
    },

    /// Authenticate with GitHub (device flow).
    Login,

    /// Remove stored GitHub credentials.
    Logout,

    /// Show next version (patch bump) for a package based on published versions.
    Bump {
        /// Path to the .deb file. If omitted, searches ./build/*.deb
        #[arg(long)]
        deb: Option<PathBuf>,
    },

    /// Publish a .deb package to the CardputerZero app store.
    Publish {
        /// Path to the .deb file. If omitted, searches ./build/*.deb
        #[arg(long)]
        deb: Option<PathBuf>,
    },

    /// Create a PR to remove a published package (you can only remove your own).
    Unpublish {
        /// Package name to remove
        package: String,
        /// Version to remove
        #[arg(long)]
        version: String,
        /// Architecture (default: arm64)
        #[arg(long, default_value = "arm64")]
        arch: String,
    },
}

fn main() -> Result<()> {
    let cli = Cli::parse();
    match cli.command {
        Command::Doctor => doctor::run(),
        Command::List { path } => list::run(&path),
        Command::Build { path } => {
            build::build_app(&path)?;
            Ok(())
        }
        Command::Run { path } => run::run_app(&path),
        Command::Watch { path } => watch::run(&path),
        Command::Deploy { path, host, deb } => deploy::run(&path, host.as_deref(), deb.as_deref()),
        Command::Login => auth::login(),
        Command::Logout => auth::logout(),
        Command::Bump { deb } => bump::run(deb.as_deref()),
        Command::Publish { deb } => publish::run(deb.as_deref()),
        Command::Unpublish {
            package,
            version,
            arch,
        } => unpublish::run(&package, &version, &arch),
    }
}
