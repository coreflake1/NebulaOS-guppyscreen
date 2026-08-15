# OpenKE relationship

**This repository (`NebulaOS-guppyscreen`) is a NebulaOS component. It is not the OpenKE project.**

[OpenKE](https://github.com/coreflake1/guppyscreen) is a separate, independently-released project
(its own SSH installer, its own version train, its own `-OpenKE` release tags) that shares an
author and a common upstream ([`ballaswag/guppyscreen`](https://github.com/ballaswag/guppyscreen))
with this repo, but is not part of NebulaOS.

**Why this matters for this wiki specifically:** this repo's `wiki/` folder was inherited from the
same OpenKE-lineage fork history, and much of it — installation, upgrading, troubleshooting, the
release/tag flow — still describes OpenKE's own SSH-installer distribution model on stock Creality
firmware, not how NebulaOS actually builds or deploys this code. Pages carrying an
`OPENKE_SPECIFIC` banner (e.g. [Releases and Deployment](Releases-and-Deployment)) are preserved for
reference but do not describe current NebulaOS behavior — see
[Integration with NebulaOS](Integration-with-NebulaOS) for what actually happens instead.

If you followed a link expecting OpenKE's SSH-installer workflow for stock Creality firmware,
[github.com/coreflake1/guppyscreen](https://github.com/coreflake1/guppyscreen) is the repo you want.

See also: [Vendored Dependencies](Vendored-Dependencies) for the OpenKE-inherited maintenance
scripts that live in this repo's tree but aren't part of any current NebulaOS build.
