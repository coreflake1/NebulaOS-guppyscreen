# CI

*(Developer doc, `NEBULAOS_CURRENT`.)* `.github/workflows/build.yml` cross-compiles the Ender-3 V3 KE
asset (`mipsel-buildroot-linux-musl-`, material theme, small screen) on pushes to `main`/`develop`/
`ke-next` and on tags.

**Toolchain (since the Final Closure mission, 2026-08-15):** runs inside
`ghcr.io/coreflake1/nebulaos-build@sha256:a6ba57c69fa1ea630b037a1d1f55cf0c044a7f5a403bde9b155ea54bca1cceba`
— the same digest-pinned image `NebulaOS-firmware`'s own `build.sh` uses, not the retired standalone
`ghcr.io/coreflake1/guppydev:latest`. Confirmed both toolchain prefixes the workflow's matrix expects
resolve to real binaries in the new image. Two real gotchas fixed during that migration, both
documented directly in the workflow file's own comments:
- An arbitrary container UID has no `/etc/passwd` entry in this image, so `HOME` defaults to `/`
  (unwritable) — set explicitly to `/tmp`.
- The image deliberately does **not** put the Bootlin toolchain on the global `PATH` (its bundled
  `autoreconf`/`automake` would shadow the system ones a v4l-utils-style autotools build needs) —
  unlike `guppydev`, which did put it on `PATH` directly.

There is also a separate `.github/workflows/wiki.yml`, which auto-syncs the `wiki/` folder to this
repo's own GitHub Wiki on every push to `main` that touches `wiki/**` — see
[Wiki Publishing](Wiki-Publishing) for how that works and a real failure it hit
(2026-08-14, fixed 2026-08-15) from the wiki never having had a first page.

See also: [Integration with NebulaOS](Integration-with-NebulaOS), and
[NebulaOS-firmware's Build Environment doc](https://github.com/coreflake1/NebulaOS-firmware/wiki/Build-Environment)
for what the shared image actually contains.
