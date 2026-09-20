# MySatKit-Firmware Feature Roadmap

Living document describing implemented functionality and candidate features per subsystem. Features are tiered by value/effort to guide development prioritization.

## Current state

| Subsystem | Implemented | Biggest gap |
|---|---|---|
| **C&DH** (ESP32-CAM) | Telemetry (text/plotter/debug), CSV mission logging, event log, Web GUI, WiFi STA, photo capture + storage | No HTTP time-setting, no OTA, no AP fallback, no I2C reply from ATmega |
| **C&DH** (ATmega328P) | I2C slave at 0x08, servo deploy/retract state machine, HC-12 power-on, LED heartbeat | RF_TURN/RF_SET command handlers are empty stubs, no I2C reply path |
| **COM** (HC-12) | Power on/off via ATmega | Almost no functionality — no telemetry downlink, no command uplink, no AT configuration |
| **EPS** | INA3221 voltage/current for battery + 2 solar panels, CSV/text/plotter output | No SoC estimation, no low-battery alarm, solar L/R current missing from JSON/Web GUI |
| **ADCS** | MPU9250 gyro/accel complementary filter, ADS1015 photosensors, gyro-bias calibration, Web GUI sun-tracker SVG | No magnetometer (yaw drifts), no sun-pointing algorithm, no control loop |
| **Payload** | BME680 (BSEC IAQ), OV2640 XGA still capture with LittleFS storage (10 rotating photos) | No periodic/timed capture, no thumbnails, BSEC pressure not sea-level compensated |
| **Debug** | Star LED on/off/blink, Signal LED WiFi status | Signal LED underused for diagnostics, likely blink-resolution bug (5s polling) |

## Telemetry paths

| Field | Text | Plotter | CSV | JSON `/get_data` | Web GUI |
|---|---|---|---|---|---|
| BME temp/gas/pres/iaq/acc | Yes | Yes (env) | Yes | Yes | Yes |
| BME humidity | No | Yes (env) | Yes | Yes | No |
| MPU roll/pitch/yaw | Yes | Yes (pos) | Yes | Yes | Yes |
| ADS ph1-4 | Yes | Yes (sun) | Yes | Yes | Yes (SVG) |
| INA battery V/I | Yes | Yes (pwr) | Yes | Yes | Yes (V) |
| INA solar V | Yes | Yes (pwr) | Yes | Yes | Yes (bar) |
| INA solar L/R I | Yes | Yes (pwr) | Yes | No | No |
| RTC datetime | Yes | No | Yes | Yes | Yes |
| motor_state | Yes | No | No | Yes | Yes (cube wings) |
| callSign | Yes (header) | No | No | Yes | Yes (header) |
| camera_ready | No | No | No | Yes | Yes (button enable) |
| logging_state | Yes | No | No | Yes | Yes (dot) |

## Event log types

Currently emitted: `BOOT`, `SHUTDOWN` (inferred from last NVS heartbeat on next boot), `COMMAND_RECEIVED "<cmd>"` (every recognized serial command), `PHOTO <id>` (each HTTP photo capture).

No sensor-threshold, power, or radio events are logged yet.

Candidate event types from Tier 0: `MQTT_CONNECT`, `MQTT_DISCONNECT`, `COMMAND_RECEIVED "HTTP: <cmd>"`, `LOW_POWER_MODE_ENTERED`, `LOW_POWER_MODE_EXITED`, `MODE_CHANGE <old> -> <new> <reason>`, `WATCHDOG_REBOOT`, `TELEMETRY_BUFFERING_STARTED`, `TELEMETRY_BUFFER_FLUSHED <count>`, `OTA_UPDATE_STARTED`, `OTA_UPDATE_COMPLETE <version>`, `OTA_UPDATE_FAILED <reason>`.

---

## Tier 1 — Highest value, largest functional gap

### A. HC-12 radio beacon (telemetry downlink)

Send regular telemetry packets over HC-12: callsign + battery voltage + attitude + IAQ + timestamp.

**Requirements:**
- Implement ATmega-side HC-12 Serial bridge (fill `RF_TURN`/`RF_SET` handlers, establish UART bridge to HC-12)
- Telemetry framing on ESP32 (compact packet format with callsign + key fields)
- Interval-based sending (configurable period)
- Event log entry on each beacon send

