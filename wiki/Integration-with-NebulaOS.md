# Integration with NebulaOS

*(Developer doc, `NEBULAOS_CURRENT`.)* NebulaOS does not consume GuppyScreen through a separate
release/tag/installer flow the way OpenKE does (see [OpenKE Relationship](OpenKE-Relationship)).
Instead, [`NebulaOS-firmware`](https://github.com/coreflake1/NebulaOS-firmware) pins an exact commit
of this repo (`GUPPYSCREEN_PIN` in `manifests/dependencies.conf`), and its build pipeline fetches,
cross-compiles, and installs it automatically as one stage of the full OS image — see
[Build From Source](https://github.com/coreflake1/NebulaOS-firmware/wiki/Build-From-Source). You
never need to clone or build this repo directly to build NebulaOS.

Every real NebulaOS build records this repo's exact commit (`git_commit_guppyscreen` in
`build-manifest.txt`) — see
[Build Provenance](https://github.com/coreflake1/NebulaOS-firmware/wiki/Build-Provenance).

**Config/theme persistence on NebulaOS's read-only-squashfs deployment:** commit `b15ad7f` fixed
`Config::init()`/`ThemeConfig::init()` to actually read real `config.json`/`theme.json` on boot
(previously silently fell back to in-memory defaults every boot — no crash, but saved settings were
never honored). **`LIVE_HARDWARE_VERIFIED`** — GuppyScreen config/theme persistence across a real
flash was explicitly confirmed during the Final Closure mission (2026-08-15). See
[Config and Theme](Config-and-Theme).

See also: [CI](CI), [Vendored Dependencies](Vendored-Dependencies).
