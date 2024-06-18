project(
  'crt',
  'c', 'cpp',
  version : '0.1.1',
  license: 'MIT',
  default_options : [
    'c_std=c11',
    'cpp_std=c++11',
    'warning_level=3',
  ]
)

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')

if get_option('buildtype').startswith('debug')
  add_project_arguments(['-Og', '-DDEBUG'], language: ['c', 'cpp'])
endif

add_project_arguments('-DPROG_NAME="' + meson.project_name() + '"', language: ['c', 'cpp'])
add_project_arguments('-DPROG_VERSION="' + meson.project_version() + '"', language: ['c', 'cpp'])
add_project_arguments('-Wshadow -Wnon-virtual-dtor', language: 'cpp')

subdir('deps')
subdir('src')

lm = cc.find_library('m')
deps += lm

include = [
  'src',
]

executable(
  'crt',
  src,
  include_directories : include,
  dependencies : deps,
  install : true
)
