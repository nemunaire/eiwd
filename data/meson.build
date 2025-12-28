# Install desktop file
install_data('module.desktop',
  install_dir: dir_module
)

# Compile theme
edje_cc = find_program('edje_cc', required: false)
if edje_cc.found()
  custom_target('theme',
    input: 'theme.edc',
    output: 'e-module-iwd.edj',
    command: [edje_cc, '@INPUT@', '@OUTPUT@'],
    install: true,
    install_dir: dir_module
  )
else
  warning('edje_cc not found, theme will not be compiled')
endif
