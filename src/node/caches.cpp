// Copyright (c) 2021-present The Bitcoin Core developers
// Copyright (c) 2026 The BitcoinRocks developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/caches.h>

#include <common/args.h>
#include <common/system.h>
#include <index/coinstatsindex.h>
#include <index/txindex.h>
#include <index/txospenderindex.h>
#include <kernel/caches.h>
#include <logging.h>
#include <node/interface_ui.h>
#include <tinyformat.h>
#include <util/byte_units.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

static constexpr size_t STANDARD_MAX_TX_INDEX_CACHE{
    1024_MiB
};

static constexpr size_t HIGH_END_MAX_TX_INDEX_CACHE{
    sizeof(size_t) == 4
        ? 1024_MiB
        : static_cast<size_t>(uint64_t{4096} << 20)
};

static constexpr size_t STANDARD_MAX_TXOSPENDER_INDEX_CACHE{
    1024_MiB
};

static constexpr size_t HIGH_END_MAX_TXOSPENDER_INDEX_CACHE{
    sizeof(size_t) == 4
        ? 1024_MiB
        : static_cast<size_t>(uint64_t{4096} << 20)
};

/*
 * Preserve Bitcoin Core's address-space safety limit on 32-bit
 * systems.
 */
static constexpr size_t MAX_32BIT_DBCACHE{
    1024_MiB
};

static constexpr size_t STANDARD_MAX_FILTER_INDEX_CACHE{
    512_MiB
};

static constexpr size_t HIGH_END_MAX_FILTER_INDEX_CACHE{
    2048_MiB
};

static constexpr size_t STANDARD_MAX_COINSTATS_INDEX_CACHE{
    128_MiB
};

static constexpr size_t HIGH_END_MAX_COINSTATS_INDEX_CACHE{
    512_MiB
};

namespace {

bool IsHighEndDatabaseHardware(
    const std::optional<size_t>& physical_memory,
    int hardware_threads
)
{
    return
        hardware_threads >=
            HIGH_END_DATABASE_MIN_THREADS &&
        physical_memory &&
        static_cast<uint64_t>(*physical_memory) >=
            HIGH_END_DATABASE_MIN_MEMORY;
}

size_t AutomaticDbCacheFromMemory(
    const std::optional<size_t>& physical_memory,
    int hardware_threads
)
{
    if (!physical_memory) {
        return DEFAULT_DB_CACHE;
    }

    const bool high_end{
        IsHighEndDatabaseHardware(
            physical_memory,
            hardware_threads
        )
    };

    const uint64_t maximum_cache{
        std::min<uint64_t>(
            high_end
                ? MAX_HIGH_END_AUTOMATIC_DB_CACHE
                : MAX_AUTOMATIC_DB_CACHE,
            sizeof(void*) == 4
                ? MAX_32BIT_DBCACHE
                : std::numeric_limits<size_t>::max()
        )
    };

    /*
     * Stock policy:
     *
     *   25% of installed physical memory
     *   minimum 512 MiB
     *   standard ceiling 12 GiB
     *   high-end ceiling 24 GiB
     */
    const uint64_t selected{
        std::clamp<uint64_t>(
            *physical_memory / 4,
            MIN_AUTOMATIC_DB_CACHE,
            maximum_cache
        )
    };

    return static_cast<size_t>(
        std::min<uint64_t>(
            selected,
            std::numeric_limits<
                size_t
            >::max()
        )
    );
}

size_t AllocateBounded(
    size_t& remaining,
    size_t target,
    size_t maximum_fraction_denominator
)
{
    if (remaining == 0) {
        return 0;
    }

    const size_t allocation{
        std::min(
            target,
            remaining /
                maximum_fraction_denominator
        )
    };

    remaining -= allocation;

    return allocation;
}

} // namespace

