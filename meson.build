# https://mesonbuild.com/
project('msnumpres', 'cpp',
        version : '0.2.3')

################################################################################
# C++ Library and Test
cpp_sources = [ 'src/main/cpp/MSNumpress.cpp' ]
cpp_include = include_directories('src/main/cpp')

install_headers([
  'src/main/cpp/MSNumpress.hpp'
], subdir: 'MSNumpress')

cpp_lib_so = library(
  'numpress', cpp_sources,
  include_directories : cpp_include,
  install : true)

cpp_lib_a = static_library(
  'numpress', cpp_sources,
  include_directories : cpp_include,
  install : true)


test('MSNumpressTest',
     executable('MSNumpressTest',
                'src/main/cpp/MSNumpressTest.cpp',
                include_directories : cpp_include,
                link_with : cpp_lib_a),
     verbose : false)

msnumpres_cpp_lib_a = declare_dependency(
  include_directories : cpp_include,
  link_with : cpp_lib_a)

msnumpres_cpp_lib_so = declare_dependency(
  include_directories : cpp_include,
  link_with : cpp_lib_so)
