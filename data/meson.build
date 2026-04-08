install_data('module.desktop', install_dir : module_dir)

edje_cc = find_program('edje_cc')

iwd_theme = custom_target('e-module-iwd.edj',
  input  : 'e-module-iwd.edc',
  output : 'e-module-iwd.edj',
  command : [edje_cc, '-id', meson.current_source_dir(), '@INPUT@', '@OUTPUT@'],
  install : true,
  install_dir : module_dir,
)
