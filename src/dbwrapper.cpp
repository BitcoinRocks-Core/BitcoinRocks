// Copyright (c) 2012-present The Bitcoin Core developers
// Copyright (c) 2026 The BitcoinRocks developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <dbwrapper.h>

#include <common/system.h>
#include <logging.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/log.h>
#include <util/obfuscation.h>
#include <util/strencodings.h>

#include <rocksdb/cache.h>
#include <rocksdb/db.h>
#include <rocksdb/env.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/iterator.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/status.h>
#include <rocksdb/table.h>
#include <rocksdb/write_batch.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

static auto CharCast(const std::byte* data)
{
    return reinterpret_cast<const char*>(data);
}

static void HandleError(const rocksdb::Status& status)
{
    if (status.ok()) {
        return;
    }

    const std::string errmsg{
        "Fatal RocksDB error: " + status.ToString()
    };

    LogError("%s", errmsg);

    // The logging category is renamed in a later conversion phase.
    LogInfo(
        "You can currently use -debug=rocksdb to obtain "
        "RocksDB diagnostic messages"
    );

    throw dbwrapper_error(errmsg);
}

bool DestroyDB(const std::string& path_str)
{
    const rocksdb::Status status{
        rocksdb::DestroyDB(
            path_str,
            rocksdb::Options{}
        )
    };

    return status.ok() || status.IsNotFound();
}

static void SetMaxOpenFiles(rocksdb::Options& options)
{
    options.max_open_files =
        sizeof(void*) < 8
            ? 64
            : 1000;
}

struct RocksDBTuning {
    const char* profile_name{"default"};
    const char* hardware_class{"standard"};

    size_t effective_cache{0};
    size_t block_cache_size{0};
    size_t write_buffer_size{0};

    size_t block_size{8U << 10};
    double bloom_bits_per_key{8.0};
    bool use_bloom_filter{true};

    int max_write_buffers{2};
    int merge_write_buffers{1};
    int background_jobs{2};
    uint32_t subcompactions{1};
};

static bool IsHighEndRocksDBHardware(int cores)
{
    constexpr uint64_t HIGH_END_MEMORY_BYTES{
        48ULL * 1024ULL * 1024ULL * 1024ULL
    };

    const std::optional<uint64_t> physical_memory{
        GetTotalRAM()
    };

    return
        cores >= 24 &&
        physical_memory &&
        *physical_memory >= HIGH_END_MEMORY_BYTES;
}

static bool HasHighEndRocksDBBudget(
    DBProfile profile,
    size_t cache_size_bytes)
{
    constexpr size_t MiB{
        1024U * 1024U
    };

    switch (profile) {
    case DBProfile::BLOCK_INDEX:
        return cache_size_bytes >= 128U * MiB;

    case DBProfile::CHAINSTATE:
        return cache_size_bytes >= 768U * MiB;

    case DBProfile::TX_INDEX:
        return cache_size_bytes >= 1536U * MiB;

    case DBProfile::BLOCK_FILTER_INDEX:
        return cache_size_bytes >= 512U * MiB;

    case DBProfile::COINSTATS_INDEX:
        return cache_size_bytes >= 192U * MiB;

    case DBProfile::DEFAULT:
        return false;
    }

    return false;
}

