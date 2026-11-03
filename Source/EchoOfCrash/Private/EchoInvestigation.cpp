#include "EchoCampaign.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "EngineUtils.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

float AEchoCampaignDirector::InteractionReach(int32 Index) const
{
 return Clues.IsValidIndex(Index) && Clues[Index].Rescue?24.f:14.f;
}
bool AEchoCampaignDirector::CanReachClue(int32 Index) const
{
 if(!Clues.IsValidIndex(Index) || Clues[Index].Complete)return false;
 const auto PC=GetWorld()->GetFirstPlayerController();
 return PC && PC->GetPawn() && FVector::Dist(SpiritContactPoint(),Clues[Index].Position)<=InteractionReach(Index);
}

FVector AEchoCampaignDirector::SpiritContactPoint() const
{
 auto PC=GetWorld()->GetFirstPlayerController();auto P=PC?PC->GetPawn():nullptr;
 if(auto G=Cast<AEchoGhostPawn>(P))if(G->Spirit)return G->Spirit->GetSocketLocation(TEXT("spine_03"));
 return P?P->GetActorLocation():FVector::ZeroVector;
}
void AEchoCampaignDirector::SetEchoAtmosphere(bool Enable)
{
 if(!Enable){if(IsValid(EchoFog))EchoFog->Destroy();EchoFog=nullptr;for(auto A:SuppressedFog)if(IsValid(A))A->SetActorHiddenInGame(false);SuppressedFog.Empty();return;}
 if(IsValid(EchoFog))return;
 for(TActorIterator<AExponentialHeightFog> It(GetWorld());It;++It)if(!It->IsHidden()){SuppressedFog.Add(*It);It->SetActorHiddenInGame(true);}
 EchoFog=GetWorld()->SpawnActor<AExponentialHeightFog>(FVector(Center.X,Center.Y,450),FRotator::ZeroRotator);
 auto Fog=EchoFog->GetComponent();Fog->SetFogDensity(.45f);Fog->SetFogHeightFalloff(.04f);Fog->SetFogInscatteringColor(FLinearColor(.55f,.65f,.66f));Fog->SetFogMaxOpacity(.97f);Fog->SetStartDistance(90);
}
FVector AEchoCampaignDirector::ConstrainSpirit(FVector P,FVector& V)
{
 FVector Limited(FMath::Clamp(P.X,Center.X-InvestigationExtent.X,Center.X+InvestigationExtent.X),FMath::Clamp(P.Y,Center.Y-InvestigationExtent.Y,Center.Y+InvestigationExtent.Y),FMath::Clamp(P.Z,35.f,520.f));
 if(!Limited.Equals(P,.01f)){if(Limited.X!=P.X)V.X=0;if(Limited.Y!=P.Y)V.Y=0;if(Limited.Z!=P.Z)V.Z=0;if(Phase==EEchoPhase::Investigation && BoundaryCooldown<=0 && Notice.IsEmpty()){SoundEvent(TEXT("SpiritBoundary"),.22f);Say(TEXT("echo_boundary"));BoundaryCooldown=16;}}
 return Limited;
}
void AEchoCampaignDirector::RequestHint()
{
 if(Phase!=EEchoPhase::Investigation || Paused || JournalOpen || InspectIndex!=INDEX_NONE || HintCooldown>0)return;
 SoundEvent(TEXT("HintChime"),.3f);
 int Target=INDEX_NONE;
 // Each request advances to one different unseen evidence target, then a life.
 for(bool Rescue:{false,true})for(int I=0;I<Clues.Num();++I)if(Target==INDEX_NONE && Clues[I].Rescue==Rescue && !Clues[I].Complete && !Clues[I].HintRevealed)Target=I;
 if(Target==INDEX_NONE)for(int Step=1;Step<=Clues.Num();++Step){int I=(FMath::Max(-1,HintTarget)+Step)%Clues.Num();if(!Clues[I].Complete){Target=I;break;}}
 if(Target==INDEX_NONE){Say(TEXT("echo_all_reached"));HintCooldown=3;return;}
 auto& C=Clues[Target];if(!C.HintRevealed){C.HintRevealed=true;HintPenalty=FMath::Min(250,HintPenalty+50);}
 HintTarget=Target;ScanTime=8;HintCooldown=3;
 const TCHAR* One[]={TEXT("echo_hint_bottle"),TEXT("echo_hint_photo")};const TCHAR* Two[]={TEXT("echo_hint_stone"),TEXT("echo_hint_knife")};const TCHAR* Three[]={TEXT("echo_hint_voucher"),TEXT("echo_hint_burgers"),TEXT("echo_hint_cola")};
 Say(C.Rescue?TEXT("echo_hint_person"):Chapter==0?One[Target]:Chapter==1?Two[Target]:Three[Target]);
}
void AEchoCampaignDirector::BeginDissolve(int32 I)
{
 auto& C=Clues[I];if(!IsValid(C.SourceActor))return;C.DissolveAge=0;C.OriginalPosition=C.SourceActor->GetActorLocation();
 TArray<AActor*> Parts{C.SourceActor};if(!C.Rescue){TArray<AActor*> AttachedProps;C.SourceActor->GetAttachedActors(AttachedProps);Parts.Append(AttachedProps);}
 for(auto A:Parts){TArray<UMeshComponent*> Meshes;A->GetComponents(Meshes);for(auto Mesh:Meshes)for(int Slot=0;Slot<Mesh->GetNumMaterials();++Slot)if(auto Original=Mesh->GetMaterial(Slot)){
  FString Name=TEXT("DM_")+Original->GetName();FString Path=TEXT("/Game/EchoCampaign/Materials/Dissolve/")+Name+TEXT(".")+Name;
  if(auto Base=LoadObject<UMaterialInterface>(nullptr,*Path)){auto MID=UMaterialInstanceDynamic::Create(Base,this);MID->SetScalarParameterValue(TEXT("EchoVisibility"),1);Mesh->SetMaterial(Slot,MID);C.DissolveMaterials.Add(MID);}
 }}
}
void AEchoCampaignDirector::TickInvestigationEffects(float Dt)
{
 HintCooldown=FMath::Max(0.f,HintCooldown-Dt);BoundaryCooldown=FMath::Max(0.f,BoundaryCooldown-Dt);
 for(auto& C:Clues)if(C.DissolveAge>=0 && IsValid(C.SourceActor)){
  C.DissolveAge+=Dt;float U=FMath::Clamp(C.DissolveAge/1.6f,0.f,1.f),Ease=U*U*(3-2*U);
  for(auto M:C.DissolveMaterials)if(M)M->SetScalarParameterValue(TEXT("EchoVisibility"),1-Ease);
  C.SourceActor->SetActorLocation(C.OriginalPosition+FVector(0,0,18*Ease));
  if(U>=1){C.SourceActor->SetActorHiddenInGame(true);if(!C.Rescue){TArray<AActor*> AttachedProps;C.SourceActor->GetAttachedActors(AttachedProps);for(auto A:AttachedProps)A->SetActorHiddenInGame(true);}C.SourceActor->SetActorLocation(C.OriginalPosition);C.DissolveAge=-1;}
 }
}