**Value:** Makes the satellite an actual radio satellite — the single biggest missing functionality in the system.

### B. HC-12 command uplink (remote control over radio)

Receive commands over HC-12 (e.g. "DEPLOY", "PHOTO", "LED", "CALIBRATE") and dispatch them through the existing `handleCommands()` pipeline.

**Requirements:**
- HC-12 Serial bridge on ATmega (shared with feature A)
- Command parsing on ESP32 (reuse `handleCommands()` logic)
- Event log entries for radio-received commands
- Optional: ACK back to sender

**Value:** Enables controlling the satellite without WiFi — true satellite operation.

### C. Signal LED diagnostic patterns

Extend Signal LED to indicate: sensor-missing, BME warmup, low battery, logging active, radio fault, calibrate-in-progress.

**Requirements:**
- Extend `evaluateSystemState()` in `control.h` with multi-condition priority
- Fix blink-resolution bug: move `updateSignalLed()` from 5s `checkSystemState()` to every `loop()` iteration
- Define color/pattern vocabulary (red=battery, yellow=sensor, cyan=radio, etc.)

**Value:** Small effort, large visual debug value during hardware testing.

---

## Tier 2 — Medium value, moderate effort

### D. EPS: Battery SoC + low-battery alarm

Estimate state-of-charge from voltage (Li-ion discharge curve), generate low-battery event at <20%, turn Signal LED red.

**Requirements:**
- SoC estimation function in `power_measure.h` (voltage→percentage lookup table)
- Threshold logic in `checkSystemState()`
- Event log entry (`BATTERY_LOW`)
- New JSON field `battery_soc` in `/get_data`
- Web GUI battery panel update

**Value:** Safety-critical for autonomous operation.

### E. EPS: Solar panel L/R current in Web GUI

Add `solar_i_l` / `solar_i_r` to `/get_data` JSON and update the Web GUI solar bar to show per-panel current.

**Requirements:**
- Two fields in `generateSensorsDataJson` (`server.h`)
- JS update in `htmlContent` solar panel section

**Value:** Small effort, fills an obvious data gap.

### F. ADCS: Magnetometer (AK8963)

Read the MPU9250's onboard AK8963 magnetometer to correct yaw drift.

**Requirements:**
- AK8963 register access in `position_sensor.h` (sensitivity adjust, raw to uT)
- Magnetometer calibration routine (hard/soft iron)
- Fuse magnetometer heading with gyro yaw in the complementary filter
- Optional: Web GUI compass visualization

**Value:** Solves the yaw-drift problem, gives true 3D attitude.

### G. C&DH: I2C reply from ATmega

Register `Wire.onRequest` on ATmega so ESP32 can query motor status, HC-12 status, and command ACK.

**Requirements:**
- Status struct on ATmega (motor state, current command, RF state)
- `Wire.onRequest` handler returning packed struct
- I2C read on ESP32-side (`Wire.requestFrom`)
- Expose status in telemetry JSON and Web GUI

**Value:** Closes the feedback loop — ESP32 knows whether commands were executed.

---

## Tier 3 — Nice-to-have, lower priority

### H. Payload: Periodic/timed photo capture

Add `StartPhoto`/`StopPhoto` serial commands with configurable interval (e.g. every 60s).

**Requirements:**
- State in logger config (or separate `photo_state.txt`)
- Interval logic in `loop()`
- Event log entries
- Web GUI indicator for auto-capture mode

**Value:** Autonomous image-sat operation.

### I. C&DH: HTTP time-setting endpoint

Add `/set_time` endpoint with a datetime-picker in the Web GUI.

**Requirements:**
- HTTP POST route in `initServer()`
- HTML form / JS datetime picker in `htmlContent`
- Call existing `setTime()` logic
- Event log entry

**Value:** Avoids needing the serial monitor for RTC sync.

### J. C&DH: WiFi AP fallback

Start AP mode with captive portal if STA connection fails after N retries.

**Requirements:**
- `WiFi.softAP` + DNS server for captive portal
- AP-side Web GUI (same `htmlContent`)
- Config flow to enter STA credentials via Web GUI
- Event log entry

**Value:** Makes the Web GUI accessible even without a router.

### K. ADCS: Sun-pointing alignment indicator

Compute sun direction from ph1-4, compare with attitude, show "alignment score" in Web GUI.

