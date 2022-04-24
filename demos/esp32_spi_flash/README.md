# flashDB demo esp32 Example

built with ESP IDF version 4.4.1 (v4.4.1-64-g4b2098bc58) using cmake.

run `idf.py build` in this directory and then flash with `idf.py flash` and monitor
the output with `idf.py monitor`.

A custom partition table is required so that a partition can be created for the FlashDB.
The size in the demo is set to 32K, see file `partitions.csv`. Custom partitions are
enabled by default in `sdkconfig.defaults`.

The file `sdkconfig.defaults` contains by default all values required for the demo to work,
i.e. the custom partitions are configured. By default the SPI flash is configured to
be 4MB.
