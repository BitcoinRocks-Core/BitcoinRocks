Full chain verification, using random transaction samples from every block in the main net chain from Block #1 to Block #961,426. This comparison was ran against a
Bitcoin Core v27.0 Release binary and a BitcoinRocks 31.1.1 Release binary:

[961,275/961,363 |  99.99%] height=961,274 blocks=961,275 headers=961,275 txids=1,412,146,994 rawtx=1,832,636 proofs=961,274 mismatches=0 elapsed=9h 55m 58s ETA=3s
[961,300/961,363 |  99.99%] height=961,299 blocks=961,300 headers=961,300 txids=1,412,242,559 rawtx=1,832,686 proofs=961,299 mismatches=0 elapsed=9h 55m 59s ETA=2s
[961,325/961,363 | 100.00%] height=961,324 blocks=961,325 headers=961,325 txids=1,412,352,816 rawtx=1,832,736 proofs=961,324 mismatches=0 elapsed=9h 56m 01s ETA=1s
[961,350/961,363 | 100.00%] height=961,349 blocks=961,350 headers=961,350 txids=1,412,450,210 rawtx=1,832,786 proofs=961,349 mismatches=0 elapsed=9h 56m 02s ETA=0s
[961,363/961,363 | 100.00%] height=961,362 blocks=961,363 headers=961,363 txids=1,412,504,589 rawtx=1,832,812 proofs=961,362 mismatches=0 elapsed=9h 56m 03s ETA=0s

Final-tip verification attempt 1/10: waiting for both nodes to expose the same current tip...
Final exact common tip verified: 961,426:00000000000000000001600ca6f6971f5bf1175b8f617d293095a8f67944b745


The following measurements were taken later at approximately mainnet Block Height 961498. All three measurements were initiated
nearly simulatenously, within 5 seconds of each other. The Bitcoin Core v27.0 node, and one BitcoinRocks 31.1.1 node maintain
optional blockfilter, coinstats, and txindex databases. One BitcoinRocks 31.1.1 node maintains only the standard blockchain index:

NOTE: Total blk*.dat record counts are physical storage-record counts, not active-chain heights. Independently operated nodes may retain different
stale or side-chain block records in their block files while still sharing the exact same active-chain tip.
Consequently, small differences in total block-record counts or logical blk*.dat payload size do not indicate a chain mismatch.


======================================================================================================================
FINAL RESULT
======================================================================================================================
Completed heights:   961,363/961,363
Raw blocks:          961,364 (707.523 GiB)
Transactions covered:1,412,509,559
Raw tx independently:1,832,814 (918.562 MiB)
Total mismatches:    0


======================================================================================================================
BITCOIN NODE STORAGE AUDIT
======================================================================================================================
Data directory:               Bitcoin Core v27.0 Data Directory
Software profile:             Bitcoin Core v27.0
Database backend:             LevelDB
Block storage format:         Raw / uncompressed flat-file block records

Scanned 5,691/5,691 blk files; records=961,505

======================================================================================================================
EXACT BLK RECORD ACCOUNTING
======================================================================================================================
Block-file XOR:              disabled
blk files:                   5,691
block records:               961,505
ZSTD-compressed block records: 0
Raw/uncompressed block records: 961,505
Logical raw block payload:   759,922,696,962 B    759.923 GB    707.733 GiB
Stored block payload:        759,922,696,962 B    759.923 GB    707.733 GiB
Block payload saved:                       0 B      0.000 GB      0.000 GiB
Payload reduction:           0.000%
Eight-byte record headers:         7,692,040 B      0.008 GB      0.007 GiB
blk apparent file size:      759,945,987,932 B    759.946 GB    707.755 GiB
blk allocated disk blocks:   759,966,986,240 B    759.967 GB    707.774 GiB
Current blk prealloc/slack:       15,598,930 B      0.016 GB      0.015 GiB

======================================================================================================================
DATA DIRECTORY COMPONENTS
======================================================================================================================
Component                   Apparent bytes / decimal GB / GiB                      Allocated                Files
----------------------------------------------------------------------------------------------------------------------
ENTIRE DATADIR              963,337,036,099 B    963.337 GB    897.178 GiB  allocated=  897.347 GiB  files=53,296
blocks/ total               866,025,999,979 B    866.026 GB    806.550 GiB  allocated=  806.600 GiB  files=11,469
  blk*.dat                  759,945,987,932 B    759.946 GB    707.755 GiB  allocated=  707.774 GiB  files=5,691
  rev*.dat                  105,917,186,291 B    105.917 GB     98.643 GiB  allocated=   98.674 GiB  files=5,691
  blocks/index                  162,825,756 B      0.163 GB      0.152 GiB  allocated=    0.152 GiB  files=87
