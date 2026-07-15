/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <utility>

#include "mzpeak/schema/psi/transform.h"
#include "mzpeak/util/compat.h" // IWYU pragma: keep

namespace MzPeak::Schema::PSI {

/******************************************************************************/
CV type_to_cv(Transform::Type t)
{
  switch (t) {
  case Transform::ZeroIntensityTrim:
    return CV("MS", "1003901");
  case Transform::ZeroIntensityInterpolation:
    return CV("MS", "1003902");
  }

  std::unreachable();
}

/******************************************************************************/
Transform::value_type to_value_type(const CV& cv)
{
  if (cv.code() == "MS") {
    if (cv.accession() == "1003901") {
      return Transform::ZeroIntensityTrim;
    } else if (cv.accession() == "1003902") {
      return Transform::ZeroIntensityInterpolation;
    }
  }
  return cv;
}

/******************************************************************************/
Transform::Transform(const CV& cv)
    : val_(to_value_type(cv))
{
}

/******************************************************************************/
Transform::Transform(Type t)
    : val_(t)
{
}

/******************************************************************************/
Transform::value_type Transform::value() const { return val_; }

/******************************************************************************/
std::optional<Transform::Type> Transform::type() const
{
  if (std::holds_alternative<Type>(val_)) {
    return std::get<Type>(val_);
  } else {
    return {};
  }
}

/******************************************************************************/
CV Transform::to_cv() const
{
  return std::visit(
      [](auto&& arg) -> CV {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Type>) {
          return type_to_cv(arg);
        } else if constexpr (std::is_same_v<T, CV>) {
          return arg;
        } else {
          static_assert(false_type<T>, "variant not handled");
        }
      },
      val_);
}

/******************************************************************************/
bool Transform::needs_delta_model() const noexcept
{
  return type()
      .and_then([](auto t) -> std::optional<bool> {
        switch (t) {
        case ZeroIntensityTrim:
          return false;
        case ZeroIntensityInterpolation:
          return true;
        }

        return {};
      })
      .value_or(false);
}

} // namespace MzPeak::Schema::PSI
