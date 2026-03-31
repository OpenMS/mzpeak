# https://mesonbuild.com/
project('MZPeak C++ Library', 'cpp',
        default_options : ['cpp_std=c++23'],
        version : '1.0.0',
        license : 'BSD3')

# Source files included in the library:
lib_sources = [
  'src/arrow.cpp',
  'src/directory.cpp',
  'src/file.cpp',
  'src/zip.cpp',
]

# Header files to include in packages:
install_headers([
  'include/mzpeak/archive.h',
  'include/mzpeak/arrow.h',
  'include/mzpeak/buffer.h',
  'include/mzpeak/directory.h',
  'include/mzpeak/file.h',
  'include/mzpeak/zip.h',
], subdir : 'mzpeak')

# Library and executable targets:
include = include_directories('include')

arrow_ver_str = '>=23.0.0'

deps = [
    dependency('arrow',
               version : arrow_ver_str,
               required : true),

    dependency('boost',
               version : '>=1.89.0',
               required : true),

    dependency('libzip',
               version :'>=1.11.4',
               required : true),

    dependency('parquet', # Part of Arrow above.
               version : arrow_ver_str,
               required : true),
]

libmzpeak = library('mzpeak', lib_sources,
                    dependencies : deps,
                    include_directories : include,
                    install : true)

# Testing
test_names = [
    'arrow',
    'directory',
    'zip',
]

foreach test : test_names
    test(test,
         executable('@0@_test'.format(test),
                    'test/@0@_test.cpp'.format(test),
                    include_directories : include,
                    dependencies : deps,
                    link_with : libmzpeak),
         verbose : false)
endforeach
