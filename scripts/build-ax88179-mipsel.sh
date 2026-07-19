#!/bin/sh
# Cross-compile ax88179_178a + its dependency modules (usbnet, mii, asix) against the KE's
# exact running kernel (4.4.94, vermagic "4.4.94 SMP preempt mod_unload MIPS32_R2 32BIT"),
# for testing USB-Ethernet dongle support as a wired-networking alternative to the KE's
# WiFi (see docs/ or the project's own network-reliability notes for why this was worth
# trying). Writes the built .ko files to scripts/vendor/modules/, ready to scp onto the
# real device and insmod (session-only - add a /module_driver/-style init script only
# after confirming it actually works).
#
# Uses the same pellcorp/k1-bash-build Docker image (pinned by digest below) as
# scripts/build-nginx-mipsel.sh / scripts/build-pillow-mipsel.sh - bundles Ingenic's own
# mips-gcc720-glibc229 toolchain, confirmed via its version banner ("Ingenic
# Linux-Release5.0.2-Default(xburst2(fp64)+glibc2.29)") to be the real vendor toolchain
# for this exact chip family, not just "a MIPS toolchain that happens to work."
#
# ============================================================================
# WHY THIS SCRIPT EXISTS: this is a kernel MODULE build (loads into the KE's existing,
# currently-running 4.4.94 kernel), not a full kernel/rootfs build - much smaller in scope
# than the nginx/Pillow/streaming-form-data builds, but with its own real gotchas:
# ============================================================================
#
# GOTCHA #1: the default container user (`developer`) can't `apt-get install` - use
# `docker run --user root`.
#
# GOTCHA #2: `bc` isn't in the image by default and is required by
# `include/generated/timeconst.h` during kernel module builds - install it inline
# (`apt-get install -y bc`) every container invocation (fresh container each time, nothing
# persists).
#
# GOTCHA #3: a pre-existing, unrelated, non-fatal Kconfig bug in this vendor kernel tree
# (`drivers/net/wireless/bcmdhd/Kconfig:29: error: recursive dependency detected!`) fires
# on every config step (`make defconfig`, `make olddefconfig`) - the `.config` is still
# written correctly regardless. Ignore the noise; don't chase it.
#
# GOTCHA #4: this vendor tree's own `x2000_module_base_linux_mmc2_defconfig` doesn't
# enable the ax88179 driver by default - flip it on via `scripts/config --module` (which
# also pulls in its real dependencies USB_USBNET and MII), then `make olddefconfig` to
# resolve the rest cleanly, rather than hand-editing .config directly (Kconfig's own
# dependency resolution needs to run for the enabled symbols to actually build correctly -
# see the ke-mainline-klipper project's own writeup of a related class of bug if you ever
# see a symbol silently vanish after a raw edit).
#
# GOTCHA #5: the built `usb` subdirectory's module build may emit a "Symbol version dump
# ./Module.symvers is missing" warning not seen on the `drivers/net` build - not
# investigated further since CONFIG_MODVERSIONS is confirmed off on both the build and the
# real device (so no per-symbol CRC check applies, only the coarse vermagic string
# matters) - but worth a first look if `insmod` unexpectedly reports a version mismatch
# despite the vermagic strings matching exactly.
# ============================================================================
#
# The real, load-bearing verification step: vermagic MUST read exactly
# "4.4.94 SMP preempt mod_unload MIPS32_R2 32BIT" on every built .ko - this is the device's
# own real vermagic (extracted via `strings` on one of its existing loaded modules over
# SSH), and since CONFIG_MODVERSIONS is off, this coarse string match is the only
# compatibility gate `insmod` actually checks.

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$SCRIPT_DIR/vendor/modules"
KERNEL_SRC="$SCRIPT_DIR/vendor/x2000_kernel-src"
KERNEL_REPO="https://github.com/Jubian540/x2000_kernel.git"
KERNEL_REF="7f14bc69e3125a92abf88b6e9525df405e1cd0e0"
K1_BASH_BUILD_IMAGE="pellcorp/k1-bash-build@sha256:0b96d1d65175c5a2e3a83a64c3212d08dd774fef0900f991e0ebc570ba896c85"

