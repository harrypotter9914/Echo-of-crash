#include "EchoCampaign.h"
#include "EchoPresentation.h"
#include "EchoGameInstance.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "CanvasItem.h"
#include "Engine/Font.h"
#include "Styling/CoreStyle.h"
#include "Materials/MaterialInstanceDynamic.h"

void AEchoCampaignHUD::Label(const FString& T,float X,float Y,float S,FLinearColor C){PaintText(T,X,Y,S,C);}

void AEchoCampaignHUD::Panel(float X,float Y,float W,float H,float A){DrawRect(FLinearColor(.025f,.029f,.027f,A*.64f),X,Y,W,H);}
void AEchoCampaignHUD::Thumbnail(UTexture* T,float X,float Y,float W,float H){if(!T)return;if(auto RT=Cast<UTextureRenderTarget2D>(T)){for(TActorIterator<AEchoPreviewRig> I(GetWorld());I;++I)if(I->Texture==RT && I->DisplayMaterial){DrawMaterialSimple(I->DisplayMaterial,X,Y,W,H);return;}}DrawTexture(T,X,Y,W,H,0,0,1,1,FLinearColor::White,BLEND_Translucent);}
void AEchoCampaignHUD::NotifyHitBoxClick(FName Box)
{
 Super::NotifyHitBoxClick(Box);AEchoCampaignDirector* D=nullptr;for(TActorIterator<AEchoCampaignDirector> I(GetWorld());I;++I){if(I->ActorHasTag(TEXT("EchoIncidentTemplate")))continue;D=*I;break;}if(!D)return;
 if(D->Paused || D->Phase==EEchoPhase::Title || D->Phase==EEchoPhase::Tutorial || D->Phase==EEchoPhase::Finale || D->Phase==EEchoPhase::Introduction){D->MenuAction(Box);return;}
 if(Box==TEXT("SkipOpening")){D->MenuAction(Box);return;}
 if(Box==TEXT("ReturnUnconfirmed")){D->ReturnWithoutDeduction();return;}
 D->SoundEvent(TEXT("UIClick"),.28f);
 if(Box.ToString().StartsWith(TEXT("Answer"))){D->SubmitDeduction(FCString::Atoi(*Box.ToString().Right(1)));return;}
 FString S=Box.ToString();if(S.StartsWith(TEXT("Clue_")))D->Inspect(FCString::Atoi(*S.Mid(5)));
 if(Box==TEXT("AskEcho")){D->RequestHint();return;}
 if(Box==TEXT("Close"))D->Inspect(INDEX_NONE);if(Box==TEXT("Continue"))D->Advance();
 if(Box==TEXT("DialogueNext"))D->Advance();
}
void AEchoCampaignHUD::DrawHUD()
{
 Super::DrawHUD();TextBounds.Reset();ContinueOrigin=ContinueSize=FVector2D::ZeroVector;if(!Canvas)return;
 AEchoCampaignDirector* D=nullptr;for(TActorIterator<AEchoCampaignDirector> I(GetWorld());I;++I){if(I->ActorHasTag(TEXT("EchoIncidentTemplate")))continue;D=*I;break;}if(!D)return;
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());float W=Canvas->SizeX,H=Canvas->SizeY,S=FMath::Min(W/1500.f,H/900.f);
 if(DrawShell(D,W,H,S))return;
 FLinearColor Mint(.40,.91,.80),Cream(.96,.94,.82),Gold(1,.76,.39),Muted(.57,.70,.72),Coral(.98,.47,.38);
 auto Center=[&](const FString& T,float X,float Y,float Size,FLinearColor C){PaintText(T,X,Y,Size,C,true);};
 auto Arc=[&](float CX,float CY,float R,float Start,float End,FLinearColor Color,float Thickness){for(int I=0;I<64;++I){float A=FMath::Lerp(Start,End,I/64.f),B=FMath::Lerp(Start,End,(I+1)/64.f);DrawLine(CX+R*FMath::Cos(A),CY+R*FMath::Sin(A),CX+R*FMath::Cos(B),CY+R*FMath::Sin(B),Color,Thickness);}};
 auto Fade=[&](float Top,float Height,float Opacity,bool Inverse=false){for(int I=0;I<160;++I){float T=(I+.5f)/160.f;if(Inverse)T=1-T;DrawRect(FLinearColor(.012,.014,.014,Opacity*T*T),0,Top+Height*I/160.f,W,Height/160.f);}};
 auto Bar=[&](float X,float Y,float Width,float Value,FLinearColor Color){DrawRect(FLinearColor(.12,.22,.25),X,Y,Width,7*S);DrawRect(Color,X,Y,Width*FMath::Clamp(Value,0.f,1.f),7*S);};
 auto Card=[&](int Index,float X,float Y,float CW,float CH,bool Reveal,bool Hit){
  auto& C=D->Clues[Index];float MX=-1,MY=-1;GetOwningPlayerController()->GetMousePosition(MX,MY);bool Hover=Hit && C.Complete && MX>=X && MX<X+CW && MY>=Y && MY<Y+CH;float Lift=Hover?5*S:0;Y-=Lift;bool Paper=D->Phase==EEchoPhase::Results;Frame(X,Y,CW,CH,Paper?FLinearColor(.85,.84,.73,.72):FLinearColor(.025,.07,.06,Hover?.75:.50),Paper?FLinearColor(.27,.36,.28,.5):FLinearColor(.72,.84,.72,C.Complete?.60:.22),9*S);
  if(C.Complete && Reveal && C.Preview){float Area=CH-(CW>110*S?42*S:0);float Size=FMath::Min(CW-12*S,Area-8*S);Thumbnail(C.Preview->Texture,X+(CW-Size)/2,Y+(Area-Size)/2,Size,Size);}
  else{Centered(C.Rescue?TEXT("+"):TEXT("?"),X+CW/2,Y+CH*.27f,1.6*S,Muted);}
  FString Caption=Reveal && C.Complete?C.Title:C.Rescue?TEXT("Unreached"):TEXT("Not found");if(CW>110*S){float Font=FitText(Caption,.78f*S,CW-20*S,27*S);Center(Caption,X+CW/2,Y+CH-34*S,Font,C.Complete?Cream:Muted);}
  if(Hit && C.Complete)AddHitBox(FVector2D(X,Y),FVector2D(CW,CH),FName(*FString::Printf(TEXT("Clue_%d"),Index)),true);
 };
 bool Investigation=D->Phase==EEchoPhase::Investigation || D->Phase==EEchoPhase::Awakening;
 bool Cinema=D->Phase==EEchoPhase::Outcome || D->Phase==EEchoPhase::Arrival || D->Phase==EEchoPhase::Opening;
 if(!Cinema && D->Phase!=EEchoPhase::Impact && D->Phase!=EEchoPhase::Results && !D->JournalOpen && D->InspectIndex==INDEX_NONE){Fade(0,290*S,.56,true);
  Label(TEXT("ECHO / THE ROAD HOME"),30*S,25*S,.88*S,Cream);
  Label(FString::Printf(TEXT("%02d  /  SPIRIT  %d"),D->Chapter+1,GI?GI->Mood:60),30*S,54*S,.78*S,Gold);
  for(int I=0;I<20;++I)DrawLine((30+I*10)*S,94*S,(33+I*10)*S,84*S,I<(GI?GI->Mood:60)/5.f?Gold:FLinearColor(.5,.5,.5,.25),3*S);
 }
 if(Investigation && !D->JournalOpen && D->InspectIndex==INDEX_NONE){
  float CX=W/2,CY=93*S,Radius=65*S;Arc(CX,CY,Radius,PI,2*PI,FLinearColor(.1,.12,.11,.7),7*S);float Fraction=FMath::Clamp(D->TimeLeft/(D->Rules.InvestigationSeconds+(D->Chapter==2?15:0)),0.f,1.f);Arc(CX,CY,Radius,PI,PI+PI*Fraction,D->TimeLeft<30?Coral:Mint,4*S);
  for(int I=0;I<=12;++I){float A=PI+I*PI/12;DrawLine(CX+FMath::Cos(A)*(Radius+6*S),CY+FMath::Sin(A)*(Radius+6*S),CX+FMath::Cos(A)*(Radius+12*S),CY+FMath::Sin(A)*(Radius+12*S),Cream,S);}
  int Seconds=FMath::Max(0,FMath::CeilToInt(D->TimeLeft));Center(FString::Printf(TEXT("%02d:%02d"),Seconds/60,Seconds%60),CX,59*S,1.5*S,Cream);Center(D->Phase==EEchoPhase::Awakening?TEXT("TIME STANDS STILL"):TEXT("TIME TO RETURN"),CX,99*S,.65*S,Mint);
  if(D->RescueCostFeedback>0)Center(FString::Printf(TEXT("RESCUED  /  -%.0fs"),D->Rules.RescueTimeCost),CX,126*S,.80*S,FLinearColor(1,.70,.43,FMath::Min(1.f,D->RescueCostFeedback)));
  int R=0,E=0;for(int I=0;I<D->Clues.Num();++I){auto& C=D->Clues[I];if(C.Rescue){Card(I,26*S+R*88*S,112*S,80*S,105*S,true,false);++R;}else{Card(I,W-(118+(D->EvidenceTotal()-1-E)*114)*S,30*S,102*S,112*S,true,true);Center(FString::FromInt(E+1),W-(67+(D->EvidenceTotal()-1-E)*114)*S,150*S,.7*S,Gold);++E;}}
  Label(FString::Printf(TEXT("BROUGHT BACK  %d / %d"),D->RescueCount(),D->RescueTotal()),28*S,230*S,.70*S,Gold);
  Label(TEXT("TAB / YOUR EVIDENCE"),W-235*S,180*S,.7*S,Mint);
  if(D->Phase==EEchoPhase::Investigation && !D->JournalOpen && D->InspectIndex==INDEX_NONE){

   // Exact labels are local interactions only; a paid hint marks just one direction.
   if(D->ScanTime>0 && D->Clues.IsValidIndex(D->HintTarget) && !D->Clues[D->HintTarget].Complete && GetOwningPawn()){
    const auto& Clue=D->Clues[D->HintTarget];FVector2D P;bool Onscreen=GetOwningPlayerController()->ProjectWorldLocationToScreen(Clue.Position,P);
    if(!Onscreen){FVector Delta=Clue.Position-GetOwningPawn()->GetActorLocation();float Side=FVector::DotProduct(Delta,FRotationMatrix(GetOwningPlayerController()->GetControlRotation()).GetUnitAxis(EAxis::Y));P=FVector2D(Side<0?180*S:W-180*S,H*.46f);}
    P.X=FMath::Clamp(P.X,180*S,W-180*S);P.Y=FMath::Clamp(P.Y,260*S,H-280*S);float Pulse=(12+3*FMath::Sin(GetWorld()->GetTimeSeconds()*4))*S;
    DrawLine(P.X-Pulse,P.Y,P.X,P.Y-Pulse,Gold,2*S);DrawLine(P.X,P.Y-Pulse,P.X+Pulse,P.Y,Gold,2*S);DrawLine(P.X+Pulse,P.Y,P.X,P.Y+Pulse,Gold,2*S);DrawLine(P.X,P.Y+Pulse,P.X-Pulse,P.Y,Gold,2*S);
    Center(TEXT("FOLLOW THE ECHO"),P.X,P.Y+21*S,.65*S,Gold);
   }
   Button(TEXT("AskEcho"),TEXT("Q / ASK THE ECHO"),W-263*S,218*S,238*S,40*S);
   Center(FString::Printf(TEXT("New direction: -50 pts  |  Used: -%d"),D->HintPenalty),W-144*S,264*S,.60*S,Gold);

  }
 }
 if(D->Phase==EEchoPhase::Driving || D->Phase==EEchoPhase::QTE || D->Phase==EEchoPhase::Avoidance || D->Phase==EEchoPhase::Departure){
  Fade(H-260*S,260*S,.52);
  if(D->Phase==EEchoPhase::QTE){float Lo[]={.4,.2,.7},Hi[]={.7,.4,.8};float CX=W/2,CY=136*S;SteeringWheel(CX,CY,72*S,D->QTEValue,Lo[D->Chapter],Hi[D->Chapter]);Frame(CX-182*S,224*S,364*S,62*S,FLinearColor(.018,.036,.030,.88),FLinearColor(.70,.80,.65,.48),8*S);Center(D->Chapter==2?TEXT("BRAKE / SPACE"):TEXT("STEER / SPACE"),CX,231*S,1.05*S,Cream);Center(TEXT("Press once inside the mint arc"),CX,258*S,.79*S,Mint);}
  else if(D->Notice.IsEmpty()){Center(D->Phase==EEchoPhase::Avoidance?TEXT("A CLOSE CALL"):D->Phase==EEchoPhase::Departure?TEXT("THE JOURNEY CONTINUES"):TEXT("STAY WITH THE ROAD"),W/2,H-105*S,1.2*S,Mint);Center(D->Phase==EEchoPhase::Avoidance?TEXT("Breathe. Your family is safe."):TEXT("SPACE in the mint arc / steer or brake. Mouse / look around."),W/2,H-66*S,.9*S,Cream);}
 }
 if(D->Phase==EEchoPhase::Introduction || D->Phase==EEchoPhase::Deduction || D->Phase==EEchoPhase::Finale){
  Frame(W*.10,H*.23,W*.8,H*.62,FLinearColor(.032,.067,.057,.95),FLinearColor(.70,.80,.65,.4),18*S);float X=W*.14,Y=H*.28;
  if(D->Phase==EEchoPhase::Introduction){Label(D->ChapterTitle,X,Y,1.6*S,Mint);Label(D->Story,X,Y+57*S,S,Cream);Label(TEXT("SPACE in the mint zone / avoid the accident. A collision opens a frozen moment."),X,Y+110*S,.95*S,Cream);Label(TEXT("Search for evidence, rescue the people, then piece together what happened."),X,Y+147*S,.95*S,Cream);
   Label(TEXT("BALANCE  1 / Story     2 / Journey     3 / Tension"),X,Y+205*S,S,Gold);Label(FString::Printf(TEXT("Selected: %s / %.0f seconds / initial mood %d"),GI && GI->Balance==EEchoBalance::Story?TEXT("Story"):GI && GI->Balance==EEchoBalance::Tension?TEXT("Tension"):TEXT("Journey"),D->Rules.InvestigationSeconds,D->Rules.StartingMood),X,Y+245*S,.9*S,Muted);Label(TEXT("ENTER / Begin       C / Resume saved journey"),X,Y+309*S,1.15*S,Mint);}
  if(D->Phase==EEchoPhase::Deduction){
   const bool Ready=D->EvidenceCount()==D->EvidenceTotal();
   Label(D->TimeLeft<=0?TEXT("TIME HAS RUN OUT"):TEXT("RECONSTRUCT THE MOMENT"),X,Y,1.45*S,Mint);
   float BodyY=Y+60*S;
   BodyY+=TextBlock(FString::Printf(TEXT("%d / %d clues recovered"),D->EvidenceCount(),D->EvidenceTotal()),W/2,BodyY,W*.68f,S,Cream)+20*S;
   if(Ready){
    BodyY+=TextBlock(TEXT("Choose the cause supported by the evidence you recovered."),W/2,BodyY,W*.68f,.9*S,Cream)+22*S;
    for(int I=0;I<D->Answers.Num();++I)Button(FName(*FString::Printf(TEXT("Answer%d"),I)),FString::Printf(TEXT("%d / %s"),I+1,*D->Answers[I]),X,BodyY+I*67*S,W*.72,55*S);
   }else{
    BodyY+=TextBlock(TEXT("The moment is fading. Without every clue, the cause cannot be confirmed. Your recovered evidence and rescued lives still count."),W/2,BodyY,W*.64f,1.0*S,Cream)+35*S;
    Button(TEXT("ReturnUnconfirmed"),TEXT("ENTER / Return to reality"),X+W*.06f,BodyY,W*.60f,60*S,true);
   }
  }
  if(D->Phase==EEchoPhase::Finale){int Mood=GI?GI->Mood:0,End=FEchoJourneyRules::Ending(Mood);Label(End==0?TEXT("HOME, FOR NOW"):End==1?TEXT("A QUIET HAND TO HOLD"):End==2?TEXT("AN EVENING TOGETHER"):TEXT("THE WEEKEND WE PROMISED"),X,Y,1.55*S,Mint);Label(End==0?TEXT("The outing can wait. Today, you choose to go home together."):End==1?TEXT("The day weighs on you. You still choose to walk in together."):End==2?TEXT("Not every moment was easy. There is room for a peaceful evening."):TEXT("The road is behind you. Hand in hand, you begin a brighter evening."),X,Y+72*S,.95*S,Cream);
   if(GI){Label(FString::Printf(TEXT("Mood %d / 100    Safe escapes %d / 3    Journey score %d"),Mood,GI->Avoided,GI->TotalScore),X,Y+140*S,1.05*S,Gold);Label(FString::Printf(TEXT("In the accidents you faced: %d people returned / %d clues / %d causes resolved"),GI->TotalSaved,GI->TotalEvidence,GI->TotalDeductions),X,Y+189*S,.95*S,Cream);}Label(TEXT("ENTER / A new journey"),X,Y+292*S,1.15*S,Mint);}
 }
 if(D->Phase==EEchoPhase::Results){
  float BW=FMath::Min(W*.9f,1200*S),BH=800*S,X=(W-BW)/2,Y=(H-BH)/2;Panel(0,0,W,H,.95);
  if(auto T=LoadObject<UTexture2D>(nullptr,TEXT("/Game/EchoCampaign/Art/T_JourneyBoardV3.T_JourneyBoardV3")))Thumbnail(T,X,Y,BW,BH);
  Cream=FLinearColor(.02,.035,.025);Mint=FLinearColor(.04,.105,.065);Gold=FLinearColor(.24,.11,.028);Muted=FLinearColor(.10,.125,.085);
  Center(TEXT("WHAT YOU BROUGHT BACK"),W/2,Y+39*S,1.5*S,Cream);
  Center(TEXT("The clues. The lives. The chance to continue."),W/2,Y+92*S,.82*S,Mint);
  float Reveal=FMath::Clamp((D->PhaseTime-D->Clues.Num()*.48f)/1.f,0.f,1.f);
  Center(FString::Printf(TEXT("%04d"),FMath::RoundToInt(D->ChapterScore*Reveal)),W/2,Y+126*S,2.4*S,Cream);
  Center(TEXT("JOURNEY POINTS / 1000"),W/2,Y+204*S,.67*S,Mint);
  for(int K=0;K<5;++K){float CX=W/2+(K-2)*37*S,CY=Y+247*S;FLinearColor C=(K<D->ChapterStars && Reveal>float(K)/5)?Gold:FLinearColor(.55,.55,.5);for(int J=0;J<10;++J){float A=-PI/2+J*PI/5,B=-PI/2+(J+1)*PI/5;DrawLine(CX+FMath::Cos(A)*(J%2?6:13)*S,CY+FMath::Sin(A)*(J%2?6:13)*S,CX+FMath::Cos(B)*((J+1)%2?6:13)*S,CY+FMath::Sin(B)*((J+1)%2?6:13)*S,C,1.8*S);}}
  Frame(W/2-485*S,Y+270*S,970*S,479*S,FLinearColor(.93,.90,.80,.96),FLinearColor(.46,.42,.28,.16),12*S);
  int Order=0;for(int Row=0;Row<2;++Row){int Col=0,CompleteSoFar=0,Total=Row==0?D->EvidenceTotal():D->RescueTotal();float RowY=Y+(276+Row*191)*S;
   Center(Row==0?TEXT("EVIDENCE RECOVERED / 400"):TEXT("PEOPLE BROUGHT BACK / 500"),W/2,RowY,.73*S,Mint);
   for(int I=0;I<D->Clues.Num();++I){auto& C=D->Clues[I];if(C.Rescue!=(Row==1))continue;float Age=D->PhaseTime-Order*.48f,Bounce=Age>=0?1+FMath::Exp(-Age*8)*FMath::Sin(Age*22)*.10:0;
    if(Age>=0){float CW=170*S*Bounce,CH=116*S*Bounce,CX=W/2+(Col-(Total-1)*.5f)*192*S-85*S,CY=RowY+32*S;Card(I,CX-(CW-170*S)/2,CY-(CH-116*S)/2,CW,CH,true,false);float Weight=Row==0?400.f:500.f;int Points=C.Complete?FMath::RoundToInt(Weight*(CompleteSoFar+1)/Total)-FMath::RoundToInt(Weight*CompleteSoFar/Total):0;if(C.Complete)++CompleteSoFar;Center(FString::Printf(TEXT("+%d"),Points),CX+85*S,CY+121*S,.68*S,Gold);}++Col;++Order;
   }
  }
  Center(D->CorrectDeduction?TEXT("CAUSE CONFIRMED  +100"):TEXT("CAUSE UNCONFIRMED  +0"),W/2,Y+661*S,.72*S,Mint);
  Center(FString::Printf(TEXT("GUIDANCE  -%d   |   BEFORE GUIDANCE  %d"),D->HintPenalty,D->RawChapterScore),W/2,Y+719*S,.68*S,Mint);
  if(Reveal>0)Center(FString::Printf(TEXT("SPIRIT  %d   %s%d   >   %d"),D->MoodBefore,D->MoodDelta>=0?TEXT("+"):TEXT(""),D->MoodDelta,GI?GI->Mood:0),W/2,Y+691*S,.80*S,Gold);
  if(!D->Notice.IsEmpty() && (!GI || GI->Subtitles)){float FS=FitText(D->Notice,.78f*S,W-50*S,28*S);DrawRect(FLinearColor(.025,.045,.035,.90),0,H-35*S,W,35*S);Center(D->Notice,W/2,H-29*S,FS,FLinearColor(.97,.94,.84));}
  Frame(W/2-340*S,Y+756*S,680*S,35*S,FLinearColor(.94,.91,.80,.98),FLinearColor(.38,.34,.23,.30),5*S);
  Center(TEXT("ENTER / Continue     R / Try again"),W/2,Y+762*S,.78*S,Cream);AddHitBox(FVector2D(W/2-340*S,Y+756*S),FVector2D(680*S,35*S),TEXT("Continue"),true);
 }
 if(D->JournalOpen && D->InspectIndex==INDEX_NONE){DrawRect(FLinearColor(.015,.019,.018,.82),0,0,W,H);Center(TEXT("COLLECTED MOMENTS"),W/2,H*.20,1.6*S,Mint);Center(TEXT("Choose a memory. Take all the time you need."),W/2,H*.27,.90*S,Cream);int Col=0;for(int I=0;I<D->Clues.Num();++I)if(!D->Clues[I].Rescue){float CX=W/2+(Col-(D->EvidenceTotal()-1)*.5f)*270*S;Card(I,CX-115*S,H*.35,230*S,245*S,true,true);Center(FString::Printf(TEXT("%d / EXAMINE"),Col+1),CX,H*.35+265*S,.72*S,Gold);++Col;}Center(TEXT("CLICK or 1-3 / Examine     TAB / Return"),W/2,H*.80,.88*S,Cream);}
 if(D->InspectIndex!=INDEX_NONE && D->Clues.IsValidIndex(D->InspectIndex)){auto& C=D->Clues[D->InspectIndex];DrawRect(FLinearColor(.015,.019,.018,.86),0,0,W,H);float Size=FMath::Min(W*.50,H*.65);if(C.Preview)Thumbnail(C.Preview->Texture,(W-Size)/2,H*.14,Size,Size);Center(C.Title,W/2,H*.08,1.6*S,Mint);TextBlock(C.Description,W/2,H*.79,W*.80f,.88*S,Cream);Center(TEXT("LEFT / RIGHT DRAG / Rotate     WHEEL / Zoom     ESC / Close"),W/2,H*.90,.82*S,Gold);Label(TEXT("CLOSE [X]"),W*.81,H*.12,.9*S,Cream);AddHitBox(FVector2D(W*.79,H*.1),FVector2D(W*.12,45*S),TEXT("Close"),true);}
 DrawConversationAndInteraction(D,W,H,S);

 if(D->Phase==EEchoPhase::Opening){Label(TEXT("A WEEKEND TOGETHER"),40*S,40*S,1.2*S,Cream);Button(TEXT("SkipOpening"),TEXT("SKIP / SPACE"),W-235*S,35*S,200*S,43*S);}
 if(Cinema && D->Phase!=EEchoPhase::Opening){Label(D->Phase==EEchoPhase::Arrival?TEXT("AFTER THE ROAD"):TEXT("AFTER THE COLLISION"),40*S,40*S,1.2*S,Cream);}
 if(D->Paused){Panel(0,0,W,H,.92);Label(TEXT("MOMENT SUSPENDED"),W*.32,H*.4,1.8*S,Mint);Label(TEXT("ESC / Resume     R / Restart this chapter"),W*.33,H*.52,1.1*S,Cream);}
}


