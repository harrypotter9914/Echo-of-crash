#include "EchoCampaign.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"

// Use Canvas's own measurement, with exactly the same font and DPI as drawing.
struct FEchoMeasuredText : FCanvasTextItem
{
 using FCanvasTextItem::FCanvasTextItem;
 using FCanvasTextItem::GetTextSize;
};
static FSlateFontInfo EchoFont(float Scale,bool Bold)
{
 return FSlateFontInfo(LoadObject<UFont>(nullptr,TEXT("/Engine/EngineFonts/Roboto.Roboto")),FMath::Max(8,FMath::RoundToInt(17*Scale)),Bold?TEXT("Bold"):TEXT("Regular"));
}
FVector2D AEchoCampaignHUD::MeasureText(const FString& T,float S,bool Bold) const
{
 FEchoMeasuredText Item(FVector2D::ZeroVector,FText::FromString(T),EchoFont(S,Bold),FLinearColor::White);
 return Item.GetTextSize(Canvas && Canvas->Canvas?Canvas->Canvas->GetDPIScale():1.f);
}
void AEchoCampaignHUD::PaintText(const FString& T,float X,float Y,float S,FLinearColor Color,bool Center,bool Bold)
{
 X=FMath::RoundToFloat(X);Y=FMath::RoundToFloat(Y);
 FEchoMeasuredText Item(FVector2D(X,Y),FText::FromString(T),EchoFont(S,Bold),Color);Item.bCentreX=Center;
 if(Color.GetLuminance()>.5f)Item.EnableShadow(FLinearColor(0,0,0,Color.A*.45f),FVector2D(0,1));
 if(AuditLayout){FVector2D Size=Item.GetTextSize(Canvas->Canvas->GetDPIScale());TextBounds.Add({T,FVector2D(X-(Center?Size.X/2:0),Y),Size});}
 Canvas->DrawItem(Item);
}
float AEchoCampaignHUD::FitText(const FString& T,float S,float W,float H,bool Bold) const
{
 for(int I=0;I<100;++I){auto Size=MeasureText(T,S,Bold);if((Size.X<=W && Size.Y<=H) || S<=8.f/17.f)break;S=FMath::Max(8.f/17.f,S-1.f/17.f);}return S;
}
TArray<FString> AEchoCampaignHUD::WrapText(const FString& T,float W,float S,bool Bold) const
{
 TArray<FString> Words,Lines;T.ParseIntoArrayWS(Words);FString Line;
 for(const auto& Word:Words){FString Next=Line.IsEmpty()?Word:Line+TEXT(" ")+Word;if(!Line.IsEmpty() && MeasureText(Next,S,Bold).X>W){Lines.Add(Line);Line=Word;}else Line=Next;}
 if(!Line.IsEmpty())Lines.Add(Line);return Lines;
}
float AEchoCampaignHUD::TextBlock(const FString& T,float X,float Y,float W,float S,FLinearColor C,bool Center,bool Bold)
{
 float Step=MeasureText(TEXT("Ag"),S,Bold).Y+FMath::Max(5.f,7*S),Top=Y;
 for(const auto& Line:WrapText(T,W,S,Bold)){PaintText(Line,X,Y,S,C,Center,Bold);Y+=Step;}
 return Y-Top;
}
void AEchoCampaignHUD::ContinueMarker(float X,float Y,float S)
{
 if(!ContinueTexture){
  // One anti-aliased chevron mask. Geometry, brightness and opacity never pulse.
  const int W=128,H=64;ContinueTexture=UTexture2D::CreateTransient(W,H,PF_B8G8R8A8);ContinueTexture->SRGB=true;ContinueTexture->Filter=TF_Bilinear;
  auto& Mip=ContinueTexture->GetPlatformData()->Mips[0];auto Pixels=static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
  auto Distance=[](FVector2D P,FVector2D A,FVector2D B){FVector2D D=B-A;return (P-A-D*FMath::Clamp(FVector2D::DotProduct(P-A,D)/D.SizeSquared(),0.,1.)).Size();};
  for(int J=0;J<H;++J)for(int I=0;I<W;++I){FVector2D P(I+.5,J+.5);float D=FMath::Min(Distance(P,{25,17},{64,43}),Distance(P,{64,43},{103,17}));uint8 Alpha=FMath::RoundToInt(255*FMath::Clamp(7.f-D,0.f,1.f));Pixels[J*W+I]=FColor(255,255,255,Alpha);}
  Mip.BulkData.Unlock();ContinueTexture->UpdateResource();
 }
 ContinueOrigin=FVector2D(X-14*S,Y-7*S);ContinueSize=FVector2D(28*S,14*S);
 DrawTexture(ContinueTexture,ContinueOrigin.X,ContinueOrigin.Y,ContinueSize.X,ContinueSize.Y,0,0,1,1,FLinearColor(.98f,.80f,.48f,1),BLEND_Translucent);
}
FString AEchoCampaignHUD::GetLayoutAudit() const
{
 FString Out=TEXT("{\"texts\":[");
 for(int I=0;I<TextBounds.Num();++I){const auto& B=TextBounds[I];FString T=B.Text.Replace(TEXT("\\"),TEXT("\\\\")).Replace(TEXT("\""),TEXT("\\\""));if(I)Out+=TEXT(",");Out+=FString::Printf(TEXT("{\"text\":\"%s\",\"x\":%.2f,\"y\":%.2f,\"w\":%.2f,\"h\":%.2f}"),*T,B.Origin.X,B.Origin.Y,B.Size.X,B.Size.Y);}
 return Out+FString::Printf(TEXT("],\"marker\":[%.3f,%.3f,%.3f,%.3f]}"),ContinueOrigin.X,ContinueOrigin.Y,ContinueSize.X,ContinueSize.Y);
}
