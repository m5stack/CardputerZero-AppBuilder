use std::path::PathBuf;
use std::process::Command;
use tauri::Manager;

const IMAGE_TAG: &str = "m5stack-appbuilder:latest";

#[derive(serde::Serialize)]
pub struct ContainerEnv {
    pub runtime: String,       // "docker" | "nerdctl"
    pub image_ready: bool,
    pub runtime_available: bool,
}

#[tauri::command]
pub fn detect_container_env() -> ContainerEnv {
    if cmd_ok("docker", &["version"]) {
        return ContainerEnv {
            runtime: "docker".into(),
            runtime_available: true,
            image_ready: cmd_ok("docker", &["image", "inspect", IMAGE_TAG]),
        };
    }
    if cmd_ok("nerdctl", &["version"]) {
        return ContainerEnv {
            runtime: "nerdctl".into(),
            runtime_available: true,
            image_ready: cmd_ok("nerdctl", &["image", "inspect", IMAGE_TAG]),
        };
    }
    ContainerEnv {
        runtime: "none".into(),
        runtime_available: false,
        image_ready: false,
    }
}

#[tauri::command]
pub fn detect_install_method() -> Vec<String> {
    let mut methods = Vec::new();
    let is_linux = cfg!(target_os = "linux");

    if is_linux {
        methods.push("docker-linux".into());
    } else {
        if cmd_ok("brew", &["--version"]) {
            methods.push("brew".into());
        }
        methods.push("lima".into());
    }
    methods
}

#[tauri::command]
pub fn get_install_commands(method: String) -> Vec<Vec<String>> {
    match method.as_str() {
        "docker-linux" => vec![
            vec!["bash".into(), "-c".into(),
                 "curl -fsSL https://get.docker.com | sh".into()],
        ],
        "brew" => vec![
            vec!["brew".into(), "install".into(), "lima".into()],
            vec!["limactl".into(), "start".into(), "--name=default".into(),
                 "template://docker".into()],
        ],
        "lima" => vec![
            vec!["bash".into(), "-c".into(),
                 "curl -fsSL https://lima-vm.io/install.sh | bash".into()],
            vec!["limactl".into(), "start".into(), "--name=default".into(),
                 "template://docker".into()],
        ],
        _ => vec![],
    }
}

#[tauri::command]
pub fn check_docker_available() -> Result<bool, String> {
    Ok(cmd_ok("docker", &["version"]) || cmd_ok("nerdctl", &["version"]))
}

#[tauri::command]
pub fn check_image_exists() -> Result<bool, String> {
    if cmd_ok("docker", &["image", "inspect", IMAGE_TAG]) {
        return Ok(true);
    }
    if cmd_ok("nerdctl", &["image", "inspect", IMAGE_TAG]) {
        return Ok(true);
    }
    Ok(false)
}

#[tauri::command]
pub fn pull_or_build_image(app_handle: tauri::AppHandle) -> Result<String, String> {
    let dockerfile_dir = resolve_dockerfile_dir(&app_handle)?;
    let runtime = active_runtime();
    let output = Command::new(&runtime)
        .args(["build", "-t", IMAGE_TAG, "."])
        .current_dir(&dockerfile_dir)
        .output()
        .map_err(|e| format!("Failed to run {} build: {}", runtime, e))?;

    if output.status.success() {
        Ok(format!("Image {} built with {}", IMAGE_TAG, runtime))
    } else {
        let stderr = String::from_utf8_lossy(&output.stderr);
        Err(format!("{} build failed: {}", runtime, stderr))
    }
}

#[tauri::command]
pub fn docker_build_project(project_dir: String, project_path: String, app_handle: tauri::AppHandle) -> Result<Vec<String>, String> {
    let full = PathBuf::from(&project_dir).join(&project_path);
    if !full.exists() {
        return Err(format!("Project path does not exist: {}", full.display()));
    }

    let config_path = full.join("app-builder.json");
    let build_script = if config_path.exists() {
        let content = std::fs::read_to_string(&config_path)
            .map_err(|e| format!("Failed to read app-builder.json: {}", e))?;
        let config: serde_json::Value = serde_json::from_str(&content)
            .map_err(|e| format!("Invalid app-builder.json: {}", e))?;

        let pkg = config["package_name"].as_str().unwrap_or("app");
        let ver = config["version"].as_str().unwrap_or("0.1");
        let bin = config["bin_name"].as_str().unwrap_or("app");
        let app = config["app_name"].as_str().unwrap_or(pkg);

        format!(
            concat!(
                "cd /workspace/{path} && rm -f build/config/config_tmp.mk && scons -j$(nproc) && ",
                "echo '=== Build done, packaging DEB ===' && ",
                "python3 /appbuilder-scripts/pack_deb.py ",
                "--package-name {pkg} --version {ver} --bin-name {bin} ",
                "--app-name '{app}' --src-folder /workspace/{path}/dist ",
                "--output-dir /workspace/{path}/dist && ",
                "echo '=== DEB package created ===' && ls -lh /workspace/{path}/dist/*.deb"
            ),
            path = project_path,
            pkg = pkg,
            ver = ver,
            bin = bin,
            app = app,
        )
    } else {
        format!("cd /workspace/{} && rm -f build/config/config_tmp.mk && scons -j$(nproc)", project_path)
    };

    let scripts_dir = resolve_scripts_dir(&app_handle)?;

    Ok(vec![
        "run".into(),
        "--rm".into(),
        "-v".into(),
        format!("{}:/workspace", project_dir),
        "-v".into(),
        format!("{}:/appbuilder-scripts:ro", scripts_dir.display()),
        "-e".into(),
        "CardputerZero=y".into(),
        "-e".into(),
        "CONFIG_REPO_AUTOMATION=y".into(),
        IMAGE_TAG.into(),
        "bash".into(),
        "-c".into(),
        build_script,
    ])
}

#[tauri::command]
pub fn get_runtime() -> String {
    active_runtime()
}

fn active_runtime() -> String {
    if cmd_ok("docker", &["version"]) {
        "docker".into()
    } else if cmd_ok("nerdctl", &["version"]) {
        "nerdctl".into()
    } else {
        "docker".into()
    }
}

fn cmd_ok(program: &str, args: &[&str]) -> bool {
    Command::new(program)
        .args(args)
        .stdout(std::process::Stdio::null())
        .stderr(std::process::Stdio::null())
        .status()
        .map(|s| s.success())
        .unwrap_or(false)
}

fn find_project_subdir(app_handle: &tauri::AppHandle, subdir: &str, marker: &str) -> Result<PathBuf, String> {
    if let Ok(res) = app_handle.path().resource_dir() {
        let candidate = res.join(subdir);
        if candidate.join(marker).exists() {
            return Ok(candidate);
        }
    }

    if let Ok(exe) = std::env::current_exe() {
        let mut dir = exe;
        for _ in 0..6 {
            if let Some(parent) = dir.parent() {
                dir = parent.to_path_buf();
                let candidate = dir.join(subdir);
                if candidate.join(marker).exists() {
                    return Ok(candidate);
                }
            }
        }
    }

    Err(format!("Could not find {}/{}", subdir, marker))
}

fn resolve_dockerfile_dir(app_handle: &tauri::AppHandle) -> Result<PathBuf, String> {
    find_project_subdir(app_handle, "docker", "Dockerfile")
}

fn resolve_scripts_dir(app_handle: &tauri::AppHandle) -> Result<PathBuf, String> {
    find_project_subdir(app_handle, "scripts", "pack_deb.py")
}
