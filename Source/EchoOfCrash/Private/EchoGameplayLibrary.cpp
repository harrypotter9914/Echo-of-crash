#include "EchoGameplayLibrary.h"

bool UEchoGameplayLibrary::IsQTEHit(float Value, float Min, float Max,
                                   bool InclusiveMin, bool InclusiveMax)
{
    if (!FMath::IsFinite(Value) || !FMath::IsFinite(Min) || !FMath::IsFinite(Max) || Min > Max)
    {
        return false;
    }
    return (InclusiveMin ? Value >= Min : Value > Min)
        && (InclusiveMax ? Value <= Max : Value < Max);
}

float UEchoGameplayLibrary::AdvanceRescueProgress(float CurrentSeconds, float DeltaSeconds,
                                                  bool bIsHeld, float RequiredSeconds)
{
    if (!bIsHeld || !FMath::IsFinite(RequiredSeconds) || RequiredSeconds <= 0.0f)
    {
        return 0.0f;
    }
    const float Current = FMath::IsFinite(CurrentSeconds) ? FMath::Max(0.0f, CurrentSeconds) : 0.0f;
    const float Delta = FMath::IsFinite(DeltaSeconds) ? FMath::Max(0.0f, DeltaSeconds) : 0.0f;
    return FMath::Clamp(Current + Delta, 0.0f, RequiredSeconds);
}
