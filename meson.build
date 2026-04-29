project('e_iwd', 'c',
  version : '0.1.0',
  license : 'MIT',
  default_options : ['c_std=gnu99', 'warning_level=2'])

cc = meson.get_compiler('c')

eldbus    = dependency('eldbus')
elementary = dependency('elementary')
enlightenment = dependency('enlightenment')

module_arch = enlightenment.get_variable(pkgconfig: 'module_arch',
                                         default_value: 'linux-gnu-@0@'.format(host_machine.cpu()))
module_dir  = join_paths(get_option('libdir'), 'enlightenment', 'modules', 'iwd')

utf8_args = cc.get_supported_arguments(['-finput-charset=UTF-8',
                                        '-fexec-charset=UTF-8'])

add_project_arguments('-DPACKAGE="e_iwd"',
                      '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
                      utf8_args,
                      language : 'c')

subdir('src')
subdir('data')
