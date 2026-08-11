project(
	'kvstore',
	'c',
	version: '0.1.0',
	default_options: [
		'c_std=c11',
		'warning_level=3',
	]
)

project_version = meson.project_version()

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
