#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EchoGameplayLibrary.generated.h"

/** Shared deterministic gameplay rules, callable from the original Blueprints. */
UCLASS()
class ECHOOFCRASH_API UEchoGameplayLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Same range semantics as the original QTE graph; rejects invalid numeric input. */
    UFUNCTION(BlueprintPure, Category="Echo|QTE", meta=(DisplayName="Is QTE Hit"))
    static bool IsQTEHit(float Value, float Min, float Max,
                        bool InclusiveMin = true, bool InclusiveMax = true);

    /** Accumulate a held rescue interaction; releasing resets progress to zero. */
    UFUNCTION(BlueprintPure, Category="Echo|Rescue")
    static float AdvanceRescueProgress(float CurrentSeconds, float DeltaSeconds,
                                      bool bIsHeld, float RequiredSeconds = 3.0f);
};