static RocksDBTuning GetTuning(
    size_t cache_size_bytes,
    DBProfile profile)
{
    RocksDBTuning tuning;

    tuning.effective_cache =
        std::max<size_t>(
            cache_size_bytes,
            1U << 20
        );

    const int cores{
        std::max(
            1,
            GetNumCores()
        )
    };

    const bool high_end{
        IsHighEndRocksDBHardware(cores) &&
        HasHighEndRocksDBBudget(
            profile,
            cache_size_bytes
        )
    };

    if (high_end) {
        tuning.hardware_class = "high-end";
    }

    size_t write_buffer_divisor{4};

    switch (profile) {
    case DBProfile::CHAINSTATE:
        tuning.profile_name = "chainstate";
        tuning.block_size = 4U << 10;
        tuning.bloom_bits_per_key = 10.0;
        tuning.max_write_buffers = 4;
        tuning.merge_write_buffers = 2;
        tuning.background_jobs =
            std::clamp(
                cores / 2,
                2,
                4
            );
        tuning.subcompactions =
            cores >= 8
                ? 2
                : 1;
        write_buffer_divisor = 8;

        if (high_end) {
            tuning.max_write_buffers = 8;
            tuning.merge_write_buffers = 2;
            tuning.background_jobs =
                std::clamp(
                    cores / 4,
                    6,
                    8
                );
            tuning.subcompactions =
                static_cast<uint32_t>(
                    std::clamp(
                        cores / 8,
                        2,
                        4
                    )
                );
            write_buffer_divisor = 16;
        }

        break;

    case DBProfile::TX_INDEX:
        tuning.profile_name = "txindex";
        tuning.block_size = 8U << 10;
        tuning.bloom_bits_per_key = 10.0;
        tuning.max_write_buffers = 4;
        tuning.merge_write_buffers = 2;
        tuning.background_jobs =
            std::clamp(
                cores / 3,
                2,
                3
            );
        tuning.subcompactions = 1;
        write_buffer_divisor = 8;

        if (high_end) {
            tuning.max_write_buffers = 8;
            tuning.merge_write_buffers = 2;
            tuning.background_jobs =
                std::clamp(
                    cores / 6,
                    4,
                    6
                );
            tuning.subcompactions =
                cores >= 32
                    ? 2
                    : 1;
            write_buffer_divisor = 16;
        }

        break;

    case DBProfile::BLOCK_FILTER_INDEX:
        tuning.profile_name = "block-filter-index";
        tuning.block_size = 16U << 10;
        tuning.bloom_bits_per_key = 8.0;
        tuning.max_write_buffers = 3;
        tuning.merge_write_buffers = 1;
        tuning.background_jobs = 2;
        tuning.subcompactions = 1;
        write_buffer_divisor = 8;

        if (high_end) {
            tuning.max_write_buffers = 6;
            tuning.merge_write_buffers = 2;
            tuning.background_jobs =
                std::clamp(
                    cores / 8,
                    3,
                    4
                );
            tuning.subcompactions = 2;
            write_buffer_divisor = 16;
        }

        break;

    case DBProfile::COINSTATS_INDEX:
        tuning.profile_name = "coinstats-index";
        tuning.block_size = 8U << 10;
        tuning.bloom_bits_per_key = 8.0;
        tuning.max_write_buffers = 3;
        tuning.merge_write_buffers = 1;
        tuning.background_jobs = 2;
        tuning.subcompactions = 1;
        write_buffer_divisor = 8;

        if (high_end) {
            tuning.max_write_buffers = 6;
            tuning.merge_write_buffers = 2;
            tuning.background_jobs =
                std::clamp(
                    cores / 8,
                    3,
                    4
                );
            tuning.subcompactions = 2;
            write_buffer_divisor = 16;
        }

        break;

    case DBProfile::BLOCK_INDEX:
        tuning.profile_name = "block-index";

        if (high_end) {
            tuning.max_write_buffers = 4;
            tuning.merge_write_buffers = 1;
            tuning.background_jobs = 3;
            tuning.subcompactions = 1;
            write_buffer_divisor = 8;
        }

        break;

    case DBProfile::DEFAULT:
        break;
    }

    tuning.block_cache_size =
        std::max<size_t>(
            tuning.effective_cache / 2,
            256U << 10
        );

    tuning.write_buffer_size =
        std::max<size_t>(
            tuning.effective_cache /
                write_buffer_divisor,
            64U << 10
        );

    return tuning;
}

