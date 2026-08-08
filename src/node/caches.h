// Copyright (c) 2015-2026 The Bitcoin Core developers
// Copyright (c) 2026 The Kvanta5 Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CACHES_H
#define BITCOIN_NODE_CACHES_H

#include <kernel/caches.h>
#include <util/byte_units.h>

#include <cstddef>
#include <cstdint>
#include <optional>

class ArgsManager;

/*
 * Minimum accepted explicit -dbcache value.
 */
static constexpr size_t MIN_DB_CACHE{
    4_MiB
};

/*
 * Fallback when physical-memory detection is unavailable.
 */
static constexpr size_t DEFAULT_DB_CACHE{
    1024_MiB
};

/*
 * Standard hardware-aware cache bounds.
 */
static constexpr size_t MIN_AUTOMATIC_DB_CACHE{
    512_MiB
};

static constexpr size_t MAX_AUTOMATIC_DB_CACHE{
    12'288_MiB
};

/*
 * High-end automatic cache ceiling.
 */
static constexpr size_t MAX_HIGH_END_AUTOMATIC_DB_CACHE{
    24'576_MiB
};

/*
 * High-end mode requires substantial CPU and memory resources.
 */
static constexpr int HIGH_END_DATABASE_MIN_THREADS{
    24
};

static constexpr uint64_t HIGH_END_DATABASE_MIN_MEMORY{
    49'152_MiB
};

/*
 * Prevent a small explicit -dbcache value from activating the
 * high-end database allocation profile merely because the host
 * has substantial hardware.
 */
static constexpr size_t HIGH_END_PROFILE_MIN_DB_CACHE{
    12'288_MiB
};

namespace node {

struct IndexCacheSizes {
    size_t tx_index{0};
    size_t txospender_index{0};
    size_t filter_index{0};
    size_t coin_stats_index{0};
};

struct CacheSizes {
    IndexCacheSizes index;
    kernel::CacheSizes kernel;

    size_t total{0};

    bool automatic{false};

    std::optional<size_t>
        physical_memory;

    int hardware_threads{0};

    bool high_end{false};
};

/*
 * Hardware-aware stock value used when -dbcache is absent.
 */
size_t GetAutomaticDbCache();

/*
 * Compatibility name used by argument help and Qt code.
 * The returned value is hardware-aware when -dbcache is unset.
 */
size_t GetDefaultDBCache();

CacheSizes CalculateCacheSizes(
    const ArgsManager& args,
    size_t n_indexes = 0
);

constexpr bool ShouldWarnOversizedDbCache(
    size_t dbcache,
    size_t total_ram
) noexcept
{
    const size_t cap{
        total_ram < 2048_MiB
            ? DEFAULT_DB_CACHE
            : (total_ram / 100) * 75
    };

    return dbcache > cap;
}

void LogOversizedDbCache(
    const ArgsManager& args
) noexcept;

} // namespace node

#endif // BITCOIN_NODE_CACHES_H
