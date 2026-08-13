# ---------------------------------------------------------------------
# Core Library
# ---------------------------------------------------------------------

kvstore_sources = files(
    'hashmap.c',
    'wal.c',
    'database.c',
    'protocol.c',
    'byteorder.c',
    'command.c',
    'network.c',
)

kvstore_lib = static_library(
    'kvstore',
    kvstore_sources,
    include_directories: inc,
    install: true,
)

kvstore_dep = declare_dependency(
    link_with: kvstore_lib,
    include_directories: inc,
)
