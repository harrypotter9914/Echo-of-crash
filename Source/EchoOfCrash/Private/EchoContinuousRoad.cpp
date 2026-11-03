#include "EchoCampaign.h"
#include "EchoGameInstance.h"
#include "EchoPresentation.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/Level.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace {
// Arc length, rather than a clock jump, drives every straight and corner.
struct FJourneyPath {
 FVector P; float Yaw; float Length=0; float Remaining; FTransform Sample; bool Found=false;
 FJourneyPath(FVector Start,float Heading,float Distance):P(Start),Yaw(Heading),Remaining(FMath::Max(0.f,Distance)){P.Z=0;}
 void Segment(float Distance,float Curvature=0){
  Distance=FMath::Max(0.f,Distance);
  auto At=[&](float S){float A=Yaw+S*Curvature;FVector Q=P;
   if(FMath::Abs(Curvature)<.000001f)Q+=FVector(FMath::Cos(Yaw),FMath::Sin(Yaw),0)*S;
   else Q+=FVector((FMath::Sin(A)-FMath::Sin(Yaw))/Curvature,(FMath::Cos(Yaw)-FMath::Cos(A))/Curvature,0);
   return FTransform(FRotator(0,FMath::RadiansToDegrees(A),0),Q);};
  if(!Found && Remaining<=Distance){Sample=At(Remaining);Found=true;}
  auto End=At(Distance);P=End.GetLocation();Yaw+=Distance*Curvature;Length+=Distance;Remaining-=Distance;
  if(!Found)Sample=End;
 }
};
FJourneyPath MakeJourney(int Chapter,const FTransform& Impact,float Distance){
 FJourneyPath R(Impact.GetLocation(),Chapter==0?PI*.5f:0,Distance);
 if(Chapter==0){
  // MainScene's authored spline points 1/2 follow the offset in this road.
  // Keep the right lane on both sides of the offset, with rounded steering.
  R.Segment(6250-R.P.Y);R.Segment(500*PI*.25f,1.f/500);
  R.Segment((500-1000*(1-FMath::Cos(PI*.25f)))/FMath::Sin(PI*.25f));
  R.Segment(500*PI*.25f,-1.f/500);
  R.Segment(13250-R.P.Y);R.Segment(1000*PI*.5f,-1.f/1000);
  R.Segment(-3739-R.P.X);
 }else if(Chapter==1){
  // Same 949 cm exit corner as the verified side-impact scene.
  R.Segment(FMath::Max(0.f,250-R.P.X-949));R.Segment(949*PI*.5f,-1.f/949);
  // Original points 8--12: south down the central avenue, then east at the
  // truck junction. The earlier invented eastern-block detour is not used.
  R.Segment(R.P.Y-250);R.Segment(500*PI*.5f,1.f/500);
  R.Segment(1030-R.P.X);
 }
 return R;
}
}
FTransform AEchoCampaignDirector::JourneyRoadAt(float Distance) const {return MakeJourney(Chapter,PlayerImpact,Distance).Sample;}
float AEchoCampaignDirector::JourneyRoadLength() const {return MakeJourney(Chapter,PlayerImpact,0).Length;}

void AEchoCampaignDirector::PreloadIncidents()
{
 for(int I=Chapter+1;I<3;++I){
  FString Name=FString::Printf(TEXT("/Game/EchoCampaign/Incidents/Case%02d"),I+1);bool Loaded=false;
  auto Level=ULevelStreamingDynamic::LoadLevelInstance(this,Name,FVector::ZeroVector,FRotator::ZeroRotator,Loaded);
  if(Loaded)PendingIncidents.Add(Level);
 }
}
void AEchoCampaignDirector::ContinueRoad()
{
 if(Chapter>=2){BeginArrival();return;}
 AEchoCampaignDirector* Next=nullptr;
 for(TActorIterator<AEchoCampaignDirector> It(GetWorld());It;++It)
  if(It->ActorHasTag(TEXT("EchoIncidentTemplate")) && It->Chapter==Chapter+1){Next=*It;break;}
 if(!Next){UE_LOG(LogTemp,Error,TEXT("Next incident is not ready; retaining the current road and camera"));return;}
 SaveCheckpoint(Chapter+1);
 StopDialogue();NoticePages.Empty();Notice.Empty();
 for(auto& C:Clues)if(IsValid(C.Preview))C.Preview->Destroy();
 for(auto A:Tableau)if(IsValid(A))A->SetActorHiddenInGame(true);
 Tableau.Empty();
 for(auto A:Next->GetLevel()->Actors)if(IsValid(A) && A->ActorHasTag(TEXT("EchoTableau")))Tableau.Add(A);
 Chapter=Next->Chapter;Clues=Next->Clues;Route=Next->Route;OpponentRoute=Next->OpponentRoute;DriveDuration=Next->DriveDuration;
 Next->Destroy();ConfigureChapter();RestoreTableauPoses();BindClues();
 Scored=false;ChapterScore=ChapterStars=MoodDelta=RawChapterScore=HintPenalty=0;
 CorrectDeduction=Arrest=ApproachHint=RescueHint=SearchHint=LowTimeSpoken=false;
 Focus=HintTarget=InspectIndex=INDEX_NONE;FocusReachable=false;
 RescueProgress=RescueCostFeedback=IdleTime=ScanTime=HintCooldown=BoundaryCooldown=FinishConfirmTime=0;
 IdleHintStage=0;LastStamp=-1;JournalOpen=false;
 if(auto GI=Cast<UEchoGameInstance>(GetGameInstance())){GI->Chapter=Chapter;MoodBefore=GI->Mood;GI->BeginInvestigation(FName(*FString::Printf(TEXT("Chapter%d"),Chapter+1)));}
 SeamlessApproach=true;BeginDrive();Say(Chapter==1?TEXT("road_two"):TEXT("road_three"));
}
