# Copyright (c) 2026 The BitcoinRocks Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

find_package(Threads REQUIRED)

#
# LZ4
#
# Prefer the pinned CMake package supplied by Depends. Native package
# managers such as Homebrew may provide LZ4 without a CMake config file,
# so fall back to pkg-config or ordinary library discovery.
#
find_package(lz4 1.10.0 EXACT CONFIG QUIET)

if(TARGET LZ4::lz4_static)
    set(BITCOINROCKS_LZ4_TARGET LZ4::lz4_static)
elseif(TARGET LZ4::lz4_shared)
    set(BITCOINROCKS_LZ4_TARGET LZ4::lz4_shared)
elseif(TARGET lz4::lz4)
    set(BITCOINROCKS_LZ4_TARGET lz4::lz4)
else()
    find_package(PkgConfig QUIET)

    if(PkgConfig_FOUND)
        pkg_check_modules(LZ4 QUIET IMPORTED_TARGET liblz4>=1.10.0)
    endif()

    if(TARGET PkgConfig::LZ4)
        set(BITCOINROCKS_LZ4_TARGET PkgConfig::LZ4)
    else()
        find_path(LZ4_INCLUDE_DIR
            NAMES lz4.h
            REQUIRED
        )

        find_library(LZ4_LIBRARY
            NAMES lz4
            REQUIRED
        )

        add_library(bitcoinrocks_lz4_native UNKNOWN IMPORTED GLOBAL)

        set_target_properties(bitcoinrocks_lz4_native PROPERTIES
            IMPORTED_LOCATION "${LZ4_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIR}"
        )

        set(BITCOINROCKS_LZ4_TARGET bitcoinrocks_lz4_native)
    endif()
endif()

# RocksDB 10.10.1 exports references to lz4::lz4.
if(NOT TARGET lz4::lz4)
    add_library(lz4::lz4 INTERFACE IMPORTED GLOBAL)

    set_target_properties(lz4::lz4 PROPERTIES
        INTERFACE_LINK_LIBRARIES "${BITCOINROCKS_LZ4_TARGET}"
    )
endif()


#
# Zstandard
#
# Prefer the pinned CMake package supplied by Depends, with native
# package-manager fallbacks for platforms that do not export a CMake
# package config.
#
find_package(zstd 1.5.7 EXACT CONFIG QUIET)

if(TARGET zstd::libzstd_static)
    set(BITCOINROCKS_ZSTD_TARGET zstd::libzstd_static)
elseif(TARGET zstd::libzstd_shared)
    set(BITCOINROCKS_ZSTD_TARGET zstd::libzstd_shared)
elseif(TARGET zstd::zstd)
    set(BITCOINROCKS_ZSTD_TARGET zstd::zstd)
else()
    find_package(PkgConfig QUIET)

    if(PkgConfig_FOUND)
        pkg_check_modules(ZSTD QUIET IMPORTED_TARGET libzstd>=1.5.7)
    endif()

    if(TARGET PkgConfig::ZSTD)
        set(BITCOINROCKS_ZSTD_TARGET PkgConfig::ZSTD)
    else()
        find_path(ZSTD_INCLUDE_DIR
            NAMES zstd.h
            REQUIRED
        )

        find_library(ZSTD_LIBRARY
            NAMES zstd
            REQUIRED
        )

        add_library(bitcoinrocks_zstd_native UNKNOWN IMPORTED GLOBAL)

        set_target_properties(bitcoinrocks_zstd_native PROPERTIES
            IMPORTED_LOCATION "${ZSTD_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIR}"
        )

        set(BITCOINROCKS_ZSTD_TARGET bitcoinrocks_zstd_native)
    endif()
endif()

# RocksDB 10.10.1 exports references to zstd::zstd.
if(NOT TARGET zstd::zstd)
    add_library(zstd::zstd INTERFACE IMPORTED GLOBAL)

    set_target_properties(zstd::zstd PROPERTIES
        INTERFACE_LINK_LIBRARIES "${BITCOINROCKS_ZSTD_TARGET}"
    )
endif()


#
# RocksDB
#
find_package(RocksDB 10.10.1 EXACT CONFIG REQUIRED)

add_library(bitcoinrocks_rocksdb INTERFACE)

target_link_libraries(bitcoinrocks_rocksdb
    INTERFACE
        RocksDB::rocksdb
        zstd::zstd
        Threads::Threads
        ${CMAKE_DL_LIBS}
)
