# gpu-mon-control

Lightweight native Win32 background daemon and CLI utility for Windows 11/10 to switch default DirectX GPUs, manage display topology, and automatically scale mouse sensitivity on stretched resolutions.

## Features

- **GPU & Display Switching:** Atomically updates Windows default high-performance GPU (`DirectXUserGlobalSettings`) and switches display topology (`internal`, `external`, `extend`).
- **Dynamic Mouse Scaler:** Low-level mouse hook (`WH_MOUSE_LL`) that compensates Y-axis sensitivity on non-native/stretched aspect ratios and automatically recalculates on display resolution changes.
- **Dual-Mode Execution:** Runs silently in the System Tray with global hotkey support, or executes synchronously via CLI arguments for mouse macros (G Hub, Razer Synapse, etc.).
- **Zero Dependencies:** Written in pure C++14 using native Win32 API.

## Quick Start

1. Download the latest release or build the solution in Visual Studio (x64 Release).
2. Run `scan_gpus.bat` (or `gpu-mon-control.exe --init-config`) to get your GPU `adapter_id` strings.
3. Edit `config.yaml` next to the executable:

```yaml
general:
  toggle_hotkey: "Ctrl+Alt+G"
  base_resolution_x: 2560
  base_resolution_y: 1080

gpus:
  PrimaryGPU:
    adapter_id: "HighPerfAdapter=10DE&2503&87F51043;"
    topology: "internal"
  SecondaryGPU:
    adapter_id: "HighPerfAdapter=10DE&1C03&37161458;"
    topology: "external"
```

## CLI Usage (For Macros & Scripts)

| Command | Description |
|---|---|
| `gpu-mon-control.exe` | Start background daemon (System Tray + Mouse Scaler + Hotkey) |
| `gpu-mon-control.exe toggle` | Switch to the next GPU profile in `config.yaml` and exit |
| `gpu-mon-control.exe switch <Name>` | Apply a specific GPU profile (e.g., `switch PrimaryGPU`) and exit |
| `gpu-mon-control.exe --init-config` | Generate a default `config.yaml` template |

> **Note:** Windows applies the updated default GPU preference only to newly launched applications.
