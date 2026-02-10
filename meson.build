# https://mesonbuild.com/
project('MZPeak C++ Library', 'cpp',
        default_options : ['cpp_std=c++23'],
        version : '1.0.0',
        license : 'BSD3')

# Source files included in the library:
lib_sources = [
]

# Header files to include in packages:
install_headers([
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

library('mzpeak', lib_sources,
        dependencies : deps,
        include_directories : include,
        install : true)
