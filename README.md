# DinoGame4Cardputer
(c) kindle15 for structure and image creation not for the game itself

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

- If you reorganize variant assets/rules files, do a **clean build** to avoid stale objects.
- Keep a single compiled definition for:
  - `getVariantAssets()`
  - `makeVariantObstacle(...)`

## Install

- Install Arduino IDE
- Install M5Stack board manager and all dependencies
- Select M5Cardputer from board manager
- Install M5Stack libraries that apply
