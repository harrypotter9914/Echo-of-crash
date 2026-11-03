#include "EchoCampaign.h"
#include "EchoGameInstance.h"
#include "EchoPresentation.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Engine/GameViewportClient.h"

// Opt-in development build harness. Normal player sessions never enter this path.
// Runs the cooked maps and real state transitions, including cinematic durations.
void AEchoCampaignDirector::TickVerification()
{
#if !UE_BUILD_SHIPPING
 static FString Mode;static bool Initialized=false,Done=false;static double Started=0,LastPickup=0,Finished=0;static int LastPhase=-1,LastChapter=-1;static FString Events;
 if(!Initialized){Initialized=true;FParse::Value(FCommandLine::Get(),TEXT("EchoVerify="),Mode);Started=FPlatformTime::Seconds();}
 if(Mode!=TEXT("perfect") && Mode!=TEXT("avoid") && Mode!=TEXT("empty"))return;
 double Now=FPlatformTime::Seconds();
 if(Done){if(Now-Finished>2)FPlatformMisc::RequestExit(false);return;}
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());if(!GI)return;
 FString Dir=FPaths::ProjectSavedDir()/TEXT("Verification");IFileManager::Get().MakeDirectory(*Dir,true);
 if(LastPhase!=int(Phase) || LastChapter!=Chapter){
  LastPhase=int(Phase);LastChapter=Chapter;
  if(!Events.IsEmpty())Events+=TEXT(",\n");
  Events+=FString::Printf(TEXT("{\"chapter\":%d,\"phase\":%d,\"mood\":%d,\"score\":%d,\"elapsed\":%.2f}"),Chapter,int(Phase),GI->Mood,GI->TotalScore,Now-Started);
  FFileHelper::SaveStringToFile(TEXT("[")+Events+TEXT("]"),*(Dir/(Mode+TEXT("-progress.json"))));
 }
 if(Phase==EEchoPhase::Title)MenuAction(TEXT("ConfirmNew"));
 else if(Phase==EEchoPhase::Tutorial)Advance();
 else if(Phase==EEchoPhase::QTE){const float Hit[]={.55,.3,.75};ResolveQTE(Mode==TEXT("avoid")?Hit[Chapter]:-1);}
 else if(Phase==EEchoPhase::Investigation){
  if(Mode==TEXT("empty"))TimeLeft=0;
  else if(Now-LastPickup>1.1){
   int Next=INDEX_NONE;for(int I=0;I<Clues.Num();++I)if(!Clues[I].Complete){Next=I;break;}
   if(Next==INDEX_NONE && Notice.IsEmpty()){Phase=EEchoPhase::Deduction;SetCursor(true);}
   else if(Next!=INDEX_NONE){if(auto P=GetWorld()->GetFirstPlayerController()->GetPawn()){P->SetActorLocation(Clues[Next].Position-(SpiritContactPoint()-P->GetActorLocation()));Interact(Next,3);}LastPickup=Now;}
  }
 }
 else if(Phase==EEchoPhase::Deduction){if(EvidenceCount()!=EvidenceTotal())ReturnWithoutDeduction();else SubmitDeduction(CorrectAnswer);}
 else if(Phase==EEchoPhase::Results && PhaseTime>10 && Notice.IsEmpty())Advance();
 // V15 includes visible officer exit, individual cuffing and passenger boarding.
 bool Timeout=Now-Started>660;
 if(Phase==EEchoPhase::Finale || Timeout){
  int ExpectedScore=Mode==TEXT("empty")?0:3000,ExpectedMood=Mode==TEXT("empty")?0:Mode==TEXT("avoid")?84:100;
  bool Passed=!Timeout && GI->TotalScore==ExpectedScore && GI->Mood==ExpectedMood && Chapter==(Mode==TEXT("empty")?0:2) && GI->Avoided==(Mode==TEXT("avoid")?3:0);
  if(Mode==TEXT("perfect"))Passed=Passed && GI->TotalSaved==10 && GI->TotalEvidence==7 && GI->TotalDeductions==3;
  FString Report=FString::Printf(TEXT("{\"mode\":\"%s\",\"passed\":%s,\"timeout\":%s,\"score\":%d,\"mood\":%d,\"rescues\":%d,\"evidence\":%d,\"deductions\":%d,\"avoided\":%d,\"events\":[%s]}"),*Mode,Passed?TEXT("true"):TEXT("false"),Timeout?TEXT("true"):TEXT("false"),GI->TotalScore,GI->Mood,GI->TotalSaved,GI->TotalEvidence,GI->TotalDeductions,GI->Avoided,*Events);
  FFileHelper::SaveStringToFile(Report,*(Dir/(Mode+TEXT(".json"))));
  FScreenshotRequest::RequestScreenshot(Dir/(Mode+TEXT(".png")),false,false);
  UE_LOG(LogTemp,Display,TEXT("ECHO_PACKAGED_VERIFICATION %s %s"),*Mode,Passed?TEXT("PASS"):TEXT("FAIL"));Done=true;Finished=Now;
 }
#endif
}
