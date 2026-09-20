# Merge request template for MySatKit-Firmware
# Used by the code-reviewer agent and human reviewers on git.smedjen.org (GitLab).
#
# Fill in every section. Mark the hardware-test checklist items that were
# actually performed. Do not remove sections — write "N/A" with a reason if
# something does not apply.

## Summary

<!-- One or two sentences: what does this MR change and why? -->

## Board(s) affected

- [ ] ESP32-CAM (`ino/MySat_main/`)
- [ ] ATmega328P (`ino/MySat_Nano_ATmega328p/`)
- [ ] Both (I2C protocol or shared change)

<!-- If both boards are touched, explain whether the change is intentional
     on both sides or whether one side is an accidental cross-board impact. -->

## Motivation

<!-- Why is this change needed? Link to issue, ROADMAP.md feature tier,
     or upstream PR if applicable. -->

## What changed

<!-- Bullet list of the logical changes. One MR should be one logical
     change when possible (CONTRIBUTING.md: "Keep PRs focused"). -->

-

## I2C protocol

<!-- If the ESP32-CAM <-> ATmega328P I2C protocol (slave address 0x08)
     changes, describe the new message format and confirm the matching
     handler exists on both boards. Otherwise write "Unchanged". -->

## Version strings

<!-- If this is a version bump, confirm all three are updated consistently
     per CONTRIBUTING.md and the +kartana suffix is applied/dropped:
       - FIRMWARE_VERSION in ino/MySat_main/console.h
       - VERSION in ino/MySat_Nano_ATmega328p/MySat_Nano_ATmega328p.ino
       - README release notes block
     Otherwise write "N/A — no version bump". -->

## Persistent state

<!-- If any persistent state is touched or relocated, call it out:
     LittleFS (/config.txt, /callsign.txt, /cal.dat, /photo_index.json,
     /logger_state.txt, mission CSV logs, saved photos),
     NVS Preferences namespace 'bsec_state', EEPROM addr 0.
     Otherwise write "Unchanged". -->

## Static inspection (build-test-runner)

<!-- Paste the summary from the build-test-runner agent, or link to the
     run. The MR is not ready to merge until this is clean. -->

- [ ] build-test-runner report attached and clean

## Code review (code-reviewer)

<!-- Paste the verdict from the code-reviewer agent, or link to the run.
     REQUEST CHANGES or BLOCK must be resolved before merge. -->

- [ ] code-reviewer verdict: APPROVE

## Hardware test

<!-- CONTRIBUTING.md requires hardware verification. Fill in the checklist. -->

- [ ] Board(s) flashed: ESP32-CAM / ATmega328P / both
- [ ] ESP32 Arduino core version: <!-- e.g. 3.3.11 -->
- [ ] Serial monitor exercised at 115200 baud
- [ ] Serial commands touched by this change verified
- [ ] Web GUI endpoints touched by this change verified
- [ ] Persistent state (LittleFS / NVS / EEPROM) verified intact
- [ ] MySat board revision tested: <!-- e.g. v1.5.6, if applicable -->

### What was verified on hardware

<!-- Brief summary: which commands/endpoints/peripherals were exercised
     and what the observed behavior was. -->

## Breaking changes / migration notes

<!-- If existing configs, LittleFS files, NVS state, or I2C protocol
     require migration, describe the steps. Otherwise write "None". -->

## Upstream contribution

<!-- If this change is intended to go back to MySatKit/MySatKit-Firmware
     upstream, confirm the +kartana suffix is dropped from version strings
     and any fork-specific assumptions are removed. Otherwise write
     "N/A — fork-only change". -->