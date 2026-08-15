# Config and theme

*(Developer doc, `NEBULAOS_CURRENT`.)* `Config::init()`/`ThemeConfig::init()` read `config.json`/
`theme.json` from disk on startup. On NebulaOS's read-only-squashfs deployment, that means reading
from the writable overlay path, not the squashfs image itself — settings saved from the UI persist
there across reboots and across an A/B slot switch (see
[A/B Slot Model](https://github.com/coreflake1/NebulaOS-firmware/wiki/A-B-Slot-Model) for the
shared, non-duplicated storage these live on).

**Commit `b15ad7f`** fixed a real bug in this path: before it, both `init()` functions silently fell
back to in-memory defaults on every boot instead of actually reading the saved files — no crash, so
it went unnoticed, but saved settings were never honored. **`LIVE_HARDWARE_VERIFIED`** — config/theme
persistence across a real flash was explicitly confirmed during the Final Closure mission
(2026-08-15), which post-dates and covers this fix.

See also: [Integration with NebulaOS](Integration-with-NebulaOS), and this repo's own
[Configuration](Configuration) page for user-facing config options.
