project(
	'kvstore',
	'c',
	version: '0.1.0',
	default_options: [
		'c_std=c11',
		'warning_level=3',
	]
)

inc = include_directories('include')

sources = files(
	'src/main.c',
	'src/hashmap.c'
)

executable(
	'kvstore',
	sources,
	include_directories: inc,
	install: true
)

test_hashmap = executable(
	'test_hashmap',
	'tests/test_hashmap.c',
	'src/hashmap.c',
	include_directories: inc,
)

test(
	'hashmap',
	test_hashmap
)

test_wal = executable(
    'test_wal',
    'tests/test_wal.c',
    'src/wal.c',
    include_directories: inc,
)

test(
    'wal',
    test_wal
)
