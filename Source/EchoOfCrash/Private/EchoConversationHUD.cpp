#include "EchoCampaign.h"
#include "EchoGameInstance.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

void AEchoCampaignHUD::DrawConversationAndInteraction(AEchoCampaignDirector* D,float W,float H,float Scale)
{
 DialogueOrigin=DialogueSize=InteractionOrigin=InteractionSize=FVector2D::ZeroVector;
 if(D->JournalOpen || D->InspectIndex!=INDEX_NONE || D->Paused)return;
 const auto GI=Cast<UEchoGameInstance>(GetGameInstance());
 const bool Awake=D->Phase==EEchoPhase::Awakening,Search=D->Phase==EEchoPhase::Investigation;
 const bool ShowDialogue=!D->Notice.IsEmpty() && D->Phase!=EEchoPhase::Results && D->Phase!=EEchoPhase::QTE && D->Phase!=EEchoPhase::Deduction && (D->DialogueID.IsNone() || !GI || GI->Subtitles);
 const float S=FMath::Clamp(Scale,.8f,1.8f),CX=W*.5f;
 const FLinearColor Cream(.98f,.96f,.88f),Gold(.98f,.78f,.44f),Mint(.66f,.90f,.78f);
 auto Text=[&](const FString& T,float X,float Y,float Size,FLinearColor C,bool Center=false,bool Bold=false){PaintText(T,X,Y,Size*S/17.f,C,Center,Bold);};
 auto Diamond=[&](float X,float Y,float R,FLinearColor Color){DrawLine(X-R,Y,X,Y-R,Color,S);DrawLine(X,Y-R,X+R,Y,Color,S);DrawLine(X+R,Y,X,Y+R,Color,S);DrawLine(X,Y+R,X-R,Y,Color,S);};
 auto Wrap=[&](const FString& Value,float Width,float Size){return WrapText(Value,Width,Size*S/17.f);};
 const float BodyHeight=MeasureText(TEXT("Ag"),22*S/17.f).Y;
 const float LineStep=BodyHeight+8*S;
 const float NameHeight=MeasureText(TEXT("Ag"),23*S/17.f,true).Y;
 float DialogueTop=H-65*S;
 if(ShowDialogue){
  if(LastDialogue!=D->Notice){LastDialogue=D->Notice;DialogueAppearedAt=GetWorld()->GetRealTimeSeconds();}
  float Fade=FMath::Clamp((GetWorld()->GetRealTimeSeconds()-DialogueAppearedAt)/.22f,0.f,1.f);
  const float Width=FMath::Min(W*.86f,1040*S),Bottom=H-(Search?56:28)*S;
  const auto Lines=Wrap(D->Notice,Width-36*S,22);
  const float BodyTop=18*S+NameHeight+18*S;
  const float Height=BodyTop+Lines.Num()*LineStep+66*S;
  DialogueTop=Bottom-Height;
  DialogueOrigin=FVector2D((W-Width)/2,DialogueTop);DialogueSize=FVector2D(Width,Height);
  // A wide, feathered veil gives contrast without putting the scene in a box.
  float Top=DialogueTop-40*S;
  for(int I=0;I<120;++I){float A=(I+.5f)/120.f,Y=FMath::RoundToFloat(Top+(H-Top)*I/120),Next=FMath::RoundToFloat(Top+(H-Top)*(I+1)/120);DrawRect(FLinearColor(.017f,.027f,.024f,.72f*A*A),0,Y,W,Next-Y);}
  FLinearColor Ink=Cream,NameInk=Gold;Ink.A=NameInk.A=Fade;
  const FString Speaker=!D->Speaker.IsEmpty()?D->Speaker:Search?TEXT("A MOMENT TO NOTICE"):TEXT("THE ROAD HOME");
  float NameHalf=MeasureText(Speaker,23*S/17.f,true).X*.5f+24*S;
  const float LineY=DialogueTop+18*S+NameHeight*.5f,Span=FMath::Min(Width*.41f,375*S);
  for(float Side:{-1.f,1.f}){
   for(int I=0;I<24;++I){float A=I/24.f,B=(I+1)/24.f;FLinearColor C=Gold;C.A=(1-A)*.6f*Fade;DrawLine(CX+Side*FMath::Lerp(NameHalf,Span,A),LineY,CX+Side*FMath::Lerp(NameHalf,Span,B),LineY,C,S);}
   Diamond(CX+Side*(NameHalf+8*S),LineY,3*S,NameInk);
  }
  Text(Speaker,CX,DialogueTop+18*S,23,NameInk,true,true);
  for(int I=0;I<Lines.Num();++I)Text(Lines[I],CX,DialogueTop+BodyTop+LineStep*I,22,Ink,true);
  const float AY=Bottom-42*S+3*S*FMath::Sin(GetWorld()->GetTimeSeconds()*2*PI/3.f);
  ContinueMarker(CX,AY,S);
  Text(TEXT("ENTER"),CX,Bottom-22*S,13,FLinearColor(.94f,.90f,.78f,1),true);
  AddHitBox(FVector2D(CX-100*S,Bottom-60*S),FVector2D(200*S,60*S),TEXT("DialogueNext"),true);

 }else LastDialogue.Empty();
 if(Search && D->Clues.IsValidIndex(D->Focus) && !D->Clues[D->Focus].Complete){
  const auto& C=D->Clues[D->Focus];
  const float Width=FMath::Min(380*S,W*.40f),TextWidth=Width-96*S;
  const auto Lines=Wrap(C.Title,TextWidth,21);
  const float MetaHeight=MeasureText(TEXT("Ag"),12*S/17.f).Y;
  const float TitleHeight=MeasureText(TEXT("Ag"),21*S/17.f).Y;
  const float ActionHeight=MeasureText(TEXT("Ag"),14*S/17.f).Y;
  const float TitleTop=14*S+MetaHeight+9*S,ActionTop=TitleTop+Lines.Num()*(TitleHeight+5*S)+7*S;
  const float Height=ActionTop+ActionHeight+(C.Rescue?24:14)*S;
  const float X=FMath::Min(CX+58*S,W-Width-30*S);
  // Reserve the complete dialogue veil and a breathing gap. Longer lines move
  // only this local prompt up; its key, progress and title remain one group.
  const float MaxBottom=ShowDialogue?DialogueTop-52*S:H-105*S;
  const float Y=FMath::Min(H*.55f-Height*.5f,MaxBottom-Height);
  InteractionOrigin=FVector2D(X,Y);InteractionSize=FVector2D(Width,Height);
  Frame(X,Y,Width,Height,FLinearColor(.025f,.048f,.039f,.85f),FLinearColor(.82f,.75f,.54f,.46f),10*S);
  DrawLine(X+14*S,Y+10*S,X+14*S,Y+Height-10*S,C.Rescue?Mint:Gold,2*S);
  Frame(X+27*S,Y+(Height-38*S)/2,38*S,38*S,FLinearColor(.92f,.90f,.79f,.98f),FLinearColor(.98f,.82f,.51f,.8f),5*S);
  Text(TEXT("E"),X+46*S,Y+(Height-MeasureText(TEXT("E"),23*S/17.f,true).Y)/2,23,FLinearColor(.12f,.18f,.14f),true,true);
  const float TX=X+82*S;
  Text(C.Rescue?TEXT("BRING THEM BACK"):TEXT("A TRACE OF THE MOMENT"),TX,Y+12*S,12,C.Rescue?Mint:Gold);
  for(int I=0;I<Lines.Num();++I)Text(Lines[I],TX,Y+TitleTop+I*(TitleHeight+5*S),21,Cream);
  float ActionY=Y+ActionTop;
  Text(!D->CanReachClue(D->Focus)?TEXT("Swim closer to reach"):C.Rescue?FString::Printf(TEXT("Hold 3s   /   -%.0fs on rescue"),D->Rules.RescueTimeCost):TEXT("Press to collect"),TX,ActionY,14,FLinearColor(.84f,.86f,.77f));
  if(C.Rescue){float BX=TX,BY=Y+Height-12*S,BW=TextWidth-8*S;DrawRect(FLinearColor(.70f,.72f,.63f,.22f),BX,BY,BW,3*S);DrawRect(Mint,BX,BY,BW*FMath::Clamp(D->RescueProgress/3,0.f,1.f),3*S);}
 }
 if(Search){
  if(!ShowDialogue)for(int I=0;I<40;++I){float A=(I+.5f)/40;DrawRect(FLinearColor(.02f,.028f,.022f,.4f*A*A),0,H-65*S+65*S*I/40,W,65*S/40);}
  Text(TEXT("WASD  Swim     SPACE / CTRL  Rise / Descend     E  Interact     Q  Hint     TAB  Evidence     F  Reconstruct"),CX,H-28*S,FMath::Min(13.f,(W-30*S)/(98*7*S)*13.f),FLinearColor(.84f,.86f,.78f,.88f),true);
 }
 if(Awake)Text(TEXT("LISTEN TO THE ECHO  /  SWIMMING BEGINS AFTER THE INTRODUCTION"),CX,163*S,12,Mint,true);
}
