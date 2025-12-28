module_sources = files(
  'e_mod_main.c',
  'iwd/iwd_dbus.c',
  'iwd/iwd_device.c',
  'iwd/iwd_network.c',
  'iwd/iwd_agent.c',
)

# TODO: Add more source files as they are created in later phases
# Phase 3: e_mod_gadget.c, e_mod_popup.c, ui/*.c files

module_deps = [
  enlightenment,
  eldbus,
  elementary,
  ecore,
  evas,
  edje,
  eina
]

module_includes = include_directories('.', 'iwd', 'ui')

shared_module('module',
  module_sources,
  dependencies: module_deps,
  include_directories: module_includes,
  name_prefix: '',
  install: true,
  install_dir: dir_module_arch
)