chainstate                   11,405,793,910 B     11.406 GB     10.622 GiB  allocated=   10.636 GiB  files=5,538
indexes/ total               85,698,863,348 B     85.699 GB     79.813 GiB  allocated=   79.919 GiB  files=36,200
  indexes/blockfilter        13,131,175,967 B     13.131 GB     12.229 GiB  allocated=   12.234 GiB  files=837
  indexes/coinstats             169,975,322 B      0.170 GB      0.158 GiB  allocated=    0.159 GiB  files=251
  indexes/txindex            72,397,712,059 B     72.398 GB     67.426 GiB  allocated=   67.526 GiB  files=35,112

======================================================================================================================
WHAT THE DATA DIRECTORY WOULD COST WITHOUT BLK COMPRESSION
======================================================================================================================
Current apparent datadir:    963,337,036,099 B    963.337 GB    897.178 GiB
Add back blk savings:                      0 B      0.000 GB      0.000 GiB
Same datadir with raw blks:  963,337,036,099 B    963.337 GB    897.178 GiB
Net whole-datadir reduction: 0.000%
Everything except blk files: 203,391,048,167 B    203.391 GB    189.423 GiB

NOTE: Bitcoin Core v27.0 uses LevelDB for database-backed chainstate/index data.
Serialized block payloads are stored separately as raw/uncompressed records in blk*.dat flat files.
The GUI's downloaded figure is not used in these calculations.


======================================================================================================================
BITCOIN NODE STORAGE AUDIT
======================================================================================================================
Data directory:               BitcoinRocks v31.1.1 Data Directory
Software profile:             BitcoinRocks v31.1.1
Database backend:             RocksDB
Block storage format:         ZSTD-compressed flat-file block records with raw fallback

Scanned 4,309/4,309 blk files; records=961,497

======================================================================================================================
EXACT BLK RECORD ACCOUNTING
======================================================================================================================
Block-file XOR:              enabled
blk files:                   4,309
block records:               961,497
ZSTD-compressed block records: 857,043
Raw fallback block records:    104,454
Logical raw block payload:   759,909,867,794 B    759.910 GB    707.721 GiB
Stored block payload:        575,962,277,590 B    575.962 GB    536.407 GiB
Block payload saved:         183,947,590,204 B    183.948 GB    171.315 GiB
Payload reduction:           24.207%
Eight-byte record headers:         7,691,976 B      0.008 GB      0.007 GiB
blk apparent file size:      575,985,908,654 B    575.986 GB    536.429 GiB
blk allocated disk blocks:   576,002,400,256 B    576.002 GB    536.444 GiB
Current blk prealloc/slack:       15,939,088 B      0.016 GB      0.015 GiB

======================================================================================================================
DATA DIRECTORY COMPONENTS
======================================================================================================================
Component                   Apparent bytes / decimal GB / GiB                      Allocated                Files
----------------------------------------------------------------------------------------------------------------------
ENTIRE DATADIR              769,335,674,840 B    769.336 GB    716.500 GiB  allocated=  716.670 GiB  files=10,620
blocks/ total               682,030,574,461 B    682.031 GB    635.190 GiB  allocated=  635.241 GiB  files=8,634
  blk*.dat                  575,985,908,654 B    575.986 GB    536.429 GiB  allocated=  536.444 GiB  files=4,309
  rev*.dat                  105,915,125,728 B    105.915 GB     98.641 GiB  allocated=   98.666 GiB  files=4,309
  blocks/index                  129,540,071 B      0.130 GB      0.121 GiB  allocated=    0.131 GiB  files=14
chainstate                   11,080,289,723 B     11.080 GB     10.319 GiB  allocated=   10.347 GiB  files=186
indexes/ total               75,812,824,877 B     75.813 GB     70.606 GiB  allocated=   70.698 GiB  files=1,790
  indexes/blockfilter        13,119,001,023 B     13.119 GB     12.218 GiB  allocated=   12.259 GiB  files=788
  indexes/coinstatsindex         95,242,798 B      0.095 GB      0.089 GiB  allocated=    0.101 GiB  files=11
  indexes/txindex            62,598,581,056 B     62.599 GB     58.299 GiB  allocated=   58.338 GiB  files=991

======================================================================================================================
WHAT THE DATA DIRECTORY WOULD COST WITHOUT BLK COMPRESSION
======================================================================================================================
Current apparent datadir:    769,335,674,840 B    769.336 GB    716.500 GiB
Add back blk savings:        183,947,590,204 B    183.948 GB    171.315 GiB
Same datadir with raw blks:  953,283,265,044 B    953.283 GB    887.814 GiB
Net whole-datadir reduction: 19.296%
Everything except blk files: 193,349,766,186 B    193.350 GB    180.071 GiB

