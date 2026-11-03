#include "EchoCampaign.h"
#include "EchoGameInstance.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Engine/GameViewportClient.h"

// Explicit development-only input regression. Never runs in an ordinary game session.
void AEchoCampaignDirector::TickControlVerification()
{
#if !UE_BUILD_SHIPPING
 static FString Mode;static bool Init=false,Enabled=false,Done=false;static double Start=0,Stamp=0,ImpactAt=0;static int Stage=-1;static FString Checks;static bool Passed=true,WheelChanged=false,HasWheel=false;static FRotator Wheel;static FVector Origin;static float Timer=0;static int Rescue=-1;static int LockStage=0;
 if(!Init){Init=true;FParse::Value(FCommandLine::Get(),TEXT("EchoVerify="),Mode);Enabled=Mode.StartsWith(TEXT("controls"));Start=FPlatformTime::Seconds();}
 if(!Enabled)return;double Now=FPlatformTime::Seconds();if(Done){if(Now-Stamp>2)FPlatformMisc::RequestExit(false);return;}
 auto PC=GetWorld()->GetFirstPlayerController();auto P=PC?Cast<AEchoGhostPawn>(PC->GetPawn()):nullptr;if(!P)return;
 auto Check=[&](const TCHAR* Name,bool Value){if(!Checks.IsEmpty())Checks+=TEXT(",");Checks+=FString::Printf(TEXT("{\"name\":\"%s\",\"passed\":%s}"),Name,Value?TEXT("true"):TEXT("false"));Passed&=Value;};
 auto Key=[&](FKey K,bool Down){PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,Down?IE_Pressed:IE_Released,FPlatformTime::Cycles64()));};
 auto Finish=[&](){FString Dir=FPaths::ProjectSavedDir()/TEXT("Verification");IFileManager::Get().MakeDirectory(*Dir,true);FString Events;for(const auto& E:AudioEvents){if(!Events.IsEmpty())Events+=TEXT(",");Events+=TEXT("\"")+E+TEXT("\"");}FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"checks\":[%s],\"audio_events\":[%s]}"),Passed?TEXT("true"):TEXT("false"),*Checks,*Events),*(Dir/(Mode+TEXT(".json"))));Done=true;Stamp=Now;};
 if(Now-Start>150){Check(TEXT("control test completed before deadline"),false);Finish();return;}
 if(Mode==TEXT("controls-look")){
#include "EchoMouseVerification.inl"
  return;
 }
 if(Stage==-1){Phase=EEchoPhase::Introduction;SetBalance(Mode==TEXT("controls-story")?EEchoBalance::Story:Mode==TEXT("controls-tension")?EEchoBalance::Tension:EEchoBalance::Journey);BeginDrive();Stage=0;}
 if(Stage==0){
  if(Phase==EEchoPhase::Driving && IsValid(DriveMesh)){auto R=DriveMesh->GetBoneRotationByName(TEXT("SM_Veh_Pickup_Wheel_rl"),EBoneSpaces::ComponentSpace);if(HasWheel && !R.Equals(Wheel,.2f))WheelChanged=true;Wheel=R;HasWheel=true;}
  if(Phase==EEchoPhase::QTE){Check(TEXT("wheel bones rotate during road travel"),WheelChanged);FVector Eye=DriveCar->GetActorTransform().InverseTransformPosition(DriveCamera->GetActorLocation());Check(TEXT("camera is inside pickup cabin"),FMath::Abs(Eye.Z-165)<1 && Eye.Y<0);Check(TEXT("driving view uses cockpit camera"),PC->GetViewTarget()==DriveCamera);ResolveQTE(-1);}
  if(Phase==EEchoPhase::Impact && !ImpactAt)ImpactAt=Now;
  if(Phase==EEchoPhase::Awakening){Check(TEXT("white impact holds at least three seconds"),ImpactAt>0 && Now-ImpactAt>=3);Check(TEXT("third person spirit uses original mesh"),P->Spirit && P->Spirit->GetSkeletalMeshAsset() && P->Spirit->GetSkeletalMeshAsset()->GetName().Contains(TEXT("Father")));Check(TEXT("small spirit and following camera restored"),P->Spirit->GetRelativeScale3D().X<.1 && P->Boom->TargetArmLength>=30 && PC->GetViewTarget()==P);Check(TEXT("spirit begins at cockpit position"),FVector::Dist(P->GetActorLocation(),PlayerImpact.TransformPosition(CockpitOffset)+FVector(0,0,10))<2);Stage=1;}
 }else if(Stage==1 && Phase==EEchoPhase::Awakening){
  if(LockStage==0 && PhaseTime>2.5f){Origin=P->GetActorLocation();Timer=TimeLeft;Key(EKeys::W,true);Key(EKeys::SpaceBar,true);Stamp=Now;LockStage=1;}
  else if(LockStage==1 && Now-Stamp>.8){Check(TEXT("intro blocks swimming after emergence"),FVector::Dist(P->GetActorLocation(),Origin)<.5f && P->SwimVelocity.IsNearlyZero());Check(TEXT("intro preserves the investigation clock"),TimeLeft==Timer);Check(TEXT("intro rejects collecting and rescuing"),!Interact(0,0));Key(EKeys::W,false);Key(EKeys::SpaceBar,false);LockStage=2;}
 }else if(Stage==1 && Phase==EEchoPhase::Investigation){Check(TEXT("movement unlocks only after all intro lines"),LockStage==2 && Notice.IsEmpty() && DialogueQueue.IsEmpty());Origin=Center+FVector(0,0,300);P->SetActorLocation(Origin);PC->SetControlRotation(FRotator::ZeroRotator);Key(EKeys::W,true);Stamp=Now;Stage=2;
 }else if(Stage==2 && Now-Stamp>.18){Check(TEXT("swimming accelerates gradually"),P->SwimVelocity.Size()>1 && P->SwimVelocity.Size()<70);Stage=3;
 }else if(Stage==3 && Now-Stamp>1.2){Check(TEXT("swim cruise is controlled"),P->SwimVelocity.Size()>75 && P->SwimVelocity.Size()<=96);Check(TEXT("swimming has an active motion loop"),ActionAudio.FindRef(TEXT("SpiritSwim")) && ActionAudio[TEXT("SpiritSwim")]->IsPlaying());Key(EKeys::W,false);Origin=P->GetActorLocation();Stamp=Now;Stage=4;
 }else if(Stage==4 && Now-Stamp>.15){Check(TEXT("release coasts rather than stops instantly"),P->SwimVelocity.Size()>5 && FVector::Dist(Origin,P->GetActorLocation())>2);Stage=5;
 }else if(Stage==5 && Now-Stamp>1.2){Check(TEXT("swimmer settles after release"),P->SwimVelocity.Size()<3);Origin=P->GetActorLocation();Key(EKeys::SpaceBar,true);Stamp=Now;Stage=6;
 }else if(Stage==6 && Now-Stamp>.75){Check(TEXT("space swims upward"),P->GetActorLocation().Z>Origin.Z+30);Key(EKeys::SpaceBar,false);Key(EKeys::LeftControl,true);Origin=P->GetActorLocation();Stamp=Now;Stage=7;
 }else if(Stage==7 && Now-Stamp>1.2){Check(TEXT("control swims downward"),P->GetActorLocation().Z<Origin.Z-35);Key(EKeys::LeftControl,false);P->SwimVelocity=FVector::ZeroVector;P->SetActorLocation(Clues[0].Position-(SpiritContactPoint()-P->GetActorLocation()));PC->SetControlRotation(FRotator(0,180,0));Stamp=Now;Stage=8;
 }else if(Stage==8 && Now-Stamp>.2){Check(TEXT("nearest evidence works without aiming"),Focus==0);Key(EKeys::E,true);Stamp=Now;Stage=9;
 }else if(Stage==9 && Now-Stamp>.1){Key(EKeys::E,false);Check(TEXT("E collects nearby evidence"),Clues[0].Complete);for(int I=0;I<Clues.Num();++I)if(Clues[I].Rescue){Rescue=I;break;}P->SetActorLocation(Clues[Rescue].Position-(SpiritContactPoint()-P->GetActorLocation()));Stamp=Now;Stage=10;
 }else if(Stage==10 && Now-Stamp>.2){
  Timer=TimeLeft;Check(TEXT("incomplete hold does not charge rescue cost"),!Interact(Rescue,2.9f) && TimeLeft==Timer);
  SetPaused(true);Check(TEXT("music pauses with the game"),MusicAudio && MusicAudio->GetPlayState()==EAudioComponentPlayState::Paused);Check(TEXT("paused rescue cannot consume time"),!Interact(Rescue,3) && TimeLeft==Timer);SetPaused(false);
  JournalOpen=true;Check(TEXT("journal blocks rescue"),!Interact(Rescue,3) && TimeLeft==Timer);JournalOpen=false;
  Key(EKeys::E,true);Stamp=Now;Stage=13;
 }else if(Stage==13 && Now-Stamp>.65){
  auto A=ActionAudio.FindRef(TEXT("RescueCharge"));Check(TEXT("holding rescue has a charge loop"),A && A->IsPlaying());SetPaused(true);Check(TEXT("rescue charge pauses"),A && A->GetPlayState()==EAudioComponentPlayState::Paused);SetPaused(false);Key(EKeys::E,false);Stamp=Now;Stage=14;
 }else if(Stage==14 && Now-Stamp>.15){
  auto A=ActionAudio.FindRef(TEXT("RescueCharge"));Check(TEXT("releasing rescue stops the charge loop"),A && !A->IsPlaying());Check(TEXT("cancelled rescue has release feedback"),AudioEvents.ContainsByPredicate([](const FString& E){return E.Contains(TEXT(",RescueRelease,"));}));Timer=TimeLeft;Key(EKeys::E,true);Stamp=Now;Stage=11;
 }else if(Stage==11 && Now-Stamp>3.4){Key(EKeys::E,false);Check(TEXT("holding E completes a rescue"),Clues[Rescue].Complete);
  Check(TEXT("completed rescue costs extra time in this difficulty"),Timer-TimeLeft>=Rules.RescueTimeCost+3 && Timer-TimeLeft<Rules.RescueTimeCost+5.5f);
  Timer=TimeLeft;Check(TEXT("repeat rescue cannot charge twice"),!Interact(Rescue,3) && TimeLeft==Timer);
  int Other=INDEX_NONE;for(int I=0;I<Clues.Num();++I)if(Clues[I].Rescue && !Clues[I].Complete){Other=I;break;}
  if(Other!=INDEX_NONE){P->SetActorLocation(Clues[Other].Position-(SpiritContactPoint()-P->GetActorLocation()));TimeLeft=0;Check(TEXT("expired clock rejects rescue"),!Interact(Other,3) && TimeLeft==0);TimeLeft=1;Check(TEXT("last-second rescue clamps clock at zero"),Interact(Other,3) && TimeLeft==0);TimeLeft=Timer;}
  SetPaused(true);for(const auto& Pair:EffectVoices)if(Pair.Key.IsValid() && Pair.Key->IsPlaying() && !Pair.Key->ComponentHasTag(TEXT("EchoUIAudio")))Check(TEXT("playing effect is paused"),Pair.Key->GetPlayState()==EAudioComponentPlayState::Paused);for(const auto& Pair:ActionAudio)if(IsValid(Pair.Value) && Pair.Value->IsPlaying())Check(TEXT("interaction loop is paused"),Pair.Value->GetPlayState()==EAudioComponentPlayState::Paused);Timer=TimeLeft;Stamp=Now;Stage=12;
 }else if(Stage==12 && Now-Stamp>.6){Check(TEXT("pause preserves timer"),TimeLeft==Timer);SetPaused(false);
  Phase=EEchoPhase::Awakening;PhaseTime=2.4f;Conversation({TEXT("awakening_hero"),TEXT("awakening_task"),TEXT("clock_begins")});
  Advance();Advance();Advance();TickPresentation(0);
  Check(TEXT("skipped intro unlocks without an eight-second wait"),Phase==EEchoPhase::Investigation && Notice.IsEmpty());Finish();}
#endif
}
