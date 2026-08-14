# ---------------------------------------------------------------------
# Server executable
# ---------------------------------------------------------------------

kvstore_server_sources = files(
    'server.c',
    'main.c',
)

kvstore_server = executable(
    'kvstore-server',
    kvstore_server_sources,
    include_directories: inc,
    dependencies: kvstore_dep,
    install: true,
)