static rocksdb::Options GetOptions(const RocksDBTuning& tuning)
{
    rocksdb::BlockBasedTableOptions table_options;

    table_options.block_cache =
        rocksdb::NewLRUCache(
            tuning.block_cache_size,
            /*num_shard_bits=*/-1,
            /*strict_capacity_limit=*/false,
            /*high_pri_pool_ratio=*/0.20
        );

    if (tuning.use_bloom_filter) {
        table_options.filter_policy.reset(
            rocksdb::NewBloomFilterPolicy(
                tuning.bloom_bits_per_key,
                false
            )
        );
    }

    table_options.block_size =
        tuning.block_size;

    table_options.cache_index_and_filter_blocks =
        true;

    table_options
        .cache_index_and_filter_blocks_with_high_priority =
            true;

    table_options
        .pin_l0_filter_and_index_blocks_in_cache =
            true;

    table_options.checksum =
        rocksdb::kCRC32c;

    rocksdb::Options options;

    options.create_if_missing =
        true;

    options.paranoid_checks =
        true;

    options.compression =
        rocksdb::kLZ4Compression;

    options.bottommost_compression =
        rocksdb::kZSTD;

    options.compaction_style =
        rocksdb::kCompactionStyleLevel;

    options.level_compaction_dynamic_level_bytes =
        true;

    options.write_buffer_size =
        tuning.write_buffer_size;

    options.max_write_buffer_number =
        tuning.max_write_buffers;

    options.min_write_buffer_number_to_merge =
        tuning.merge_write_buffers;

    options.target_file_size_base =
        std::max<uint64_t>(
            options.target_file_size_base,
            DBWRAPPER_MAX_FILE_SIZE
        );

    options.max_background_jobs =
        tuning.background_jobs;

    options.max_subcompactions =
        tuning.subcompactions;

    options.bytes_per_sync =
        1U << 20;

    options.wal_bytes_per_sync =
        1U << 20;

    options.keep_log_file_num =
        2;

    options.max_log_file_size =
        1U << 20;

    SetMaxOpenFiles(options);

    options.table_factory.reset(
        rocksdb::NewBlockBasedTableFactory(
            table_options
        )
    );

    return options;
}

struct CDBBatch::WriteBatchImpl {
    rocksdb::WriteBatch batch;
};

CDBBatch::CDBBatch(const CDBWrapper& parent_in)
    : parent{parent_in},
      m_impl_batch{
          std::make_unique<CDBBatch::WriteBatchImpl>()
      }
{
    Clear();
}

CDBBatch::~CDBBatch() = default;

void CDBBatch::Clear()
{
    m_impl_batch->batch.Clear();
}

void CDBBatch::WriteImpl(
    std::span<const std::byte> key,
    DataStream& value)
{
    const rocksdb::Slice db_key{
        CharCast(key.data()),
        key.size()
    };

    dbwrapper_private::GetObfuscation(parent)(
        value
    );

    const rocksdb::Slice db_value{
        CharCast(value.data()),
        value.size()
    };

    HandleError(
        m_impl_batch->batch.Put(
            db_key,
            db_value
        )
    );
}

void CDBBatch::EraseImpl(
    std::span<const std::byte> key)
{
    const rocksdb::Slice db_key{
        CharCast(key.data()),
        key.size()
    };

    HandleError(
        m_impl_batch->batch.Delete(
            db_key
        )
    );
}

size_t CDBBatch::ApproximateSize() const
{
    return m_impl_batch->batch.GetDataSize();
}

struct RocksDBContext {
    rocksdb::Options options;

    rocksdb::ReadOptions readoptions;
    rocksdb::ReadOptions iteroptions;

    rocksdb::WriteOptions writeoptions;
    rocksdb::WriteOptions syncoptions;

    std::unique_ptr<rocksdb::Env> penv;
    std::unique_ptr<rocksdb::DB> pdb;

    std::string path;
    bool delete_on_close{false};
};

