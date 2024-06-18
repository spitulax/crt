cwd = meson.current_source_dir()

sqlite3 = declare_dependency(
  dependencies : cc.find_library('sqlite3', dirs : [ cwd / 'sqlite3/build/.libs' ], static : true),
  include_directories : include_directories('sqlite3'),
)

deps = [
  sqlite3
]
