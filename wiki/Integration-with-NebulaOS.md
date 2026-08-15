# Integration with NebulaOS

NebulaOS doesn't consume GuppyScreen through a separate release/tag/installer flow the way OpenKE
does (see [OpenKE Relationship](OpenKE-Relationship)). Instead,
[`NebulaOS-firmware`](https://github.com/coreflake1/NebulaOS-firmware) pins an exact commit of this
repo and its build pipeline fetches, cross-compiles, and installs it automatically as one stage of
the full OS image — see
[Build From Source](https://github.com/coreflake1/NebulaOS-firmware/wiki/Build-From-Source). You
never need to clone or build this repo directly to build NebulaOS.

Every real NebulaOS build records this repo's exact commit in `build-manifest.txt` — see
[Build Provenance](https://github.com/coreflake1/NebulaOS-firmware/wiki/Build-Provenance).

Also worth knowing: config and theme actually persist now on NebulaOS's read-only-squashfs setup —
see [Config and Theme](Config-and-Theme) for the fix and how we confirmed it on real hardware.

See also: [CI](CI), [Vendored Dependencies](Vendored-Dependencies).
