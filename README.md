# gpu-mon-control

Lightweight Windows background tool for Windows 10/11. Its primary purpose is to correct mouse-axis sensitivity when the active display uses a non-native aspect ratio or a stretched resolution. The correction is calculated from the reference resolution configured by the user in `config.yaml`.

## Features

- **GPU & Display Switching:** Selects the configured GPU and applies the matching display output mode in the same operation.
- **Dynamic Mouse Scaler:** Corrects mouse-axis sensitivity on non-native/stretched aspect ratios using the configured reference resolution and updates when the display configuration changes.
- **Synchronized Output Switching:** A GPU profile always contains both an `adapter_id` and a `topology`; selecting or toggling a profile applies the current GPU preference and the corresponding display output mode together.
- **Dual-Mode Execution:** Runs silently in the System Tray with global hotkey support, or executes commands for mouse macros and scripts (G Hub, Razer Synapse, etc.).
- **Zero Dependencies:** No additional runtime packages are required.

## Quick Start

1. Download the latest release or build the solution in Visual Studio (x64 Release).
2. Run `gpu-ids.bat` to list GPU hardware identifiers and create values for the configuration.
3. Run `gpu-mon-control.exe --init-config` to create the initial `config.yaml` template.
4. Set the reference resolution and GPU profiles in `config.yaml` next to the executable:

```yaml
general:
  toggle_hotkey: "Shift+F8"
  base_resolution_x: 1920
  base_resolution_y: 1080

gpus:
  PrimaryGPU:
    adapter_id: "HighPerfAdapter=VENDOR&DEVICE&SUBSYSTEM;"
    topology: "internal"
  SecondaryGPU:
    adapter_id: "HighPerfAdapter=VENDOR&DEVICE&SUBSYSTEM;"
    topology: "external"
```

## Commands (For Macros & Scripts)

| Command | Description |
|---|---|
| `gpu-mon-control.exe` | Start the background tool (System Tray + Mouse Scaler + Hotkey) |
| `gpu-mon-control.exe toggle` | Switch to the next GPU profile in `config.yaml` and exit |
| `gpu-mon-control.exe switch <Name>` | Apply a specific GPU profile (e.g., `switch PrimaryGPU`) and exit |
| `gpu-mon-control.exe --init-config` | Generate a default `config.yaml` template |

> **Note:** Windows applies a changed GPU preference to newly launched applications. Already-running applications may require a restart.

## Primary Use Case: Non-Native Aspect Ratios

The software is primarily intended for systems where a game or application uses a stretched or otherwise non-native aspect ratio. The reference dimensions are not hardcoded to a particular developer monitor: they are configured by the user:

```yaml
general:
  base_resolution_x: 1920
  base_resolution_y: 1080
```

The tool compares the current display dimensions with these reference values and recalculates the axis correction whenever the display configuration changes. The same calculation is also performed after the configuration is reloaded.

## Synchronized GPU and Display Output Switching

GPU switching is profile-based. Each profile contains a GPU identifier and the display mode that must be applied with it:

```yaml
gpus:
  PrimaryGPU:
    adapter_id: "HighPerfAdapter=VENDOR&DEVICE&SUBSYSTEM;"
    topology: "internal"
  SecondaryGPU:
    adapter_id: "HighPerfAdapter=VENDOR&DEVICE&SUBSYSTEM;"
    topology: "external"
```

The `switch`, `toggle`, tray menu, and global hotkey all use the same flow:

1. resolve the selected profile;
2. select its configured GPU;
3. apply its configured display output mode;
4. update the tray tooltip.

This keeps the selected GPU and display output synchronized.

## GPU ID Discovery

`gpu-ids.bat` is a helper for preparing `config.yaml`. It does not change GPU settings. The script:

- queries installed graphics adapters;
- prints the adapter name and Windows hardware identifier;
- derives a configuration value when possible;
- prints the current Windows GPU preference.

Run it from the project or release directory:

```cmd
gpu-ids.bat
```

Copy the printed values into the corresponding profiles, then assign `internal`, `external`, or `extend` to each profile. The values should be verified on the target machine before use.
