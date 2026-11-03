#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "EchoJourneyRules.h"
#include "EchoGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEchoInvestigationChanged);

/** Session state survives accident-level transitions. IDs are unique within each accident. */
UCLASS(BlueprintType)
class ECHOOFCRASH_API UEchoGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UPROPERTY() bool FrontEndEntered=false;
    UPROPERTY() bool StartAtTutorial=false;
    UPROPERTY() bool PreferencesLoaded=false;
    UPROPERTY(BlueprintReadWrite) float VoiceVolume=1;
    UPROPERTY(BlueprintReadWrite) float SoundVolume=.8f;
    UPROPERTY(BlueprintReadWrite) float LookSensitivity=1;
    UPROPERTY(BlueprintReadWrite) bool Subtitles=true;
    UPROPERTY(BlueprintReadWrite, Category="Echo|Campaign") int32 Mood = 60;
    UPROPERTY(BlueprintReadWrite, Category="Echo|Campaign") int32 TotalScore = 0;
    UPROPERTY(BlueprintReadWrite, Category="Echo|Campaign") int32 Avoided = 0;
    UPROPERTY(BlueprintReadWrite, Category="Echo|Campaign") EEchoBalance Balance = EEchoBalance::Journey;
    UPROPERTY(BlueprintReadWrite, Category="Echo|Campaign") bool JourneyStarted = false;
    UPROPERTY() bool ContinueDriving = false;
    UPROPERTY(BlueprintReadWrite, Category="Echo|Campaign")
    int32 Chapter = 0;
    UPROPERTY(BlueprintReadWrite, Category="Echo|Campaign")
    int32 TotalSaved = 0;
    UPROPERTY(BlueprintReadWrite, Category="Echo|Campaign")
    int32 TotalEvidence = 0;
    UPROPERTY(BlueprintReadWrite, Category="Echo|Campaign")
    int32 TotalDeductions = 0;
    UPROPERTY(BlueprintReadOnly, Category="Echo|Investigation")
    FName CurrentAccident;

    UPROPERTY(BlueprintReadOnly, Category="Echo|Investigation")
    TSet<FName> CollectedEvidence;

    UPROPERTY(BlueprintReadOnly, Category="Echo|Investigation")
    TSet<FName> RescuedPeople;

    UPROPERTY(BlueprintAssignable, Category="Echo|Investigation")
    FEchoInvestigationChanged OnInvestigationChanged;

    /** Starts a new investigation, clearing the previous accident's state. */
    UFUNCTION(BlueprintCallable, Category="Echo|Investigation")
    void BeginInvestigation(FName AccidentId);

    /** Returns true only when new, nonempty evidence was added. */
    UFUNCTION(BlueprintCallable, Category="Echo|Investigation")
    bool CollectEvidence(FName EvidenceId);

    /** Returns true only on the first successful rescue of a nonempty ID. */
    UFUNCTION(BlueprintCallable, Category="Echo|Investigation")
    bool RecordRescue(FName PersonId);
};
