use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
pub struct Instance {
    id: String,
    name: String,
    version: String,
    status: String,
    icon: Option<String>,
    last_played: Option<String>,
    mod_count: Option<u32>,
    play_time: Option<u64>,
}

// Learn more about Tauri commands at https://tauri.app/develop/calling-rust/
#[tauri::command]
fn greet(name: &str) -> String {
    format!("Hello, {}! You've been greeted from Rust!", name)
}

#[tauri::command]
async fn get_instances() -> Result<Vec<Instance>, String> {
    // TODO: Integrate with existing MultiMC C++ backend
    // This would call into the existing InstanceList or similar classes
    // For now, returning mock data
    Ok(vec![
        Instance {
            id: "1".to_string(),
            name: "Vanilla 1.20.4".to_string(),
            version: "1.20.4".to_string(),
            status: "active".to_string(),
            icon: Some("Pickaxe".to_string()),
            last_played: Some("2h ago".to_string()),
            mod_count: Some(0),
            play_time: Some(12340),
        },
    ])
}

#[tauri::command]
async fn launch_instance(instance_id: String) -> Result<bool, String> {
    // TODO: Call into MinecraftInstance::launch() from C++ backend
    println!("Launching instance: {}", instance_id);
    Ok(true)
}

#[tauri::command]
async fn create_instance(name: String, version: String) -> Result<String, String> {
    // TODO: Call InstanceCreationTask or similar from C++ backend
    println!("Creating instance: {} with version {}", name, version);
    Ok("new-instance-id".to_string())
}

#[tauri::command]
async fn delete_instance(instance_id: String) -> Result<bool, String> {
    // TODO: Call into instance deletion logic from C++ backend
    println!("Deleting instance: {}", instance_id);
    Ok(true)
}

#[tauri::command]
async fn get_instance_details(instance_id: String) -> Result<Instance, String> {
    // TODO: Get instance details from C++ backend
    println!("Getting details for instance: {}", instance_id);
    Ok(Instance {
        id: instance_id,
        name: "Example Instance".to_string(),
        version: "1.20.4".to_string(),
        status: "stopped".to_string(),
        icon: None,
        last_played: None,
        mod_count: Some(0),
        play_time: Some(0),
    })
}

#[tauri::command]
async fn get_available_versions() -> Result<Vec<String>, String> {
    // TODO: Get from Minecraft version metadata
    Ok(vec![
        "1.20.4".to_string(),
        "1.20.1".to_string(),
        "1.19.4".to_string(),
    ])
}

#[tauri::command]
async fn get_java_versions() -> Result<Vec<serde_json::Value>, String> {
    // TODO: Call into Java detection from C++ backend
    Ok(vec![
        serde_json::json!({
            "path": "/usr/lib/jvm/java-17-openjdk",
            "version": "17.0.9"
        }),
    ])
}

#[tauri::command]
async fn get_settings() -> Result<serde_json::Value, String> {
    // TODO: Read from settings.json or INI file
    Ok(serde_json::json!({
        "theme": "dark",
        "language": "en_US"
    }))
}

#[tauri::command]
async fn update_settings(settings: serde_json::Value) -> Result<bool, String> {
    // TODO: Write to settings file
    println!("Updating settings: {:?}", settings);
    Ok(true)
}

#[tauri::command]
async fn get_accounts() -> Result<Vec<serde_json::Value>, String> {
    // TODO: Get from AccountList in C++ backend
    Ok(vec![])
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![
            greet,
            get_instances,
            launch_instance,
            create_instance,
            delete_instance,
            get_instance_details,
            get_available_versions,
            get_java_versions,
            get_settings,
            update_settings,
            get_accounts,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
