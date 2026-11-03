#pragma once

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"

namespace EchoMouseLook
{
 constexpr float MinSensitivity = .2f;
 constexpr float MaxSensitivity = 4.f;
 constexpr float DegreesPerCount = .18f;

 inline float Gain(float Sensitivity)
 {
  return DegreesPerCount * FMath::Clamp(Sensitivity, MinSensitivity, MaxSensitivity);
 }

 inline void Read(APlayerController* PC, float& X, float& Y)
 {
  // Use accumulated device movement before axis sensitivity/FOV processing.
  // GetInputMouseDelta is already scaled by the default 0.07 axis gain;
  // multiplying that again made the original camera almost unresponsive.
  // Mouse displacement is not a velocity: never multiply by DeltaSeconds.
  X = PC && PC->PlayerInput ? PC->PlayerInput->GetRawKeyValue(EKeys::MouseX) : 0.f;
  Y = PC && PC->PlayerInput ? PC->PlayerInput->GetRawKeyValue(EKeys::MouseY) : 0.f;
 }
}