**Requirements:**
- Vector math from photosensor data (4-quadrant → azimuth/elevation)
- Compare with MPU attitude
- Web GUI UI element (gauge or percentage)

**Value:** Pedagogical visualization of the ADCS concept.

### L. COM: HC-12 AT configuration workflow

Implement `SetRadio` to switch channel/baud/power via AT commands.

**Requirements:**
- Fill ATmega `RF_SET` handler (enter AT mode, pass through AT commands from ESP32)
- AT command sequence from ESP32 (channel, baud, power, UART rate)
- Serial prompt on ESP32 for configuration
- Persist radio config to LittleFS
- Event log entry

**Value:** Makes the radio configurable without removing the HC-12 module.

---

## Tier 0 — Ground station integration (FleetEmulator / Heretek)

Features required for integration with the K8s satellite fleet simulator. The ESP32-CAM appears as a "real" satellite alongside simulated digital twins, with telemetry flowing through the same MQTT → Telegraf → InfluxDB → Grafana pipeline.

### M. MQTT telemetry publishing

Publish telemetry as JSON to MQTT topic `telemetry/{sat_id}` in the same format as simulated digital twins, so downstream systems cannot distinguish real from simulated.

**Requirements:**
- Add `PubSubClient` library dependency
- MQTT client in `server.h` or new `mqtt.h`, connecting to broker configured via `SetWIFI` or separate `SetMQTT` command
- `generateSensorsDataJson()` extended with `"type": "real"` marker and `sat_id` field (configurable via `SetCallSign` or new `SetSatId` command)
- Publishing interval aligned with existing telemetry loop (default 5s, configurable)
- Auto-reconnect on WiFi/MQTT disconnect
- Event log entry on connect/disconnect

**Value:** Unifies real and simulated telemetry — the satellite appears in Grafana alongside the 50 digital twins with a `type=real` label.

### N. HTTP command endpoint

Add `/command` POST endpoint so the K8s ground station bridge can send commands via HTTP (WiFi path), complementing the HC-12 uplink (radio path, feature B).

**Requirements:**
- HTTP POST route `/command` in `initServer()`, accepting JSON body `{"cmd": "PHOTO", "params": {...}}`
- Dispatch through existing `handleCommands()` pipeline
- HTTP 200 response with command result/ACK
- Event log entry (`COMMAND_RECEIVED "HTTP: <cmd>"`)
- Reuse parsing logic from serial command handler

**Value:** Enables the ground station bridge pod to control the satellite via WiFi without serial or radio.

### O. HTTP health endpoints for K8s probes

Add `/health`, `/ready`, `/started` endpoints returning HTTP 200 when the satellite is operational. Used by Kubernetes liveness/readiness probes in the ground station bridge deployment.

**Requirements:**
- `/health` — returns 200 if ESP32-CAM is running and WiFi is connected, 503 otherwise
- `/ready` — returns 200 if sensors are initialized and MQTT broker is reachable, 503 otherwise
- `/started` — returns 200 after initial setup (sensor init, WiFi connect, RTC sync) completes
- Minimal response body (e.g. `{"status": "ok"}`) to keep payload small

**Value:** Enables K8s to detect contact loss and simulate reboot via pod restart.

### P. Adaptive telemetry rate

Reduce telemetry publishing frequency when battery is low or the satellite is in eclipse (no solar input), matching the `ENTER_LOW_POWER` behavior of simulated twins.

**Requirements:**
- Telemetry interval configurable at runtime (already partially via `SwitchTelemetry`)
- Automatic interval increase when `battery_soc` < 20% (requires feature D) or solar panel voltage below threshold
- Event log entry (`LOW_POWER_MODE_ENTERED`, `LOW_POWER_MODE_EXITED`)
- RESTORE normal rate when battery recovers
- MQTT publishing rate follows the same interval

**Value:** Extends battery lifetime during eclipse — mirrors the digital twin power management logic.

### Q. Mode manager (state machine)

Implement a top-level mode manager with discrete states: `NOMINAL`, `SAFE`, `LOW_POWER`, `EMERGENCY`. States drive subsystem behavior and are reported in telemetry.

**Requirements:**
- Mode enum and transition logic in `control.h` (extends `evaluateSystemState()`)
- State transitions triggered by: battery thresholds (D), sensor failures, radio faults, manual command
- Each mode defines: which sensors are active, telemetry rate, camera availability, LED pattern (C)
- Mode reported in `/get_data` JSON as `mode` field
- Event log entries on every mode transition (`MODE_CHANGE <old> -> <new> <reason>`)
- MQTT topic `status/{sat_id}` for mode announcements

