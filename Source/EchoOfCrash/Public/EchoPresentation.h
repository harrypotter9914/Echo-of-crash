#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EchoPresentation.generated.h"

UCLASS(Blueprintable)
class ECHOOFCRASH_API AEchoEvidenceProp : public AActor
{
 GENERATED_BODY()
public:
 AEchoEvidenceProp();
 virtual void OnConstruction(const FTransform& Transform) override;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Framed = true;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Picture;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Back;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Edges;
};

UCLASS()
class ECHOOFCRASH_API AEchoPreviewRig : public AActor
{
 GENERATED_BODY()
public:
 AEchoPreviewRig();
 void Build(AActor* Source,bool Portrait);
 void Rotate(float X,float Y);
 void Zoom(float Delta);
 void Capture();
 void ResetView();
 FTransform InitialModel,InitialCamera;
 UPROPERTY() TObjectPtr<class UTextureRenderTarget2D> Texture;
 UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> DisplayMaterial;
 UPROPERTY() TObjectPtr<class USceneCaptureComponent2D> Camera;
 UPROPERTY() TObjectPtr<class USceneComponent> Model;
 float Distance = 140;
};

/** In-world, timed cinematic; Blueprint subclasses can replace the choreography. */
UCLASS(Blueprintable)
class ECHOOFCRASH_API AEchoStoryScene : public AActor
{
 GENERATED_BODY()
public:
 AEchoStoryScene();
 void Start(class AEchoCampaignDirector* OwnerDirector,bool IsArrival);
 void StartOpening(class AEchoCampaignDirector* OwnerDirector);
 bool Opening=false;
 int32 DialogueBeat=-1;
 UPROPERTY() TObjectPtr<AActor> OpeningCar;
 UPROPERTY() TArray<TObjectPtr<AActor>> CastSources;
 UFUNCTION(BlueprintCallable) void PreviewAt(float Seconds);
 UPROPERTY() TObjectPtr<AActor> Door;
 UPROPERTY() TArray<TObjectPtr<AActor>> EntryDoors;
 TArray<FRotator> EntryDoorRotations;
 bool FadingOut=false;
 bool RealityRevealed=false;
 virtual void Tick(float Dt) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 UPROPERTY() TObjectPtr<class AEchoCampaignDirector> Director;
 UPROPERTY() TObjectPtr<class ACameraActor> Camera;
 UPROPERTY() TArray<TObjectPtr<AActor>> Actors;
 UPROPERTY() TArray<TObjectPtr<class UPoseableMeshComponent>> People;
 UPROPERTY() TMap<TObjectPtr<class UPoseableMeshComponent>,TObjectPtr<class USkeletalMeshComponent>> MotionSources;
 UPROPERTY() TArray<TObjectPtr<class UAnimSequence>> MotionClips;
 UPROPERTY() TMap<FName,TObjectPtr<class UAnimSequence>> PerformanceClips;
 TMap<class UPoseableMeshComponent*,TArray<FTransform>> PreviousPoses;
 TMap<class UPoseableMeshComponent*,int32> PreviousGestures;
 TMap<class UPoseableMeshComponent*,float> GestureChangedAt;
 UPROPERTY() TObjectPtr<AActor> PoliceCar;
 UPROPERTY() TArray<TObjectPtr<AActor>> Cuffs;
 UPROPERTY(BlueprintReadOnly) FName AftermathBeat;
 UPROPERTY(BlueprintReadOnly) float AftermathTime=0;
 UPROPERTY(BlueprintReadOnly) int32 CuffedCount=0;
 UPROPERTY(BlueprintReadOnly) bool CelebrationStarted=false;
 void TickPoliceAftermath(float Dt);
 void TickSceneAudio(float Dt);
 void DoorAudio(FName ID,float Open,FVector Position);
 TMap<FName,float> DoorAudioState;
 TMap<UPoseableMeshComponent*,FVector> FootAudioPosition;
 TMap<UPoseableMeshComponent*,float> FootAudioDistance;
 int FootAudioIndex=0,AudioCuffedCount=0;
 bool SceneAudioPreview=false,SceneEngineStarted=false,SceneEngineStopped=false;
 UPROPERTY() TObjectPtr<class UAudioComponent> SceneEngine;
 UPROPERTY() TObjectPtr<class UAudioComponent> Siren;
 FVector Origin,Forward,Right;
 float Clock=0;
 float PerformanceClock=0;
 // Camera rig state is independent of pose grounding and obstruction correction.
 FVector ShotPosition=FVector::ZeroVector,ShotTarget=FVector::ZeroVector;
 FVector ShotPositionVelocity=FVector::ZeroVector,ShotTargetVelocity=FVector::ZeroVector;
 bool ShotInitialized=false;
 bool Arrival=false;
 int32 Ending=0;
 class UPoseableMeshComponent* Person(const TCHAR* Path,FVector P,float Yaw);
 UFUNCTION(BlueprintCallable, Category="Presentation|Preview")
 void Pose(class UPoseableMeshComponent* Mesh,int32 Gesture,float T,float ClipTime=-1);
 void Shot(FVector Position,FVector Target,float Dt);
 void UpdateFraming();
};
