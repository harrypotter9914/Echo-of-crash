#pragma once
#include "CoreMinimal.h"
#include "EchoJourneyRules.generated.h"

UENUM(BlueprintType)
enum class EEchoBalance : uint8 { Story, Journey, Tension };

/** All presentation uses the same normalized score, independent of chapter item count. */
USTRUCT(BlueprintType)
struct FEchoJourneyRules
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float InvestigationSeconds = 100;
 // Additional cost on a completed rescue; the three-second hold also uses real time.
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float RescueTimeCost = 8;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float HintDelay = 22;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 StartingMood = 60;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 AvoidanceMood = 8;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EmptyCaseLoss = 60;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float LossScale = 1;
 static FEchoJourneyRules Preset(EEchoBalance P)
 {
  FEchoJourneyRules R;
  if(P==EEchoBalance::Story){R.InvestigationSeconds=120;R.RescueTimeCost=5;R.HintDelay=18;R.StartingMood=70;R.AvoidanceMood=10;R.EmptyCaseLoss=35;R.LossScale=.65f;}
  if(P==EEchoBalance::Tension){R.InvestigationSeconds=80;R.RescueTimeCost=12;R.HintDelay=30;R.StartingMood=50;R.AvoidanceMood=6;R.EmptyCaseLoss=65;R.LossScale=1.25f;}
  return R;
 }
 static int32 Score(int32 E,int32 ET,int32 R,int32 RT,bool Correct)
 {
  return FMath::Clamp(FMath::RoundToInt(400.f*E/FMath::Max(1,ET)+500.f*R/FMath::Max(1,RT))+(Correct?100:0),0,1000);
 }
 static int32 Stars(int32 S){return S>=950?5:S>=800?4:S>=600?3:S>=350?2:1;}
 int32 MoodChange(int32 Score,bool Empty) const
 {
  if(Empty)return -EmptyCaseLoss;
  int32 Change=FMath::RoundToInt((Score-650)/20.f);
  return Change<0?FMath::RoundToInt(Change*LossScale):Change;
 }
 static int32 Ending(int32 Mood){return Mood<=0?0:Mood<35?1:Mood<75?2:3;}
};
