set (SOURCES)

list (APPEND SOURCES "main.c")

list (APPEND SOURCES "../../../src/fdb.c" "../../../src/fdb_file.c" "../../../src/fdb_kvdb.c" "../../../src/fdb_tsdb.c" "../../../src/fdb_utils.c")

list (APPEND SOURCES "../../../port/fal/src/fal_flash.c" "../../../port/fal/src/fal_partition.c" "../../../port/fal/src/fal_rtt.c" "../../../port/fal/src/fal.c")

list (APPEND SOURCES "../../../samples/kvdb_basic_sample.c" "../../../samples/kvdb_type_blob_sample.c" "../../../samples/kvdb_type_string_sample.c" "../../../samples/tsdb_sample.c")

idf_component_register(SRCS "${SOURCES}"
                    INCLUDE_DIRS "../components/FlashDB/inc" "../../../inc" "../../../port/fal/inc"
                    REQUIRES "spi_flash")