if ! command -v docker >/dev/null 2>&1; then
	echo "docker is required." >&2
	exit 1
fi
mkdir -p "$OUT_DIR"

# ----------------------------------------------------------------------------
# Step 1: fetch the exact-version-matching kernel source (found via GitHub code
# search for exact strings pulled from the live device - x2000_module_base,
# halley5_v20 - see the ke-mainline-klipper project's FIRMWARE.md sec 4a for
# the full provenance story). Not an Ingenic-sanctioned release - an unofficial
# community mirror of an internal SDK drop, GPLv2 so redistribution isn't a
# legal problem, but treat accordingly (kept local, not republished further).
# ----------------------------------------------------------------------------
if [ ! -d "$KERNEL_SRC/.git" ]; then
	echo "=== cloning kernel source (exact 4.4.94 vermagic match) ==="
	git clone "$KERNEL_REPO" "$KERNEL_SRC"
	git -C "$KERNEL_SRC" checkout "$KERNEL_REF"
else
	echo "=== kernel source already present, skipping clone ==="
fi

# ----------------------------------------------------------------------------
# Step 2: configure + build the four modules inside the container.
# ----------------------------------------------------------------------------
echo "=== building ax88179_178a + usbnet + mii + asix ==="
docker run --rm --user root -v "$KERNEL_SRC:/src" -w /src "$K1_BASH_BUILD_IMAGE" sh -c '
	set -e
	apt-get update >/dev/null 2>&1
	apt-get install -y -qq bc >/dev/null 2>&1
	export ARCH=mips
	export CROSS_COMPILE=mips-linux-gnu-
	export PATH=/opt/toolchains/mips-gcc720-glibc229/bin:$PATH
	make x2000_module_base_linux_mmc2_defconfig
	./scripts/config --module CONFIG_USB_USBNET
	./scripts/config --module CONFIG_USB_NET_AX88179_178A
	./scripts/config --module CONFIG_MII
	make olddefconfig
	make -j$(nproc) modules_prepare
	make -j$(nproc) M=drivers/net modules
	make -j$(nproc) M=drivers/net/usb modules
'

# ----------------------------------------------------------------------------
# Step 3: verify vermagic (the only real compatibility gate here) before
# copying anywhere - fail loudly if it doesn't match exactly.
# ----------------------------------------------------------------------------
echo "=== verifying vermagic ==="
EXPECTED="vermagic=4.4.94 SMP preempt mod_unload MIPS32_R2 32BIT"
FAIL=0
for f in \
	"$KERNEL_SRC/drivers/net/mii.ko" \
	"$KERNEL_SRC/drivers/net/usb/ax88179_178a.ko" \
	"$KERNEL_SRC/drivers/net/usb/usbnet.ko" \
	"$KERNEL_SRC/drivers/net/usb/asix.ko"; do
	got=$(strings "$f" | grep "^vermagic=" || true)
	if [ "$got" != "$EXPECTED" ]; then
		echo "MISMATCH in $f: got '$got', expected '$EXPECTED'" >&2
		FAIL=1
	else
		echo "OK: $(basename "$f")"
	fi
done
[ "$FAIL" -eq 0 ] || { echo "vermagic verification FAILED - do not insmod these" >&2; exit 1; }

cp "$KERNEL_SRC/drivers/net/mii.ko" \
   "$KERNEL_SRC/drivers/net/usb/ax88179_178a.ko" \
   "$KERNEL_SRC/drivers/net/usb/usbnet.ko" \
   "$KERNEL_SRC/drivers/net/usb/asix.ko" \
   "$OUT_DIR/"

echo "=== Done ==="
echo "Wrote $OUT_DIR/{mii,ax88179_178a,usbnet,asix}.ko"
sha256sum "$OUT_DIR"/*.ko
echo
echo "Next (needs the real printer, confirmed idle via a FRESH print_stats check - do not"
echo "trust an old check): scp these four files to the device, then load in dependency"
echo "order: insmod mii.ko && insmod usbnet.ko && insmod asix.ko && insmod ax88179_178a.ko"
echo "- then check dmesg/ip link for a new interface. Session-only until confirmed working;"
echo "only add a /module_driver/*.sh-style init script for persistence after that."
