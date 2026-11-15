# gpu-mon-control

Lightweight native Win32 background daemon and CLI utility for Windows 11/10. The primary purpose of the software is to correct mouse-axis sensitivity when the active display uses a non-native aspect ratio or a stretched resolution. The correction is calculated from the reference resolution configured by the user in `config.yaml`.

## Features

- **GPU & Display Switching:** Updates Windows default high-performance GPU (`DirectXUserGlobalSettings`) and applies the selected display topology (`internal`, `external`, `extend`) in the same switch operation.
- **Dynamic Mouse Scaler:** Low-level mouse hook (`WH_MOUSE_LL`) that compensates mouse-axis sensitivity on non-native/stretched aspect ratios using the configured reference resolution and automatically recalculates on display resolution changes.
- **Synchronized Output Switching:** A GPU profile always contains both an `adapter_id` and a `topology`; selecting or toggling a profile applies the current GPU preference and the corresponding display output mode together.
- **Dual-Mode Execution:** Runs silently in the System Tray with global hotkey support, or executes synchronously via CLI arguments for mouse macros (G Hub, Razer Synapse, etc.).
- **Zero Dependencies:** Written in pure C++14 using native Win32 API.

## Quick Start

1. Download the latest release or build the solution in Visual Studio (x64 Release).
2. Run `gpu-ids.bat` to list GPU hardware identifiers and DirectX adapter ID candidates.
3. Run `gpu-mon-control.exe --init-config` to create the initial `config.yaml` template.
4. Set the reference resolution and GPU profiles in `config.yaml` next to the executable:

```yaml
general:
  toggle_hotkey: "Shift+F8"
  base_resolution_x: 1920
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

## Primary Use Case: Non-Native Aspect Ratios

The software is primarily intended for systems where a game or application uses a stretched or otherwise non-native aspect ratio. The reference dimensions are not hardcoded to a particular developer monitor: they are configured by the user:

```yaml
general:
  base_resolution_x: 1920
  base_resolution_y: 1080
```

`MouseScaler` compares the current Windows display metrics with these reference values and recalculates the axis correction whenever `WM_DISPLAYCHANGE` is received. The same calculation is also performed after the configuration is reloaded.

## Synchronized GPU and Display Output Switching

GPU switching is profile-based. A profile is not only a GPU identifier; it also defines the display topology that must be applied with it:

```yaml
gpus:
  PrimaryGPU:
    adapter_id: "HighPerfAdapter=10DE&2503&87F51043;"
    topology: "internal"
  SecondaryGPU:
    adapter_id: "HighPerfAdapter=10DE&1C03&37161458;"
    topology: "external"
```

The `switch`, `toggle`, tray menu, and global hotkey all use the same flow:

1. resolve the selected profile;
2. write its `adapter_id` to `DirectXUserGlobalSettings`;
3. apply its `topology` through `SetDisplayConfig`;
4. update the tray tooltip.

This keeps the active GPU profile and the selected display output synchronized. Windows applies the DirectX GPU preference to newly launched applications; already-running applications may require a restart.

## GPU ID Discovery

`gpu-ids.bat` is a helper for preparing `config.yaml`. It does not change GPU settings. The script:

- queries installed adapters through `Win32_VideoController`;
- prints the adapter name and `PNPDeviceID`;
- derives a `HighPerfAdapter=VENDOR&DEVICE&SUBSYSTEM;` candidate when possible;
- prints the current `DirectXUserGlobalSettings` registry value.

Run it from the project or release directory:

```cmd
gpu-ids.bat
```

Copy the printed `adapter_id` candidates into the corresponding `gpus` profiles, then assign the required `topology` to each profile. The derived value should be verified against the actual DirectX preference on the target machine before use.
