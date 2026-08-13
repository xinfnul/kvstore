# ---------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------

test_hashmap = executable(
    'test_hashmap',
    'test_hashmap.c',
    dependencies: kvstore_dep,
)

test(
    'hashmap',
    test_hashmap,
    suite: 'unit',
)

test_wal = executable(
    'test_wal',
    'test_wal.c',
    dependencies: kvstore_dep,
)

test(
    'wal',
    test_wal,
    suite: 'unit',
)

test_database = executable(
    'test_database',
    'test_database.c',
    dependencies: kvstore_dep,
)

test(
    'database',
    test_database,
    suite: 'unit',
)

test_protocol = executable(
    'test_protocol',
    'test_protocol.c',
    dependencies: kvstore_dep,
)

test(
    'protocol',
    test_protocol,
    suite: 'unit',
)

test_command = executable(
    'test_command',
    'test_command.c',
    dependencies: kvstore_dep,
)

test(
    'command',
    test_command,
    suite: 'unit',
)

test_network = executable(
    'test_network',
    'test_network.c',
    dependencies: kvstore_dep,
)

test(
    'network',
    test_network,
    suite: 'unit',
)