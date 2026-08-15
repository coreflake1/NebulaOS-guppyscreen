# Testing

*(Developer doc, `NEBULAOS_CURRENT`.)* Offline, host-native logic tests — no cross-compile, no
LVGL/SDL2 needed — run with:

```sh
make test
```

Covers: `test-z-compensate-status`, `test-z-offset-persistence`, `test-contract-fixture`,
`test-integration-harness`, `test-subscription-baseline-ordering`, `test-config-theme-parse-safety`
(`tests/*.cpp`, using `tests/minitest.h`).

`test_z_compensate_status.cpp` covers the structured-status contract the Recalibration Wizard
consumes from `NebulaOS-klipper`'s `z_compensate` module (structured status via Klipper's
object-status/webhooks mechanism, not parsed gcode text) — see
[NebulaOS-klipper's Z Compensation page](https://github.com/coreflake1/NebulaOS-klipper/wiki/Z-Compensation).

`test_config_theme_parse_safety.cpp` covers the `Config::init()`/`ThemeConfig::init()` path fixed
in `b15ad7f` — see [Config and Theme](Config-and-Theme).

For interactive/visual testing, build the SDL simulator target (see
[Development and Simulator](Development-and-Simulator)) rather than round-tripping to real
hardware for every UI change; real-hardware qualification is still the final gate before anything
ships — see [Integration with NebulaOS](Integration-with-NebulaOS).
