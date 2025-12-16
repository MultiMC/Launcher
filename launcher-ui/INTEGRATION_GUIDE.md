# Integration Guide: Connecting Tauri Frontend to C++ Backend

This guide explains how to integrate the new Tauri/React frontend with the existing MultiMC C++ backend.

## Integration Approaches

### Option 1: FFI (Foreign Function Interface)

Use the `cxx` crate to call C++ functions from Rust.

#### Setup

1. Add dependencies to `src-tauri/Cargo.toml`:
```toml
[dependencies]
cxx = "1.0"

[build-dependencies]
cxx-build = "1.0"
```

2. Create C++ bridge header:
```cpp
// bridge.h
#pragma once
#include <memory>
#include <string>

namespace multimc {
  struct InstanceInfo {
    std::string id;
    std::string name;
    std::string version;
  };

  std::vector<InstanceInfo> get_instances();
  bool launch_instance(const std::string& id);
}
```

3. Create Rust bridge in `src-tauri/src/bridge.rs`:
```rust
#[cxx::bridge(namespace = "multimc")]
mod ffi {
    struct InstanceInfo {
        id: String,
        name: String,
        version: String,
    }

    unsafe extern "C++" {
        include!("bridge.h");
        
        fn get_instances() -> Vec<InstanceInfo>;
        fn launch_instance(id: &str) -> bool;
    }
}
```

### Option 2: IPC (Inter-Process Communication)

Run the existing MultiMC as a subprocess and communicate via sockets/pipes.

#### Implementation

```rust
use std::process::Command;
use std::net::TcpStream;

#[tauri::command]
async fn launch_backend() -> Result<(), String> {
    Command::new("./multimc-backend")
        .arg("--ipc-mode")
        .spawn()
        .map_err(|e| e.to_string())?;
    Ok(())
}

#[tauri::command]
async fn get_instances_ipc() -> Result<Vec<Instance>, String> {
    let stream = TcpStream::connect("127.0.0.1:9876")
        .map_err(|e| e.to_string())?;
    
    // Send command and receive response
    // Parse JSON response
    Ok(vec![])
}
```

### Option 3: Shared Library

Compile MultiMC core as a shared library (.so/.dll).

#### Steps

1. Modify CMakeLists.txt to build shared library:
```cmake
add_library(multimc_core SHARED
    launcher/Application.cpp
    launcher/InstanceList.cpp
    # ... other sources
)
```

2. Create C API wrapper:
```cpp
// multimc_api.h
extern "C" {
    void* multimc_init();
    const char* multimc_get_instances(void* app);
    bool multimc_launch_instance(void* app, const char* id);
    void multimc_cleanup(void* app);
}
```

3. Load in Rust:
```rust
use libloading::{Library, Symbol};

#[tauri::command]
async fn get_instances() -> Result<Vec<Instance>, String> {
    unsafe {
        let lib = Library::new("libmultimc_core.so")
            .map_err(|e| e.to_string())?;
        
        let init: Symbol<unsafe extern fn() -> *mut c_void> = 
            lib.get(b"multimc_init").map_err(|e| e.to_string())?;
        
        let app = init();
        
        // Call other functions...
    }
    Ok(vec![])
}
```

### Option 4: REST API (Recommended for Gradual Migration)

Add a REST API to existing MultiMC and call it from Tauri.

#### C++ Side

```cpp
// Add httplib or similar
#include "httplib.h"

void startApiServer() {
    httplib::Server svr;
    
    svr.Get("/api/instances", [](const auto& req, auto& res) {
        auto instances = InstanceList::getInstances();
        nlohmann::json j = instances;
        res.set_content(j.dump(), "application/json");
    });
    
    svr.listen("localhost", 8080);
}
```

#### Rust Side

```rust
use reqwest;

#[tauri::command]
async fn get_instances() -> Result<Vec<Instance>, String> {
    let resp = reqwest::get("http://localhost:8080/api/instances")
        .await
        .map_err(|e| e.to_string())?
        .json::<Vec<Instance>>()
        .await
        .map_err(|e| e.to_string())?;
    
    Ok(resp)
}
```

## Recommended Migration Path

### Phase 1: Dual Frontend (Weeks 1-2)
- Keep Qt frontend running
- Add REST API to C++ backend
- Test Tauri frontend with mock data

### Phase 2: API Integration (Weeks 3-4)  
- Connect Tauri to REST API
- Implement all core features
- Test thoroughly on all platforms

### Phase 3: Feature Parity (Weeks 5-6)
- Implement all Qt features in Tauri
- Add missing functionality
- User testing and feedback

### Phase 4: Transition (Weeks 7-8)
- Make Tauri the default
- Keep Qt as fallback option
- Monitor for issues

### Phase 5: Deprecation (Weeks 9+)
- Remove Qt frontend
- Optionally port backend to Rust
- Full Tauri/Rust stack

## File Mappings

| Qt C++ File | Tauri Equivalent |
|------------|------------------|
| MainWindow.cpp | App.tsx |
| InstanceView.cpp | DevicePanel.tsx |
| InstanceWindow.cpp | (Future) InstanceDetail.tsx |
| LauncherPage.cpp | (Future) Settings.tsx |
| Application.cpp | src-tauri/src/lib.rs |

## Data Flow

```
User Interaction
    ↓
React Component
    ↓
API Service (api.ts)
    ↓
Tauri Command (invoke)
    ↓
Rust Handler (lib.rs)
    ↓
FFI/IPC/REST → C++ Backend
    ↓
Qt Backend Logic
    ↓
Response ← ← ← ← ←
```

## Testing Integration

```rust
#[cfg(test)]
mod tests {
    use super::*;

    #[tokio::test]
    async fn test_get_instances() {
        let instances = get_instances().await.unwrap();
        assert!(!instances.is_empty());
    }

    #[tokio::test]
    async fn test_launch_instance() {
        let result = launch_instance("test-id".to_string()).await;
        assert!(result.is_ok());
    }
}
```

## Debugging Tips

1. **Enable Rust logging**:
```rust
env_logger::init();
log::debug!("Calling C++ function");
```

2. **Check C++ crashes**:
```bash
gdb ./multimc-backend
(gdb) run
(gdb) bt  # on crash
```

3. **Monitor IPC**:
```bash
netstat -tulpn | grep 8080
tcpdump -i lo port 8080
```

## Performance Considerations

- **FFI**: Fastest, but complex setup
- **IPC**: Medium speed, good isolation
- **REST**: Slowest, but easiest to debug
- **Shared Lib**: Fast, but ABI compatibility issues

## Security Notes

- Validate all inputs from frontend
- Use HTTPS for production REST APIs  
- Don't expose internal paths to frontend
- Implement authentication for multi-user setups

## Next Steps

1. Choose integration approach
2. Set up basic connection
3. Implement one feature end-to-end
4. Iterate and expand
5. Add error handling
6. Write tests
7. Optimize performance
