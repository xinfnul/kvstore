# ---------------------------------------------------------------------
# Client executable
# ---------------------------------------------------------------------

kvstore_client_sources = files(
    'client.c',
    'main.c',
)

executable(
    'kvstore-client',
    kvstore_client_sources,
    include_directories: inc,
    dependencies: kvstore_dep,
    install: true,
)
