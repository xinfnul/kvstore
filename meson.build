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

subdir('src')
subdir('server')
subdir('tests')

install_headers(
  'include/hashmap.h',
  'include/wal.h',
  'include/database.h',
  'include/protocol.h',
  'include/byteorder.h',
  'include/command.h',
  'include/network.h',
  'server/server.h',
  subdir: 'kvstore',
)
