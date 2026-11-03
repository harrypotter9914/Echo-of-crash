#include "EchoPresentation.h"
#include "EchoCampaign.h"
#include "Misc/ScopeExit.h"
#include "EchoGameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "EchoPoseRefinement.h"

static void SetPerformanceScale(UPoseableMeshComponent* Mesh,FVector Scale)
{
 if(Mesh->GetOwner()->GetActorScale3D().Equals(Scale))return;
 Mesh->GetOwner()->SetActorScale3D(Scale);
 // The poseable proxy caches reverse culling. Recreate it when crossing the
 // mirrored passenger-side transition, or the body can render inside out.
 Mesh->MarkRenderStateDirty();
}
static void GroundPerformance(UPoseableMeshComponent* Mesh,FVector Feet,float BaseFloor)
{
 FCollisionQueryParams Query;Query.bTraceComplex=true;Query.AddIgnoredActor(Mesh->GetOwner());float Correction=-BIG_NUMBER;
 // Feet can occupy different stair treads. Probe each heel and toe instead of
 // choosing the floor under the actor centre and burying the leading foot.
 for(FName Bone:{FName(TEXT("foot_l")),FName(TEXT("foot_r")),FName(TEXT("ball_l")),FName(TEXT("ball_r"))}){
  if(Mesh->GetBoneIndex(Bone)==INDEX_NONE)continue;FVector P=Mesh->GetBoneLocationByName(Bone,EBoneSpaces::WorldSpace);float Floor=BaseFloor;FHitResult Hit;
  if(Mesh->GetWorld()->LineTraceSingleByChannel(Hit,P+FVector(0,0,60),P-FVector(0,0,120),ECC_Visibility,Query)
   && !Hit.bStartPenetrating && Hit.ImpactNormal.Z>.7f && Hit.ImpactPoint.Z<=Feet.Z+35)Floor=Hit.ImpactPoint.Z;
  float SoleOffset=Bone.ToString().StartsWith(TEXT("ball"))?.5f:5.f;Correction=FMath::Max(Correction,Floor+SoleOffset-P.Z);
 }
 if(Correction>-BIG_NUMBER)Mesh->GetOwner()->AddActorWorldOffset(FVector(0,0,Correction));
}

