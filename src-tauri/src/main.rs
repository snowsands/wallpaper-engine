// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::fs::OpenOptions;
use std::io::Write;
use tauri_plugin_dialog;

fn main() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .invoke_handler(tauri::generate_handler![set_wallpaper, stop_wallpaper])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}

#[tauri::command]
fn set_wallpaper(file_name: String) -> Result<(), String> {
    //let exe_dir = std::env::current_dir().map_err(|e| e.to_string())?;

    //let full_path = exe_dir.join("src").join("videos").join(fileName);

    let cmd = format!("set {}", file_name);
    send_pipe(&cmd)
}

fn send_pipe(command: &str) -> Result<(), String> {
    let mut pipe = OpenOptions::new()
        .write(true)
        .open(r"\\.\pipe\wallpaper_engine")
        .map_err(|e| format!("failed to connect to engine: {}", e))?;

    pipe.write_all(command.as_bytes())
        .map_err(|e| format!("failed to send command: {}", e))?;

    Ok(())
}

#[tauri::command]
fn stop_wallpaper() -> Result<(), String> {
    send_pipe("stop")
}
