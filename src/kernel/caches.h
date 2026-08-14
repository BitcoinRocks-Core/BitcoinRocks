// Copyright (c) 2024-present The Bitcoin Core developers
// Copyright (c) 2026 The BitcoinRocks developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_CACHES_H
#define BITCOIN_KERNEL_CACHES_H

#include <util/byte_units.h>

#include <algorithm>
#include <cstddef>

/*
 * Fallback used by kernel-only tools that do not use node-level
 * physical-memory auto-tuning.
 */
static constexpr size_t DEFAULT_KERNEL_CACHE{
    450_MiB
};

/*
 * Default database write batch size retained for the node-level
 * database batching interface.
 */
static constexpr size_t DEFAULT_DB_CACHE_BATCH{
    32_MiB
};

/*
 * Standard block-tree RocksDB bounds.
 */
static constexpr size_t MIN_BLOCK_DB_CACHE{
    16_MiB
};

static constexpr size_t MAX_BLOCK_DB_CACHE{
    64_MiB
};

/*
 * High-end block-tree RocksDB ceiling.
 */
static constexpr size_t HIGH_END_MAX_BLOCK_DB_CACHE{
    256_MiB
};

/*
 * Standard chainstate RocksDB bounds.
 */
static constexpr size_t MIN_COINS_DB_CACHE{
    64_MiB
};

static constexpr size_t MAX_COINS_DB_CACHE{
    512_MiB
};

/*
 * High-end chainstate RocksDB ceiling.
 */
static constexpr size_t HIGH_END_MAX_COINS_DB_CACHE{
    2048_MiB
};

namespace kernel {

struct CacheSizes {
    size_t block_tree_db;
    size_t coins_db;
    size_t coins;

    explicit CacheSizes(
        size_t total_cache,
        bool high_end = false
    )
    {
        const size_t original_total{
            total_cache
        };

        const size_t block_tree_target{
            high_end
                ? std::clamp(
                    original_total / 64,
                    64_MiB,
                    HIGH_END_MAX_BLOCK_DB_CACHE
                )
                : std::clamp(
                    original_total / 128,
                    MIN_BLOCK_DB_CACHE,
                    MAX_BLOCK_DB_CACHE
                )
        };

        /*
         * Preserve support for deliberately small explicit cache
         * values by bounding the block database to one eighth of
         * the remaining kernel budget.
         */
        block_tree_db = std::min(
            total_cache / 8,
            block_tree_target
        );

        total_cache -= block_tree_db;

        const size_t coins_db_target{
            high_end
                ? std::clamp(
                    original_total / 8,
                    512_MiB,
                    HIGH_END_MAX_COINS_DB_CACHE
                )
                : std::clamp(
                    original_total / 16,
                    MIN_COINS_DB_CACHE,
                    MAX_COINS_DB_CACHE
                )
        };

        /*
         * Always preserve at least three quarters of the remaining
         * kernel budget for the in-memory UTXO cache.
         */
        coins_db = std::min(
            total_cache / 4,
            coins_db_target
        );

        total_cache -= coins_db;

        coins = total_cache;
    }
};

} // namespace kernel

#endif // BITCOIN_KERNEL_CACHES_H
