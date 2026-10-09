// No console window behind the launcher in release builds on Windows.
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

fn main() {
    openrac_launcher_lib::run()
}
