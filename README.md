# DinoGame4Cardputer

A Chrome-Dino style endless runner for **M5Cardputer** (ESP32-S3), inspired by the pacing of [dinogame.app](https://dinogame.app/).

## Features

- Endless runner gameplay loop
- Fixed-timestep update for stable motion
- Jump + duck controls from Cardputer keyboard
- Procedural obstacle queue with difficulty scaling
- Variant-based gameplay/assets (`GAME_VARIANT`)
- Bird obstacle support in Variant 3
- Session score + high score tracking
- Game-over restart flow

## Controls

- **Jump:** `Space`, `Enter`, or `;`
- **Duck (hold):** `,`, `/`, `d`, or `D`
- **Restart:** `Space` / `Enter` on game over

## Hardware

- M5Cardputer (ESP32-S3)
- Built-in display and keyboard

## Project Structure

- `src/main.cpp` — core game loop, physics, rendering, input
- `src/variant_assets.cpp` — compile-time selected sprite tables
- `src/variant_rules.cpp` — compile-time selected obstacle/gap rules
- `include/variant_config.h` — `GAME_VARIANT` selection
- `include/variant_assets.h` / `include/variant_rules.h` — shared interfaces

## Build (PlatformIO)

### Option A: VS Code + PlatformIO Extension

1. Install [PlatformIO](https://platformio.org/) (VS Code extension).
2. Open this folder in VS Code.
3. Build/upload using PlatformIO tasks, or run in terminal:

```bash
pio run -t upload
```

### Option B: WSL (Windows) + PlatformIO Core

1. Install **WSL**.
2. Install a Linux distro (recommended: **Debian** or **Ubuntu**).
3. In WSL, install required tools:
   - `git`
   - `python3`
   - `python3-pip`
   - `pipx` (recommended)
4. Install PlatformIO Core:

```bash
pipx install platformio
```

5. Change into the project directory (example on `D:` drive):

```bash
cd /mnt/d/dino_game_port_for_cardputer_v1
```

6. Build/upload:

```bash
pio run -e m5cardputer_v3 -t upload
```

## Variant Build Examples

### Default environment

```bash
pio run -t upload
```

### Variant 3 (birds-enabled)

```bash
pio run -e m5cardputer_v3 -t clean
pio run -e m5cardputer_v3 -t upload #only if uploading to device via port, otherwise will error
pio run -e m5cardputer_v3           #Saves to project folder under /build/ upload via esp32 webtool.
                                    #Can also install M5Launcher and install firmware.bin via MMC.
```

## Notes

- If you reorganize variant assets/rules files, do a **clean build** to avoid stale objects.
- Keep a single compiled definition for:
  - `getVariantAssets()`
  - `makeVariantObstacle(...)`
  - `variantBaseGap(...)`
  - `variantGapRandomMax()`