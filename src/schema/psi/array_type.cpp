/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/schema/psi/array_type.h"

namespace MzPeak::Schema::PSI {

/******************************************************************************/
std::string array_type_to_string(ArrayType v)
{
  using enum ArrayType;

  switch (v) {
  case Mz:
    return "MS:1000514";
  case Intensity:
    return "MS:1000515";
  case Charge:
    return "MS:1000516";
  case SignalToNoise:
    return "MS:1000517";
  case RelativeTimeOffset:
    return "MS:1000595";
  case ElectromagneticRadiation:
    return "MS:1000617";
  case FlowRate:
    return "MS:1000820";
  case Pressure:
    return "MS:1000821";
  case Temperature:
    return "MS:1000822";
  case MeanCharge:
    return "MS:1002478";
  case Resolution:
    return "MS:1002529";
  case SignalBaseline:
    return "MS:1002530";
  case Noise:
    return "MS:1002742";
  case SampledNoiseMz:
    return "MS:1002743";
  case SampledNoiseIntensity:
    return "MS:1002744";
  case SampledNoiseBaseline:
    return "MS:1002745";
  case IonMobility:
    return "MS:1002893";
  case Mass:
    return "MS:1003143";
  case ScanningQuadrupolePositionLowerBoundMz:
    return "MS:1003157";
  case ScanningQuadrupolePositionUpperBoundMz:
    return "MS:1003158";
  case NonStandard:
    return "MS:1000786";
  default:
    return "MS:1000786";
  }
}

/******************************************************************************/
ArrayType array_type_from_string(const std::string_view& s)
{
  using enum ArrayType;

  if (s == "MS:1000514") {
    return Mz;
  } else if (s == "MS:1000515") {
    return Intensity;
  } else if (s == "MS:1000516") {
    return Charge;
  } else if (s == "MS:1000517") {
    return SignalToNoise;
  } else if (s == "MS:1000595") {
    return RelativeTimeOffset;
  } else if (s == "MS:1000617") {
    return ElectromagneticRadiation;
  } else if (s == "MS:1000820") {
    return FlowRate;
  } else if (s == "MS:1000821") {
    return Pressure;
  } else if (s == "MS:1000822") {
    return Temperature;
  } else if (s == "MS:1002478") {
    return MeanCharge;
  } else if (s == "MS:1002529") {
    return Resolution;
  } else if (s == "MS:1002530") {
    return SignalBaseline;
  } else if (s == "MS:1002742") {
    return Noise;
  } else if (s == "MS:1002743") {
    return SampledNoiseMz;
  } else if (s == "MS:1002744") {
    return SampledNoiseIntensity;
  } else if (s == "MS:1002745") {
    return SampledNoiseBaseline;
  } else if (s == "MS:1002893") {
    return IonMobility;
  } else if (s == "MS:1003143") {
    return Mass;
  } else if (s == "MS:1003157") {
    return ScanningQuadrupolePositionLowerBoundMz;
  } else if (s == "MS:1003158") {
    return ScanningQuadrupolePositionUpperBoundMz;
  } else if (s == "MS:1000786") {
    return NonStandard;
  } else {
    return NonStandard;
  }
}

} // namespace MzPeak::Schema::PSI
