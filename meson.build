project(
	'project_convergent',
	'c',
	version: '0.10.1',
	license: 'MIT',
	meson_version: '>=0.51.2',
	default_options: [
		'c_std=c11',
		'warning_level=2',
		'werror=true',
	],
)

add_project_arguments([
	'-DWLR_USE_UNSTABLE',
], language: 'c')

cc = meson.get_compiler('c')

add_project_arguments(cc.get_supported_arguments([
	'-Wundef',
	'-Wlogical-op',
	'-Wmissing-include-dirs',
	'-Wold-style-definition',
	'-Wpointer-arith',
	'-Winit-self',
	'-Wstrict-prototypes',
	'-Wimplicit-fallthrough=2',
	'-Wendif-labels',
	'-Wstrict-aliasing=2',
	'-Woverflow',
	'-Wmissing-prototypes',

	'-Wno-missing-braces',
	'-Wno-missing-field-initializers',
	'-Wno-unused-parameter',
]), language: 'c')

# Compute the relative path used by compiler invocations.
source_root = meson.current_source_dir().split('/')
build_root = meson.build_root().split('/')
relative_dir_parts = []
i = 0
in_prefix = true
foreach p : build_root
	if i >= source_root.length() or not in_prefix or p != source_root[i]
		in_prefix = false
		relative_dir_parts += '..'
	endif
	i += 1
endforeach
i = 0
in_prefix = true
foreach p : source_root
	if i >= build_root.length() or not in_prefix or build_root[i] != p
		in_prefix = false
		relative_dir_parts += p
	endif
	i += 1
endforeach
relative_dir = join_paths(relative_dir_parts) + '/'

# Clang complains about some zeroed initializer lists (= {0}), even though they
# are valid
if cc.get_id() == 'clang'
	add_project_arguments('-Wno-missing-field-initializers', language: 'c')
	add_project_arguments('-Wno-missing-braces', language: 'c')
endif

wlroots = dependency('wlroots')
wayland_protos = dependency('wayland-protocols', version: '>=1,17')
wayland_client = dependency('wayland-client')
wayland_cursor = dependency('wayland-cursor')
wayland_egl = dependency('wayland-egl')
egl = dependency('egl')
glesv2 = dependency('glesv2')

protocol_files = []

subdir('protocol')
#subdir('examples')

executable(
    'bar',
    [
      'main.c',
      get_variable('wlr_layer_shell_unstable_v1_c'),
      get_variable('wlr_layer_shell_unstable_v1_client_h'),
      get_variable('xdg_shell_c'),
      get_variable('xdg_shell_client_h')
    ],
    dependencies: [
      wayland_client,
      wayland_cursor,
      wlroots,
      wayland_egl,
      egl,
      glesv2
    ]
)
