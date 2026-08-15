# OpenKE relationship

This repo (`NebulaOS-guppyscreen`) is a NebulaOS component. It's not the OpenKE project.

[OpenKE](https://github.com/coreflake1/guppyscreen) is a separate, independently-released project —
its own SSH installer, its own release train, its own `-OpenKE` tags — that shares an author and a
common upstream ([`ballaswag/guppyscreen`](https://github.com/ballaswag/guppyscreen)) with this
repo, but isn't part of NebulaOS.

Why this matters for the wiki specifically: this repo's `wiki/` folder was inherited from that same
OpenKE-lineage fork history, and a chunk of it — installation, upgrading, troubleshooting, the
release/tag flow — still describes OpenKE's own SSH-installer setup on stock Creality firmware, not
how NebulaOS actually builds or deploys this. Pages that call this out (like
[Releases and Deployment](Releases-and-Deployment)) are kept around for reference, but they're not
describing current NebulaOS behavior — see [Integration with NebulaOS](Integration-with-NebulaOS)
for what actually happens instead.

If you followed a link expecting OpenKE's SSH-installer workflow for stock firmware,
[github.com/coreflake1/guppyscreen](https://github.com/coreflake1/guppyscreen) is the repo you
want.

See also: [Vendored Dependencies](Vendored-Dependencies) for the OpenKE-inherited maintenance
scripts that live in this repo's tree but aren't part of any current NebulaOS build.