CDBWrapper::CDBWrapper(const DBParams& params)
    : m_db_context{
          std::make_unique<RocksDBContext>()
      },
      m_name{
          fs::PathToString(
              params.path.stem()
          )
      }
{
    DBContext().readoptions.verify_checksums =
        true;

    DBContext().iteroptions =
        DBContext().readoptions;

    DBContext().iteroptions.fill_cache =
        false;

    DBContext().writeoptions.disableWAL =
        false;

    DBContext().syncoptions.disableWAL =
        false;

    DBContext().syncoptions.sync =
        true;

    const RocksDBTuning tuning{
        GetTuning(
            params.cache_bytes,
            params.profile
        )
    };

    DBContext().options =
        GetOptions(tuning);

    if (params.memory_only) {
        DBContext().penv.reset(
            rocksdb::NewMemEnv(
                rocksdb::Env::Default()
            )
        );

        if (!DBContext().penv) {
            throw dbwrapper_error(
                "Failed to create RocksDB in-memory environment"
            );
        }

        DBContext().options.env =
            DBContext().penv.get();
    }

    DBContext().path =
        params.memory_only
            ? "/bitcoinrocks-memory-db"
            : fs::PathToString(
                  params.path
              );

    DBContext().delete_on_close =
        false;

    LogDebug(
        BCLog::ROCKSDB,
        "RocksDB tuning: "
        "db=%s profile=%s class=%s "
        "cache=%.1fMiB "
        "block_cache=%.1fMiB "
        "write_buffer=%.1fMiB "
        "write_buffers=%d "
        "merge_buffers=%d "
        "background_jobs=%d "
        "subcompactions=%u "
        "compression=LZ4 "
        "bottommost=ZSTD "
        "max_open_files=%d\n",
        m_name,
        tuning.profile_name,
        tuning.hardware_class,
        tuning.effective_cache / 1048576.0,
        tuning.block_cache_size / 1048576.0,
        tuning.write_buffer_size / 1048576.0,
        tuning.max_write_buffers,
        tuning.merge_write_buffers,
        tuning.background_jobs,
        tuning.subcompactions,
        DBContext().options.max_open_files
    );

    if (!params.memory_only && params.wipe_data && fs::exists(params.path)) {
        LogInfo(
            "Wiping RocksDB in %s",
            DBContext().path
        );

        const rocksdb::Status status{
            rocksdb::DestroyDB(
                DBContext().path,
                DBContext().options
            )
        };

        if (
            !status.ok() &&
            !status.IsNotFound()
        ) {
            HandleError(status);
        }
    }

    if (!params.memory_only) {
        TryCreateDirectories(
            params.path
        );
    }

    LogInfo(
        "Opening RocksDB in %s%s",
        DBContext().path,
        params.memory_only
            ? " (ephemeral)"
            : ""
    );

    rocksdb::DB* raw_db{nullptr};

    HandleError(
        rocksdb::DB::Open(
            DBContext().options,
            DBContext().path,
            &raw_db
        )
    );

    DBContext().pdb.reset(
        raw_db
    );

    LogInfo(
        "Opened RocksDB successfully"
    );

    if (params.options.force_compact) {
        LogInfo(
            "Starting database compaction of %s",
            DBContext().path
        );

        CompactFull();

        LogInfo(
            "Finished database compaction of %s",
            DBContext().path
        );
    }

    if (
        !Read(
            OBFUSCATION_KEY,
            m_obfuscation
        ) &&
        params.obfuscate &&
        IsEmpty()
    ) {
        const Obfuscation obfuscation{
            FastRandomContext{}
                .randbytes<Obfuscation::KEY_SIZE>()
        };

        assert(!m_obfuscation);

        Write(
            OBFUSCATION_KEY,
            obfuscation
        );

        m_obfuscation =
            obfuscation;

        LogInfo(
            "Wrote new obfuscation key for %s: %s",
            DBContext().path,
            m_obfuscation.HexKey()
        );
    }

    LogInfo(
        "Using obfuscation key for %s: %s",
        DBContext().path,
        m_obfuscation.HexKey()
    );
}

