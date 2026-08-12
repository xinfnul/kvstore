project(
	'kvstore',
	'c',
	version: '0.1.0',
	default_options: [
		'c_std=c11',
		'warning_level=3',
		'werror=true',
		'buildtype=debugoptimized'
	]
)

inc = include_directories('include')

# ---------------------------------------------------------------------
# Core Library
# ---------------------------------------------------------------------

kvstore_sources = files(
	'src/hashmap.c',
	'src/wal.c',
	'src/database.c',
	'src/protocol.c',
)

kvstore_lib = static_library(
	'kvstore',
	kvstore_sources,
	include_directories: inc,
	install: true
)

kvstore_dep = declare_dependency(
	link_with: kvstore_lib,
	include_directories: inc,
)

# ---------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------

executable(
	'kvstore',
	'src/main.c',
	dependencies: kvstore_dep,
	install: true
)

# ---------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------

test_hashmap = executable(
	'test_hashmap',
	'tests/test_hashmap.c',
	dependencies: kvstore_dep,
)

test(
	'hashmap',
	test_hashmap,
	suite: 'unit',
)

test_wal = executable(
    'test_wal',
    'tests/test_wal.c',
    dependencies: kvstore_dep,
  )

test(
    'wal',
    test_wal,
    suite: 'unit',
)

test_database = executable(
	'test_database',
	'tests/test_database.c',
	dependencies: kvstore_dep,
)

test(
	'database',
	test_database,
	suite: 'unit',
)

test_protocol = executable(
	'test_protocol',
	'tests/test_protocol.c',
	dependencies: kvstore_dep,
)

test(
	'protocol',
	test_protocol,
	suite: 'unit',
)

# ---------------------------------------------------------------------
# Installation
# ---------------------------------------------------------------------

install_headers(
  'include/hashmap.h',
  'include/wal.h',
  'include/database.h',
  'include/protocol.h',
  subdir: 'kvstore',
)