AEchoEvidenceProp::AEchoEvidenceProp()
{
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("EvidenceRoot"));
 Picture=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Image"));Picture->SetupAttachment(RootComponent);
 Back=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backing"));Back->SetupAttachment(RootComponent);
 for(int I=0;I<4;++I){auto C=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Frame%d"),I));C->SetupAttachment(RootComponent);Edges.Add(C);}
}
void AEchoEvidenceProp::OnConstruction(const FTransform& T)
{
 Super::OnConstruction(T);
 auto Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 Picture->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane")));
 Picture->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Framed?TEXT("/Game/EchoCampaign/Art/M_WeddingPhoto.M_WeddingPhoto"):TEXT("/Game/EchoCampaign/Art/M_MealVoucher.M_MealVoucher")));
 Picture->SetRelativeLocation(FVector(0,0,Framed?1.5:.15));Picture->SetRelativeScale3D(Framed?FVector(.27,.21,1):FVector(.16,.23,1));
 Back->SetStaticMesh(Cube);Back->SetRelativeScale3D(Framed?FVector(.30,.24,.025):FVector(.16,.23,.002));
 auto Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/EchoCampaign/Art/M_Frame.M_Frame"));if(Material)Back->SetMaterial(0,Framed?Material:LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/EchoCampaign/Art/M_Exterior.M_Exterior")));
 for(int I=0;I<4;++I){auto C=Edges[I];C->SetStaticMesh(Cube);C->SetVisibility(Framed);C->SetRelativeLocation(I<2?FVector(I==0?-14:14,0,1.8):FVector(0,I==2?-11:11,1.8));C->SetRelativeScale3D(I<2?FVector(.025,.24,.025):FVector(.27,.025,.025));if(Material)C->SetMaterial(0,Material);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
 Picture->SetCollisionEnabled(ECollisionEnabled::NoCollision);Back->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
AEchoPreviewRig::AEchoPreviewRig()
{
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 Model=CreateDefaultSubobject<USceneComponent>(TEXT("Turntable"));Model->SetupAttachment(RootComponent);
 Camera=CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PreviewCamera"));Camera->SetupAttachment(RootComponent);
 Camera->bCaptureEveryFrame=false;Camera->bCaptureOnMovement=false;Camera->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
 Camera->CaptureSource=ESceneCaptureSource::SCS_SceneColorHDR;Camera->FOVAngle=40;
 Camera->ShowFlags.SetAtmosphere(false);Camera->ShowFlags.SetFog(false);Camera->ShowFlags.SetVolumetricFog(false);Camera->ShowFlags.SetSkyLighting(false);
 Camera->PostProcessSettings.bOverride_AutoExposureMethod=true;Camera->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
 Camera->PostProcessSettings.bOverride_AutoExposureBias=true;Camera->PostProcessSettings.AutoExposureBias=8;
 auto Light=CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioKey"));Light->SetupAttachment(RootComponent);Light->SetRelativeLocation(FVector(120,-90,150));Light->SetIntensity(9500);Light->SetAttenuationRadius(500);
 auto Fill=CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioFill"));Fill->SetupAttachment(RootComponent);Fill->SetRelativeLocation(FVector(-100,80,80));Fill->SetIntensity(4500);Fill->SetLightColor(FLinearColor(.65,.9,1));Fill->SetAttenuationRadius(500);
}
void AEchoPreviewRig::Build(AActor* Source,bool Portrait)
{
 if(!Source)return;
 Texture=NewObject<UTextureRenderTarget2D>(this);Texture->ClearColor=FLinearColor(0,0,0,1);Texture->InitAutoFormat(768,768);Camera->TextureTarget=Texture;
 if(auto Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/EchoCampaign/Art/M_EvidencePreview.M_EvidencePreview"))){DisplayMaterial=UMaterialInstanceDynamic::Create(Base,this);DisplayMaterial->SetTextureParameterValue(TEXT("Preview"),Texture);}
 FVector Center,Extent;Source->GetActorBounds(false,Center,Extent,true);
 float Scale=80/FMath::Max(1.f,Extent.GetMax()*2);
 if(Portrait){Center=Source->GetActorTransform().TransformPosition(FVector(0,0,140));Scale=1.25f;}
 TArray<UMeshComponent*> Components;Source->GetComponents(Components);
 if(!Portrait){TArray<AActor*> AttachedProps;Source->GetAttachedActors(AttachedProps);for(auto Child:AttachedProps){TArray<UMeshComponent*> Extra;Child->GetComponents(Extra);Components.Append(Extra);}}
 if(!Portrait){FBox VisibleBounds(ForceInit);for(auto C:Components)if(C->IsVisible())VisibleBounds+=C->Bounds.GetBox();if(VisibleBounds.IsValid){Center=VisibleBounds.GetCenter();Extent=VisibleBounds.GetExtent();Scale=90/FMath::Max(1.f,Extent.Size()*2);}}
 for(auto C:Components){if(!C->IsVisible())continue;UMeshComponent* Copy=nullptr;
  if(auto Static=Cast<UStaticMeshComponent>(C)){auto M=NewObject<UStaticMeshComponent>(this);M->SetStaticMesh(Static->GetStaticMesh());Copy=M;}
  else if(auto Skel=Cast<USkeletalMeshComponent>(C)){auto M=NewObject<UPoseableMeshComponent>(this);M->SetSkinnedAssetAndUpdate(Skel->GetSkeletalMeshAsset());M->RegisterComponent();if(!Portrait){Skel->TickAnimation(0,false);Skel->RefreshBoneTransforms();M->CopyPoseFromSkeletalComponent(Skel);}Copy=M;}
  if(!Copy)continue;Copy->AttachToComponent(Model,FAttachmentTransformRules::KeepRelativeTransform);Copy->RegisterComponent();
  Copy->SetRelativeLocation((C->GetComponentLocation()-Center)*Scale);Copy->SetRelativeRotation(C->GetComponentRotation());Copy->SetRelativeScale3D(C->GetComponentScale()*Scale);
  if(Portrait){Copy->SetRelativeLocation(FVector(0,0,-153));Copy->SetRelativeRotation(FRotator(0,-90,0));Copy->SetRelativeScale3D(FVector(1));if(auto P=Cast<UPoseableMeshComponent>(Copy)){for(bool Left:{true,false}){FName Bone=Left?TEXT("UpperArm_L"):TEXT("UpperArm_R");const FQuat Ref=P->GetBoneRotationByName(Bone,EBoneSpaces::ComponentSpace).Quaternion();P->SetBoneRotationByName(Bone,(FRotator(Left?-68:68,0,0).Quaternion()*Ref).Rotator(),EBoneSpaces::ComponentSpace);}}}
  Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision);Copy->SetCastShadow(false);for(int I=0;I<C->GetNumMaterials();++I)Copy->SetMaterial(I,C->GetMaterial(I));Camera->ShowOnlyComponent(Copy);
 }
 if(Cast<AEchoEvidenceProp>(Source))Model->SetRelativeRotation((FRotator(0,0,90).Quaternion()*FRotator(-65,0,0).Quaternion()*Source->GetActorQuat().Inverse()).Rotator());
 else if(!Portrait){if(auto Mesh=Source->FindComponentByClass<UStaticMeshComponent>())if(Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetName().Contains(TEXT("Knife")))Model->SetRelativeRotation((FRotator(-45,0,20).Quaternion()*Source->GetActorQuat().Inverse()).Rotator());}
 if(Portrait)Model->SetRelativeRotation(FRotator::ZeroRotator);
 else if(auto Mesh=Source->FindComponentByClass<UStaticMeshComponent>())if(Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetName().Contains(TEXT("Buger")))Model->SetRelativeRotation(FRotator(0,90,0));
 Camera->SetRelativeLocation(Portrait?FVector(110,0,0):FVector(Distance,0,75));Camera->SetRelativeRotation((-Camera->GetRelativeLocation()).Rotation());InitialModel=Model->GetRelativeTransform();InitialCamera=Camera->GetRelativeTransform();Capture();
 // Poseable meshes upload their final bone buffers after registration. Capture again
 // after that first render update so inventory portraits never retain the bind pose.
 FTimerHandle RefreshPreview;GetWorldTimerManager().SetTimer(RefreshPreview,this,&AEchoPreviewRig::Capture,.3f,false);
}
void AEchoPreviewRig::Rotate(float X,float Y){Model->AddLocalRotation(FRotator(Y*.7f,X*.7f,0));Capture();}
void AEchoPreviewRig::Zoom(float Delta){Distance=FMath::Clamp(Distance+Delta,85.f,250.f);Camera->SetRelativeLocation(FVector(Distance,0,Distance*.44f));Camera->SetRelativeRotation((-Camera->GetRelativeLocation()).Rotation());Capture();}
void AEchoPreviewRig::Capture(){Camera->CaptureScene();}
void AEchoPreviewRig::ResetView(){Model->SetRelativeTransform(InitialModel);Camera->SetRelativeTransform(InitialCamera);Distance=140;Capture();}
// Analytic two-bone reach preserves arm lengths; moving only the hand stretches the skin.
static void ReachHand(UPoseableMeshComponent* M,bool Left,FVector Target)
{
 FName Upper=Left?TEXT("UpperArm_L"):TEXT("UpperArm_R"),Lower=Left?TEXT("lowerarm_l"):TEXT("lowerarm_r"),Hand=Left?TEXT("Hand_L"):TEXT("Hand_R");
 FTransform U=M->GetBoneTransformByName(Upper,EBoneSpaces::WorldSpace),L=M->GetBoneTransformByName(Lower,EBoneSpaces::WorldSpace),H=M->GetBoneTransformByName(Hand,EBoneSpaces::WorldSpace);
 FVector A=U.GetLocation(),B=L.GetLocation(),C=H.GetLocation(),D=(Target-A).GetSafeNormal();float L1=FVector::Distance(A,B),L2=FVector::Distance(B,C),Distance=FMath::Clamp(float(FVector::Distance(A,Target)),FMath::Abs(L1-L2)+.01f,L1+L2-.01f);
 Target=A+D*Distance;float Along=(L1*L1-L2*L2+Distance*Distance)/(2*Distance);float Height=FMath::Sqrt(FMath::Max(0.f,L1*L1-Along*Along));
 FVector Bend=M->GetOwner()->GetActorRightVector()*.35f+M->GetOwner()->GetActorForwardVector()*(Left?.15f:-.15f)-FVector::UpVector;Bend=(Bend-D*FVector::DotProduct(Bend,D)).GetSafeNormal();FVector Joint=A+D*Along+Bend*Height;
 U.SetRotation(FQuat::FindBetweenNormals((B-A).GetSafeNormal(),(Joint-A).GetSafeNormal())*U.GetRotation());
 FQuat ForearmTurn=FQuat::FindBetweenNormals((C-B).GetSafeNormal(),(Target-Joint).GetSafeNormal());
 L.SetRotation(ForearmTurn*L.GetRotation());L.SetLocation(Joint);H.SetRotation(ForearmTurn*H.GetRotation());H.SetLocation(Target);
 M->SetBoneTransformByName(Upper,U,EBoneSpaces::WorldSpace);M->SetBoneTransformByName(Lower,L,EBoneSpaces::WorldSpace);M->SetBoneTransformByName(Hand,H,EBoneSpaces::WorldSpace);
}
// Cinematic-only derivative of the original pickup, with separately hinged doors.
static AActor* CinematicPickup(UWorld* World,FVector Location)
{
 auto A=World->SpawnActor<AActor>();auto Body=NewObject<UStaticMeshComponent>(A,TEXT("PickupBody"));A->SetRootComponent(Body);Body->RegisterComponent();Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/EchoCampaign/Vehicles/SM_PickupBody.SM_PickupBody")));Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);A->SetActorLocation(Location);
 for(int I=0;I<2;++I){auto Door=NewObject<UStaticMeshComponent>(A,I==0?TEXT("PickupDoorL"):TEXT("PickupDoorR"));Door->SetupAttachment(Body);Door->RegisterComponent();Door->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,I==0?TEXT("/Game/EchoCampaign/Vehicles/SM_PickupDoorL.SM_PickupDoorL"):TEXT("/Game/EchoCampaign/Vehicles/SM_PickupDoorR.SM_PickupDoorR")));Door->SetRelativeLocation(FVector(94,I==0?-88:88,100));Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
 A->Tags.Add(TEXT("EchoCinematicPickup"));return A;
}
static void PickupDoors(AActor* Car,float Open)
{
 if(!Car)return;TArray<UStaticMeshComponent*> Parts;Car->GetComponents(Parts);for(auto P:Parts){if(P->GetFName()==TEXT("PickupDoorL"))P->SetRelativeRotation(FRotator(0,74*Open,0));else if(P->GetFName()==TEXT("PickupDoorR"))P->SetRelativeRotation(FRotator(0,-74*Open,0));}
}
// Curves follow the reach/pull/release beats sampled from our retargeted clips.
// The hand correction and the hinged mesh consume the same normalized phase.
static float EntryDoor(float T){return FMath::SmoothStep(.8f,1.6f,T)*(1-FMath::SmoothStep(3.3f,4.5f,T));}
static float ExitDoor(float T){return FMath::SmoothStep(.45f,1.45f,T)*(1-FMath::SmoothStep(3.5f,4.65f,T));}
static void DoorHand(UPoseableMeshComponent* Person,AActor* Car,bool Passenger,float Open,float T,bool Entering)
{
 float A=Entering?.65f:.32f,B=Entering?1.75f:1.65f,C=Entering?3.15f:3.35f,D=Entering?4.6f:4.8f;
 auto Envelope=[T](float Start,float End){return FMath::SmoothStep(Start,Start+.18f,T)*(1-FMath::SmoothStep(End-.18f,End,T));};
 float Weight=FMath::Max(Envelope(A,B),Envelope(C,D));if(Weight<=0)return;
 float Side=Passenger?1.f:-1.f;FVector Hinge(94,Side*88,100);
 FVector Handle=Hinge+FRotator(0,-Side*74*Open,0).RotateVector(FVector(-95,Side*2,15));
 ReachHand(Person,true,FMath::Lerp(Person->GetBoneLocationByName(TEXT("Hand_L"),EBoneSpaces::WorldSpace),Car->GetActorTransform().TransformPosition(Handle),Weight));
 EchoPose::SoftenWrists(Person);
}
#include "EchoPoliceProps.inl"