namespace node {

size_t GetAutomaticDbCache()
{
    return AutomaticDbCacheFromMemory(
        GetTotalRAM(),
        std::max(
            1,
            GetNumCores()
        )
    );
}

size_t GetDefaultDBCache()
{
    return GetAutomaticDbCache();
}

CacheSizes CalculateCacheSizes(
    const ArgsManager& args,
    size_t n_indexes
)
{
    const std::optional<size_t>
        physical_memory{
            GetTotalRAM()
        };

    const int hardware_threads{
        std::max(
            1,
            GetNumCores()
        )
    };

    const bool high_end_hardware{
        IsHighEndDatabaseHardware(
            physical_memory,
            hardware_threads
        )
    };

    bool automatic{true};

    size_t total_cache{
        AutomaticDbCacheFromMemory(
            physical_memory,
            hardware_threads
        )
    };

    if (
        std::optional<int64_t> db_cache =
            args.GetIntArg("-dbcache")
    ) {
        automatic = false;

        if (*db_cache < 0) {
            db_cache = 0;
        }

        const uint64_t db_cache_bytes{
            SaturatingLeftShift<uint64_t>(
                *db_cache,
                20
            )
        };

        const uint64_t maximum_explicit_cache{
            sizeof(void*) == 4
                ? MAX_32BIT_DBCACHE
                : std::numeric_limits<size_t>::max()
        };

        total_cache = std::max<size_t>(
            MIN_DB_CACHE,
            std::min<uint64_t>(
                db_cache_bytes,
                maximum_explicit_cache
            )
        );
    }

    const bool high_end{
        high_end_hardware &&
        static_cast<uint64_t>(total_cache) >=
            HIGH_END_PROFILE_MIN_DB_CACHE
    };

    const size_t original_total{
        total_cache
    };

    size_t remaining{
        total_cache
    };

    IndexCacheSizes index_sizes;

    if (
        args.GetBoolArg(
            "-txindex",
            DEFAULT_TXINDEX
        )
    ) {
        const size_t maximum{
            high_end
                ? HIGH_END_MAX_TX_INDEX_CACHE
                : STANDARD_MAX_TX_INDEX_CACHE
        };

        const size_t target{
            std::clamp(
                original_total / 8,
                64_MiB,
                maximum
            )
        };

        index_sizes.tx_index =
            AllocateBounded(
                remaining,
                target,
                4
            );
    }

    if (
        args.GetBoolArg(
            "-txospenderindex",
            DEFAULT_TXOSPENDERINDEX
        )
    ) {
        const size_t maximum{
            high_end
                ? HIGH_END_MAX_TXOSPENDER_INDEX_CACHE
                : STANDARD_MAX_TXOSPENDER_INDEX_CACHE
        };

        const size_t target{
            std::clamp(
                original_total / 8,
                64_MiB,
                maximum
            )
        };

        index_sizes.txospender_index =
            AllocateBounded(
                remaining,
                target,
                4
            );
    }

    if (n_indexes > 0) {
        const size_t maximum{
            high_end
                ? HIGH_END_MAX_FILTER_INDEX_CACHE
                : STANDARD_MAX_FILTER_INDEX_CACHE
        };

        const size_t combined_target{
            std::clamp(
                original_total / 16,
                32_MiB,
                maximum
            )
        };

        const size_t combined_allocation{
            std::min(
                combined_target,
                remaining / 4
            )
        };

        index_sizes.filter_index =
            combined_allocation /
            n_indexes;

        remaining -=
            index_sizes.filter_index *
            n_indexes;
    }

    if (
        args.GetBoolArg(
            "-coinstatsindex",
            DEFAULT_COINSTATSINDEX
        )
    ) {
        const size_t maximum{
            high_end
                ? HIGH_END_MAX_COINSTATS_INDEX_CACHE
                : STANDARD_MAX_COINSTATS_INDEX_CACHE
        };

        const size_t target{
            std::clamp(
                original_total / 64,
                16_MiB,
                maximum
            )
        };

        index_sizes.coin_stats_index =
            AllocateBounded(
                remaining,
                target,
                8
            );
    }

    return {
        .index = index_sizes,
        .kernel =
            kernel::CacheSizes{
                remaining,
                high_end
            },
        .total = original_total,
        .automatic = automatic,
        .physical_memory =
            physical_memory,
        .hardware_threads =
            hardware_threads,
        .high_end = high_end,
    };
}


void LogOversizedDbCache(
    const ArgsManager& args
) noexcept
{
    const auto total_ram{
        GetTotalRAM()
    };

    const auto configured_cache{
        args.GetIntArg("-dbcache")
    };

    /*
     * Automatic cache selection is deliberately bounded to a
     * conservative fraction of RAM. Only warn about explicit user
     * overrides.
     */
    if (!total_ram || !configured_cache) {
        return;
    }

    int64_t cache_mib{
        *configured_cache
    };

    if (cache_mib < 0) {
        cache_mib = 0;
    }

    const uint64_t cache_bytes{
        SaturatingLeftShift<uint64_t>(
            cache_mib,
            20
        )
    };

    const uint64_t maximum_cache{
        sizeof(void*) == 4
            ? MAX_32BIT_DBCACHE
            : std::numeric_limits<size_t>::max()
    };

    const size_t selected{
        std::max<size_t>(
            MIN_DB_CACHE,
            std::min<uint64_t>(
                cache_bytes,
                maximum_cache
            )
        )
    };

    if (
        ShouldWarnOversizedDbCache(
            selected,
            *total_ram
        )
    ) {
        InitWarning(
            bilingual_str{
                tfm::format(
                    _(
                        "A %zu MiB dbcache may be too large "
                        "for a system memory of only %zu MiB."
                    ),
                    selected >> 20,
                    *total_ram >> 20
                )
            }
        );
    }
}

} // namespace node
