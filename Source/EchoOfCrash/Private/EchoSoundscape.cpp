#include "EchoCampaign.h"
#include "EchoPresentation.h"
#include "EchoGameInstance.h"
#include "Components/AudioComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Misc/Paths.h"
#include "Sound/SoundAttenuation.h"
#include "GameFramework/PlayerController.h"

static USoundBase* EchoSound(FName Name)
{
 FString N=Name.ToString();
 const TMap<FName,FString> Library={
  {TEXT("DoorOpen"),TEXT("SW_SUV_OpenDoor_001")},{TEXT("DoorClose"),TEXT("SW_SUV_Door_001")},
  {TEXT("EngineStart"),TEXT("SW_SUVStartEngine")},{TEXT("EngineStop"),TEXT("SW_SUV_StopEngine")},
  {TEXT("EngineIdle"),TEXT("SW_SUVIdlingLoop")},{TEXT("Horn"),TEXT("SW_SUV_HornShort")}};
 FString P=Library.Contains(Name)?TEXT("/Game/SUV03/Sound/")+Library[Name]:TEXT("/Game/EchoCampaign/Audio/")+N;
 return LoadObject<USoundBase>(nullptr,*(P+TEXT(".")+FPaths::GetBaseFilename(P)));
}
void AEchoCampaignDirector::SoundEvent(FName Name,float Gain,FVector Location,bool Spatial)
{
 if(ActorHasTag(TEXT("EchoIncidentTemplate")))return;
 auto& Sound=AudioCache.FindOrAdd(Name);if(!Sound)Sound=EchoSound(Name);
 if(!Sound){UE_LOG(LogTemp,Error,TEXT("ECHO_AUDIO_MISSING %s"),*Name.ToString());return;}
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());float V=Gain*(GI?GI->SoundVolume:1.f);
 UAudioComponent* A=Spatial?UGameplayStatics::SpawnSoundAtLocation(this,Sound,Location,FRotator::ZeroRotator,V):UGameplayStatics::SpawnSound2D(this,Sound,V);
 if(A){
  if(Spatial){FSoundAttenuationSettings S;S.bAttenuate=true;S.bSpatialize=true;S.AttenuationShapeExtents=FVector(500);S.FalloffDistance=2400;A->AdjustAttenuation(S);}
  const bool UI=Name==TEXT("UIClick") || Name==TEXT("UIBack") || Name==TEXT("UIHover");
  if(UI)A->ComponentTags.Add(TEXT("EchoUIAudio"));
  EffectVoices.Add(A,Gain);A->SetPaused(Paused && !UI);
 }
 AudioEvents.Add(FString::Printf(TEXT("%.3f,%d,%s,%.3f"),GetWorld()->GetTimeSeconds(),int(Phase),*Name.ToString(),Gain));
 if(AudioEvents.Num()>2048)AudioEvents.RemoveAt(0,256);
}
void AEchoCampaignDirector::TickSoundscape(float Dt)
{
 if(ActorHasTag(TEXT("EchoIncidentTemplate")))return;
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());const float V=GI?GI->SoundVolume:1.f;
 for(auto It=EffectVoices.CreateIterator();It;++It){if(!It.Key().IsValid())It.RemoveCurrent();else It.Key()->SetVolumeMultiplier(It.Value()*V);}
 if(Paused)return;
 const bool Speaking=IsValid(VoiceAudio) && VoiceAudio->IsPlaying();
 AudioDuck=FMath::FInterpTo(AudioDuck,Speaking?.28f:1.f,Dt,Speaking?9.f:1.5f);
 FName Desired=TEXT("MusicJourney");
 if(Phase==EEchoPhase::Awakening || Phase==EEchoPhase::Investigation || Phase==EEchoPhase::Deduction)Desired=TEXT("MusicEcho");
 if(Phase==EEchoPhase::Results || Phase==EEchoPhase::Outcome)Desired=ChapterStars>=4?TEXT("MusicHope"):TEXT("MusicHome");
 if(Phase==EEchoPhase::Arrival || Phase==EEchoPhase::Finale)Desired=GI && GI->Mood>=65?TEXT("MusicHope"):TEXT("MusicHome");
 if(Desired!=MusicName){
  if(IsValid(PreviousMusicAudio)){PreviousMusicAudio->Stop();PreviousMusicAudio->DestroyComponent();}
  PreviousMusicAudio=MusicAudio;MusicName=Desired;MusicCrossfade=0;
  MusicAudio=UGameplayStatics::SpawnSound2D(this,EchoSound(Desired),.001f,1,0,nullptr,false,false);
  AudioEvents.Add(FString::Printf(TEXT("%.3f,%d,%s,loop"),GetWorld()->GetTimeSeconds(),int(Phase),*Desired.ToString()));
 }
 MusicCrossfade=FMath::Min(1.f,MusicCrossfade+Dt/2.2f);
 const float Target=Phase==EEchoPhase::Impact?0.f:Phase==EEchoPhase::QTE?.055f:.21f;
 MusicGain=FMath::FInterpTo(MusicGain,Target,Dt,4.f);
 if(IsValid(MusicAudio))MusicAudio->SetVolumeMultiplier(MusicGain*MusicCrossfade*AudioDuck*V);
 if(IsValid(PreviousMusicAudio)){PreviousMusicAudio->SetVolumeMultiplier(MusicGain*(1-MusicCrossfade)*AudioDuck*V);if(MusicCrossfade>=1){PreviousMusicAudio->Stop();PreviousMusicAudio->DestroyComponent();PreviousMusicAudio=nullptr;}}
 if(EngineAudio)EngineAudio->SetVolumeMultiplier(.22f*V*(Speaking?.5f:1));
 if(OpponentAudio)OpponentAudio->SetVolumeMultiplier(.18f*V*(Speaking?.55f:1));
 if(Atmosphere)Atmosphere->SetVolumeMultiplier(.08f*V*AudioDuck);
 if(Cinema && Cinema->Siren)Cinema->Siren->SetVolumeMultiplier(.18f*V*(Speaking?.45f:1));
 if(AudioPhase!=int(Phase)){
  if(Phase==EEchoPhase::QTE){SoundEvent(TEXT("QTEWarning"),.32f);SoundEvent(TEXT("Horn"),.18f);}
  if(Phase==EEchoPhase::Awakening)SoundEvent(TEXT("SpiritRise"),.3f);
  if(Phase==EEchoPhase::Results)SoundEvent(ChapterStars>=4?TEXT("Success"):TEXT("Unresolved"),.28f);
  if(Phase==EEchoPhase::Deduction)SoundEvent(TimeLeft<=0?TEXT("TimeExpired"):TEXT("HintChime"),.25f);
  AudioPhase=int(Phase);
 }
 TickInteractionAudio(Dt);
 if(Phase==EEchoPhase::Investigation && TimeLeft<=10 && TimeLeft>0){int Second=FMath::CeilToInt(TimeLeft);if(Second!=LastTimerSecond){LastTimerSecond=Second;SoundEvent(TEXT("TimerPulse"),.16f);}}
}
void AEchoCampaignDirector::SetSoundLoop(FName Name,float Gain,float Pitch)
{
 auto& A=ActionAudio.FindOrAdd(Name);
 if(Gain<=.0001f){if(IsValid(A) && A->IsPlaying()){A->Stop();AudioEvents.Add(FString::Printf(TEXT("%.3f,%d,%s,loop_stop"),GetWorld()->GetTimeSeconds(),int(Phase),*Name.ToString()));}return;}
 if(!IsValid(A)){
  auto& Sound=AudioCache.FindOrAdd(Name);if(!Sound)Sound=EchoSound(Name);
  if(!Sound){UE_LOG(LogTemp,Error,TEXT("ECHO_AUDIO_MISSING %s"),*Name.ToString());return;}
  A=UGameplayStatics::CreateSound2D(this,Sound,.001f,Pitch,0,nullptr,false,false);
 }
 if(!A)return;
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());A->SetVolumeMultiplier(Gain*(GI?GI->SoundVolume:1));A->SetPitchMultiplier(Pitch);
 if(!A->IsPlaying()){A->Play();AudioEvents.Add(FString::Printf(TEXT("%.3f,%d,%s,loop_start"),GetWorld()->GetTimeSeconds(),int(Phase),*Name.ToString()));}
}
void AEchoCampaignDirector::TickInteractionAudio(float Dt)
{
 const bool Modal=JournalOpen || InspectIndex!=INDEX_NONE;
 const bool Searching=Phase==EEchoPhase::Investigation && !Modal && TimeLeft>0;
 const bool Spirit=(Searching || Phase==EEchoPhase::Awakening) && !Modal;
 auto PC=GetWorld()->GetFirstPlayerController();auto G=PC?Cast<AEchoGhostPawn>(PC->GetPawn()):nullptr;
 const float Swim=Searching && G?FMath::Clamp(G->SwimVelocity.Size()/165.f,0.f,1.f):0;
 const float Duck=.45f+.55f*AudioDuck;
 SetSoundLoop(TEXT("SpiritHover"),Spirit?.12f*Duck*(Phase==EEchoPhase::Awakening?FMath::SmoothStep(.2f,2.2f,PhaseTime):1.f):0);
 SetSoundLoop(TEXT("SpiritSwim"),Swim>.02f?Swim*.24f*Duck:0,.82f+Swim*.38f);
 SetSoundLoop(TEXT("QTETension"),Phase==EEchoPhase::QTE?.18f:0,.85f+FMath::Clamp(PhaseTime/3.5f,0.f,1.f)*.35f);
 const int Target=Searching && Clues.IsValidIndex(Focus) && Clues[Focus].Rescue && !Clues[Focus].Complete && CanReachClue(Focus) && RescueProgress>0?Focus:INDEX_NONE;
 if(Target!=AudioRescueTarget){
  if(AudioRescueTarget!=INDEX_NONE && Searching && Clues.IsValidIndex(AudioRescueTarget) && !Clues[AudioRescueTarget].Complete)SoundEvent(TEXT("RescueRelease"),.2f);
  if(Target!=INDEX_NONE)SoundEvent(TEXT("RescueStart"),.24f);
  AudioRescueTarget=Target;
 }
 float Progress=FMath::Clamp(RescueProgress/3.f,0.f,1.f);
 SetSoundLoop(TEXT("RescueCharge"),Target!=INDEX_NONE?(.18f+.22f*Progress)*Duck:0,.82f+.48f*Progress);
 ReachSoundCooldown=FMath::Max(0.f,ReachSoundCooldown-Dt);
 int Reach=Searching && CanReachClue(Focus)?Focus:INDEX_NONE;
 if(Reach!=INDEX_NONE && Reach!=AudioReachTarget && ReachSoundCooldown<=0){SoundEvent(TEXT("ReachReady"),.17f);ReachSoundCooldown=1.2f;}
 AudioReachTarget=Reach;
}
void AEchoCampaignDirector::EndPlay(const EEndPlayReason::Type Reason)
{
 StopDialogue();
 for(auto A:{MusicAudio.Get(),PreviousMusicAudio.Get(),EngineAudio.Get(),OpponentAudio.Get(),Atmosphere.Get()})if(IsValid(A)){A->Stop();A->DestroyComponent();}
 for(auto& Pair:EffectVoices)if(Pair.Key.IsValid())Pair.Key->Stop();EffectVoices.Empty();
 for(auto& Pair:ActionAudio)if(IsValid(Pair.Value)){Pair.Value->Stop();Pair.Value->DestroyComponent();}ActionAudio.Empty();
 Super::EndPlay(Reason);
}
void AEchoStoryScene::DoorAudio(FName ID,float Open,FVector Position)
{
 if(SceneAudioPreview || !Director)return;
 float& Old=DoorAudioState.FindOrAdd(ID);
 if(Old<=.025f && Open>.025f)Director->SoundEvent(ID==TEXT("Building")?TEXT("BuildingOpen"):TEXT("DoorOpen"),.55f,Position,true);
 if(Old>.025f && Open<=.025f)Director->SoundEvent(ID==TEXT("Building")?TEXT("BuildingClose"):TEXT("DoorClose"),.65f,Position,true);
 Old=Open;
}
void AEchoStoryScene::TickSceneAudio(float Dt)
{
 if(SceneAudioPreview || !IsValid(Director) || Director->Paused)return;
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());float V=GI?GI->SoundVolume:1;
 for(auto M:People){
  if(!IsValid(M) || M->GetOwner()->IsHidden())continue;
  const FVector P=M->GetOwner()->GetActorLocation();auto Old=FootAudioPosition.Find(M);
  float Distance=Old?FVector::Dist2D(*Old,P):0;FootAudioPosition.Add(M,P);
  int Gesture=PreviousGestures.FindRef(M);
  if((Gesture==1 || Gesture==12) && Distance>0 && Distance<45){float& Accum=FootAudioDistance.FindOrAdd(M);Accum+=Distance;if(Accum>=52){Accum=FMath::Fmod(Accum,52.f);Director->SoundEvent(FName(*FString::Printf(TEXT("Footstep%d"),(FootAudioIndex++)%5)),.24f,P,true);}}
  else FootAudioDistance.FindOrAdd(M)=0;
 }
 if(CuffedCount>AudioCuffedCount){Director->SoundEvent(TEXT("CuffClick"),.5f,People.IsValidIndex(CuffedCount)?People[CuffedCount]->GetOwner()->GetActorLocation():Origin,true);AudioCuffedCount=CuffedCount;}
 const bool Motor=Opening?Clock>=18.5f:Arrival?Clock<.6f:RealityRevealed;
 if(Motor && !SceneEngineStarted){SceneEngineStarted=true;if(Opening)Director->SoundEvent(TEXT("EngineStart"),.3f,OpeningCar->GetActorLocation(),true);auto Car=Opening || Arrival?OpeningCar:PoliceCar;if(IsValid(Car)){SceneEngine=UGameplayStatics::SpawnSoundAttached(EchoSound(TEXT("EngineIdle")),Car->GetRootComponent(),NAME_None,FVector::ZeroVector,EAttachLocation::KeepRelativeOffset,true,.12f,1,0,nullptr,nullptr,false);if(SceneEngine){FSoundAttenuationSettings A;A.AttenuationShapeExtents=FVector(450);A.FalloffDistance=2000;SceneEngine->AdjustAttenuation(A);}}}
 if(SceneEngine){
  float Moving=!Opening && !Arrival && AftermathBeat==TEXT("Departing")?FMath::Clamp((AftermathTime-35)*.08f,0.f,1.f):0;
  SceneEngine->SetPitchMultiplier(.8f+Moving*.65f);SceneEngine->SetVolumeMultiplier(.12f*V*(Director->Notice.IsEmpty()?1.f:.4f));
  if(!Opening && Arrival && Clock>=.6f && !SceneEngineStopped){SceneEngineStopped=true;Director->SoundEvent(TEXT("EngineStop"),.3f);SceneEngine->FadeOut(.4f,0);}
 }
}
