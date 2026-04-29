mod build_mgr;
mod docker_mgr;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .plugin(tauri_plugin_dialog::init())
        .setup(|app| {
            if cfg!(debug_assertions) {
                app.handle().plugin(
                    tauri_plugin_log::Builder::default()
                        .level(log::LevelFilter::Info)
                        .build(),
                )?;
            }
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            build_mgr::discover_projects,
            build_mgr::get_build_command,
            build_mgr::get_build_env,
            docker_mgr::detect_container_env,
            docker_mgr::detect_install_method,
            docker_mgr::get_install_commands,
            docker_mgr::check_docker_available,
            docker_mgr::check_image_exists,
            docker_mgr::pull_or_build_image,
            docker_mgr::docker_build_project,
            docker_mgr::get_runtime,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
