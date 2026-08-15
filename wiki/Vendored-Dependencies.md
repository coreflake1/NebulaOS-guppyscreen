# Vendored dependencies

*(Developer doc.)* Two genuinely different things share the word "vendored" in this repo — don't
conflate them.

## GuppyScreen's own build submodules (`NEBULAOS_CURRENT`)

`lvgl` (LVGL v8), `lv_drivers`, `libhv`, `spdlog` — real git submodules, fetched by
`git clone --recurse-submodules`. `wpa_supplicant` is vendored in-tree. These are used by every
build, standalone or as part of NebulaOS — see
[Building from Source](https://github.com/coreflake1/NebulaOS-firmware/wiki/Build-From-Source).

## `docker/k1-bash-build/` toolchain reconstruction and `scripts/build-*-mipsel.sh` (`OPENKE_SPECIFIC` — not a normal-build dependency)

`docker/k1-bash-build/` and the `scripts/build-{nginx,pillow,streaming-form-data,curl,openrc,
ft2font}-mipsel.sh` maintenance scripts, plus `docs/VENDORING.md`'s vendored-Klipper-mods system
(`k1/k1_mods/`, `scripts/installer.sh`) — these are all inherited from this repo's OpenKE lineage.
They support OpenKE's own SSH installer on stock Creality firmware, rebuilding pieces of its
toolchain and dependency chain from scratch if an upstream image ever disappears.

**A normal NebulaOS build or NebulaOS-guppyscreen CI run never touches any of this.** These are
rare, one-off maintenance/re-vendoring scripts carried over in the repo tree, not part of the
current build or deployment path for NebulaOS. See `docker/README.md` and `docs/VENDORING.md` in
this repo for the full detail if you're specifically maintaining OpenKE's own installer chain.

See also: [OpenKE Relationship](OpenKE-Relationship), [CI](CI).
