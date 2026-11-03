#include "EchoCampaign.h"
#include "EchoPresentation.h"
#include "Framework/Application/SlateApplication.h"
#include "EchoGameInstance.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/AudioComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"

// Continue through each authored junction instead of extending its approach
// ray into buildings or water. Distance zero is always the placed impact point.
static FTransform RoadExit(int Chapter,bool Opponent,const FTransform& Impact,float Distance)
{
 FVector P=Impact.GetLocation();P.Z=0;
 FVector F=Opponent?(Chapter==0?FVector(0,-1,0):Chapter==1?FVector(-1,0,0):FVector(1,0,0)):(Chapter==0?FVector(0,1,0):FVector(1,0,0));
 if(Opponent && Chapter==2){
  const float Start=FMath::DegreesToRadians(Impact.Rotator().Yaw),Radius=700.f,Arc=Radius*Start;
  const float A=FMath::Max(0.f,Start-Distance/Radius);
  P+=FVector(Radius*(FMath::Sin(Start)-FMath::Sin(A)),Radius*(FMath::Cos(A)-FMath::Cos(Start)),0);
  if(Distance>Arc)P.X+=Distance-Arc;
  return FTransform(FRotator(0,FMath::RadiansToDegrees(A),0),P);
 }
 if(Opponent && Chapter==1){
  // The collision is on the sports car's right front SIDE, partway through its
  // corner. Complete that same turn before recovering into the westbound lane.
  const float Start=FMath::DegreesToRadians(Impact.Rotator().Yaw),Radius=280.f,Arc=Radius*(PI-Start);
  float A=Start+FMath::Clamp(Distance/Radius,0.f,PI-Start);
  P+=FVector(Radius*(FMath::Sin(A)-FMath::Sin(Start)),Radius*(FMath::Cos(Start)-FMath::Cos(A)),0);
  if(Distance<=Arc)return FTransform(FRotator(0,FMath::RadiansToDegrees(A),0),P);
  const float Beyond=Distance-Arc,Shift=13750.f-P.Y,Lead=900.f,Length=1200.f;
  float U=FMath::Clamp((Beyond-Lead)/Length,0.f,1.f),Ease=U*U*U*(10+U*(-15+6*U)),Slope=U>0 && U<1?30*U*U*(1-U)*(1-U):0;
  return FTransform(FVector(-1,Shift/Length*Slope,0).Rotation(),P+FVector(-Beyond,Shift*Ease,0));
 }
 // The sports car recovers from its wide turn into the westbound lane.
 // The truck finishes its left turn into the eastbound lane while we yield.
 if(Opponent && Chapter>0){
  float Length=Chapter==1?900.f:1100.f,Shift=Chapter==1?-500.f:300.f,Lead=Chapter==2?400.f:0.f,U=FMath::Clamp((Distance+Lead)/Length,0.f,1.f);
  float Ease=U*U*U*(10+U*(-15+6*U)),Slope=U>0 && U<1?30*U*U*(1-U)*(1-U):0;
  FVector Heading=F+FVector(0,Shift/Length*Slope,0);
  return FTransform(Heading.Rotation(),P+F*Distance+FVector(0,Shift*Ease,0));
 }
 float Lead=0,Radius=0,Turn=0;
 if(Chapter==1 && !Opponent){Radius=949;Lead=FMath::Max(0.f,250.f-P.X-Radius);Turn=-1;}
 if(Radius<=0 || Distance<=Lead)return FTransform(F.Rotation(),P+F*Distance);
 P+=F*Lead;Distance-=Lead;FVector Right(-F.Y,F.X,0);float Arc=Radius*PI*.5f,A=FMath::Min(Distance/Radius,PI*.5f);
 P+=F*(Radius*FMath::Sin(A))+Right*(Turn*Radius*(1-FMath::Cos(A)));
 FVector Heading=F*FMath::Cos(A)+Right*(Turn*FMath::Sin(A));
 if(Distance>Arc)P+=Heading*(Distance-Arc);
 return FTransform(Heading.Rotation(),P);
}

