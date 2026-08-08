# BitcoinRocks RocksDB Storage Backend

BitcoinRocks replaces Bitcoin Core's LevelDB database backend with RocksDB.

RocksDB is used for the node's persistent key/value databases, including the
block index, chainstate, and enabled optional indexes. BitcoinRocks applies
workload-specific RocksDB configuration automatically and normally does not
require manual database tuning.

This document describes the automatic configuration and the settings that
users may override.

There is not currently any auto-migration implementation in BitcoinRocks to
automatically convert current Bitcoin Core LevelDB databases to RocksDB
databases. This was a concious design decision focused on primarily on
eliminating a a vector through with database corruption could occur and
conversion time, as it was estimated that it would take as long or longer
to convert an existing set of LevelDB databases and raw block files versus
performing a fresh chain sync from Genesis.

## RocksDB Version

BitcoinRocks is developed and tested against the RocksDB version specified by
the BitcoinRocks Depends build system.

Users building BitcoinRocks from source are strongly encouraged to build the
required dependencies using `depends/` rather than substituting arbitrary
system versions of RocksDB, LZ4, or Zstandard.

The versions selected by Depends are the versions against which BitcoinRocks
is developed and tested.

## Automatic Database Cache

When `-dbcache` is not explicitly configured, BitcoinRocks selects the total
database cache from the amount of physical memory detected on the host.

The automatic policy is:

| Host class | Automatic database cache |
| --- | --- |
| Standard | 25% of installed physical memory |
| Minimum | 512 MiB |
| Standard maximum | 12 GiB |
| High-end maximum | 24 GiB |

If physical-memory detection is unavailable, BitcoinRocks falls back to a
1 GiB database cache.

A host is eligible for the high-end database profile when it has at least:

- 24 logical processors
- 48 GiB of physical memory

The selected total cache is subsequently divided among the databases and
in-memory UTXO cache according to the databases and optional indexes enabled
by the user.

BitcoinRocks logs the selected cache size and whether the standard or
high-end database profile is active during startup.

## Workload-Specific RocksDB Profiles

BitcoinRocks does not configure every RocksDB database identically.

Separate internal profiles are used for workloads such as:

- block index
- chainstate
- transaction index
- block-filter index
- coin statistics index

Each profile may use different block sizes, Bloom-filter parameters, write
buffer allocations, background-job limits, and subcompaction settings.

The number of available logical processors, the cache budget assigned to the
database, and the detected hardware class are considered when selecting these
values.

This allows relatively small systems to avoid excessive RocksDB memory and
thread usage while allowing larger systems to make greater use of available
CPU and memory.

These internal RocksDB parameters are intentionally automatic and are not
normally exposed as individual user configuration options.

## User Override: `dbcache`

Users who prefer to control total database memory explicitly may use the
standard Bitcoin Core-compatible `dbcache` option.

For example, in `bitcoinrocks.conf`:

dbcache=4096

or from the command line:

bitcoinrocksd -dbcache=4096

The value is specified in MiB.

Providing an explicit dbcache value disables automatic selection of the
total database cache. BitcoinRocks will continue to distribute that cache
among the chainstate, block index, enabled optional indexes, and UTXO cache
using its workload-aware allocation logic.

The minimum accepted database-cache value is 4 MiB.

The Database Cache setting exposed by the graphical interface provides the
same user override.

For most users, leaving dbcache unset is recommended.

## Database Compression

BitcoinRocks enables compression within RocksDB.

Frequently accessed RocksDB levels use LZ4 compression while bottommost data
uses Zstandard compression. This provides a balance between database access
speed and persistent storage efficiency.

BitcoinRocks also enables database checksums and workload-specific Bloom
filters where appropriate.

These settings are selected internally and do not normally require user
configuration.

Block Storage Is Separate

RocksDB compression should not be confused with BitcoinRocks compressed block
storage.

Bitcoin block payloads stored in blk*.dat are handled by the BitcoinRocks
block-storage layer, not RocksDB. BitcoinRocks independently compresses block
records with Zstandard when doing so produces a worthwhile space reduction,
while retaining compatibility with uncompressed block records.

Therefore:

RocksDB compression reduces the storage used by database/index data.
Block-record compression reduces the storage used by blk*.dat.
The two mechanisms operate independently.

See the BitcoinRocks disk-storage comparison document for measured storage
results.

## Optional Indexes

Optional indexes continue to be enabled with their normal configuration
options, for example:

txindex=1
blockfilterindex=1
coinstatsindex=1

Enabling additional indexes increases database cache and disk requirements.
BitcoinRocks automatically accounts for enabled indexes when distributing the
available database-cache budget.

No special RocksDB configuration is required to enable an index.

## Diagnostics

BitcoinRocks reports its selected database configuration in debug.log.

RocksDB-specific diagnostic logging may be enabled with:

debug=rocksdb

Startup logging includes the selected workload profile, hardware class,
effective cache budget, write-buffer configuration, background jobs,
subcompactions, and compression configuration.

This information can be useful when diagnosing performance or comparing
configuration across systems.

## Manual RocksDB Tuning

BitcoinRocks intentionally does not expose the full RocksDB option set through
bitcoinrocks.conf.

RocksDB has a very large number of interacting tuning parameters, and arbitrary
combinations can significantly increase memory usage, write amplification, or
compaction load.

BitcoinRocks instead provides automatic workload-specific profiles while
retaining dbcache as the primary user-facing memory control.

Users modifying RocksDB behavior beyond these supported controls should treat
such changes as source-level development changes rather than normal node
configuration.
