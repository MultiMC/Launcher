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

#[tauri::command]
async fn add_account(username: String, _password: String) -> Result<bool, String> {
    // TODO: Add Microsoft account
    println!("Adding account: {}", username);
    Ok(true)
}

#[tauri::command]
async fn remove_account(account_id: String) -> Result<bool, String> {
    // TODO: Remove account
    println!("Removing account: {}", account_id);
    Ok(true)
}

#[tauri::command]
async fn open_instance_folder(instance_id: String) -> Result<bool, String> {
    // TODO: Open instance folder in file manager
    println!("Opening folder for instance: {}", instance_id);
    // Would use tauri::api::shell::open() or system command
    Ok(true)
}

#[tauri::command]
async fn copy_instance(instance_id: String, new_name: String) -> Result<String, String> {
    // TODO: Copy instance using C++ backend
    println!("Copying instance {} to {}", instance_id, new_name);
    Ok(format!("{}-copy", instance_id))
}

#[tauri::command]
async fn rename_instance(instance_id: String, new_name: String) -> Result<bool, String> {
    // TODO: Rename instance in C++ backend
    println!("Renaming instance {} to {}", instance_id, new_name);
    Ok(true)
}

#[tauri::command]
async fn update_instance(instance_id: String, settings: serde_json::Value) -> Result<bool, String> {
    // TODO: Update instance settings
    println!("Updating instance {} settings: {:?}", instance_id, settings);
    Ok(true)
}

#[tauri::command]
async fn get_instance_mods(instance_id: String) -> Result<Vec<serde_json::Value>, String> {
    // TODO: Get mods from instance's mods folder
    println!("Getting mods for instance: {}", instance_id);
    Ok(vec![
        serde_json::json!({
            "id": "mod1",
            "name": "JEI (Just Enough Items)",
            "version": "15.2.0.27",
            "enabled": true,
            "fileName": "jei-1.20.1-forge-15.2.0.27.jar"
        }),
        serde_json::json!({
            "id": "mod2",
            "name": "Optifine",
            "version": "HD U I5",
            "enabled": false,
            "fileName": "OptiFine_1.20.1_HD_U_I5.jar"
        })
    ])
}

#[tauri::command]
async fn toggle_mod(instance_id: String, mod_id: String, enabled: bool) -> Result<bool, String> {
    // TODO: Enable/disable mod (rename .jar to .jar.disabled or vice versa)
    println!("Toggling mod {} in instance {} to {}", mod_id, instance_id, enabled);
    Ok(true)
}

#[tauri::command]
async fn remove_mod(instance_id: String, mod_id: String) -> Result<bool, String> {
    // TODO: Remove mod file
    println!("Removing mod {} from instance {}", mod_id, instance_id);
    Ok(true)
}

#[tauri::command]
async fn install_mod(instance_id: String, mod_path: String) -> Result<bool, String> {
    // TODO: Copy mod file to instance mods folder
    println!("Installing mod {} to instance {}", mod_path, instance_id);
    Ok(true)
}

#[tauri::command]
async fn get_instance_resource_packs(instance_id: String) -> Result<Vec<serde_json::Value>, String> {
    // TODO: Get resource packs from instance
    println!("Getting resource packs for instance: {}", instance_id);
    Ok(vec![])
}

#[tauri::command]
async fn get_instance_shader_packs(instance_id: String) -> Result<Vec<serde_json::Value>, String> {
    // TODO: Get shader packs from instance
    println!("Getting shader packs for instance: {}", instance_id);
    Ok(vec![])
}

#[tauri::command]
async fn get_instance_worlds(instance_id: String) -> Result<Vec<serde_json::Value>, String> {
    // TODO: Get worlds from instance saves folder
    println!("Getting worlds for instance: {}", instance_id);
    Ok(vec![
        serde_json::json!({
            "id": "world1",
            "name": "My World",
            "lastPlayed": "2024-01-15T10:30:00Z",
            "gameMode": "Survival"
        })
    ])
}

#[tauri::command]
async fn get_instance_screenshots(instance_id: String) -> Result<Vec<serde_json::Value>, String> {
    // TODO: Get screenshots from instance screenshots folder
    println!("Getting screenshots for instance: {}", instance_id);
    Ok(vec![])
}

#[tauri::command]
async fn install_mod_loader(
    instance_id: String,
    loader_type: String,
    version: String,
) -> Result<bool, String> {
    // TODO: Install mod loader (Forge, Fabric, etc.)
    println!("Installing {} {} for instance {}", loader_type, version, instance_id);
    Ok(true)
}

#[tauri::command]
async fn get_mod_loader_versions(
    loader_type: String,
    minecraft_version: String,
) -> Result<Vec<String>, String> {
    // TODO: Fetch available mod loader versions
    println!("Getting {} versions for Minecraft {}", loader_type, minecraft_version);

    // Return mock versions based on loader type
    match loader_type.as_str() {
        "forge" => Ok(vec![
            "47.2.0".to_string(),
            "47.1.3".to_string(),
            "47.0.35".to_string(),
        ]),
        "fabric" => Ok(vec![
            "0.15.3".to_string(),
            "0.15.2".to_string(),
            "0.15.1".to_string(),
        ]),
        "quilt" => Ok(vec![
            "0.23.0".to_string(),
            "0.22.0".to_string(),
        ]),
        "liteloader" => Ok(vec![
            "1.12.2".to_string(),
        ]),
        _ => Ok(vec![]),
    }
}

#[tauri::command]
async fn check_for_updates() -> Result<serde_json::Value, String> {
    // TODO: Check for launcher updates
    Ok(serde_json::json!({
        "available": false
    }))
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
            add_account,
            remove_account,
            open_instance_folder,
            copy_instance,
            rename_instance,
            update_instance,
            get_instance_mods,
            toggle_mod,
            remove_mod,
            install_mod,
            get_instance_resource_packs,
            get_instance_shader_packs,
            get_instance_worlds,
            get_instance_screenshots,
            install_mod_loader,
            get_mod_loader_versions,
            check_for_updates,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