**Value:** Gives the satellite autonomous behavior — it can survive without ground intervention, matching the digital twin probe-based lifecycle simulation.

### R. Watchdog timer

Enable the ESP32 hardware watchdog so a firmware freeze triggers an automatic reboot, simulating a satellite power cycle.

**Requirements:**
- `esp_task_wdt_init()` in `setup()` with configurable timeout (default 30s)
- `esp_task_wdt_reset()` in `loop()`
- On reboot, event log records `WATCHDOG_REBOOT` (inferred from NVS on next boot, same pattern as `SHUTDOWN`)
- Post-reboot: restore mode to `SAFE` (not `NOMINAL`) until ground station confirms `NOMINAL`

**Value:** Prevents permanent loss of satellite due to software freeze — essential for autonomous operation.

### S. Store-and-forward telemetry buffering

Buffer telemetry packets to LittleFS when MQTT broker is unreachable, and flush the buffer when connection is restored.

**Requirements:**
- Ring buffer on LittleFS (reuse `data_logger.h` infrastructure)
- Max buffer size configurable (default 100 packets ~ 50KB)
- On MQTT reconnect: flush buffer in order, then resume live publishing
- Event log entries (`TELEMETRY_BUFFERING_STARTED`, `TELEMETRY_BUFFER_FLUSHED <count>`)
- Web GUI indicator for buffering state

**Value:** Prevents telemetry gaps during WiFi outages — the satellite never silently drops data.

### T. OTA firmware update

Enable over-the-air firmware updates via HTTP so the ground station can push new firmware without physical access.

**Requirements:**
- `Update` library (ESP32 core) + HTTP endpoint `/update` (POST, multipart or raw binary)
- Firmware signature verification (optional, HMAC-SHA256)
- Event log entry (`OTA_UPDATE_STARTED`, `OTA_UPDATE_COMPLETE <version>`, `OTA_UPDATE_FAILED <reason>`)
- On reboot after update: report new version in `/get_data` and event log
- Web GUI button for manual upload

**Value:** Enables remote firmware rollouts to the satellite fleet — mirrors the K8s rolling update pattern for digital twins.

---

## Cross-cutting notes

### Known bugs / inconsistencies

- `updateSignalLed()` is called only every 5s via `checkSystemState()`, but blink intervals are 800/200ms — blink is effectively broken (feature C fixes this).
- `initINA()` in `power_measure.h` always returns `true` — no real INA3221 presence detection; a missing sensor won't be flagged.
- `get_photo()` in `camera.h:213` is an empty stub — dead code.
- Humidity is collected and in CSV/JSON/plotter but not displayed in text-mode telemetry or Web GUI.
- `index.html` is a stale standalone prototype (per AGENTS.md); `htmlContent` in `server.h` is the live GUI.
- Solar L/R currents are in text/plotter/CSV but omitted from `/get_data` JSON (feature E fixes this).

### Serial commands (current set)

`ChangeTime`, `SetWIFI`, `TurnLed`, `SolarDeploy`, `SolarRetract`, `SolarMove`, `Calibrate`, `TurnConsole`, `SetCallSign`, `SwitchTelemetry`, `SelectPlotterMode`, `DebugModeOn`, `DebugModeOff`, `SetRadio`, `StartLogging`, `StopLogging`, `AuditFileSystem`, `ListLogFiles`, `BlinkLed`, `SendEventLog`, `DeleteLogging`.

Candidate serial commands from Tier 0: `SetMQTT` (broker IP/port), `SetSatId` (satellite identifier), `SetMode` (manual mode override), `StartOTA` / `StopOTA`.

### HTTP routes (current set)

`/`, `/get_data`, `/light_on`, `/motor_on`, `/get_photo`, `/get_photo_list`, `/get_photo_by_id`, `/get_log_list`, `/download_log`, `/download_event_log`. Unknown paths fall through to LittleFS static file serving.

Candidate HTTP routes from Tier 0: `/command` (POST — command uplink), `/health` (GET — K8s liveness), `/ready` (GET — K8s readiness), `/started` (GET — startup complete), `/update` (POST — OTA firmware).