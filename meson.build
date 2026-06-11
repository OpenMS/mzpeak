# https://mesonbuild.com/
project('MZPeak C++ Library', 'cpp',
        default_options : ['cpp_std=c++23'],
        version : '1.0.0',
        license : 'BSD3')

# Source files included in the library:
lib_sources = [
  'src/data/arrays.cpp',
  'src/data/metadata.cpp',
  'src/directory.cpp',
  'src/file.cpp',
  'src/index.cpp',
  'src/open.cpp',
  'src/query.cpp',
  'src/schema/array_index.cpp',
  'src/schema/buffer_format.cpp',
  'src/schema/data_kind.cpp',
  'src/schema/entity_type.cpp',
  'src/schema/file.cpp',
  'src/schema/psi/array_type.cpp',
  'src/schema/psi/data_type.cpp',
  'src/spectra.cpp',
  'src/spectrum.cpp',
  'src/util/arrow.cpp',
  'src/util/parquet.cpp',
  'src/util/struct.cpp',
  'src/zip.cpp',
]

# Header files to include in packages:
install_headers([
  'include/mzpeak.h',
  'include/mzpeak/archive.h',
  'include/mzpeak/data/arrays.h',
  'include/mzpeak/data/encoding.h',
  'include/mzpeak/data/metadata.h',
  'include/mzpeak/directory.h',
  'include/mzpeak/exception.h',
  'include/mzpeak/file.h',
  'include/mzpeak/index.h',
  'include/mzpeak/open.h',
  'include/mzpeak/query.h',
  'include/mzpeak/schema/array_index.h',
  'include/mzpeak/schema/buffer_format.h',
  'include/mzpeak/schema/data_kind.h',
  'include/mzpeak/schema/entity_type.h',
  'include/mzpeak/schema/file.h',
  'include/mzpeak/schema/psi/array_type.h',
  'include/mzpeak/schema/psi/data_type.h',
  'include/mzpeak/spectra.h',
  'include/mzpeak/spectrum.h',
  'include/mzpeak/util/arrow.h',
  'include/mzpeak/util/enumerable_proxy.h',
  'include/mzpeak/util/parquet.h',
  'include/mzpeak/util/parquet_types.h',
  'include/mzpeak/util/struct.h',
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
               modules : ['json', 'program_options'],
               required : true),

    dependency('libzip',
               version :'>=1.11.4',
               required : true),

    dependency('parquet', # Part of Arrow above.
               version : arrow_ver_str,
               required : true),
]

libmzpeak_so = library(
  'mzpeak', lib_sources,
  dependencies : deps,
  include_directories : include,
  install : true)

libmzpeak_a = static_library(
  'mzpeak', lib_sources,
  dependencies : deps,
  include_directories : include,
  install : true)

# Testing
test_names = [
    'arrow',
    'data_arrays',
    'directory',
    'index',
    'parquet',
    'query',
    'spectra',
    'struct',
    'util',
    'zip',
]

foreach test : test_names
    test(test,
         executable('@0@_test'.format(test),
                    'test/@0@_test.cpp'.format(test),
                    include_directories : include,
                    dependencies : deps,
                    link_with : libmzpeak_so),
         verbose : false)
endforeach

# Examples
example_names = [
  'read_spectra'
]

foreach example : example_names
  executable(
    example,
    'examples/@0@.cpp'.format(example),
    include_directories : include,
    dependencies : deps,
    link_with : libmzpeak_so,
    install : true,
    install_dir : 'examples')
endforeach

# Tools
tool_names = [
  'mzp-inspect'
]

foreach tool : tool_names
  executable(
    tool,
    'bin/@0@.cpp'.format(tool),
    include_directories : include,
    dependencies : deps,
    link_with : libmzpeak_so,
    install : true)
endforeach
