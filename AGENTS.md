# AGENTS.md

## Scope and layout

- Repository root contains `gpu-mon-control.sln`, `README.md`, `LICENSE`, and helper scripts.
- All application sources and headers live directly in `gpu-mon-control/`.
- The Visual Studio project is `gpu-mon-control/gpu-mon-control.vcxproj`; every new `.cpp` or `.h` must be registered in its `<ClCompile>` or `<ClInclude>` item group. Do not place source files at the repository root.
- Target C++14-compatible code and preserve the existing Win32/STL, zero-runtime-dependency approach.

## Architecture

- `main.cpp` is the single entry point. With no arguments it starts the single-instance tray daemon; with arguments it dispatches CLI operations. It creates a hidden top-level window (not `HWND_MESSAGE`) so it receives broadcast `WM_DISPLAYCHANGE` notifications.
- `CliDispatcher` implements `--init-config`, `switch <profile>`, `toggle`, `--help`, and `--verbose`. Keep CLI, tray, and hotkey actions on a shared orchestration flow that can grow to N GPUs, three or more displays, and optional profile actions such as GPU-only, display-only, or multi-step changes.
- `ConfigManager` loads the executable-adjacent `config.yaml` using the lightweight parser, including GPU profiles, `toggle_hotkey`, and `base_resolution_x/y`. Do not hardcode adapter IDs, GPU names, resolutions, or user paths.
- `GpuSwitcher`, `DisplayController`, and `MouseScaler` are independent hardware primitives. They must not depend on one another's internal state; orchestration belongs in `CliDispatcher`, `main.cpp`, tray handling, and configuration layers.
- `GpuSwitcher` reads/writes `HKCU\Software\Microsoft\DirectX\UserGpuPreferences`, value `DirectXUserGlobalSettings`, to select a configured DirectX GPU profile. Keep it independent of display and mouse-scaling operations.
- `DisplayController` currently applies `internal`, `external`, or `extend` using `SetDisplayConfig`. Keep it based on native CCD APIs and structure future `QueryDisplayConfig` path/mode management for three or more displays without coupling it to GPU selection. Do not add DDC/CI, `dxva2.lib`, or monitor VCP `0x60` input switching.
- `MouseScaler` owns the `WH_MOUSE_LL` hook. It recalculates from the current `SM_CXSCREEN`/`SM_CYSCREEN` dimensions and configured reference resolution, and changes only the Y-axis when the aspect ratios differ. Native/matching aspect ratios must leave cursor movement untouched. Preserve the user enable/disable state across display changes and config reloads.
- `TrayController` owns the notification icon and tooltip. Tray state should reflect the actual hook state; do not re-enable the hook from display-change or reload paths after a manual disable.
- Future hardware discovery, including DXGI adapter enumeration through `dxgi.lib`, must be a standalone discovery/configuration-generation layer. Runtime switching must consume configuration data rather than perform discovery implicitly.

## Windows/runtime constraints

- `SetDisplayConfig` is the display-topology mechanism; it must remain hardware-agnostic and work as the display model expands. Do not assume a fixed GPU count, monitor count, cabling arrangement, or input-switching capability.
- The binary is intended to behave as a GUI application for macro-triggered CLI commands without flashing a console. Preserve `wWinMain` and the `/SUBSYSTEM:WINDOWS` linker behavior in `main.cpp`; use a standard hidden top-level `HWND`, never `HWND_MESSAGE`, so it receives broadcast `WM_DISPLAYCHANGE` notifications. Verify both Debug and Release x64 configurations.
- Use native Win32 APIs and the existing STL/custom parser. Avoid third-party dependencies, hardware-specific assumptions, and local user paths in source or documentation.

## Change and validation rules

- Make minimal changes and preserve existing error handling and Unicode Win32 conventions (`...W` APIs, `std::wstring`).
- Update the `.vcxproj` whenever adding or removing source/header files.
- Build the solution after code changes. Test daemon startup, CLI `switch`/`toggle`, tray/hotkey operation, display-change notifications, aspect-ratio transitions, and manual scaler disable/reenable behavior where Windows hardware is available.