NOTE: BitcoinRocks v31.1.1 uses RocksDB for database-backed chainstate/index data. Its blk*.dat flat files can store individual block payloads as ZSTD-compressed records, with raw fallback when compression is not beneficial.
The GUI's downloaded figure is not used anywhere in these calculations.
For compressed records, this report compares the exact serialized block sizes declared inside the ZSTD frames with the exact payload bytes physically stored.


======================================================================================================================
BITCOIN NODE STORAGE AUDIT
======================================================================================================================
Data directory:               BitcoinRocks v31.1.1 Data Directory
Software profile:             BitcoinRocks v31.1.1
Database backend:             RocksDB
Block storage format:         ZSTD-compressed flat-file block records with raw fallback

Scanned 4,308/4,308 blk files; records=961,497

======================================================================================================================
EXACT BLK RECORD ACCOUNTING
======================================================================================================================
Block-file XOR:              enabled
blk files:                   4,308
block records:               961,497
ZSTD-compressed block records: 857,043
Raw fallback block records:    104,454
Logical raw block payload:   759,909,867,794 B    759.910 GB    707.721 GiB
Stored block payload:        575,962,277,590 B    575.962 GB    536.407 GiB
Block payload saved:         183,947,590,204 B    183.948 GB    171.315 GiB
Payload reduction:           24.207%
Eight-byte record headers:         7,691,976 B      0.008 GB      0.007 GiB
blk apparent file size:      575,971,162,531 B    575.971 GB    536.415 GiB
blk allocated disk blocks:   575,983,591,424 B    575.984 GB    536.427 GiB
Current blk prealloc/slack:        1,192,965 B      0.001 GB      0.001 GiB

======================================================================================================================
DATA DIRECTORY COMPONENTS
======================================================================================================================
Component                   Apparent bytes / decimal GB / GiB                      Allocated                Files
----------------------------------------------------------------------------------------------------------------------
ENTIRE DATADIR              693,026,959,533 B    693.027 GB    645.432 GiB  allocated=  645.475 GiB  files=8,830
blocks/ total               682,000,976,765 B    682.001 GB    635.163 GiB  allocated=  635.202 GiB  files=8,630
  blk*.dat                  575,971,162,531 B    575.971 GB    536.415 GiB  allocated=  536.427 GiB  files=4,308
  rev*.dat                  105,914,476,417 B    105.914 GB     98.641 GiB  allocated=   98.664 GiB  files=4,308
  blocks/index                  115,337,809 B      0.115 GB      0.107 GiB  allocated=    0.111 GiB  files=12
chainstate                   11,013,083,095 B     11.013 GB     10.257 GiB  allocated=   10.261 GiB  files=191
indexes/ total                            0 B      0.000 GB      0.000 GiB  allocated=    0.000 GiB  files=0

======================================================================================================================
WHAT THE DATA DIRECTORY WOULD COST WITHOUT BLK COMPRESSION
======================================================================================================================
Current apparent datadir:    693,026,959,533 B    693.027 GB    645.432 GiB
Add back blk savings:        183,947,590,204 B    183.948 GB    171.315 GiB
Same datadir with raw blks:  876,974,549,737 B    876.975 GB    816.746 GiB
Net whole-datadir reduction: 20.975%
Everything except blk files: 117,055,797,002 B    117.056 GB    109.017 GiB

NOTE: BitcoinRocks v31.1.1 uses RocksDB for database-backed chainstate/index data. Its blk*.dat flat files can store individual block payloads as ZSTD-compressed records, with raw fallback when compression is not beneficial.
The GUI's downloaded figure is not used anywhere in these calculations.
For compressed records, this report compares the exact serialized block sizes declared inside the ZSTD frames with the exact payload bytes physically stored.


Summary:
Bitcoin Core v27 blk*.dat:        707.755 GiB
BitcoinRocks indexed blk*.dat:    536.429 GiB
Difference:                       ~171.326 GiB on disk footprint savings.

Optional indexes
Core v27:        79.813 GiB
BitcoinRocks:    70.606 GiB
Saved:            9.207 GiB
Reduction:       ~11.54%

Core txindex:        67.426 GiB
RocksDB txindex:     58.299 GiB
Saved:                9.126 GiB
Reduction:           ~13.54%

Coinstats Index:
Core:       0.158 GiB
RocksDB:    0.089 GiB
Reduction:  ~44%

Core Databases required for mininum "full node" operation:
chainstate
Core:         10.622 GiB
BitcoinRocks: 10.319 GiB
Reduction:     ~2.85%

blocks/index
Core:          0.152 GiB
BitcoinRocks:  0.121 GiB
Reduction:    ~20.44%

Two independent machines running Bitcoin Rocks v31.1.1 maintain databases that match byte for byte:
961,497 records
857,043 ZSTD-compressed
104,454 raw fallback
759,909,867,794 B logical payload
575,962,277,590 B stored payload
183,947,590,204 B saved
24.207% reduction
