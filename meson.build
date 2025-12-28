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

# Get Enlightenment version and module architecture
e_version = enlightenment.version()

# Detect system ABI (gnu, musl, etc.)
cc = meson.get_compiler('c')
if cc.has_header('features.h')
  # GNU libc systems
  system_abi = 'gnu'
else
  # Try to detect from system - fallback to 'unknown'
  system_abi = 'unknown'
endif

# Installation paths
module_name = 'iwd'
# Format: <system>-<abi>-<cpu>-<version>
# Example: linux-gnu-x86_64-0.27.1
module_arch = '@0@-@1@-@2@-@3@'.format(
  host_machine.system(),
  system_abi,
  host_machine.cpu_family(),
  e_version
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

# Internationalization
i18n = import('i18n')
if get_option('nls')
  subdir('po')
endif

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
