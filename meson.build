project('e-iwd', 'c',
  version: '0.1.0',
  default_options: ['c_std=c11', 'warning_level=2'],
  meson_version: '>= 0.56.0'
)

# Dependencies
enlightenment = dependency('enlightenment')
eldbus = dependency('eldbus')
elementary = dependency('elementary')
ecore = dependency('ecore')
evas = dependency('evas')
edje = dependency('edje')
eina = dependency('eina')

# Get Enlightenment module API version
e_version = enlightenment.version().split('.')
e_major = e_version[0]
e_minor = e_version[1]

# Installation paths
module_name = 'iwd'
module_arch = '@0@-@1@-@2@.@3@'.format(
  host_machine.system(),
  host_machine.cpu_family(),
  e_major,
  e_minor
)

dir_module = join_paths(get_option('libdir'), 'enlightenment', 'modules', module_name)
dir_module_arch = join_paths(dir_module, module_arch)

# Configuration
conf_data = configuration_data()
conf_data.set_quoted('PACKAGE', meson.project_name())
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set_quoted('MODULE_ARCH', module_arch)

configure_file(
  output: 'config.h',
  configuration: conf_data
)

# Add configuration include and feature test macros
add_project_arguments(
  '-include', 'config.h',
  '-D_GNU_SOURCE',
  language: 'c'
)

# Subdirectories
subdir('src')
subdir('data')

# Summary
summary({
  'Module name': module_name,
  'Module architecture': module_arch,
  'Installation path': dir_module_arch,
  'Enlightenment version': enlightenment.version(),
}, section: 'Configuration')
