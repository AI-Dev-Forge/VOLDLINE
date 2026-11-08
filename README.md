<div align="center">

# ⚡ VOLD LINE ⚡
### Next-Gen High Performance External ESP & Overlay for Counter-Strike 2

[![GitHub Release](https://img.shields.io/github/v/release/AI-Dev-Forge/VOLDLINE?style=for-the-badge&color=blue)](https://github.com/AI-Dev-Forge/VOLDLINE/releases)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen?style=for-the-badge)](https://github.com/AI-Dev-Forge/VOLDLINE)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011-0078D6?style=for-the-badge&logo=windows)](https://github.com/AI-Dev-Forge/VOLDLINE)
[![C++](https://img.shields.io/badge/Language-C%2B%2B20-blue?style=for-the-badge&logo=c%2B%2B)](https://github.com/AI-Dev-Forge/VOLDLINE)
[![License](https://img.shields.io/badge/License-MIT-orange?style=for-the-badge)](LICENSE)

<br />

<img src="https://capsule-render.vercel.app/api?type=waving&color=gradient&customColorList=1,2,9,20,24&height=180&section=header&text=VOLD%20LINE%20ESP&fontSize=50&fontAlignY=35&animation=twinkling&desc=External%20DirectX11%20Overlay%20%E2%80%A2%20ImGui%20GUI%20%E2%80%A2%20Live%20Auto-Updater&descAlignY=62&descSize=18" width="100%" />

<br />

**VOLD LINE** is a lightweight, high-performance external visual overlay designed exclusively for **Counter-Strike 2 (CS2)**. Built with modern C++20, DirectX 11, and Dear ImGui, it offers ultra-smooth rendering, minimal CPU/GPU overhead, persistent user configuration, and a built-in instant auto-updater.

</div>

---

## 🎯 Key Features

<table>
  <tr>
    <td width="50%">
      <h3>🎯 Aim Assist / Aimbot</h3>
      <ul>
        <li><b>Configurable FOV Circle:</b> Visual field-of-view display rendered dynamically on screen.</li>
        <li><b>Smooth Aiming Curves:</b> Adjustable smoothing coefficient for natural, humanized mouse movement.</li>
        <li><b>Target Snapline:</b> Visual indicators pointing directly toward the active prioritized target.</li>
        <li><b>Hold-to-Aim Activation:</b> Configurable keypress activation mode (e.g., hold Left Mouse Button).</li>
        <li><b>Target Prioritization:</b> Automatically acquires and locks onto the nearest valid target within your FOV.</li>
        <li><b>Team Safeguard:</b> Automatically targets enemies only, ignoring friendly teammates.</li>
      </ul>
      <br />
      <h3>👁️ Advanced Visual ESP</h3>
      <ul>
        <li><b>2D & Corner Boxes:</b> Fully customizable bounding boxes with health-based coloring or solid outlines.</li>
        <li><b>Box Fill:</b> Subtle translucent background shading with configurable alpha transparency.</li>
        <li><b>Dynamic Health Bar:</b> Vertical responsive health indicators with adaptive gradient colors (Green ➔ Red).</li>
        <li><b>HP Text & Dot:</b> Clean numerical HP display with selectable formats (<code>100 HP</code>, <code>100</code>, <code>[100%]</code>) and overhead status dots.</li>
        <li><b>Head Circle ESP:</b> Precision indicators aligned to enemy head hitboxes with customizable radius and color.</li>
        <li><b>Snaplines / Tracers:</b> Target guide lines customizable from Bottom, Center, or Top screen origins.</li>
        <li><b>Distance Calculation:</b> Real-time in-game distance measurement in meters displayed under player feet.</li>
        <li><b>Team Filtering:</b> Enemy-only filter to keep your view uncluttered.</li>
      </ul>
    </td>
    <td width="50%">
      <h3>🕹️ Crosshair & HUD Suite</h3>
      <ul>
        <li><b>Custom Crosshair Styles:</b> Classic Cross (+), T-Style, Circle, Dot Only, Cross+Dot, and Circle+Dot.</li>
        <li><b>Dynamic Fine-Tuning:</b> Custom size, line thickness, center gap, screen offset (X/Y), and black outlines.</li>
        <li><b>Dual-Tone Colors:</b> Full RGBA color picker with separate dot color override support.</li>
        <li><b>Live Interactive Preview:</b> Visual crosshair preview window rendered right inside the settings menu.</li>
        <li><b>Realtime Watermark & FPS:</b> Built-in frame counter and status indicator.</li>
        <li><b>VOLD Modern Dark/Lime UI:</b> Polished Dear ImGui user interface with custom color accents and styling.</li>
      </ul>
      <br />
      <h3>💾 Persistent Config & Settings System</h3>
      <ul>
        <li><b>Auto-Save & Auto-Load:</b> All customized features and settings automatically load on startup and save on exit into <code>settings.ini</code>.</li>
        <li><b>Manual Control:</b> Dedicated <i>Save Settings</i> and <i>Reset Defaults</i> buttons directly inside the menu.</li>
        <li><b>Full Portability:</b> Human-readable INI configuration file generated right alongside the executable.</li>
      </ul>
      <br />
      <h3>⌨️ Interactive Keybind & Hotkey Manager</h3>
      <ul>
        <li><b>Live Keybinding:</b> Rebind the menu toggle key in real-time by clicking and pressing any key on your keyboard.</li>
        <li><b>Key Presets:</b> Quick-select dropdown menu with common keys (<code>INSERT</code>, <code>DELETE</code>, <code>HOME</code>, <code>PAGE UP</code>, <code>F1-F11</code>, <code>TILDE</code>, etc.).</li>
        <li><b>Persistent Key Choice:</b> Your bound toggle key is preserved across computer restarts and app launches.</li>
      </ul>
      <br />
      <h3>🚀 Smart Auto-Updater & Process Guardian</h3>
      <ul>
        <li><b>Zero-Effort Updates:</b> Automatically checks GitHub Releases for new updates and CS2 offset patches on launch.</li>
        <li><b>Hot-Swap Upgrade:</b> Downloads and replaces the executable seamlessly without touching files manually.</li>
        <li><b>Auto-Attach & Standby Loop:</b> Continuously monitors for CS2; automatically connects whenever <code>cs2.exe</code> starts, restarts, or reconnects.</li>
      </ul>
    </td>
  </tr>
</table>

---

## 🎮 Hotkeys & Controls

| Key | Action | Description |
| :--- | :--- | :--- |
| <kbd>INSERT</kbd> *(Default)* | **Toggle Menu** | Show / Hide the interactive ImGui configuration GUI (Rebindable to any key) |
| <kbd>END</kbd> | **Quick Exit** | Safely shutdown overlay, save settings, and exit application |
| <kbd>Mouse 1 (Left Click)</kbd> | **Aim Assist** | Hold to smoothly track and lock onto nearest target in FOV (when Aimbot is enabled) |

---

## ⚡ Quick Start for Users

1. Download the latest version of `VOLD-LINE.exe` from [**Releases**](https://github.com/AI-Dev-Forge/VOLDLINE/releases).
2. Launch `VOLD-LINE.exe`.
3. Launch **Counter-Strike 2**. (The software will automatically detect and link to CS2).
4. Press <kbd>INSERT</kbd> (or your custom bound key) in-game to customize your features!
5. **No need to re-configure every time:** Your configured crosshairs, ESP colors, aimbot settings, and hotkeys will automatically be remembered in `settings.ini` for your next session.

> [!TIP]
> Whenever a new update or CS2 offset patch is published, the program will automatically notify you with a popup to update on startup.

---

## 🛠️ Developer Setup & Compilation

### Requirements:
- **Visual Studio 2022 / 2025** with *Desktop development with C++*
- Windows 10 / 11 (64-bit)
- DirectX 11 End-User Runtimes
- Windows 10/11 SDK

### Build Instructions:
```bash
# Clone the repository
git clone https://github.com/AI-Dev-Forge/VOLDLINE.git
cd VOLDLINE

# Open solution or compile via MSBuild (x64)
msbuild "VOLD-LINE\VOLD-LINE.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:PreferredToolArchitecture=x64
```
The compiled executable will be generated at `VOLD-LINE\x64\Release\VOLD-LINE.exe`.

---

## 📜 Disclaimer

> [!WARNING]
> This project is created for **educational and research purposes only**. Use at your own risk. The developers assume no liability for misuse, game bans, or infractions against terms of service.

<div align="center">

<img src="https://capsule-render.vercel.app/api?type=waving&color=gradient&customColorList=1,2,9,20,24&height=100&section=footer" width="100%" />

</div>