void AEchoCampaignDirector::BindClues()
{
 for(auto A:Tableau)if(A){
  if(A->ActorHasTag(TEXT("EchoPlayerCar")))PlayerImpact=A->GetActorTransform();
  if(A->ActorHasTag(TEXT("EchoOpponentCar")))OpponentImpact=A->GetActorTransform();
  for(int I=0;I<Clues.Num();++I)if(A->ActorHasTag(FName(*FString::Printf(TEXT("EchoClue_%d"),I)))){
   Clues[I].SourceActor=A;Clues[I].Position=A->GetActorLocation();
   if(Clues[I].Rescue)if(auto C=A->FindComponentByClass<USkeletalMeshComponent>())Clues[I].Position=C->GetSocketLocation(TEXT("head"));
  }
 }
 FVector End=PlayerImpact.GetLocation();End.Z=0;
 FVector F=Chapter==0?FVector(0,1,0):FVector(1,0,0);
 // Route assets expose road centerline points. Retain authored bends and meet the placed accident.
 if(Route.Num()<2)Route={End-F*3000,End-F*1800,End-F*700,End};
 else{for(auto& P:Route)P.Z=0;Route.Last()=End;}
 FVector OpponentEnd=OpponentImpact.GetLocation();OpponentEnd.Z=0;
 if(OpponentRoute.Num()<2){FVector OF=Chapter==0?FVector(0,-1,0):FVector(0,1,0);OpponentRoute={OpponentEnd-OF*2900,OpponentEnd-OF*1800,OpponentEnd-OF*800,OpponentEnd};}
 else {for(auto& P:OpponentRoute)P.Z=0;OpponentRoute.Last()=OpponentEnd;}
 Center=(PlayerImpact.GetLocation()+OpponentImpact.GetLocation())*.5f;Center.Z=150;
}
void AEchoCampaignDirector::PreparePreviews()
{
 for(int I=0;I<Clues.Num();++I){auto& C=Clues[I];if(C.SourceActor && !C.Preview){C.Preview=GetWorld()->SpawnActor<AEchoPreviewRig>(FVector(0,0,-50000-I*1500),FRotator::ZeroRotator);C.Preview->Build(C.SourceActor,C.Rescue);}}
}
void AEchoCampaignDirector::Inspect(int32 I)
{
 if(I!=INDEX_NONE && (Phase!=EEchoPhase::Investigation || !Clues.IsValidIndex(I) || !Clues[I].Complete))return;
 if(Clues.IsValidIndex(InspectIndex) && Clues[InspectIndex].Preview)Clues[InspectIndex].Preview->ResetView();
 SoundEvent(I==INDEX_NONE?TEXT("UIBack"):TEXT("Paper"),.25f);InspectIndex=I;if(VoiceAudio)VoiceAudio->SetPaused(I!=INDEX_NONE || JournalOpen);RescueProgress=0;auto PC=GetWorld()->GetFirstPlayerController();SetCursor(I!=INDEX_NONE || JournalOpen);PC->GetMousePosition(InspectMouseX,InspectMouseY);
 if(Clues.IsValidIndex(I) && Clues[I].Preview)Clues[I].Preview->Capture();
}
void AEchoCampaignDirector::SetBalance(EEchoBalance Preset)
{
 if(Phase!=EEchoPhase::Introduction)return;
 if(auto GI=Cast<UEchoGameInstance>(GetGameInstance())){if(Chapter!=0 || GI->TotalScore>0){SetNotice(TEXT("Balance is locked for this journey. Start a new trip to change it."));return;}GI->Balance=Preset;Rules=FEchoJourneyRules::Preset(Preset);GI->Mood=Rules.StartingMood;MoodBefore=GI->Mood;}
 SetNotice(Preset==EEchoBalance::Story?TEXT("Story / 120 seconds, earlier guidance, gentler mood loss."):Preset==EEchoBalance::Journey?TEXT("Journey / 100 seconds, balanced recovery and consequences."):TEXT("Tension / 80 seconds, later guidance, stronger consequences."));
}
void AEchoCampaignDirector::BeginImpact()
{
 if(Phase==EEchoPhase::Impact)return;
 Phase=EEchoPhase::Impact;PhaseTime=0;DriveCar->SetActorTransform(PlayerImpact);OpponentCar->SetActorTransform(OpponentImpact);
 if(EngineAudio)EngineAudio->Stop();if(OpponentAudio)OpponentAudio->Stop();StopDialogue();PlayFeedback(TEXT("Impact"));SoundEvent(TEXT("WhiteVeil"),.4f);
 auto PC=GetWorld()->GetFirstPlayerController();if(PC->PlayerCameraManager)PC->PlayerCameraManager->StartCameraFade(0,1,.15,FLinearColor::White,false,true);
}
void AEchoCampaignDirector::TickAvoidance(float Dt)
{
 if(!DriveCar || !OpponentCar)return;
 // Crossing traffic needs braking as well as steering: a lane change alone
 // moves the pickup into the sports car's later path through the junction.
 float T=PhaseTime;bool CrossTraffic=Chapter>0;float StopTime=Chapter==1?.25f:.55f,Restart=FMath::Max(0.f,T-(Chapter==1?3.2f:3.f));
 float BrakingSpeed=T<StopTime?Speed*(1-T/StopTime):FMath::Min(650.f,220*Restart);
 RollWheels(Dt,CrossTraffic?BrakingSpeed:Speed,650);
 float Travel=CrossTraffic?(T<StopTime?Speed*(T-T*T/(2*StopTime)):Speed*StopTime*.5f+(Restart<650.f/220?110*Restart*Restart:650*Restart-650.f*650/440)):Speed*T;
 auto Ease=[](float U){U=FMath::Clamp(U,0.f,1.f);return U*U*U*(10+U*(-15+6*U));};
 auto Derivative=[](float U){return U>0 && U<1?30*U*U*(1-U)*(1-U):0.f;};
 float Lane=Chapter==1?-210.f:-450.f,TurnTime=Chapter==1?.65f:.65f;
 float Side=Chapter==2?0:T<2?Lane*Ease(T/TurnTime):Lane*(1-Ease((T-2)/2.2f));
 float SideVelocity=Chapter==2?0:T<2?Lane/TurnTime*Derivative(T/TurnTime):-Lane/2.2f*Derivative((T-2)/2.2f);
 PlayerExitDistance=FVector::DotProduct(AvoidStart-PlayerImpact.GetLocation(),AvoidForward)+Travel;
 auto Road=RoadExit(Chapter,false,PlayerImpact,PlayerExitDistance);FVector RoadForward=Road.GetRotation().GetForwardVector(),RoadRight=Road.GetRotation().GetRightVector();
 FVector P=Road.GetLocation()+RoadRight*Side;FVector Velocity=RoadForward*FMath::Max(50.f,Speed)+RoadRight*SideVelocity;
 FRotator R=Velocity.Rotation();R.Roll=Chapter<2?-SideVelocity*.003:0;R.Pitch=CrossTraffic && T<.55?-4*FMath::Sin(PI*T/.55):0;DriveCar->SetActorLocationAndRotation(P,R);
 float Distance=OpponentStartDistance+T*650;float Length=OpponentSpline->GetSplineLength();OpponentExitDistance=Distance-Length;
 if(Distance<=Length){FVector OP=OpponentSpline->GetLocationAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World);FVector OF=OpponentSpline->GetDirectionAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World);
  // On a successful yield, finish steering into the opposing lane before
  // reaching the dangerous frozen-impact pose; keep position/tangent continuous.
  OpponentCar->SetActorLocationAndRotation(OP,OF.Rotation());}
 else OpponentCar->SetActorTransform(RoadExit(Chapter,true,OpponentImpact,OpponentExitDistance));
 UpdateCockpit(Dt);
 if(T>.8 && T-Dt<=.8){PlayFeedback(TEXT("NearMiss"));Conversation({Chapter==2?FName(TEXT("truck_clear")):FName(TEXT("near_miss")),TEXT("near_miss_wife")});}
 if(EngineAudio)EngineAudio->SetPitchMultiplier(CrossTraffic && Restart<=0?.65:1.15);
 if(T>6)CompleteAvoidance();
}
void AEchoCampaignDirector::CompleteAvoidance()
{
 if(Scored)return;Scored=true;ChapterScore=1000;ChapterStars=5;
 if(auto GI=Cast<UEchoGameInstance>(GetGameInstance())){MoodBefore=GI->Mood;GI->Mood=FMath::Clamp(GI->Mood+Rules.AvoidanceMood,0,100);MoodDelta=GI->Mood-MoodBefore;++GI->Avoided;GI->TotalScore+=ChapterScore;}
 if(Chapter>0)Speed=650;
 // Keep the same city, pickup, cabin and camera for the complete road journey.
 Phase=EEchoPhase::Departure;PhaseTime=0;DepartureFadeAt=-1;AvoidStart=DriveCar->GetActorLocation();AvoidForward=DriveCar->GetActorForwardVector();
 // Let the near-miss exchange finish naturally during departure.
}
void AEchoCampaignDirector::BeginOutcome()
{
 SetEchoAtmosphere(false);Phase=EEchoPhase::Outcome;PhaseTime=0;GetWorld()->GetFirstPlayerController()->bShowMouseCursor=true;
 if(Atmosphere)Atmosphere->Stop();
 if(auto Pawn=GetWorld()->GetFirstPlayerController()->GetPawn())Pawn->SetActorHiddenInGame(true);
 Cinema=GetWorld()->SpawnActor<AEchoStoryScene>();Cinema->Start(this,false);
 GetWorld()->GetFirstPlayerController()->PlayerCameraManager->StartCameraFade(1,0,.8f,FLinearColor::Black,false,false);
 Conversation({TEXT("reality")});
}
void AEchoCampaignDirector::BeginDeparture()
{
 auto PC=GetWorld()->GetFirstPlayerController();
 if(Cinema){Cinema->Destroy();Cinema=nullptr;}
 for(auto A:Tableau)if(A)A->SetActorHiddenInGame(true);
 if(auto GI=Cast<UEchoGameInstance>(GetGameInstance()))if(GI->Mood<=0){BeginArrival();return;}
 // Clear the incident before revealing the restored pickup: no visible despawning in the shot.
 bool WonQTE=QTESuccess;SeamlessApproach=false;BeginDrive();QTESuccess=WonQTE;
 if(OpponentCar)OpponentCar->Destroy();if(OpponentAudio)OpponentAudio->Stop();
 FVector P=PlayerImpact.GetLocation();P.Z=0;PlayerExitDistance=0;AvoidForward=Chapter==0?FVector(0,1,0):FVector(1,0,0);AvoidStart=P;
 DriveCar->SetActorLocationAndRotation(P,AvoidForward.Rotation());UpdateCockpit(0);Phase=EEchoPhase::Departure;PhaseTime=0;DepartureFadeAt=-1;
 if(PC->PlayerCameraManager)PC->PlayerCameraManager->StartCameraFade(1,0,.8,FLinearColor::Black,false,false);
 bool WifeSaved=false;for(auto& C:Clues)if(C.Title.Contains(TEXT("wife")) && C.Complete)WifeSaved=true;Conversation({WifeSaved?FName(TEXT("continue_wife")):FName(TEXT("continue_guide"))});
}
void AEchoCampaignDirector::BeginArrival()
{
 SetEchoAtmosphere(false);
 if(DriveCar)DriveCar->Destroy();if(OpponentCar)OpponentCar->Destroy();if(EngineAudio)EngineAudio->Stop();if(OpponentAudio)OpponentAudio->Stop();
 if(IsValid(Cinema)){Cinema->Destroy();Cinema=nullptr;}
 Phase=EEchoPhase::Arrival;PhaseTime=0;Cinema=GetWorld()->SpawnActor<AEchoStoryScene>();Cinema->Start(this,true);
 GetWorld()->GetFirstPlayerController()->bShowMouseCursor=true;
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());int Ending=FEchoJourneyRules::Ending(GI?GI->Mood:0);
 Conversation(Ending==0?TArray<FName>{TEXT("ending_home"),TEXT("ending_home_hero")}:Ending==1?TArray<FName>{TEXT("ending_low"),TEXT("ending_low_hero")}:Ending==2?TArray<FName>{TEXT("ending_calm"),TEXT("ending_calm_wife")}:TArray<FName>{TEXT("ending_happy"),TEXT("ending_happy_wife")});
}
void AEchoCampaignDirector::TickPresentation(float Dt)
{
 auto PC=GetWorld()->GetFirstPlayerController();
 if(InspectIndex!=INDEX_NONE){if(Clues.IsValidIndex(InspectIndex) && Clues[InspectIndex].Preview){auto P=Clues[InspectIndex].Preview;float X=0,Y=0;PC->GetMousePosition(X,Y);float DX=X-InspectMouseX,DY=Y-InspectMouseY;InspectMouseX=X;InspectMouseY=Y;const auto& Buttons=FSlateApplication::Get().GetPressedMouseButtons();if(Buttons.Contains(EKeys::LeftMouseButton)||Buttons.Contains(EKeys::RightMouseButton))P->Rotate(DX,DY);if(PC->WasInputKeyJustPressed(EKeys::MouseScrollUp))P->Zoom(-15);if(PC->WasInputKeyJustPressed(EKeys::MouseScrollDown))P->Zoom(15);}return;}
 if(Phase==EEchoPhase::Impact && PhaseTime>=3.2)StartInvestigation();
 else if(Phase==EEchoPhase::Awakening){
  if(AwakeningDialoguePending && PhaseTime>=2.3f){AwakeningDialoguePending=false;Conversation(Chapter==0?TArray<FName>{TEXT("awakening_hero"),TEXT("awakening_guide"),TEXT("awakening_task"),TEXT("echo_rescue_lesson"),TEXT("clock_begins")}:TArray<FName>{TEXT("awakening_again"),TEXT("awakening_task"),TEXT("clock_begins")});}
  if(!AwakeningDialoguePending && PhaseTime>=2.3f && Notice.IsEmpty()){Phase=EEchoPhase::Investigation;PhaseTime=0;SetCursor(false);}}
 else if(Phase==EEchoPhase::Avoidance)TickAvoidance(Dt);
 else if(Phase==EEchoPhase::Departure){
  Speed=QTESuccess?FMath::FInterpConstantTo(FMath::Max(360.f,Speed),650.f,Dt,110.f):FMath::Min(650.f,200*PhaseTime);
  RollWheels(Dt,Speed,650);
  float HandoffRemainder=0;
  if(DriveCar){PlayerExitDistance+=Speed*Dt;if(Chapter<2){HandoffRemainder=FMath::Max(0.f,PlayerExitDistance-JourneyRoadLength());PlayerExitDistance=FMath::Min(PlayerExitDistance,JourneyRoadLength());}
   auto Road=Chapter<2?JourneyRoadAt(PlayerExitDistance):RoadExit(Chapter,false,PlayerImpact,PlayerExitDistance);
   Steering=FMath::FindDeltaAngleDegrees(DriveCar->GetActorRotation().Yaw,Road.Rotator().Yaw)/FMath::Max(Dt,.001f)*.45f;
   DriveCar->SetActorTransform(Road);UpdateCockpit(Dt);}
  if(QTESuccess && IsValid(OpponentCar)){
   OpponentExitDistance+=650*Dt;OpponentCar->SetActorTransform(RoadExit(Chapter,true,OpponentImpact,OpponentExitDistance));
   // Retire distant traffic only after it has driven away and left the view.
   // It must never disappear at the successful QTE or run beyond the city's roads.
   if(DriveCar && FVector::Dist2D(OpponentCar->GetActorLocation(),DriveCar->GetActorLocation())>6000){
    FVector2D Screen;int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
    bool Visible=PC->ProjectWorldLocationToScreen(OpponentCar->GetActorLocation(),Screen) && Screen.X>-300 && Screen.X<Width+300 && Screen.Y>-300 && Screen.Y<Height+300;
    if(!Visible){if(OpponentAudio)OpponentAudio->Stop();OpponentCar->Destroy();}
   }
  }
  if(Chapter<2){if(PlayerExitDistance>=JourneyRoadLength()-.01f){WheelAngle-=HandoffRemainder/36*180/PI;ContinueRoad();if(Phase==EEchoPhase::Driving && HandoffRemainder>0)TickDrive(HandoffRemainder/650.f);}}
  else {
   if(PhaseTime>3.2 && Notice.IsEmpty() && DepartureFadeAt<0){DepartureFadeAt=PhaseTime;if(PC->PlayerCameraManager)PC->PlayerCameraManager->StartCameraFade(0,1,.8,FLinearColor::Black,false,true);}
   if(DepartureFadeAt>=0 && PhaseTime>=DepartureFadeAt+.9f){if(PC->PlayerCameraManager)PC->PlayerCameraManager->StartCameraFade(1,0,.8,FLinearColor::Black,false,false);ContinueRoad();}
  }}
 else if(Phase==EEchoPhase::Results){int Stamp=FMath::FloorToInt(PhaseTime/.48f);if(Stamp!=LastStamp && Stamp<Clues.Num()){LastStamp=Stamp;PlayFeedback(TEXT("Stamp"));}}
}

void AEchoCampaignDirector::RollWheels(float Dt,float PlayerSpeed,float OtherSpeed)
{
 WheelAngle=FMath::Fmod(WheelAngle+PlayerSpeed/36*180/PI*Dt,360.f);
 for(auto Pair:{TPair<UPoseableMeshComponent*,float>(DriveMesh,PlayerSpeed),TPair<UPoseableMeshComponent*,float>(OpponentMesh,OtherSpeed)}){
  auto M=Pair.Key;if(!IsValid(M) || !IsValid(M->GetOwner()))continue;
  for(int I=0;I<M->GetNumBones();++I){FName N=M->GetBoneName(I);if(N.ToString().Contains(TEXT("wheel"),ESearchCase::IgnoreCase)){auto R=M->GetBoneRotationByName(N,EBoneSpaces::ComponentSpace);R.Pitch+=Pair.Value/36*180/PI*Dt;M->SetBoneRotationByName(N,R,EBoneSpaces::ComponentSpace);}}
 }
}
