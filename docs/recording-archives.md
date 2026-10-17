# Recording storage

Completed match JSONL files are stored as separate Deflate-compressed entries in
`recordings/recordings.zip`. The active match stays as a loose JSONL file. The
background writer consolidates older files before opening a match and after
closing one. Statistics reads both forms directly and deduplicates identical
recordings, including leftovers after an interrupted consolidation.

Archive replacement uses a temporary file, flushes it to disk, reads every entry
to verify CRCs, and compares each loose original byte-for-byte before atomically
replacing the archive. Only then are those originals removed. Compression
failures leave the originals available. Do not run the standalone compactor
while the game is recording.

Limits remain bounded: 10 MiB per expanded entry, 512 MiB compressed storage,
512 MiB expanded per archive operation, and 10,000 entries. Statistics retains
its existing 200,000 graph-point limit. Compression saves disk space but does
not remove these memory limits or recover events that were never recorded.

The ZIP implementation is the official miniz 3.1.2 amalgamation (MIT), vendored
under `runtime/third_party/miniz`, including its license. Source release:
https://github.com/richgel999/miniz/releases/tag/3.1.2
Downloaded release ZIP SHA256:
`f0446d863f9c19926ad9483c523fdc42e42b8d4a6a431d27e09d49c79a140d9a`.

`scripts/recording_archive_test.cpp` verifies real-recording byte and Statistics
parity, appending, idempotence, interruption recovery, corruption preservation,
and expansion limits. `scripts/Compact-Recordings.cpp` is an offline migration
utility; installations should back up recordings before running it.
