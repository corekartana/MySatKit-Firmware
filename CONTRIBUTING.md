# Contributing to MySatKit-Firmware

Thank you for contributing! This firmware targets two boards cooperating over I2C: an ESP32-CAM (main OBC) and an ATmega328P (auxiliary controller). There is no CLI build, automated test suite, linter, or typechecker — all verification is done on hardware via the Arduino IDE.

## Build & flash

- **Arduino IDE 2.0+** is the only supported toolchain.
- **Board selections** (set in Boards Manager):
  - `ino/MySat_main` → **AI Thinker ESP32-CAM**
  - `ino/MySat_Nano_ATmega328p` → **Arduino Nano** (ATmega328P)
- **ESP32 Arduino core 3.x** is required for `MySat_main` (tested with 3.3.11). The firmware uses the pin-based LEDC API (`ledcAttach`/`ledcWrite(pin, …)`); core 2.x is no longer compatible.
- Third-party libraries are bundled in `libraries.zip`. Extract them into the Arduino sketchbook `libraries/` folder — never into this repo.
- `WebServer.h` comes from the ESP32 Arduino core's built-in `WebServer` library. Do not install a separate copy in the sketchbook `libraries/` folder (it triggers a "multiple libraries" warning).
- LittleFS static assets under `ino/MySat_main/data/` are flashed via the Arduino IDE **"Sketch data Upload"** tool, separately from the firmware upload.

## Code style

- Match the existing style in `ino/`. Keep changes minimal and focused.
- Do not add comments unless necessary for non-obvious logic.
- Use the existing logging macros defined in `sensors_data.h`:
  - `LOG_INFO(msg)`, `LOG_WARN(msg)`, `LOG_ERROR(msg)` for ESP32-CAM logs.
  - `logDebug(msg)` for verbose debug output (only prints when `debug_mode_active` is true).
- ATmega328P firmware uses raw `Serial.print` — no logging macros.
- Do not edit vendored libraries (`libraries.zip`) to fix firmware bugs. Fix in the `.ino`/`.h` files under `ino/` instead.

## Serial console commands

When adding a new serial console command to `MySat_main`:
1. Add the dispatch case in `handleCommands()` in `console.h` (commands are case-insensitive).
2. Emit a `writeEventLog(...)` entry as the existing dispatch does — every recognized command logs `"COMMAND_RECEIVED \"<command>\""`.

## Version strings

Keep these in sync when bumping a release:
- `FIRMWARE_VERSION` in `ino/MySat_main/console.h` (ESP32-CAM)
- `VERSION` define in `ino/MySat_Nano_ATmega328p/MySat_Nano_ATmega328p.ino` (ATmega)
- The README release notes block

## Fork versioning

This is a fork of `MySatKit/MySatKit-Firmware`. To distinguish fork releases from upstream, a `+kartana` build metadata suffix is appended to version strings (per [SemVer 2.0.0](https://semver.org) build metadata rules):

- ESP32-CAM: `v.1.4.2+kartana` (in `FIRMWARE_VERSION`, `console.h`)
- ATmega328P: `v.1.3.0+kartana` (in `VERSION`, `MySat_Nano_ATmega328p.ino`)
- README release notes block: `V.1.4.2+kartana`

The `+` separator means the fork version has the same precedence as the upstream base version — it identifies the variant without claiming to be a different release. When changes are contributed back to upstream, the `+kartana` suffix is dropped.

## Testing

There are no automated tests. Verify changes by:
1. Flashing to hardware via the Arduino IDE.
2. Opening the serial monitor at **115200 baud**.
3. ESP32-CAM logs use the `LOG_*` macros; ATmega uses raw `Serial.print`.
4. Exercise affected serial commands and Web GUI endpoints.

In your PR description, include:
- Which board(s) were tested (ESP32-CAM, ATmega328P, or both).
- The ESP32 Arduino core version used (e.g. 3.3.11).
- The MySat board revision tested, if applicable.
- A brief summary of what was verified on hardware.

## Branch workflow

- `main` is the default branch.
- Releases are prepared on `release_X.Y` branches and merged to `main` via PR.
- Use the same pattern for version-bump PRs.
- Keep PRs focused; one logical change per PR when possible.

## Persistent state

Be aware of these storage locations when making changes — do not relocate them silently:
- **LittleFS (ESP32):** `/config.txt` (WiFi), `/callsign.txt`, `/cal.dat` (MPU calibration), `/photo_index.json`, `/logger_state.txt`, mission CSV logs, saved photos.
- **NVS Preferences namespace `bsec_state`:** BSEC sensor state blob.
- **EEPROM addr 0:** servo motor deployed/retracted state.

## Questions

If something is unclear after reading the code and this file, open an issue before starting work.