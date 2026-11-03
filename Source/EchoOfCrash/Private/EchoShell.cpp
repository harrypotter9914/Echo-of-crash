#include "EchoCampaign.h"
#include "EchoGameInstance.h"
#include "EchoPresentation.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "EchoMouseLook.h"

void AEchoCampaignHUD::Centered(const FString& T,float X,float Y,float S,FLinearColor C)
{
 PaintText(T,X,Y,S,C,true,true);
}
void AEchoCampaignHUD::Frame(float X,float Y,float W,float H,FLinearColor Fill,FLinearColor Edge,float R)
{
 R=FMath::Min(R,FMath::Min(W,H)/2);
 DrawRect(Fill,X,Y+R,W,H-2*R);
 for(int I=0;I<12;++I){float A=R*I/12,B=R*(I+1)/12,Inset=R-FMath::Sqrt(FMath::Max(0.f,R*R-FMath::Square(R-(A+B)/2)));DrawRect(Fill,X+Inset,Y+A,W-2*Inset,B-A);DrawRect(Fill,X+Inset,Y+H-B,W-2*Inset,B-A);}
 DrawLine(X+R,Y,X+W-R,Y,Edge,1);DrawLine(X+R,Y+H,X+W-R,Y+H,Edge,1);DrawLine(X,Y+R,X,Y+H-R,Edge,1);DrawLine(X+W,Y+R,X+W,Y+H-R,Edge,1);
 for(int Corner=0;Corner<4;++Corner){float CX=Corner==0||Corner==3?X+R:X+W-R,CY=Corner<2?Y+R:Y+H-R,Start=PI+Corner*PI/2;for(int I=0;I<8;++I){float A=Start+I*PI/16,B=A+PI/16;DrawLine(CX+R*FMath::Cos(A),CY+R*FMath::Sin(A),CX+R*FMath::Cos(B),CY+R*FMath::Sin(B),Edge,1);}}
}
void AEchoCampaignHUD::Button(FName ID,const FString& T,float X,float Y,float W,float H,bool Primary,bool Enabled)
{
 float MX=-1,MY=-1;GetOwningPlayerController()->GetMousePosition(MX,MY);bool Hover=Enabled && MX>=X && MX<=X+W && MY>=Y && MY<=Y+H;
 if(Hover && AudioHoverButton!=ID){AudioHoverButton=ID;double Now=GetWorld()->GetRealTimeSeconds();if(Now-LastHoverSound>.09){for(TActorIterator<AEchoCampaignDirector> It(GetWorld());It;++It)if(!It->ActorHasTag(TEXT("EchoIncidentTemplate"))){It->SoundEvent(TEXT("UIHover"),.18f);break;}LastHoverSound=Now;}}
 else if(!Hover && AudioHoverButton==ID)AudioHoverButton=NAME_None;
 float S=FMath::Min(Canvas->SizeX/1500.f,Canvas->SizeY/900.f);
 FLinearColor Fill=Primary?FLinearColor(.95,.76,.40,Hover?1:.97):FLinearColor(.085,.12,.095,Hover?.97:.87);
 FLinearColor Text=Primary?FLinearColor(.025,.055,.037):FLinearColor(.98,.96,.88);
 if(!Enabled){Fill.A=.35;Text.A=.65;}
 Frame(X+2*S,Y+4*S,W,H,FLinearColor(0,0,0,.32),FLinearColor::Transparent,5*S);
 Frame(X,Y,W,H,Fill,FLinearColor(.90,.75,.44,Hover?.95:.60),5*S);
 DrawLine(X+12*S,Y+5*S,X+W-12*S,Y+5*S,FLinearColor(.98,.88,.65,.27),S);
 DrawLine(X+12*S,Y+H-5*S,X+W-12*S,Y+H-5*S,FLinearColor(.85,.74,.49,.25),S);
 for(int I=0;I<3;++I){float DY=(H/2)+(I-1)*6*S;DrawLine(X+7*S,Y+DY-2*S,X+12*S,Y+DY+2*S,FLinearColor(.87,.73,.44,.5),S);DrawLine(X+W-12*S,Y+DY-2*S,X+W-7*S,Y+DY+2*S,FLinearColor(.87,.73,.44,.5),S);}
 if(Hover){float U=FMath::Fmod(GetWorld()->GetRealTimeSeconds()*.35f,1.f);DrawLine(X+W*U,Y+H,X+FMath::Min(W,W*U+28*S),Y+H,FLinearColor(1,.88,.56),2*S);}
 float Font=FitText(T,1.02f*S,W-44*S,H-18*S,true);float TextHeight=MeasureText(T,Font,true).Y;Centered(T,X+W/2,Y+(H-TextHeight)/2,Font,Text);
 if(Hover)DrawLine(X+20*S,Y+H-7*S,X+W-20*S,Y+H-7*S,FLinearColor(.99,.77,.4),2*S);
 if(Enabled)AddHitBox(FVector2D(X,Y),FVector2D(W,H),ID,true,100);
}
bool AEchoCampaignHUD::DrawShell(AEchoCampaignDirector* D,float W,float H,float S)
{
 bool Title=D->Phase==EEchoPhase::Title,Tutorial=D->Phase==EEchoPhase::Tutorial,Intro=D->Phase==EEchoPhase::Introduction,Final=D->Phase==EEchoPhase::Finale;
 if(!Title && !Tutorial && !Intro && !Final && !D->Paused)return false;
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());FLinearColor Cream(.97,.94,.84),Mint(.68,.87,.75),Gold(.99,.77,.43),Muted(.69,.76,.7);
 if(Title || Tutorial){
  if(auto Art=LoadObject<UTexture2D>(nullptr,TEXT("/Game/EchoCampaign/Art/T_JourneyMenuV6.T_JourneyMenuV6"))){float AR=float(Art->GetSizeX())/Art->GetSizeY(),Screen=W/H;float U=Screen<AR?Screen/AR:1,V=Screen>AR?AR/Screen:1;DrawTexture(Art,0,0,W,H,0,(1-V)/2,U,V,FLinearColor::White,BLEND_Opaque);}
  else DrawRect(FLinearColor(.04,.12,.105),0,0,W,H);
 }else DrawRect(FLinearColor(.014,.027,.024,.82),0,0,W,H);
 float PX=(W-1000*S)/2,PY=(H-720*S)/2;
 auto Heading=[&](const FString& A,const FString& B){
  const float Font=FitText(A,1.75*S,860*S,50*S,true),Top=PY+32*S;
  Centered(A,W/2,Top,Font,Cream);
  Centered(B,W/2,Top+MeasureText(A,Font,true).Y+8*S,FitText(B,.9*S,880*S,24*S,true),Muted);
 };
 auto Row=[&](FName ID,const FString& Text,int N,bool Primary=false){Button(ID,Text,PX+210*S,PY+(170+N*68)*S,580*S,53*S,Primary);};
 if(D->MenuPage || D->Paused || Tutorial || Intro || Final){
  Frame(PX+7*S,PY+10*S,1000*S,720*S,FLinearColor(0,0,0,.45),FLinearColor::Transparent,8*S);
  Frame(PX,PY,1000*S,720*S,FLinearColor(.87,.82,.68,.985),FLinearColor(.34,.29,.18,.95),8*S);
  Frame(PX+10*S,PY+10*S,980*S,700*S,FLinearColor(0,0,0,0),FLinearColor(.47,.42,.28,.45),5*S);
  // An itinerary card: stitched edges, road dashes and stamped corner marks.
  for(int I=0;I<34;++I){DrawLine(PX+(24+I*28)*S,PY+23*S,PX+(35+I*28)*S,PY+23*S,FLinearColor(.38,.34,.22,.32),S);DrawLine(PX+(24+I*28)*S,PY+697*S,PX+(35+I*28)*S,PY+697*S,FLinearColor(.38,.34,.22,.32),S);}
  for(int I=0;I<4;++I){float X=PX+(I%2?960:40)*S,Y=PY+(I<2?42:678)*S;DrawLine(X-5*S,Y,X+5*S,Y,FLinearColor(.47,.35,.19),S);DrawLine(X,Y-5*S,X,Y+5*S,FLinearColor(.47,.35,.19),S);}
  DrawLine(PX+70*S,PY+119*S,PX+930*S,PY+119*S,FLinearColor(.39,.34,.23,.38),S);
  Cream=FLinearColor(.035,.055,.04);Mint=FLinearColor(.045,.12,.075);Gold=FLinearColor(.22,.105,.025);Muted=FLinearColor(.10,.13,.09);
 }
 if(D->MenuPage==1){
  Heading(TEXT("YOUR FIELD GUIDE"),TEXT("A small guide to the moments that matter"));
  const TCHAR* Keys[]={TEXT("SPACE"),TEXT("W A S D / MOUSE"),TEXT("E / HOLD E"),TEXT("Q"),TEXT("TAB / 1 - 3"),TEXT("F"),TEXT("ESC")};
  const TCHAR* Help[]={TEXT("Avoid a collision when the marker reaches the mint arc."),TEXT("Float and look around. SHIFT moves faster; SPACE / CTRL changes height."),TEXT("Examine nearby evidence. Hold 3 seconds. A completed rescue also consumes time."),TEXT("Reveal one direction. Each new target costs 50 points."),TEXT("Open your collection. Inspect, drag to rotate, scroll to zoom."),TEXT("Reconstruct the cause after collecting all clues."),TEXT("Pause. Your investigation clock stops while paused or inspecting.")};
  for(int I=0;I<7;++I){Label(Keys[I],PX+45*S,PY+(155+I*59)*S,.82*S,Gold);TextBlock(Help[I],PX+245*S,PY+(155+I*59)*S,695*S,.81*S,Cream,false);}
  Centered(TEXT("Evidence 400 + lives 500 + supported deduction 100 = 1000 points"),W/2,PY+592*S,.88*S,Mint);Button(TEXT("MenuBack"),TEXT("BACK"),W/2-180*S,PY+635*S,360*S,48*S);return true;
 }
 if(D->MenuPage==2){
  Heading(TEXT("MAKE YOURSELF COMFORTABLE"),TEXT("Settings are saved automatically"));
  const FString Names[]={TEXT("Dialogue volume"),TEXT("Music and effects"),TEXT("Mouse sensitivity")};
  for(int I=0;I<3;++I){float Y=PY+(170+I*86)*S;float Value=GI?(I==0?GI->VoiceVolume:I==1?GI->SoundVolume:GI->LookSensitivity):1;Label(Names[I],PX+80*S,Y+14*S,1.05*S,Cream);Centered(FString::Printf(TEXT("%d%%"),FMath::RoundToInt(Value*100)),PX+690*S,Y+15*S,S,Gold);Button(FName(*FString::Printf(TEXT("SettingMinus%d"),I)),TEXT("-"),PX+575*S,Y,65*S,52*S);Button(FName(*FString::Printf(TEXT("SettingPlus%d"),I)),TEXT("+"),PX+775*S,Y,65*S,52*S);}
  Button(TEXT("Subtitles"),GI && GI->Subtitles?TEXT("SUBTITLES: ON"):TEXT("SUBTITLES: OFF"),PX+210*S,PY+444*S,580*S,53*S);
  Centered(TEXT("20% - 400%  /  Applies to driving and spirit look"),W/2,PY+407*S,.82*S,Muted);
  Button(TEXT("ResetMouse"),TEXT("RESET MOUSE TO 100%"),PX+210*S,PY+513*S,580*S,48*S);
  Centered(TEXT("Keyboard + mouse  /  English voices and text"),W/2,PY+583*S,.86*S,Muted);Button(TEXT("MenuBack"),TEXT("BACK"),W/2-180*S,PY+625*S,360*S,48*S);return true;
 }
 if(D->MenuPage==3){
  Heading(TEXT("THE PEOPLE BEHIND THE JOURNEY"),TEXT("ECHO OF CRASH"));
  const TCHAR* Credits[]={TEXT("Game by  /  Yibo Wang"),TEXT("World and characters  /  Synty Polygon Town & Polygon City"),TEXT("Animation performances  /  Adobe Mixamo & Epic Games"),TEXT("Engine  /  Unreal Engine 5.8"),TEXT("Additional illustration  /  AI-generated artwork, directed for this game"),TEXT("English voices  /  Local Qwen3-TTS synthetic performances"),TEXT("Adaptation and implementation  /  Yibo Wang")};
  for(int I=0;I<7;++I)Centered(Credits[I],W/2,PY+(160+I*51)*S,.94*S,I==0?Gold:Cream);
  Centered(TEXT("Thank you for taking the road with us."),W/2,PY+555*S,1.05*S,Mint);Button(TEXT("MenuBack"),TEXT("BACK"),W/2-180*S,PY+630*S,360*S,48*S);return true;
 }
 if(D->MenuPage==4){Heading(TEXT("A FRESH START"),TEXT("Starting a new journey will replace your chapter checkpoint."));Row(TEXT("ConfirmNew"),TEXT("START A NEW JOURNEY"),2,true);Row(TEXT("MenuBack"),TEXT("KEEP MY JOURNEY"),3);return true;}
 if(D->Paused){Heading(TEXT("TAKE A BREATH"),TEXT("Your moment is safe. The clock is paused."));Row(TEXT("Resume"),TEXT("BACK TO THE ROAD"),0,true);Row(TEXT("Help"),TEXT("FIELD GUIDE"),1);Row(TEXT("Settings"),TEXT("SETTINGS"),2);Row(TEXT("Restart"),TEXT("RESTART THIS CHAPTER"),3);Row(TEXT("Home"),TEXT("TITLE SCREEN"),4);Centered(TEXT("Progress resumes from the beginning of your current chapter."),W/2,PY+593*S,.81*S,Muted);return true;}
 if(Title){
  float X=W*.065f;Label(TEXT("A WEEKEND. THREE MOMENTS. A WAY BACK."),X,H*.12,.75*S,Gold);
  const float TitleX=X-10*S,TitleY=H*.17f,TitleW=560*S,TitleH=280*S;
  // All three words share one authored glyph treatment; the breaks are in the
  // transparent artwork itself, rather than a line drawn over ordinary text.
  if(auto Art=LoadObject<UTexture2D>(nullptr,TEXT("/Game/EchoCampaign/Art/T_TitleWordmarkV18.T_TitleWordmarkV18"))){
   DrawTexture(Art,TitleX,TitleY,TitleW,TitleH,0,0,1,1,FLinearColor::White,BLEND_Translucent);
  }else Label(TEXT("ECHO OF CRASH"),X,TitleY,FitText(TEXT("ECHO OF CRASH"),2.8*S,TitleW,TitleH),Cream);
  Label(TEXT("Keep hold of what matters."),X,TitleY+TitleH+16*S,1.08*S,Mint);
  float Y=H*.55f;Button(TEXT("New"),TEXT("NEW JOURNEY"),X,Y,405*S,56*S,true);Button(TEXT("ContinueSave"),TEXT("CONTINUE JOURNEY"),X,Y+68*S,405*S,52*S,false,D->HasCheckpoint);Button(TEXT("Help"),TEXT("FIELD GUIDE"),X,Y+132*S,194*S,48*S);Button(TEXT("Settings"),TEXT("SETTINGS"),X+211*S,Y+132*S,194*S,48*S);Button(TEXT("Credits"),TEXT("CREDITS"),X,Y+192*S,194*S,48*S);Button(TEXT("Quit"),TEXT("QUIT"),X+211*S,Y+192*S,194*S,48*S);
  Label(TEXT("An original journey by Yibo Wang"),X,H-47*S,.72*S,Muted);return true;
 }
 if(Tutorial){
  Heading(D->TutorialPage==0?TEXT("BEFORE WE LEAVE"):D->TutorialPage==1?TEXT("ON THE ROAD"):TEXT("IF THE WORLD STANDS STILL"),D->TutorialPage==0?TEXT("01 / Choose the pace of your journey"):D->TutorialPage==1?TEXT("02 / A quick decision can keep you safe"):TEXT("03 / You can still make a difference"));
  if(D->TutorialPage==0){
   Centered(TEXT("You and your wife are heading out for a quiet weekend together."),W/2,PY+145*S,1.0*S,Cream);
   Centered(TEXT("Watch the traffic. Press SPACE inside the mint arc to steer or brake."),W/2,PY+189*S,.96*S,Cream);
   const TCHAR* Names[]={TEXT("STORY / EASY"),TEXT("JOURNEY / NORMAL"),TEXT("TENSION / HARD")};const TCHAR* Desc[]={TEXT("120 seconds / rescue costs 5 seconds"),TEXT("100 seconds / rescue costs 8 seconds"),TEXT("80 seconds / rescue costs 12 seconds")};
   for(int I=0;I<3;++I){float Y=PY+(268+I*86)*S;Button(FName(*FString::Printf(TEXT("Balance%d"),I)),FString(Names[I])+(GI && int(GI->Balance)==I?TEXT("  *"):TEXT("")),PX+65*S,Y,280*S,58*S,GI && int(GI->Balance)==I);Label(Desc[I],PX+380*S,Y+17*S,.98*S,Cream);}
   Centered(TEXT("Every mode includes ALL evidence. Time, rescue cost and guidance differ."),W/2,PY+559*S,.87*S,Gold);
   Button(TEXT("TutorialNext"),TEXT("NEXT  /  ENTER"),W/2-230*S,PY+626*S,460*S,53*S,true);
  }else if(D->TutorialPage==1){
   Centered(TEXT("Driving is automatic. You are looking out from the driver's seat."),W/2,PY+155*S,S,Cream);
   Centered(TEXT("When time slows, press SPACE once inside the highlighted arc."),W/2,PY+195*S,S,Cream);
   SteeringWheel(W/2,PY+355*S,88*S,D->QTEValue,.4f,.7f);
   Centered(D->TutorialPracticeHit?TEXT("NICE TIMING / You would avoid the collision."):TEXT("TRY IT HERE / SPACE when the gold marker meets the mint arc"),W/2,PY+487*S,.95*S,D->TutorialPracticeHit?Mint:Gold);
   Centered(TEXT("Miss the window and the impact opens a frozen moment to investigate."),W/2,PY+542*S,.88*S,Muted);
   Button(TEXT("TutorialBack"),TEXT("BACK"),PX+80*S,PY+626*S,230*S,53*S);Button(TEXT("TutorialNext"),TEXT("NEXT / ENTER"),PX+345*S,PY+626*S,575*S,53*S,true);
  }else{
   const TCHAR* Steps[]={TEXT("01   SWIM AND LOOK  /  WASD + mouse / SPACE up / CTRL down"),TEXT("02   FIND THE TRACES  /  E near an object"),TEXT("03   BRING THEM BACK  /  hold E for three seconds"),TEXT("04   KEEP YOUR EVIDENCE  /  TAB or number keys"),TEXT("05   RECONSTRUCT THE CAUSE  /  F when you are ready")};
   for(int I=0;I<5;++I){Frame(PX+80*S,PY+(153+I*68)*S,840*S,53*S,FLinearColor(.77,.75,.61,.60),FLinearColor(.40,.43,.28,.25),8*S);Label(Steps[I],PX+105*S,PY+(168+I*68)*S,.98*S,Cream);}
   Centered(TEXT("Stay within the mist. Q reveals one direction: -50 points per new target."),W/2,PY+521*S,.87*S,Mint);
   Centered(FString::Printf(TEXT("Each completed rescue costs %.0f extra seconds. Listen to the Echo before swimming."),D->Rules.RescueTimeCost),W/2,PY+560*S,.87*S,Gold);
   Button(TEXT("TutorialBack"),TEXT("BACK"),PX+80*S,PY+626*S,230*S,53*S);Button(TEXT("BeginOpening"),TEXT("LET'S GO  /  ENTER"),PX+345*S,PY+626*S,575*S,53*S,true);
  }return true;
 }
 if(Intro){Heading(D->ChapterTitle,D->Story);Centered(TEXT("Watch the road. A quick decision can keep everyone safe."),W/2,PY+237*S,1.04*S,Cream);Centered(TEXT("SPACE  /  steer or brake when the marker enters the mint arc"),W/2,PY+300*S,S,Gold);Row(TEXT("Continue"),TEXT("CONTINUE THE JOURNEY  /  ENTER"),4,true);return true;}
 if(Final){
  int Mood=GI?GI->Mood:0,Ending=FEchoJourneyRules::Ending(Mood);
  const TCHAR* Titles[]={TEXT("HOME, FOR NOW"),TEXT("A QUIET HAND TO HOLD"),TEXT("AN EVENING TOGETHER"),TEXT("THE WEEKEND WE PROMISED")};
  const TCHAR* Lines[]={TEXT("The outing can wait. You do not have to face a hard day alone."),TEXT("Some days are heavy. You still choose to walk in together."),TEXT("The road was not easy. There is room for a peaceful evening."),TEXT("You kept hold of what matters. A brighter evening is waiting.")};
  Heading(Titles[Ending],Lines[Ending]);
  Frame(PX+120*S,PY+156*S,760*S,240*S,FLinearColor(.78,.76,.62),FLinearColor(.40,.44,.28,.4),15*S);
  const FString Points=FString::Printf(TEXT("%d"),GI?GI->TotalScore:0);
  const float ScoreFont=2.8*S,ScoreTop=PY+166*S;
  Centered(Points,W/2,ScoreTop,ScoreFont,Gold);Centered(TEXT("JOURNEY POINTS"),W/2,ScoreTop+MeasureText(Points,ScoreFont,true).Y+10*S,.76*S,Mint);
  Centered(FString::Printf(TEXT("SPIRIT  %d / 100     SAFE ESCAPES  %d / 3"),Mood,GI?GI->Avoided:0),W/2,PY+300*S,S,Cream);
  Centered(FString::Printf(TEXT("%d lives reached    /    %d clues found    /    %d causes understood"),GI?GI->TotalSaved:0,GI?GI->TotalEvidence:0,GI?GI->TotalDeductions:0),W/2,PY+348*S,.87*S,Muted);
  if(D->DialogueID==TEXT("ending_guide") && GI && GI->Subtitles){Centered(TEXT("THE ECHO"),W/2,PY+418*S,.64*S,Gold);Centered(TEXT("A road is made of moments. You cannot keep them all."),W/2,PY+444*S,.9*S,Mint);Centered(TEXT("You can choose how you meet the next one."),W/2,PY+471*S,.9*S,Mint);}
  else Centered(TEXT("Every moment you chose to care made a difference."),W/2,PY+444*S,1.04*S,Mint);
  Button(TEXT("New"),TEXT("A NEW JOURNEY"),PX+100*S,PY+530*S,390*S,57*S,true);Button(TEXT("Credits"),TEXT("CREDITS"),PX+510*S,PY+530*S,390*S,57*S);Button(TEXT("Home"),TEXT("TITLE SCREEN"),PX+100*S,PY+603*S,390*S,50*S);Button(TEXT("Quit"),TEXT("QUIT"),PX+510*S,PY+603*S,390*S,50*S);return true;
 }return true;
}
void AEchoCampaignDirector::MenuAction(FName Action)
{
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());if(!GI)return;
 SoundEvent(TEXT("UIClick"),.28f);FString A=Action.ToString();
 if(Action==TEXT("MenuBack")){MenuPage=0;return;}
 if(Action==TEXT("Help")){MenuPage=1;return;}if(Action==TEXT("Settings")){MenuPage=2;return;}if(Action==TEXT("Credits")){MenuPage=3;return;}
 if(Action==TEXT("Resume")){SetPaused(false);return;}
 if(Action==TEXT("Restart")){SetPaused(false);StopDialogue();Retry();return;}
 if(Action==TEXT("ContinueSave")){ContinueSavedJourney();return;}
 if(Action==TEXT("Home")){StopDialogue();GI->FrontEndEntered=false;GI->StartAtTutorial=false;GI->ContinueDriving=false;UGameplayStatics::OpenLevel(this,TEXT("/Game/EchoCampaign/Maps/Episode01"));return;}
 if(Action==TEXT("Quit")){UKismetSystemLibrary::QuitGame(this,GetWorld()->GetFirstPlayerController(),EQuitPreference::Quit,false);return;}
 if(Action==TEXT("New") && HasCheckpoint){MenuPage=4;return;}
 if(Action==TEXT("New") || Action==TEXT("ConfirmNew")){
  GI->TotalSaved=GI->TotalEvidence=GI->TotalDeductions=GI->TotalScore=GI->Avoided=0;GI->JourneyStarted=false;GI->FrontEndEntered=true;GI->Mood=FEchoJourneyRules::Preset(GI->Balance).StartingMood;
  if(Phase==EEchoPhase::Title){Phase=EEchoPhase::Tutorial;TutorialPage=0;MenuPage=0;SetCursor(true);}else{GI->StartAtTutorial=true;StopDialogue();UGameplayStatics::OpenLevel(this,TEXT("/Game/EchoCampaign/Maps/Episode01"));}return;
 }
 if(A.StartsWith(TEXT("Balance"))){auto Prior=Phase;Phase=EEchoPhase::Introduction;SetBalance(EEchoBalance(FCString::Atoi(*A.Right(1))));Phase=Prior;Notice.Empty();return;}
 if(Action==TEXT("TutorialNext")){TutorialPage=FMath::Min(2,TutorialPage+1);return;}if(Action==TEXT("TutorialBack")){TutorialPage=FMath::Max(0,TutorialPage-1);return;}
 if(Action==TEXT("BeginOpening")){BeginOpening();return;}
 if(Action==TEXT("SkipOpening")){EndOpening();return;}
 if(Action==TEXT("Continue")){Advance();return;}
 if(Action==TEXT("Subtitles")){GI->Subtitles=!GI->Subtitles;SavePreferences();return;}
 if(A==TEXT("ResetMouse")){GI->LookSensitivity=1.f;SavePreferences();return;}
 if(A.StartsWith(TEXT("Setting"))){int I=FCString::Atoi(*A.Right(1));float Delta=A.Contains(TEXT("Plus"))?.1f:-.1f;float& V=I==0?GI->VoiceVolume:I==1?GI->SoundVolume:GI->LookSensitivity;V=FMath::Clamp(FMath::RoundToFloat((V+Delta)*10)/10,I==2?EchoMouseLook::MinSensitivity:0.f,I==2?EchoMouseLook::MaxSensitivity:1.f);SavePreferences();return;}
}
void AEchoCampaignDirector::BeginOpening()
{
 if(auto GI=Cast<UEchoGameInstance>(GetGameInstance())){GI->JourneyStarted=true;GI->FrontEndEntered=true;}
 StopDialogue();Notice.Empty();Phase=EEchoPhase::Opening;PhaseTime=0;MenuPage=0;SaveCheckpoint(0);
 for(auto A:Tableau)if(A)A->SetActorHiddenInGame(true);
 Cinema=GetWorld()->SpawnActor<AEchoStoryScene>();Cinema->StartOpening(this);SetCursor(true);
}
void AEchoCampaignDirector::EndOpening()
{
 StopDialogue();Notice.Empty();if(Cinema){Cinema->Destroy();Cinema=nullptr;}BeginDrive();SetCursor(false);
 GetWorld()->GetFirstPlayerController()->PlayerCameraManager->StartCameraFade(1,0,.8f,FLinearColor::Black,false,false);
 Say(TEXT("road_one"));
}


