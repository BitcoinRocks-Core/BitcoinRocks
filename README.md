BitcoinRocks
============

https://github.com/BitcoinRocks-Core/BitcoinRocks

BitcoinRocks is a Bitcoin full-node implementation derived from Bitcoin Core,
with a focus on database performance, storage efficiency, node configurability,
and selected forward-looking performance improvements.

BitcoinRocks v31.1.1 is based on Bitcoin Core v31.1 and remains compatible with
the Bitcoin network and Bitcoin consensus rules.

This initial BitcoinRocks release is provided as source code. Pre-built release
binaries are not currently provided.

What is BitcoinRocks?
---------------------

BitcoinRocks connects to the Bitcoin peer-to-peer network to download and fully
validate blocks and transactions. It also includes the Bitcoin wallet, RPC
interface, command-line utilities, and graphical user interface.

BitcoinRocks is not a separate cryptocurrency and does not define a separate
blockchain. It is an alternative Bitcoin node implementation built from the
Bitcoin Core codebase.

The primary changes in BitcoinRocks v31.1.1 include:

- Replacement of the LevelDB database backend with RocksDB.
- Hardware-aware automatic database-cache configuration.
- Workload-specific RocksDB tuning for chainstate and optional indexes.
- LZ4 and Zstandard compression within the RocksDB storage backend.
- Zstandard-compressed `blk*.dat` block records with transparent raw-record
  compatibility and fallback when compression is not beneficial.
- Reduced disk usage for blockchain and index storage.
- Parallel block-input prevout fetching during block validation, backported
  from post-v31.1 Bitcoin Core development.
- User-selectable transaction relay policy profiles.
- BitcoinRocks-specific branding, configuration naming, and application
  integration.

Further technical information is available in the [doc folder](doc/).

BitcoinRocks-Specific Documentation
-----------------------------------

The following documents describe significant BitcoinRocks-specific behavior:

- [RocksDB automatic configuration](doc/BitcoinRocks-RocksDB-AutoConfig-Documentation.md)
- [BitcoinRocks v31.1.1 disk storage comparison](doc/BitcoinRocks-31.1.1-Disk-Storage-Usage-Comparison-Results.md)
- [Parallel prevout fetching](doc/BitcoinRocks-Parallel-Prevout-Fetch.md)
- [User policy settings](doc/BitcoinRocks-User-Policy-Settings.md)

RocksDB and Build Dependencies
------------------------------

BitcoinRocks replaces Bitcoin Core's LevelDB backend with RocksDB and uses LZ4
and Zstandard as part of its database and block-storage implementation.

Users building BitcoinRocks from source are strongly encouraged to use the
BitcoinRocks Depends system. The source tree specifies the RocksDB, LZ4, and
Zstandard versions and build configuration against which BitcoinRocks is
developed and tested.

See the platform-specific build documentation in the [doc folder](doc/) and
the [depends documentation](depends/README.md) for additional information.

Configuration
-------------

The primary BitcoinRocks configuration file is:

    bitcoinrocks.conf

BitcoinRocks retains the familiar Bitcoin Core configuration model and
command-line option format while adding BitcoinRocks-specific functionality.

Database memory is automatically selected according to available system
resources when `dbcache` is not explicitly configured. Users who prefer a
manual database-memory budget may continue to set `dbcache` explicitly.

See the
[RocksDB automatic configuration documentation](doc/BitcoinRocks-RocksDB-AutoConfig-Documentation.md)
for details.

Development
-----------

The `main` branch contains the current BitcoinRocks development and release
history.

Official source release points are identified with version tags such as:

    v31.1.1

BitcoinRocks is derived from Bitcoin Core and continues to incorporate relevant
upstream Bitcoin Core development while maintaining the BitcoinRocks-specific
storage, database, performance, policy, and application changes.

The contribution workflow is described in
[CONTRIBUTING.md](CONTRIBUTING.md), and developer information can be found in
[doc/developer-notes.md](doc/developer-notes.md).

Testing
-------

BitcoinRocks inherits Bitcoin Core's extensive unit, functional, fuzz, and
integration testing infrastructure and adds or modifies tests where required
for BitcoinRocks-specific functionality.

Unit tests can be compiled and executed with CTest when tests were enabled
during build configuration:

    ctest

Further information about unit tests is available in
[src/test/README.md](src/test/README.md).

Functional and integration tests are located under [test/](test/) and can be
run using the functional test runner from the configured build tree.

BitcoinRocks modifies security-critical Bitcoin software. Changes should be
reviewed and tested carefully, particularly changes affecting validation,
database handling, transaction policy, block storage, or wallet behavior.

Upstream Bitcoin Core
---------------------

BitcoinRocks is derived from the Bitcoin Core project:

https://bitcoincore.org

Bitcoin Core source code is available at:

https://github.com/bitcoin/bitcoin

BitcoinRocks retains substantial Bitcoin Core code, documentation, testing
infrastructure, and copyright attribution.

License
-------

BitcoinRocks is released under the terms of the MIT license.

See [COPYING](COPYING) for the full license text or:

https://opensource.org/license/MIT

Source Repository
-----------------

The official BitcoinRocks source repository is:

https://github.com/BitcoinRocks-Core/BitcoinRocks

Issues and source-development reports may be submitted through:

https://github.com/BitcoinRocks-Core/BitcoinRocks/issues

Donations
---------

If you have found BitcoinRocks to be useful to yourself, your organization, or
to the broader Bitcoin ecosystem, please consider assisting in furthering the
development and maintenance of BitcoinRocks by helping the developer stay in a
steady supply of coffee.

BTC: 1A1gc5mi9N4Dth7QVCiffF2Cuy1yXAadbp