CDBWrapper::~CDBWrapper()
{
    if (!m_db_context) {
        return;
    }

    const bool delete_on_close{
        DBContext().delete_on_close
    };

    const std::string path{
        DBContext().path
    };

    DBContext().pdb.reset();

    if (delete_on_close) {
        const rocksdb::Status status{
            rocksdb::DestroyDB(
                path,
                DBContext().options
            )
        };

        if (
            !status.ok() &&
            !status.IsNotFound()
        ) {
            LogError(
                "Failed to destroy ephemeral RocksDB at %s: %s",
                path,
                status.ToString()
            );
        }
    }
}

void CDBWrapper::WriteBatch(
    CDBBatch& batch,
    bool sync)
{
    const bool log_memory{
        LogAcceptCategory(
            BCLog::ROCKSDB,
            util::log::Level::Debug
        )
    };

    double memory_before{0};

    if (log_memory) {
        memory_before =
            DynamicMemoryUsage() /
            1024.0 /
            1024.0;
    }

    HandleError(
        DBContext().pdb->Write(
            sync
                ? DBContext().syncoptions
                : DBContext().writeoptions,
            &batch.m_impl_batch->batch
        )
    );

    if (log_memory) {
        const double memory_after{
            DynamicMemoryUsage() /
            1024.0 /
            1024.0
        };

        LogDebug(
            BCLog::ROCKSDB,
            "RocksDB WriteBatch memory usage: "
            "db=%s bytes=%u "
            "before=%.1fMiB after=%.1fMiB\n",
            m_name,
            static_cast<unsigned>(
                batch.ApproximateSize()
            ),
            memory_before,
            memory_after
        );
    }
}

std::optional<std::string>
CDBWrapper::GetProperty(
    const std::string& property) const
{
    std::string value;

    if (
        DBContext().pdb->GetProperty(
            property,
            &value
        )
    ) {
        return value;
    }

    return std::nullopt;
}

void CDBWrapper::CompactFull()
{
    rocksdb::CompactRangeOptions options;

    // This operation runs synchronously during forced startup
    // compaction, before the database enters normal service.
    options.exclusive_manual_compaction = true;

    // Move compacted data to the lowest level capable of holding it.
    options.change_level = true;
    options.target_level = -1;

    // Rewrite bottommost files while avoiding redundant rewrites of
    // files generated earlier during this same manual compaction.
    options.bottommost_level_compaction =
        rocksdb::BottommostLevelCompaction::kForceOptimized;

    // Reuse the automatically selected profile-specific concurrency.
    options.max_subcompactions =
        std::max<uint32_t>(
            1,
            DBContext().options.max_subcompactions
        );

    HandleError(
        DBContext().pdb->CompactRange(
            options,
            nullptr,
            nullptr
        )
    );
}

size_t CDBWrapper::DynamicMemoryUsage() const
{
    static constexpr const char*
        MEMORY_PROPERTIES[] = {
            "rocksdb.block-cache-usage",
            "rocksdb.cur-size-all-mem-tables",
            "rocksdb.estimate-table-readers-mem",
        };

    uint64_t total{0};
    bool found_property{false};

    for (
        const char* property :
        MEMORY_PROPERTIES
    ) {
        uint64_t value{0};

        if (
            DBContext().pdb->GetIntProperty(
                property,
                &value
            )
        ) {
            total += value;
            found_property = true;
        }
    }

    if (!found_property) {
        LogDebug(
            BCLog::ROCKSDB,
            "Failed to obtain RocksDB "
            "memory-usage properties\n"
        );

        return 0;
    }

    return static_cast<size_t>(
        std::min<uint64_t>(
            total,
            std::numeric_limits<size_t>::max()
        )
    );
}

