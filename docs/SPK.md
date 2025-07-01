# SPK - Stormground Packager

An archive library for games, with a focus on speed and efficiency, without compromising quality.

## Goals

1. Be able to create highly compressed, speedily decompressible, and safe archives.

2. Provide a stable, thread safe, and easy API for creating, modifying, and reading SPK archives.

3. Have as little impact on cpu time and memory usage as possible.

### Format Goals

1. Be fast, and corrupt resistant while still achieving high compression ratios (< 40% ideally).

### API Goals

1. Be able to accept a list of requested files, retrieve them asyncronously, and provide a waitable handle for each
requested file.

2. Allow the option to validate any and all elements present in an archive, including the *entirety* of the archive.

3. Allow the storage and access of in-memory decompressed files, and the ability to write them into an archive.

4. Provide a computable structure representing the elements in an archive, including directories and unwritten files.