void AEchoCampaignHUD::SteeringWheel(float X,float Y,float R,float Marker,float Low,float High)
{
 float S=R/72.f;FLinearColor Leather(.055,.067,.052,.98),Metal(.74,.73,.63),Gold(.98,.77,.40),Mint(.38,.95,.72);
 auto Ring=[&](float Rad,float A,float B,FLinearColor Color,float Thick){for(int I=0;I<80;++I){float U=FMath::Lerp(A,B,I/80.f),V=FMath::Lerp(A,B,(I+1)/80.f);DrawLine(X+Rad*FMath::Cos(U),Y+Rad*FMath::Sin(U),X+Rad*FMath::Cos(V),Y+Rad*FMath::Sin(V),Color,Thick);}};
 Ring(R+3*S,0,2*PI,FLinearColor(0,0,0,.6),24*S);Ring(R,0,2*PI,Metal,20*S);Ring(R,0,2*PI,Leather,15*S);Ring(R-9*S,0,2*PI,Gold,1.5*S);
 for(int I=0;I<32;++I){float A=2*PI*I/32;DrawLine(X+(R-3*S)*FMath::Cos(A),Y+(R-3*S)*FMath::Sin(A),X+(R+3*S)*FMath::Cos(A+.025),Y+(R+3*S)*FMath::Sin(A+.025),FLinearColor(.43,.43,.32),S);}
 for(float A:{PI*.12f,PI*.88f,PI*1.5f}){DrawLine(X,Y,X+FMath::Cos(A)*(R-10*S),Y+FMath::Sin(A)*(R-10*S),Metal,13*S);DrawLine(X,Y,X+FMath::Cos(A)*(R-10*S),Y+FMath::Sin(A)*(R-10*S),Leather,7*S);}
 Ring(19*S,0,2*PI,Leather,25*S);Ring(21*S,0,2*PI,Gold,2*S);
 Frame(X-28*S,Y-11*S,56*S,25*S,FLinearColor(.12,.15,.12),Gold,4*S);Centered(TEXT("SPACE"),X,Y-5*S,.57*S,FLinearColor(.98,.94,.82));
 // The timing track is separate from the physical wheel rim.
 Ring(R+30*S,PI,2*PI,FLinearColor(.12,.18,.14,.95),8*S);Ring(R+30*S,PI+Low*PI,PI+High*PI,Mint,8*S);
 for(int I=0;I<=10;++I){float A=PI+I*PI/10;DrawLine(X+(R+38*S)*FMath::Cos(A),Y+(R+38*S)*FMath::Sin(A),X+(R+43*S)*FMath::Cos(A),Y+(R+43*S)*FMath::Sin(A),Metal,S);}
 float A=PI+Marker*PI,CX=X+(R+30*S)*FMath::Cos(A),CY=Y+(R+30*S)*FMath::Sin(A);float T=7*S;
 DrawLine(CX-T,CY,CX,CY-T,Gold,3*S);DrawLine(CX,CY-T,CX+T,CY,Gold,3*S);DrawLine(CX+T,CY,CX,CY+T,Gold,3*S);DrawLine(CX,CY+T,CX-T,CY,Gold,3*S);
}
