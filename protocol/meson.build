wl_protocol_dir = wayland_protos.get_variable(pkgconfig: 'pkgdatadir')

wayland_scanner_dep = dependency('wayland-scanner', required: false, native: true)
if wayland_scanner_dep.found()
	wayland_scanner = find_program(
		wayland_scanner_dep.get_variable(pkgconfig: 'wayland_scanner'),
		native: true,
	)
else
	wayland_scanner = find_program('wayland-scanner', native: true)
endif

protocols = {
    # Stable upstream sources
    'xdg-shell': wl_protocol_dir / 'stable/xdg-shell/xdg-shell.xml',

    # Other protocols
	'wlr-layer-shell-unstable-v1': 'wlr-layer-shell-unstable-v1.xml',
}

foreach name, path : protocols
	code = custom_target(
		name.underscorify() + '_c',
		input: path,
		output: '@BASENAME@-protocol.c',
		command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
	)
	protocol_files += code

	server_header = custom_target(
		name.underscorify() + '_server_h',
		input: path,
		output: '@BASENAME@-protocol.h',
		command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
	)
	protocol_files += server_header

	client_header = custom_target(
		name.underscorify() + '_client_h',
		input: path,
		output: '@BASENAME@-client-protocol.h',
		command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
		build_by_default: false,
	)

	set_variable(name.underscorify() + '_c', code)
	set_variable(name.underscorify() + '_server_h', server_header)
	set_variable(name.underscorify() + '_client_h', client_header)
endforeach
