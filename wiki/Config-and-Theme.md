# Config and theme

`Config::init()`/`ThemeConfig::init()` read `config.json`/`theme.json` from disk on startup. On
NebulaOS's read-only-squashfs setup, that means reading from the writable overlay, not the squashfs
image itself — settings saved from the UI stick around across reboots and across an A/B slot switch
(see [A/B Slot Model](https://github.com/coreflake1/NebulaOS-firmware/wiki/A-B-Slot-Model) for how
that storage is shared).

Commit `b15ad7f` fixed a real bug here: before it, both `init()` functions silently fell back to
in-memory defaults on every boot instead of actually reading your saved files. No crash, so it
went unnoticed for a while, but your settings never actually stuck. We've since confirmed on real
hardware, across a real flash, that config and theme now persist properly.

See also: [Integration with NebulaOS](Integration-with-NebulaOS), and this repo's own
[Configuration](Configuration) page for the user-facing config options.
