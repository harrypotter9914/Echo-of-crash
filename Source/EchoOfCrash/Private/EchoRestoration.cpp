#include "EchoCampaign.h"
#include "EchoGameInstance.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "EchoPoseRefinement.h"
#include "EchoMouseLook.h"

AEchoGhostPawn::AEchoGhostPawn()
{
 PrimaryActorTick.bCanEverTick=true;
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("GhostRoot"));
 Spirit=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("OriginalSpirit"));Spirit->SetupAttachment(RootComponent);
 Spirit->SetRelativeScale3D(FVector(.090496f));Spirit->SetRelativeRotation(FRotator(0,-90,25.526729f));Spirit->SetRelativeLocation(FVector(0,0,-8));
 Spirit->SetCollisionEnabled(ECollisionEnabled::NoCollision);Spirit->SetCastShadow(false);Spirit->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
 Boom=CreateDefaultSubobject<USpringArmComponent>(TEXT("SpiritFollow"));Boom->SetupAttachment(RootComponent);Boom->TargetArmLength=34;Boom->TargetOffset=FVector(0,0,1);Boom->SocketOffset=FVector::ZeroVector;Boom->bUsePawnControlRotation=true;Boom->bDoCollisionTest=false;Boom->bEnableCameraLag=true;Boom->CameraLagSpeed=9;Boom->CameraLagMaxDistance=3;
 Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("GhostCamera"));Camera->SetupAttachment(Boom);Camera->SetFieldOfView(84);Camera->bUsePawnControlRotation=false;Camera->PostProcessSettings.bOverride_BloomIntensity=true;Camera->PostProcessSettings.BloomIntensity=.22f;
 auto Glow=CreateDefaultSubobject<UPointLightComponent>(TEXT("SpiritGlow"));Glow->SetupAttachment(RootComponent);Glow->SetLightColor(FLinearColor(.72f,.94f,1.f));Glow->SetIntensity(45);Glow->SetAttenuationRadius(100);
}
void AEchoGhostPawn::BeginPlay()
{
 Super::BeginPlay();
 Spirit->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/PolygonTown/Meshes/Characters/SK_Character_Father_01.SK_Character_Father_01")));
 Spirit->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/EchoCampaign/Art/M_SpiritOriginal.M_SpiritOriginal")));
 Spirit->PlayAnimation(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/EchoCampaign/Animation/SpiritSwim.SpiritSwim")),true);
 Spirit->SetPlayRate(.85f);SetActorHiddenInGame(true);
}
void AEchoGhostPawn::Tick(float Dt)
{
 Super::Tick(Dt);auto PC=Cast<APlayerController>(Controller);if(!PC)return;
 AEchoCampaignDirector* D=nullptr;for(TActorIterator<AEchoCampaignDirector> It(GetWorld());It;++It){if(It->ActorHasTag(TEXT("EchoIncidentTemplate")))continue;D=*It;break;}
 if(!D)return;bool Visible=D->Phase==EEchoPhase::Investigation || D->Phase==EEchoPhase::Awakening;
 SetActorHiddenInGame(!Visible);
 bool Active=Visible && !D->Paused && !D->JournalOpen && D->InspectIndex==INDEX_NONE;
 Spirit->bPauseAnims=!Active;if(!Active){SwimVelocity=FVector::ZeroVector;return;}
 // Emerge from the driver's eye position while the white veil recedes.
 // This clears the original pitched crash vehicle without teleporting the spirit.
 if(D->Phase==EEchoPhase::Awakening){float Rise=85*FMath::SmoothStep(0.f,2.f,D->PhaseTime);AddActorWorldOffset(FVector(0,0,Rise-EmergenceApplied));EmergenceApplied=Rise;}
 SwimClock+=Dt;FRotator R=PC->GetControlRotation();
 FVector Input=FVector::ZeroVector;
 if(D->Phase==EEchoPhase::Awakening)SwimVelocity=FVector::ZeroVector;
 if(D->Phase==EEchoPhase::Investigation && !PC->IsInputKeyDown(EKeys::LeftAlt)){
  float MX,MY;EchoMouseLook::Read(PC,MX,MY);
  ApplyLookInput(MX,MY);R=PC->GetControlRotation();
  Input=R.Vector()*float(PC->IsInputKeyDown(EKeys::W)-PC->IsInputKeyDown(EKeys::S));
  Input+=FRotationMatrix(FRotator(0,R.Yaw,0)).GetUnitAxis(EAxis::Y)*float(PC->IsInputKeyDown(EKeys::D)-PC->IsInputKeyDown(EKeys::A));
  Input.Z+=float(PC->IsInputKeyDown(EKeys::SpaceBar)-PC->IsInputKeyDown(EKeys::LeftControl));
 }
 float MaxSpeed=PC->IsInputKeyDown(EKeys::LeftShift)?165.f:95.f;
 FVector Desired=Input.GetClampedToMaxSize(1)*MaxSpeed;
 float Response=Input.IsNearlyZero()?3.8f:2.8f;
 SwimVelocity=FMath::Lerp(SwimVelocity,Desired,1-FMath::Exp(-Response*Dt));
 FVector P=D->ConstrainSpirit(GetActorLocation()+SwimVelocity*Dt,SwimVelocity);SetActorLocation(P);
 float Yaw=SwimVelocity.Size2D()>8?SwimVelocity.Rotation().Yaw:R.Yaw;
 FRotator Target(0,Yaw-90,25.526729f+SwimVelocity.Size()/MaxSpeed*12);
 Spirit->SetWorldRotation(FMath::RInterpTo(Spirit->GetComponentRotation(),Target,Dt,4));
 Spirit->SetRelativeLocation(FVector(0,0,-8+FMath::Sin(SwimClock*1.7f)*1.3f));Spirit->SetPlayRate(.72f+SwimVelocity.Size()/MaxSpeed*.38f);
 // Orbit the swimmer's body rather than the off-centre animation root. This
 // keeps the figure near screen centre even while looking steeply up or down.
 FVector Feet=(Spirit->GetSocketLocation(TEXT("foot_l"))+Spirit->GetSocketLocation(TEXT("foot_r")))*.5f;
 FVector Focus=(Spirit->GetSocketLocation(TEXT("head"))+Feet)*.5f;
 Boom->TargetOffset=FMath::VInterpTo(Boom->TargetOffset,Focus-GetActorLocation(),Dt,5.f);
}

void AEchoGhostPawn::ApplyLookInput(float X,float Y)
{
 if(auto PC=Cast<APlayerController>(Controller)){float Sens=1;if(auto GI=Cast<UEchoGameInstance>(GetGameInstance()))Sens=GI->LookSensitivity;const float Gain=EchoMouseLook::Gain(Sens);auto R=PC->GetControlRotation();R.Yaw+=X*Gain;R.Pitch=FMath::Clamp(FRotator::NormalizeAxis(R.Pitch)+Y*Gain,-70.f,70.f);PC->SetControlRotation(R);}
}
void AEchoCampaignDirector::ApplyCockpitLook(float X,float Y)
{
 float Sens=1;if(auto GI=Cast<UEchoGameInstance>(GetGameInstance()))Sens=GI->LookSensitivity;
 const float Gain=EchoMouseLook::Gain(Sens);
 CockpitYaw=FMath::Clamp(CockpitYaw+X*Gain,-48.f,48.f);CockpitPitch=FMath::Clamp(CockpitPitch+Y*Gain,-28.f,20.f);
}

void AEchoCampaignDirector::RestoreTableauPoses()
{
 for(auto A:Tableau)if(IsValid(A))if(auto M=A->FindComponentByClass<USkeletalMeshComponent>()){
  if(A->ActorHasTag(TEXT("EchoPlayerCar")) || A->ActorHasTag(TEXT("EchoOpponentCar")))continue;
  M->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
  M->bPauseAnims=false;
  if(Chapter==1 && M->GetSkeletalMeshAsset()){
   FString N=M->GetSkeletalMeshAsset()->GetName();const TCHAR* BP=N.Contains(TEXT("SchoolBoy"))?TEXT("/Game/PolygonTown/Meshes/Characters/SitKiss2_ANBP.SitKiss2_ANBP_C"):N.Contains(TEXT("SchoolGirl"))?TEXT("/Game/PolygonTown/Meshes/Characters/SitKiss_ANBP.SitKiss_ANBP_C"):nullptr;
   if(BP){M->SetAnimationMode(EAnimationMode::AnimationBlueprint);M->SetAnimInstanceClass(LoadClass<UAnimInstance>(nullptr,BP));}
  }
  M->TickAnimation(.6f,false);M->RefreshBoneTransforms();M->bPauseAnims=true;
 }
}

// Keep the imported driving performance, then solve both arms onto this pickup's wheel.
static void GripWheel(UPoseableMeshComponent* M,bool Left,FVector Target,FVector Pole)
{
 FName Upper=Left?TEXT("UpperArm_L"):TEXT("UpperArm_R"),Lower=Left?TEXT("lowerarm_l"):TEXT("lowerarm_r"),Hand=Left?TEXT("Hand_L"):TEXT("Hand_R");
 FTransform U=M->GetBoneTransformByName(Upper,EBoneSpaces::WorldSpace),L=M->GetBoneTransformByName(Lower,EBoneSpaces::WorldSpace),H=M->GetBoneTransformByName(Hand,EBoneSpaces::WorldSpace);
 FVector A=U.GetLocation(),B=L.GetLocation(),C=H.GetLocation(),D=(Target-A).GetSafeNormal();float L1=FVector::Distance(A,B),L2=FVector::Distance(B,C),Dist=FMath::Clamp(float(FVector::Distance(A,Target)),FMath::Abs(L1-L2)+.01f,L1+L2-.01f);
 Target=A+D*Dist;float Along=(L1*L1-L2*L2+Dist*Dist)/(2*Dist),Height=FMath::Sqrt(FMath::Max(0.f,L1*L1-Along*Along));FVector Bend=(Pole-D*FVector::DotProduct(Pole,D)).GetSafeNormal(),Joint=A+D*Along+Bend*Height;
 U.SetRotation(FQuat::FindBetweenNormals((B-A).GetSafeNormal(),(Joint-A).GetSafeNormal())*U.GetRotation());FQuat Turn=FQuat::FindBetweenNormals((C-B).GetSafeNormal(),(Target-Joint).GetSafeNormal());L.SetRotation(Turn*L.GetRotation());L.SetLocation(Joint);H.SetRotation(Turn*H.GetRotation());H.SetLocation(Target);
 M->SetBoneTransformByName(Upper,U,EBoneSpaces::WorldSpace);M->SetBoneTransformByName(Lower,L,EBoneSpaces::WorldSpace);M->SetBoneTransformByName(Hand,H,EBoneSpaces::WorldSpace);
}

void AEchoCampaignDirector::UpdateCockpit(float Dt)
{
 if(!IsValid(DriveCar)||!IsValid(DriveCamera))return;
 USkeletalMeshComponent* Sampler=nullptr;UPoseableMeshComponent* Driver=nullptr;
 TArray<USkeletalMeshComponent*> Samples;DriveCar->GetComponents(Samples);for(auto M:Samples)if(M->GetFName()==TEXT("DriverSampler"))Sampler=M;
 TArray<UPoseableMeshComponent*> Poses;DriveCar->GetComponents(Poses);for(auto M:Poses)if(M->GetFName()==TEXT("CockpitDriver"))Driver=M;
 if(Sampler && Driver && DriveMesh){
  Sampler->TickAnimation(0,false);Sampler->RefreshBoneTransforms();Driver->CopyPoseFromSkeletalComponent(Sampler);
  EchoPose::SoftenWrists(Driver);
  float WheelTurn=FMath::Clamp(Steering*1.7f,-65.f,65.f);if(Phase==EEchoPhase::Avoidance && Chapter<2)WheelTurn=FMath::Sin(PhaseTime*PI/2.1f)*-48.f;
  const FName Wheel(TEXT("SM_Veh_Pickup_SteeringW"));DriveMesh->ResetBoneTransformByName(Wheel);auto W=DriveMesh->GetBoneTransformByName(Wheel,EBoneSpaces::ComponentSpace);FQuat Turn(FVector::ForwardVector,FMath::DegreesToRadians(WheelTurn));W.SetRotation(Turn*W.GetRotation());DriveMesh->SetBoneTransformByName(Wheel,W,EBoneSpaces::ComponentSpace);
  for(bool Left:{true,false}){FVector Grip=W.GetLocation()+Turn.RotateVector(FVector(-3,Left?-17:17,8));GripWheel(Driver,Left,DriveCar->GetActorTransform().TransformPosition(Grip),DriveCar->GetActorRightVector()*(Left?-1:1)-FVector::UpVector*.8f);}
  Driver->HideBoneByName(TEXT("head"),EPhysBodyOp::PBO_None);
 }

 auto PC=GetWorld()->GetFirstPlayerController();float MX=0,MY=0;if(PC && !Paused && LastCockpitInputFrame!=GFrameCounter){EchoMouseLook::Read(PC,MX,MY);LastCockpitInputFrame=GFrameCounter;}
 ApplyCockpitLook(MX,MY);
 FVector Seat=DriveCar->GetActorTransform().TransformPosition(CockpitOffset);
 float Vibration=FMath::Clamp(Speed/800.f,0.f,1.f)*.18f*FMath::Sin(GetWorld()->GetTimeSeconds()*13);
 DriveCamera->SetActorLocation(Seat+FVector(0,0,Vibration));
 DriveCamera->SetActorRotation(DriveCar->GetActorRotation()+FRotator(-3+CockpitPitch,CockpitYaw,0));
}
