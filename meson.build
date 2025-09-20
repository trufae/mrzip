project('mzip', 'c',
  version : '0.2.0',
  default_options : ['c_std=c99', 'warning_level=2'])

executable('mzip', 'src/main.c', 'src/lib/mzip.c',
  include_directories: include_directories('src/include'),
  install : true)
