/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

// Compatibility with compilers that don't fully support C++23.

/// std::move_only_function
#if defined(_LIBCPP_VERSION) && !defined(__cpp_lib_move_only_function)
#include <boost/compat/move_only_function.hpp>
namespace std {
template <class S> using move_only_function = boost::compat::move_only_function<S>;
} // namespace std
#endif

// #endif
