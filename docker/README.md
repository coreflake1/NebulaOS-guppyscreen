# Vendored cross-compile toolchain

*(Developer doc.)* **Scope note:** everything in this document is about rare, one-off
maintenance/re-vendoring scripts (`scripts/build-{nginx,pillow,streaming-form-data,curl,openrc,
ft2font}-mipsel.sh`) inherited from this repo's OpenKE lineage — they support OpenKE's own
installer, not any current NebulaOS build or CI run. **A normal NebulaOS build or CI run never
touches this directory.** See
[Vendored Dependencies](https://github.com/coreflake1/NebulaOS-guppyscreen/wiki/Vendored-Dependencies)
for the full NEBULAOS_CURRENT vs. OPENKE_SPECIFIC breakdown.

Every `scripts/build-*-mipsel.sh` script (nginx, Pillow, streaming-form-data,
curl, OpenRC, ft2font) pins its Docker image by exact sha256 digest, which is reproducible as long
as that image stays on Docker Hub — but if it ever disappears, the whole chain breaks.
`docker/k1-bash-build/` lets you rebuild that toolchain from scratch, independent of Docker Hub,
the same vendoring philosophy as `scripts/vendor/nginx-src/`, `pillow-src/`, etc.

**Not the upstream maintainer's own Dockerfile source** — pellcorp hasn't published one anywhere
we could find. Reconstructed from `docker history --no-trunc` against the real pinned image, which
(since it was built with BuildKit) preserves the exact `RUN`/`COPY`/`ENV` commands used,
byte-for-byte. **Verified, not just plausible-looking**: built it locally and confirmed
`build-nginx-mipsel.sh`/`build-pillow-mipsel.sh`/etc. behave identically against it.

- **`docker/k1-bash-build/`** — glibc, dynamically-linked. Used by every
  `scripts/build-{nginx,pillow,streaming-form-data,curl,openrc,ft2font}-mipsel.sh` script
  (reconstructs `pellcorp/k1-bash-build`). Bundles `mips-gcc720-glibc229` (a real, versioned
  GitHub release from `ballaswag/k1-discovery` — the Dockerfile downloads it directly, not
  vendored here since it's already stably hosted) plus a device-mirroring sysroot
  (`k1-sysroot-min.tar.gz`, vendored here, ~11MB — glibc runtime headers/libs only, no dev
  userland, see `scripts/build-pillow-mipsel.sh`'s own header comment for what that does and
  doesn't contain).

## Building and using it

```sh
docker build -t openke-k1-bash-build:local docker/k1-bash-build/
# then edit the relevant scripts/build-*-mipsel.sh's K1_BASH_BUILD_IMAGE variable to
# "openke-k1-bash-build:local" instead of the pinned upstream digest, or just retag:
docker tag openke-k1-bash-build:local pellcorp/k1-bash-build@sha256:0b96d1d65175c5a2e3a83a64c3212d08dd774fef0900f991e0ebc570ba896c85
```

Only rebuild this from source if the pinned upstream image ever actually becomes unavailable —
day to day, the pinned digest is simpler and already reproducible.

## GuppyScreen's own toolchain (this is what a normal NebulaOS build actually uses)

GuppyScreen itself (the touchscreen C++ binary) is built with a *different* toolchain — musl,
fully static. **Current state (2026-08-15+, `NEBULAOS_CURRENT`):** both the normal NebulaOS build
(via `NebulaOS-firmware`'s `./build.sh`) and this repo's own CI (`.github/workflows/build.yml`)
cross-compile it inside `NebulaOS-firmware`'s single, digest-pinned
`ghcr.io/coreflake1/nebulaos-build` image — see
[`NebulaOS-firmware`'s Build Environment doc](https://github.com/coreflake1/NebulaOS-firmware/wiki/Build-Environment).

**`HISTORICAL`:** before the Final Closure mission (2026-08-15), this used a separate, standalone
image, `ghcr.io/coreflake1/guppydev` (built from **[`docker/Dockerfile`](../docker/Dockerfile)**,
top-level, not under this directory) — a mutable `:latest` tag. That image is retired; do not treat
any doc that still describes it as CI's current toolchain as accurate. The unified image bundles
the identical Bootlin `mips32el--musl` toolchain `guppydev` provided, downloaded straight from
Bootlin's own permanent, checksum-verified URL, just on a non-default `PATH` entry now rather than
the image's global `PATH`.
