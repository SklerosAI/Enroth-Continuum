# Pinned Mac MM7 dependency source

These are the five exact source revisions selected by the upstream dependency release tag `deps_r8_master`. They are ordinary files in Enroth Continuum's repository, not submodules or downloaded build outputs. `SOURCES.json` records each original project, commit, archive SHA-256, and primary license file. The entire upstream source archives, including license and bundled notice files, were extracted without edits. No proprietary Might & Magic content belongs here.

The local-only build gate still refuses the build until original Enroth Continuum recipes compile these sources and all other required sources are repository-owned. The previously cached binary archives are not made redistributable by these snapshots. Review FFmpeg's effective enabled components and all applicable bundled notices before distributing a new linked MM7 binary.

The sibling `libdwarf-lite/` is a separate exact cpptrace backend source pin with its own provenance and LGPL notices. It is not one of the five prebuilt-archive families in `SOURCES.json`.
