# Cross-project contract fixtures

`z_compensate_status_contract.json` is a **literal copy** of
`ke-mainline-klipper/docs/z_compensate_status_contract_fixture.json` - not hand-typed here,
generated there by driving the real `ZCompensate` Python class through
`klippy_extras/test_z_compensate_contract_fixture.py`
(`python3 -m unittest klippy_extras.test_z_compensate_contract_fixture -v`) and serializing
its actual `get_status()` output at each of the four canonical states.

Kept in sync manually - the contract is small and stable (four fields, four states), not
worth a cross-repo code-generation pipeline. If the contract ever changes on the Klipper
side, regenerate the fixture there first, then copy it here verbatim.

Consumed by `tests/test_contract_fixture.cpp`, which parses this exact file through the
real `parse_z_compensate_status()`/`ZCompensateStatusTracker` C++ code - the same
byte-for-byte JSON both sides of the wire are meant to agree on.
