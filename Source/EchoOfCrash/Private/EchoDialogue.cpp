#include "EchoCampaign.h"
#include "EchoGameInstance.h"
#include "EchoPresentation.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/PlayerController.h"
#include "EchoMouseLook.h"

struct FEchoLine {const TCHAR* ID;const TCHAR* Speaker;const TCHAR* Text;};
static const FEchoLine EchoLines[]={
#include "EchoDialogue.inl"
};
void AEchoCampaignDirector::StopDialogue(){if(VoiceAudio){VoiceAudio->Stop();VoiceAudio->DestroyComponent();VoiceAudio=nullptr;}DialogueQueue.Empty();DialogueID=NAME_None;Speaker.Empty();}
void AEchoCampaignDirector::Say(FName ID)
{
 if(VoiceAudio){VoiceAudio->Stop();VoiceAudio->DestroyComponent();VoiceAudio=nullptr;}
 NoticePages.Empty();DialogueQueue.Empty();DialogueID=ID;
 for(const auto& Line:EchoLines)if(ID==FName(Line.ID)){
  Speaker=Line.Speaker;Notice=Line.Text;NoticeTime=FMath::Max(4.f,Notice.Len()*.064f+1);
  FString P=FString::Printf(TEXT("/Game/EchoCampaign/Audio/Voice/VO_%s.VO_%s"),Line.ID,Line.ID);
  if(auto Sound=LoadObject<USoundBase>(nullptr,*P)){
   auto GI=Cast<UEchoGameInstance>(GetGameInstance());
   VoiceAudio=UGameplayStatics::SpawnSound2D(this,Sound,GI?GI->VoiceVolume:1,1,0,nullptr,false,false);
   NoticeTime=FMath::Max(NoticeTime,Sound->GetDuration()+.45f);
  }
  return;
 }
 Speaker.Empty();Notice.Empty();NoticeTime=0;
}
void AEchoCampaignDirector::Conversation(const TArray<FName>& IDs){StopDialogue();DialogueQueue=IDs;NextDialogue();}
void AEchoCampaignDirector::NextDialogue()
{
 if(DialogueQueue.Num()){FName Next=DialogueQueue[0];DialogueQueue.RemoveAt(0);TArray<FName> Remaining=MoveTemp(DialogueQueue);Say(Next);DialogueQueue=MoveTemp(Remaining);}
 else if(NoticePages.Num()){Notice=NoticePages[0];NoticePages.RemoveAt(0);NoticeTime=FMath::Max(5.f,Notice.Len()*.065f);}
 else{StopDialogue();Notice.Empty();NoticeTime=0;}
}
void AEchoCampaignDirector::LoadPreferences()
{
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());if(!GI || GI->PreferencesLoaded)return;GI->PreferencesLoaded=true;
 if(auto P=Cast<UEchoPreferences>(UGameplayStatics::LoadGameFromSlot(TEXT("EchoPreferences"),0))){GI->VoiceVolume=FMath::Clamp(P->VoiceVolume,0.f,1.f);GI->SoundVolume=FMath::Clamp(P->SoundVolume,0.f,1.f);GI->LookSensitivity=FMath::Clamp(P->LookSensitivity,EchoMouseLook::MinSensitivity,EchoMouseLook::MaxSensitivity);GI->Subtitles=P->Subtitles;}
}
void AEchoCampaignDirector::SavePreferences()
{
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());if(!GI)return;
 auto P=Cast<UEchoPreferences>(UGameplayStatics::CreateSaveGameObject(UEchoPreferences::StaticClass()));
 P->VoiceVolume=GI->VoiceVolume;P->SoundVolume=GI->SoundVolume;P->LookSensitivity=GI->LookSensitivity;P->Subtitles=GI->Subtitles;
 UGameplayStatics::SaveGameToSlot(P,TEXT("EchoPreferences"),0);
 if(VoiceAudio)VoiceAudio->SetVolumeMultiplier(GI->VoiceVolume);
 if(EngineAudio)EngineAudio->SetVolumeMultiplier(.22f*GI->SoundVolume);
 if(OpponentAudio)OpponentAudio->SetVolumeMultiplier(.18f*GI->SoundVolume);
 if(Atmosphere)Atmosphere->SetVolumeMultiplier(.2f*GI->SoundVolume);
 if(Cinema && Cinema->Siren)Cinema->Siren->SetVolumeMultiplier(.15f*GI->SoundVolume);
}
void AEchoCampaignDirector::SetCursor(bool Visible)
{
 auto PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
 PC->bShowMouseCursor=Visible;PC->bEnableClickEvents=Visible;
 if(Visible)PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
 else PC->SetInputMode(FInputModeGameOnly().SetConsumeCaptureMouseDown(false));
}
void AEchoCampaignDirector::SetPaused(bool Value)
{
 SoundEvent(Value?TEXT("UIBack"):TEXT("UIClick"),.2f);Paused=Value;MenuPage=0;
 if(IsValid(MusicAudio))MusicAudio->SetPaused(Value);if(IsValid(PreviousMusicAudio))PreviousMusicAudio->SetPaused(Value);
 for(auto& Pair:EffectVoices)if(Pair.Key.IsValid() && !Pair.Key->ComponentHasTag(TEXT("EchoUIAudio")))Pair.Key->SetPaused(Value);
 for(auto& Pair:ActionAudio)if(IsValid(Pair.Value))Pair.Value->SetPaused(Value);
 if(Cinema && Cinema->SceneEngine)Cinema->SceneEngine->SetPaused(Value);
 if(VoiceAudio)VoiceAudio->SetPaused(Value);
 if(Cinema && Cinema->Siren)Cinema->Siren->SetPaused(Value);
 if(EngineAudio)EngineAudio->SetPaused(Value);if(OpponentAudio)OpponentAudio->SetPaused(Value);if(Atmosphere)Atmosphere->SetPaused(Value);
 SetCursor(Value || JournalOpen || InspectIndex!=INDEX_NONE || Phase==EEchoPhase::Title || Phase==EEchoPhase::Tutorial || Phase==EEchoPhase::Introduction || Phase==EEchoPhase::Results || Phase==EEchoPhase::Deduction || Phase==EEchoPhase::Finale);
}