AEchoStoryScene::AEchoStoryScene(){PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));}
UPoseableMeshComponent* AEchoStoryScene::Person(const TCHAR* Path,FVector P,float Yaw)
{
 auto A=GetWorld()->SpawnActor<AActor>();Actors.Add(A);
 auto M=NewObject<UPoseableMeshComponent>(A);A->SetRootComponent(M);M->RegisterComponent();M->SetSkinnedAssetAndUpdate(LoadObject<USkeletalMesh>(nullptr,Path));M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 A->SetActorLocationAndRotation(P,FRotator(0,Yaw-90,0));People.Add(M);
 auto Sampler=NewObject<USkeletalMeshComponent>(A);Sampler->SetupAttachment(M);Sampler->RegisterComponent();Sampler->SetSkeletalMesh(Cast<USkeletalMesh>(M->GetSkinnedAsset()));Sampler->SetVisibility(false);Sampler->SetCastShadow(false);Sampler->SetHiddenInGame(true);Sampler->SetCollisionEnabled(ECollisionEnabled::NoCollision);Sampler->SetComponentTickEnabled(false);Sampler->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;MotionSources.Add(M,Sampler);
 return M;
}
void AEchoStoryScene::Pose(UPoseableMeshComponent* M,int32 G,float T,float ClipTime)
{
 if(!M || !M->GetSkinnedAsset())return;
 bool Changed=!PreviousGestures.Contains(M)||PreviousGestures[M]!=G;
 if(Changed){bool First=!PreviousGestures.Contains(M);int Previous=PreviousGestures.FindRef(M);auto& Snapshot=PreviousPoses.FindOrAdd(M);Snapshot.Empty();if(!First)for(int I=0;I<M->GetNumBones();++I)Snapshot.Add(M->GetBoneTransformByName(M->GetBoneName(I),EBoneSpaces::ComponentSpace));
  if((Previous==13 || Previous==14) && G!=13 && G!=14 && Snapshot.Num()){
   // Exit clips carry root translation. The walking trajectory already starts
   // outside the door; retaining that translation blends the passenger through
   // the car a second time when her mirrored scale returns to normal.
   int RootIndex=M->GetBoneIndex(TEXT("Root"));if(Snapshot.IsValidIndex(RootIndex)){FVector Offset=Snapshot[RootIndex].GetLocation();Offset.Z=0;for(auto& Bone:Snapshot)Bone.AddToTranslation(-Offset);}
  }
  PreviousGestures.Add(M,G);GestureChangedAt.Add(M,T);}
 float Elapsed=FMath::Max(0.f,T-GestureChangedAt.FindRef(M));
 if(Opening && G==13)Elapsed=FMath::Max(0.f,T-13.f);
 else if(Arrival && !Opening && G==14)Elapsed=FMath::Max(0.f,T);
 if(ClipTime>=0)Elapsed=ClipTime;
 auto Mesh=Cast<USkeletalMesh>(M->GetSkinnedAsset());bool Female=Mesh->GetName().Contains(TEXT("Mother"));bool City=Mesh->GetSkeleton()->GetName().Contains(TEXT("City"));
 const TCHAR* Performance=nullptr;
 if(G==2)Performance=TEXT("Cheering");else if(G==3)Performance=TEXT("Defeated");else if(G==4)Performance=TEXT("Laughing");else if(G==6)Performance=TEXT("Talking");else if(G==7)Performance=TEXT("Clapping");else if(G==8)Performance=TEXT("Opening_Door_Inwards");else if(G==9)Performance=TEXT("Standing_Fist_Pump");
 else if(G==13)Performance=TEXT("Entering_Car");else if(G==14)Performance=TEXT("Exiting_Car");else if(G==15)Performance=TEXT("Driving");
 else if(G==11)Performance=Female?TEXT("Female_Start_Walking"):TEXT("Start_Walking");else if(G==12)Performance=Female?TEXT("Female_Stop_Walking"):TEXT("Stop_Walking");
 else if(G==1)Performance=!Arrival?nullptr:!Opening && Ending<=1?TEXT("Sad_Walk"):TEXT("Happy_Walk");
 FString Path=Performance?FString::Printf(TEXT("/Game/EchoCampaign/Animation/Mixamo%s/MX_%s.MX_%s"),City?TEXT("City"):TEXT("Town"),Performance,Performance):FString::Printf(TEXT("/Game/EchoCampaign/Animation/%s/%s.%s"),City?TEXT("City"):TEXT("Town"),G==1?(Female && !City?TEXT("MF_Walk_Fwd"):TEXT("MM_Walk_InPlace")):(Female && !City?TEXT("MF_Idle"):TEXT("MM_Idle")),G==1?(Female && !City?TEXT("MF_Walk_Fwd"):TEXT("MM_Walk_InPlace")):(Female && !City?TEXT("MF_Idle"):TEXT("MM_Idle")));
 FName Key(*Path);UAnimSequence* Clip=PerformanceClips.FindRef(Key);if(!Clip){Clip=LoadObject<UAnimSequence>(nullptr,*Path);PerformanceClips.Add(Key,Clip);}
 if(auto Sampler=MotionSources.FindRef(M))if(Clip){
  Sampler->SetAnimationMode(EAnimationMode::AnimationSingleNode);Sampler->SetAnimation(Clip);
  bool Once=G==3 || G==8 || G==9 || G==11 || G==12 || G==13 || G==14;
  float Offset=G==1 && !Arrival?FMath::Max(0,People.IndexOfByKey(M))*.18f:0.f;
  float Time=Once?FMath::Min(Elapsed,Clip->GetPlayLength()-.01f):FMath::Fmod(Elapsed+Offset,FMath::Max(.01f,Clip->GetPlayLength()));
  Sampler->SetPosition(Time,false);Sampler->TickAnimation(0,false);Sampler->RefreshBoneTransforms();M->CopyPoseFromSkeletalComponent(Sampler);
 }
 // Trajectory is authored by the scene. Keep the animated body's vertical weight shift.
 if(G!=13 && G!=14 && M->GetBoneIndex(TEXT("Root"))!=INDEX_NONE){auto Root=M->GetBoneTransformByName(TEXT("Root"),EBoneSpaces::ComponentSpace);auto P=Root.GetLocation();P.X=0;P.Y=0;Root.SetLocation(P);M->SetBoneTransformByName(TEXT("Root"),Root,EBoneSpaces::ComponentSpace);}
 float Blend=FMath::SmoothStep(0.f,1.f,FMath::Clamp(Elapsed/.32f,0.f,1.f));auto& Snapshot=PreviousPoses.FindOrAdd(M);
 if(Blend<1 && Snapshot.Num()==M->GetNumBones()){
  TArray<FTransform> Current,Blended;for(int I=0;I<M->GetNumBones();++I)Current.Add(M->GetBoneTransformByName(M->GetBoneName(I),EBoneSpaces::ComponentSpace));Blended.SetNum(Current.Num());
  const auto& Ref=Mesh->GetRefSkeleton();
  // Blend in parent space. Interpolating independent component-space joint
  // positions shortens the arms during the exit-to-walk transition.
  for(int I=0;I<Current.Num();++I){int32 Parent=Ref.GetParentIndex(I);FTransform A=Parent==INDEX_NONE?Snapshot[I]:Snapshot[I].GetRelativeTransform(Snapshot[Parent]);FTransform B=Parent==INDEX_NONE?Current[I]:Current[I].GetRelativeTransform(Current[Parent]);FTransform Local;Local.Blend(A,B,Blend);Blended[I]=Parent==INDEX_NONE?Local:Local*Blended[Parent];M->SetBoneTransformByName(M->GetBoneName(I),Blended[I],EBoneSpaces::ComponentSpace);}
 }
 EchoPose::SoftenWrists(M);
}
void AEchoStoryScene::Start(AEchoCampaignDirector* D,bool IsArrival)
{
 Director=D;Arrival=IsArrival;Clock=0;ShotInitialized=false;
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());Ending=FEchoJourneyRules::Ending(GI?GI->Mood:60);
 Forward=FRotator(0,D->PlayerImpact.Rotator().Yaw,0).Vector();Right=FVector::CrossProduct(FVector::UpVector,Forward);
 Origin=D->Center;Origin.Z=0;
 Camera=GetWorld()->SpawnActor<ACameraActor>();Actors.Add(Camera);UpdateFraming();auto& PP=Camera->GetCameraComponent()->PostProcessSettings;PP.bOverride_BloomIntensity=true;PP.BloomIntensity=.12f;PP.bOverride_MotionBlurAmount=true;PP.MotionBlurAmount=0;GetWorld()->GetFirstPlayerController()->SetViewTargetWithBlend(Camera,.5);
 const TCHAR* Father=TEXT("/Game/PolygonTown/Meshes/Characters/SK_Character_Father_01.SK_Character_Father_01");
 const TCHAR* Mother=TEXT("/Game/PolygonTown/Meshes/Characters/SK_Character_Mother_01.SK_Character_Mother_01");
 if(Arrival){
  // Isolated exterior set uses the same low-poly asset collection as the playable city.
  Origin=FVector(80000,80000,30);Forward=FVector(1,0,0);Right=FVector(0,1,0);
  // The road tiles lie at Origin.Z. Recess the backing terrain below their
  // undersides: coplanar grass/road surfaces otherwise flicker during camera moves.
  auto Floor=GetWorld()->SpawnActor<AActor>();Actors.Add(Floor);auto C=NewObject<UStaticMeshComponent>(Floor,TEXT("ExteriorBackingGround"));Floor->SetRootComponent(C);C->RegisterComponent();C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetWorldLocation(Origin-FVector(0,0,33));C->SetWorldScale3D(FVector(90,90,.5));C->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/EchoCampaign/Art/M_Garden.M_Garden")));
  auto Prop=[&](const TCHAR* Path,FVector P,FVector Scale,FRotator R=FRotator::ZeroRotator,const TCHAR* Mat=nullptr){auto A=GetWorld()->SpawnActor<AActor>();Actors.Add(A);auto M=NewObject<UStaticMeshComponent>(A);A->SetRootComponent(M);M->SetMobility(EComponentMobility::Movable);M->RegisterComponent();M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Path));M->SetCollisionEnabled(ECollisionEnabled::QueryOnly);M->SetCollisionResponseToAllChannels(ECR_Block);if(Mat)M->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Mat));A->SetActorLocationAndRotation(Origin+P,R);A->SetActorScale3D(Scale);if(FString(Path).Contains(TEXT("Shop_01_Door_L")) || FString(Path).Contains(TEXT("Shop_01_Door_R"))){EntryDoors.Add(A);EntryDoorRotations.Add(R);}return A;};
  const TCHAR* Cube=TEXT("/Engine/BasicShapes/Cube.Cube"),*Cream=TEXT("/Game/EchoCampaign/Art/M_Exterior.M_Exterior"),*Roof=TEXT("/Game/EchoCampaign/Art/M_Roof.M_Roof"),*Glass=TEXT("/Game/EchoCampaign/Art/M_Window.M_Window");
  if(Ending==0){
#include "EchoHomeSet.inl"
  // Keep the authored front-door path clear in both the opening and return-home shot.
  for(auto A:Actors)if(auto Mesh=A->FindComponentByClass<UStaticMeshComponent>())if(Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetName().Contains(TEXT("Bush"))){FVector P=A->GetActorLocation()-Origin;if(P.X>-450 && P.X<400 && FMath::Abs(P.Y)<120)A->AddActorWorldOffset(Right*340);}
  Prop(Cube,FVector(65,0,2),FVector(7.4f,1.45f,.04f),FRotator::ZeroRotator,Cream);
  }else{
#include "EchoRestaurantSet.inl"
  }
  auto Sign=GetWorld()->SpawnActor<AActor>();Actors.Add(Sign);auto Text=NewObject<UTextRenderComponent>(Sign);Sign->SetRootComponent(Text);Text->RegisterComponent();Text->SetText(FText::FromString(Ending==0?TEXT(""):TEXT("WEEKEND / TABLE FOR TWO")));Text->SetWorldSize(Ending==0?24:18);Text->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);Text->SetTextRenderColor(FColor(245,211,140));Sign->SetActorLocationAndRotation(Origin+FVector(425,0,Ending==0?280:238),FRotator(0,180,0));
  Person(Father,Origin-Forward*250-Right*35,0);Person(Mother,Origin-Forward*250+Right*35,0);
  OpeningCar=CinematicPickup(GetWorld(),Origin+FVector(-630,0,2));Actors.Add(OpeningCar);

 }else{
  // Stage the conversation behind the pickup, on its clear inboard side.
  // The old fixed roadside offset sent police through street furniture after
  // the right-hand-traffic migration, and left speakers across the wreck.
  Origin=D->PlayerImpact.GetLocation();Origin.Z=0;
  // The investigation tableau is a suspended instant. Never reuse its floating
  // occupants, collision particles or pitched vehicles in the real-world aftermath.
  for(auto A:D->Tableau)if(A){
   A->SetActorHiddenInGame(true);
   if(!A->ActorHasTag(TEXT("EchoPlayerCar")) && !A->ActorHasTag(TEXT("EchoOpponentCar")))continue;
   auto Source=A->FindComponentByClass<USkeletalMeshComponent>();if(!Source)continue;
   auto Car=GetWorld()->SpawnActor<AActor>();Actors.Add(Car);Car->Tags.Add(TEXT("EchoAftermathVehicle"));
   auto Mesh=NewObject<UPoseableMeshComponent>(Car);Car->SetRootComponent(Mesh);Mesh->RegisterComponent();Mesh->SetSkinnedAssetAndUpdate(Source->GetSkeletalMeshAsset());Mesh->CopyPoseFromSkeletalComponent(Source);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   for(int I=0;I<Source->GetNumMaterials();++I)Mesh->SetMaterial(I,Source->GetMaterial(I));
   FVector Position=A->GetActorLocation();Position.Z=0;
   Car->SetActorScale3D(A->GetActorScale3D());Car->SetActorLocationAndRotation(Position,FRotator(0,A->GetActorRotation().Yaw,0));
   // Ground by the mesh's lowest point (tyres), rather than the raised crash pivot.
   auto Bounds=Mesh->CalcBounds(Mesh->GetComponentTransform());Position.Z+=2.f-(Bounds.Origin.Z-Bounds.BoxExtent.Z);Car->SetActorLocation(Position);
  }
  Person(Father,Origin-Forward*300-Right*350,Forward.Rotation().Yaw+180);CastSources.Add(nullptr);
  for(auto& Clue:D->Clues)if(Clue.Rescue && Clue.Complete && Clue.SourceActor && Clue.SourceActor->ActorHasTag(TEXT("EchoCulprit"))){if(auto C=Clue.SourceActor->FindComponentByClass<USkeletalMeshComponent>()){Person(*C->GetSkeletalMeshAsset()->GetPathName(),Origin-Forward*600-Right*(350+(People.Num()-1)*120),Forward.Rotation().Yaw);CastSources.Add(Clue.SourceActor);}}
  if(D->Arrest){
   Person(TEXT("/Game/PolygonCity/Meshes/Characters/SK_Character_Male_Police.SK_Character_Male_Police"),Origin-Forward*1030-Right*230,Forward.Rotation().Yaw);
   PoliceCar=CinematicPolice(GetWorld());Actors.Add(PoliceCar);
   for(int I=1;I<People.Num()-1;++I){auto C=MakeCuffs(GetWorld());Cuffs.Add(C);Actors.Add(C);}

   PoliceCar->SetActorHiddenInGame(true);
  }
  for(int I=0;I<People.Num();++I){bool Driver=D->Arrest && I==People.Num()-1;Pose(People[I],Driver?15:0,0,Driver?1.f:-1.f);if(!Driver)GroundPerformance(People[I],People[I]->GetOwner()->GetActorLocation(),0);People[I]->GetOwner()->SetActorHiddenInGame(Driver);}
 }
 if(Arrival)Shot(OpeningCar->GetActorLocation()+FVector(370,-460,230),OpeningCar->GetActorLocation()+FVector(0,0,115),0);
 else Shot(Origin-Forward*850-Right*650+FVector(0,0,380),Origin+FVector(0,0,100),0);
}
void AEchoStoryScene::StartOpening(AEchoCampaignDirector* D)
{
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());int Mood=GI?GI->Mood:60;if(GI)GI->Mood=0;
 Start(D,true);if(GI)GI->Mood=Mood;Opening=true;
 for(auto A:Actors)if(A && A->ActorHasTag(TEXT("EchoCinematicPickup")))A->SetActorHiddenInGame(true);Ending=3;
 OpeningCar=CinematicPickup(GetWorld(),Origin+FVector(-470,-220,2));Actors.Add(OpeningCar);
 for(auto PersonMesh:People){PersonMesh->GetOwner()->SetActorRotation(FRotator(0,90,0));Pose(PersonMesh,0,0);}
 // Establish the same side of the set as the opening dolly, under the fade.
 // Crossing from the opposite side used to create an unintended opening whip.
 ShotInitialized=false;Shot(Origin+FVector(-960,-1050,310),Origin+FVector(405,60,145),0);
 GetWorld()->GetFirstPlayerController()->SetViewTarget(Camera);
 GetWorld()->GetFirstPlayerController()->PlayerCameraManager->StartCameraFade(1,0,.8f,FLinearColor::Black,false,false);
}
void AEchoStoryScene::Shot(FVector P,FVector Target,float Dt)
{
 if(!Camera)return;
 if(!ShotInitialized){ShotPosition=P;ShotTarget=Target;ShotPositionVelocity=ShotTargetVelocity=FVector::ZeroVector;ShotInitialized=true;}
 else {
  // Analytic critically damped response: preserve velocity through shot changes,
  // smooth BOTH the dolly and the look-at point, without frame-dependent lerps.
  // A long loading frame must not skip most of the camera transition at once.
  const float Step=FMath::Clamp(Dt,0.f,.1f);
  auto Damp=[Step](FVector& Value,FVector& Velocity,const FVector& Goal,float Omega){
   const FVector Error=Value-Goal,Term=(Velocity+Error*Omega)*Step;
   const float Decay=FMath::Exp(-Omega*Step);
   Value=Goal+(Error+Term)*Decay;Velocity=(Velocity-Term*Omega)*Decay;
  };
  Damp(ShotPosition,ShotPositionVelocity,P,5.f);Damp(ShotTarget,ShotTargetVelocity,Target,5.f);
 }
 FVector Position=ShotPosition;Target=ShotTarget;
 if(!Arrival){FHitResult Hit;FCollisionQueryParams Query;Query.bTraceComplex=true;Query.AddIgnoredActors(Actors);if(Director)Query.AddIgnoredActors(Director->Tableau);
  if(GetWorld()->LineTraceSingleByChannel(Hit,Target,Position,ECC_Visibility,Query) && !Hit.bStartPenetrating)Position=Hit.ImpactPoint+Hit.ImpactNormal*35;}
 Camera->SetActorLocation(Position);Camera->SetActorRotation((Target-Position).Rotation());
}
void AEchoStoryScene::UpdateFraming()
{
 if(!Camera)return;
 auto C=Camera->GetCameraComponent();
 C->SetConstraintAspectRatio(false);
 C->bOverrideAspectRatioAxisConstraint=true;
 C->SetAspectRatioAxisConstraint(AspectRatio_MaintainXFOV);
 int32 W=0,H=0;if(auto PC=GetWorld()->GetFirstPlayerController())PC->GetViewportSize(W,H);
 // Preserve the entire authored 16:9 composition: reveal sides on wide screens,
 // reveal height on narrow screens. Never stretch the world or add pillarboxing.
 float Aspect=H>0?float(W)/H:16.f/9.f;
 float FOV=FMath::RadiansToDegrees(2*FMath::Atan(FMath::Tan(FMath::DegreesToRadians(24.f))*FMath::Max(1.f,Aspect/(16.f/9.f))));
 C->SetFieldOfView(FOV);
}
void AEchoStoryScene::Tick(float Dt)
{
 Super::Tick(Dt);UpdateFraming();if(!Director || Director->Paused)return;Clock+=Dt;PerformanceClock+=Dt;
 ON_SCOPE_EXIT {if(IsValid(this) && IsValid(Director))TickSceneAudio(Dt);};
 if(!Opening && !Arrival && !RealityRevealed && Director->DialogueID==TEXT("reality") && !Director->Notice.IsEmpty())Clock=FMath::Min(Clock,2.4f);
 if(Opening){
  float EntryPhase=FMath::Max(0.f,Clock-13);PickupDoors(OpeningCar,EntryDoor(EntryPhase));DoorAudio(TEXT("Pickup"),EntryDoor(EntryPhase),OpeningCar->GetActorLocation());
  // The imported entry clip starts facing the side door, then turns into the seat.
  float WalkT=FMath::Clamp((Clock-3.0f)/10.f,0.f,1.f);
  FVector StableWalkTarget=Origin+FVector(405,32,145);
  for(int I=0;I<People.Num();++I){auto M=People[I];FVector Entry=OpeningCar->GetActorLocation()+FVector(12,I==0?-202:202,0);
   FVector Start=Origin+FVector(405,60+(I==0?-28:28),45);
   FVector B=Origin+FVector(60,Start.Y-Origin.Y,0),C=Origin+FVector(-20,Entry.Y-Origin.Y,0);float DeltaY=Entry.Y-Start.Y;
   FVector Tangent(-200,DeltaY*.3f,0),P,Direction;
   if(WalkT<.32f){float U=WalkT/.32f;P=FMath::CubicInterp(Start,B-Start,B,Tangent,U);Direction=FMath::CubicInterpDerivative(Start,B-Start,B,Tangent,U);}
   else if(WalkT<.7f){float U=(WalkT-.32f)/.38f;P=FMath::CubicInterp(B,Tangent,C,Tangent,U);Direction=FMath::CubicInterpDerivative(B,Tangent,C,Tangent,U);}
   else {float U=(WalkT-.7f)/.3f;P=FMath::CubicInterp(C,Tangent,Entry,Entry-C,U);Direction=FMath::CubicInterpDerivative(C,Tangent,Entry,Entry-C,U);}
   if(I==0)StableWalkTarget=P+FVector(0,0,100);
   if(Clock<13){M->GetOwner()->SetActorLocation(P);M->GetOwner()->SetActorRotation(FRotator(0,Direction.Rotation().Yaw-90,0));Pose(M,Clock<3?0:1,Clock);
    GroundPerformance(M,P,Origin.Z);
   }else{float E=Clock-13;Entry.Z=Origin.Z-1+35*FMath::SmoothStep(1.4f,3.4f,E);M->GetOwner()->SetActorLocation(Entry);M->GetOwner()->SetActorRotation(FRotator::ZeroRotator);SetPerformanceScale(M,FVector(1,I==0?1:-1,1));Pose(M,13,Clock);DoorHand(M,OpeningCar,I==1,EntryDoor(EntryPhase),EntryPhase,true);}
  }
  // Follow the authored path before animated foot contact moves the body.
  if(Clock<13)Shot(Origin+FVector(-960,-1050,310),StableWalkTarget,Dt);
  else Shot(OpeningCar->GetActorLocation()+FVector(370,-480,245),OpeningCar->GetActorLocation()+FVector(0,-65,125),Dt);
  if(Clock>.7 && DialogueBeat<0){DialogueBeat=0;Director->Say(TEXT("opening_wife"));}if(Clock>6.8 && DialogueBeat<1){DialogueBeat=1;Director->Say(TEXT("opening_hero"));}if(Clock>14 && DialogueBeat<2){DialogueBeat=2;Director->Say(TEXT("opening_wife_reply"));}
  if(Clock>19.2 && !FadingOut){FadingOut=true;GetWorld()->GetFirstPlayerController()->PlayerCameraManager->StartCameraFade(0,1,.8f,FLinearColor::Black,false,true);}
  if(Clock>20.2)Director->EndOpening();return;
 }
 if(Arrival){
  PickupDoors(OpeningCar,ExitDoor(Clock));DoorAudio(TEXT("Pickup"),ExitDoor(Clock),OpeningCar->GetActorLocation());
  float Walk=FMath::Clamp((Clock-12.f)/12.f,0.f,1.f);
  float Travel=(Walk<.04f?Walk*Walk/.08f:Walk>.96f?.96f-FMath::Square(1-Walk)/.08f:Walk-.02f)/.96f;
  float DoorCenter=Ending==0?60.f:0.f;
  FVector P=Origin+Forward*FMath::Lerp(-250.f,Ending==0?390.f:735.f,Travel)+Right*DoorCenter;
  float Step=FMath::SmoothStep(0.f,1.f,FMath::Clamp(float(P.X-Origin.X-300)/120.f,0.f,1.f));P.Z+=Step*(Ending==0?45.f:23.f);
  for(int I=0;I<EntryDoors.Num();++I){float Open=FMath::SmoothStep(18.f,19.5f,Clock);EntryDoors[I]->SetActorRotation(EntryDoorRotations[I]+FRotator(0,(I==0?-78:78)*Open,0));if(I==0)DoorAudio(TEXT("Building"),Open,EntryDoors[I]->GetActorLocation());}
  for(int I=0;I<People.Num();++I){auto M=People[I];float Side=I==0?-1.f:1.f;FVector Feet=P+Right*(Side*35);
   if(Clock<5.8f){
    float Lift=35*(1-FMath::SmoothStep(.9f,2.7f,Clock));M->GetOwner()->SetActorLocation(OpeningCar->GetActorLocation()+FVector(0,Side*44,Lift-2));M->GetOwner()->SetActorRotation(FRotator(0,-90,0));SetPerformanceScale(M,FVector(I==0?1:-1,1,1));Pose(M,14,Clock);DoorHand(M,OpeningCar,I==1,ExitDoor(Clock),Clock,false);
   }else{
    SetPerformanceScale(M,FVector::OneVector);FRotator Facing(0,-90,0);
    if(Clock<11.5f){float U=FMath::Clamp((Clock-5.8f)/5.7f,0.f,1.f),V=1-U;FVector A=Origin+FVector(-613,Side*208,0),B=Origin+FVector(-480,Side*230,0),C=Origin+FVector(-270,DoorCenter+Side*230,0),E=Origin+FVector(-250,DoorCenter+Side*35,0);Feet=V*V*V*A+3*V*V*U*B+3*V*U*U*C+U*U*U*E;FVector Tangent=3*V*V*(B-A)+6*V*U*(C-B)+3*U*U*(E-C);Facing.Yaw=Tangent.Rotation().Yaw-90;}
    else if(Clock<12)Facing.Yaw=FMath::Lerp(Side<0?0.f:-180.f,-90.f,FMath::SmoothStep(11.5f,12.f,Clock));
    M->GetOwner()->SetActorLocation(Feet);M->GetOwner()->SetActorRotation(Facing);Pose(M,Clock<11.5f?1:Clock<12?0:Clock<22.5f?1:Clock<24.2f?12:0,Clock);
    GroundPerformance(M,Feet,Origin.Z);
   }
  }
  if(People.Num()==2 && Clock>12){float Hold=FMath::SmoothStep(12.f,13.3f,Clock);FVector Target=P+Forward*10+FVector(0,0,74+FMath::Sin(Clock*2)*.8f);ReachHand(People[0],false,FMath::Lerp(People[0]->GetBoneLocationByName(TEXT("Hand_R"),EBoneSpaces::WorldSpace),Target-Right*3,Hold));ReachHand(People[1],true,FMath::Lerp(People[1]->GetBoneLocationByName(TEXT("Hand_L"),EBoneSpaces::WorldSpace),Target+Right*3,Hold));EchoPose::RestingPalm(People[0],false,Forward,Hold);EchoPose::RestingPalm(People[1],true,Forward,Hold);}
  if(Clock<6)Shot(OpeningCar->GetActorLocation()+FVector(370,-460,230),OpeningCar->GetActorLocation()+FVector(0,0,115),Dt);
  else if(Clock<12)Shot(Origin+FVector(-60,-580,240),Origin+FVector(-400,0,110),Dt);
  // Keep the exit and walking dolly on the same side. Crossing through the
  // look-at point at 12s caused a fast overhead pivot even with smooth positions.
  else {FVector Follow=P;Follow.X=FMath::Min(Follow.X,Origin.X+120);float Blend=FMath::SmoothStep(12.f,14.f,Clock);
   Shot(FMath::Lerp(Origin+FVector(-60,-580,240),Follow-Forward*500-Right*420+FVector(0,0,240),Blend),FMath::Lerp(Origin+FVector(-400,0,110),P+Forward*70+FVector(0,0,105),Blend),Dt);}
  if(Clock>25.4f && !FadingOut){FadingOut=true;GetWorld()->GetFirstPlayerController()->PlayerCameraManager->StartCameraFade(0,1,1.2f,FLinearColor::Black,false,true);}
  if(Clock>27){GetWorld()->GetFirstPlayerController()->PlayerCameraManager->StopCameraFade();Director->Phase=EEchoPhase::Finale;Director->PhaseTime=0;Director->SetCursor(true);Director->Say(TEXT("ending_guide"));SetActorTickEnabled(false);}return;
 }

 if(Director->Arrest){TickPoliceAftermath(Dt);return;}

 // Unresolved outcomes retain their quiet, grounded conversation.
 float T=Clock-2.5f;FVector HeroPosition=Origin-Forward*300-Right*350,Group=Origin-Forward*580-Right*400;
 auto Speaking=[&](int I){if(Director->Notice.IsEmpty())return false;FName ID=Director->DialogueID;if(I==0)return ID.ToString().StartsWith(TEXT("hero_"));bool Boy=People[I]->GetSkinnedAsset()->GetName().Contains(TEXT("SchoolBoy"));return Boy?(ID==TEXT("student_protest") || ID==TEXT("culprit_taunt")):(ID==TEXT("student_regret") || ID==TEXT("driver_remorse") || ID==TEXT("trucker_remorse"));};
 if(T<0){for(auto M:People){Pose(M,0,PerformanceClock);GroundPerformance(M,M->GetOwner()->GetActorLocation(),0);}Shot(Origin-Forward*850-Right*650+FVector(0,0,360),Origin+Forward*100+FVector(0,0,90),Dt);return;}
 if(DialogueBeat<0){DialogueBeat=0;bool BoyPresent=false;for(auto Source:CastSources)if(Source)for(const auto& Clue:Director->Clues)if(Clue.SourceActor==Source && Clue.Title.Contains(TEXT("schoolboy")))BoyPresent=true;
  Director->Conversation(People.Num()>1?(BoyPresent?TArray<FName>{TEXT("unresolved"),TEXT("culprit_taunt"),FName(Director->RescueCount()==Director->RescueTotal()?TEXT("hero_partial"):TEXT("hero_loss"))}:TArray<FName>{TEXT("unresolved"),FName(Director->RescueCount()==Director->RescueTotal()?TEXT("hero_partial"):TEXT("hero_loss"))}):TArray<FName>{TEXT("unresolved_death"),FName(Director->RescueCount()==Director->RescueTotal()?TEXT("hero_partial"):TEXT("hero_loss"))});
 }
 for(int I=0;I<People.Num();++I){auto M=People[I];bool Boy=M->GetSkinnedAsset()->GetName().Contains(TEXT("SchoolBoy"));Pose(M,Speaking(I)?(Boy?4:6):3,PerformanceClock);GroundPerformance(M,M->GetOwner()->GetActorLocation(),0);}
 Shot(Origin-Forward*850-Right*740+FVector(0,0,240),(People.Num()>1?Group:HeroPosition)+FVector(0,0,105),Dt);
 if(T>12 && Director->Notice.IsEmpty()){Director->BeginDeparture();SetActorTickEnabled(false);}
}

void AEchoStoryScene::EndPlay(const EEndPlayReason::Type R){if(SceneEngine){SceneEngine->Stop();SceneEngine->DestroyComponent();}if(Siren)Siren->Stop();for(auto A:Actors)if(IsValid(A))A->Destroy();Super::EndPlay(R);}

void AEchoStoryScene::PreviewAt(float Seconds){SceneAudioPreview=true;Clock=FMath::Clamp(Seconds,0.f,Arrival?26.9f:Director && Director->Arrest?90.f:14.4f)-.1f;PerformanceClock=Clock;Tick(.1f);SetActorTickEnabled(false);}



#include "EchoPoliceScene.inl"