std::optional<std::string>
CDBWrapper::ReadImpl(
    std::span<const std::byte> key) const
{
    const rocksdb::Slice db_key{
        CharCast(key.data()),
        key.size()
    };

    std::string value;

    const rocksdb::Status status{
        DBContext().pdb->Get(
            DBContext().readoptions,
            db_key,
            &value
        )
    };

    if (status.IsNotFound()) {
        return std::nullopt;
    }

    if (!status.ok()) {
        LogError(
            "RocksDB read failure: %s",
            status.ToString()
        );

        HandleError(status);
    }

    return value;
}

bool CDBWrapper::ExistsImpl(
    std::span<const std::byte> key) const
{
    const rocksdb::Slice db_key{
        CharCast(key.data()),
        key.size()
    };

    std::string value;

    const rocksdb::Status status{
        DBContext().pdb->Get(
            DBContext().readoptions,
            db_key,
            &value
        )
    };

    if (status.IsNotFound()) {
        return false;
    }

    if (!status.ok()) {
        LogError(
            "RocksDB existence-check failure: %s",
            status.ToString()
        );

        HandleError(status);
    }

    return true;
}

size_t CDBWrapper::EstimateSizeImpl(
    std::span<const std::byte> key_begin,
    std::span<const std::byte> key_end) const
{
    const rocksdb::Slice begin{
        CharCast(key_begin.data()),
        key_begin.size()
    };

    const rocksdb::Slice end{
        CharCast(key_end.data()),
        key_end.size()
    };

    const rocksdb::Range range{
        begin,
        end
    };

    uint64_t size{0};

    DBContext().pdb->GetApproximateSizes(
        &range,
        1,
        &size
    );

    return static_cast<size_t>(
        std::min<uint64_t>(
            size,
            std::numeric_limits<size_t>::max()
        )
    );
}

bool CDBWrapper::IsEmpty()
{
    std::unique_ptr<CDBIterator> iterator{
        NewIterator()
    };

    iterator->SeekToFirst();

    return !iterator->Valid();
}

struct CDBIterator::IteratorImpl {
    const std::unique_ptr<rocksdb::Iterator>
        iterator;

    explicit IteratorImpl(
        rocksdb::Iterator* iterator_in)
        : iterator{iterator_in}
    {
    }
};

CDBIterator::CDBIterator(
    const CDBWrapper& parent_in,
    std::unique_ptr<IteratorImpl> iterator_in)
    : parent{parent_in},
      m_impl_iter{
          std::move(iterator_in)
      }
{
}

CDBIterator* CDBWrapper::NewIterator()
{
    return new CDBIterator{
        *this,
        std::make_unique<
            CDBIterator::IteratorImpl
        >(
            DBContext().pdb->NewIterator(
                DBContext().iteroptions
            )
        )
    };
}

void CDBIterator::SeekImpl(
    std::span<const std::byte> key)
{
    const rocksdb::Slice db_key{
        CharCast(key.data()),
        key.size()
    };

    m_impl_iter->iterator->Seek(
        db_key
    );
}

std::span<const std::byte>
CDBIterator::GetKeyImpl() const
{
    const rocksdb::Slice key{
        m_impl_iter->iterator->key()
    };

    return {
        reinterpret_cast<const std::byte*>(
            key.data()
        ),
        key.size()
    };
}

std::span<const std::byte>
CDBIterator::GetValueImpl() const
{
    const rocksdb::Slice value{
        m_impl_iter->iterator->value()
    };

    return {
        reinterpret_cast<const std::byte*>(
            value.data()
        ),
        value.size()
    };
}

CDBIterator::~CDBIterator() = default;

bool CDBIterator::Valid() const
{
    return m_impl_iter->iterator->Valid();
}

void CDBIterator::SeekToFirst()
{
    m_impl_iter->iterator->SeekToFirst();
}

void CDBIterator::Next()
{
    m_impl_iter->iterator->Next();
}

namespace dbwrapper_private {

const Obfuscation&
GetObfuscation(
    const CDBWrapper& wrapper)
{
    return wrapper.m_obfuscation;
}

} // namespace dbwrapper_private
