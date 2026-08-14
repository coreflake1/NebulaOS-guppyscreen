# NebulaOS GuppyScreen

**NebulaOS GuppyScreen is the touchscreen UI component of [NebulaOS](https://github.com/coreflake1/NebulaOS)**,
a from-scratch custom OS/firmware for the Creality Ender-3 V3 KE. It's a KE-focused fork of
[GuppyScreen](https://github.com/ballaswag/guppyscreen) — full print control, an interactive 3D bed
mesh, and an on-screen calibration suite, running directly on the printer's display with no X11,
Wayland, or display server.

> **This repository is a NebulaOS component. It is not the OpenKE project.**
> OpenKE is a separate, independently-released project (its own installer, its own version train,
> maintained at [`coreflake1/guppyscreen`](https://github.com/coreflake1/guppyscreen)) that shares an
> author and a common upstream ([`ballaswag/guppyscreen`](https://github.com/ballaswag/guppyscreen))
> with this repo, but is not part of NebulaOS. If you followed a link expecting OpenKE's SSH-installer
> workflow for stock Creality firmware, that's the repo you want.

## Features

- 🖨️ **Print control & status** — temps, fans, LED, movement/homing, file browser (incl. USB sticks), Spoolman
- 🟦 **Interactive 3D bed mesh** — rotate / zoom / pan colour height map (plus a table view)
- 🎯 **Guided Calibration hub** — a single numbered menu: Axis Twist, a combined Z-offset + bed mesh
  Recalibration Wizard (consumes NebulaOS-klipper's `z_compensate` structured status contract), Input
  Shaper, E-Steps Calibration, Skew Correction, TMC Autotune
- 🎚️ **Fine-tune mid-print** — speed, flow, Z-offset, pressure advance (firmware retraction is its own panel)
- 📷 **Camera** — persistent image tuning (contrast/saturation)
- 🔔 **Buzzer beeps & songs** — real-pitch `M300`, `PLAY_TUNE` jingles (editable `songs.conf`), soft touchscreen click
- 🔒 **Print-state safety locks** — anything that could ruin a running job is blocked or asks first
- 📐 Tuned **480×272** layout

## Building

There are two separate workflows depending on what you're trying to do.

### As part of a complete NebulaOS build (do this unless you're specifically developing GuppyScreen)

Use [`NebulaOS-firmware`](https://github.com/coreflake1/NebulaOS-firmware). Its
`manifests/dependencies.conf` pins an exact commit of this repo (`GUPPYSCREEN_PIN`) and its build
pipeline fetches, cross-compiles, and installs it automatically as part of the full OS image — you
never need to clone or build this repo directly.

### Standalone (developing GuppyScreen itself)

```bash
git clone --recurse-submodules https://github.com/coreflake1/NebulaOS-guppyscreen.git
cd NebulaOS-guppyscreen
```

Submodules: `lvgl` (LVGL v8), `lv_drivers`, `libhv`, `spdlog`. `wpa_supplicant` is vendored in-tree.
Full prerequisites, the SDL simulator target, and the MIPS cross-build for the actual printer hardware
are covered in **[Building from Source](wiki/Building-from-Source.md)**.

Offline, host-native logic tests (no cross-compile, no LVGL/SDL2 needed) run with `make test`.

## Compatibility

| | |
|---|---|
| **Printer** | Creality Ender-3 V3 KE, running NebulaOS |
| **SoC / arch** | Ingenic XBurst2 X2000 — MIPS (mipsel) |
| **Display** | 480×272 |

## Recent change: read-only-rootfs config/theme fix

Commit `b15ad7f` fixes `Config::init()`/`ThemeConfig::init()` to actually read real
`config.json`/`theme.json` content on NebulaOS's read-only-squashfs deployment (previously they
silently fell back to in-memory defaults on every boot instead of crashing — no crash, but saved
settings were never honored either). **The physically-qualified golden-reference printer build
(`nebulaos-canonical-baseline-2026-08-14-prtouch-qualified`) predates this fix** — it's part of the
current canonical source state, not yet re-verified on real hardware. See
[`NebulaOS-firmware`'s `manifests/dependencies.conf`](https://github.com/coreflake1/NebulaOS-firmware/blob/main/manifests/dependencies.conf)
for the pin history.

## Documentation

Developer docs for this repo live in [`wiki/`](wiki/) and [`DEVELOPMENT.md`](DEVELOPMENT.md). Much of
`wiki/` (installation, upgrading, troubleshooting) was inherited from this fork's OpenKE-lineage
history and describes OpenKE's own SSH-installer distribution model on stock firmware, not how
NebulaOS builds or deploys this component — for NebulaOS build/install instructions, start at
[`NebulaOS-firmware`](https://github.com/coreflake1/NebulaOS-firmware) instead.

## License & credits

**GPL-3.0** — see [LICENSE](./LICENSE). Builds on
[ballaswag/guppyscreen](https://github.com/ballaswag/guppyscreen),
[probielodan/guppyscreen](https://github.com/probielodan/guppyscreen), and
[pellcorp/grumpyscreen](https://github.com/pellcorp/grumpyscreen), with the 3D bed mesh from
[prestonbrown/guppyscreen](https://github.com/prestonbrown/guppyscreen). Vendored Klipper mods keep
their own upstream licenses and credits.
