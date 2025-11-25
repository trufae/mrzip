project('otezip', 'c',
  version : '0.2.0',
  default_options : ['c_std=c99', 'warning_level=2'])

executable('otezip', 'src/main.c', 'src/lib/otezip.c',
  include_directories: include_directories('src/include'),
  install : true)
