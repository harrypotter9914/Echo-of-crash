#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/SaveGame.h"
#include "EchoJourneyRules.h"
#include "EchoCampaign.generated.h"

UENUM(BlueprintType)
enum class EEchoPhase : uint8 { Introduction, Driving, QTE, Investigation, Deduction, Results, Finale, Impact, Awakening, Avoidance, Outcome, Departure, Arrival, Title, Tutorial, Opening };

UCLASS()
class ECHOOFCRASH_API UEchoPreferences : public USaveGame
{
 GENERATED_BODY()
public:
 UPROPERTY() float VoiceVolume=1;
 UPROPERTY() float SoundVolume=.8f;
 UPROPERTY() float LookSensitivity=1;
 UPROPERTY() bool Subtitles=true;
};

UCLASS()
class ECHOOFCRASH_API UEchoCampaignSave : public USaveGame
{
 GENERATED_BODY()
public:
 UPROPERTY() int32 Chapter = 0;
 UPROPERTY() int32 Saved = 0;
 UPROPERTY() int32 Evidence = 0;
 UPROPERTY() int32 Deductions = 0;
 UPROPERTY() int32 Mood = 60;
 UPROPERTY() int32 TotalScore = 0;
 UPROPERTY() int32 Avoided = 0;
 UPROPERTY() EEchoBalance Balance = EEchoBalance::Journey;
};

USTRUCT(BlueprintType)
struct FEchoClue
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Title;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Description;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Position = FVector::ZeroVector;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Rescue = false;
 UPROPERTY(BlueprintReadOnly) bool Complete = false;
 UPROPERTY(BlueprintReadOnly) bool HintRevealed = false;
 float DissolveAge = -1;
 FVector OriginalPosition = FVector::ZeroVector;
 UPROPERTY() TArray<TObjectPtr<class UMaterialInstanceDynamic>> DissolveMaterials;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> SourceActor;
 UPROPERTY() TObjectPtr<class AEchoPreviewRig> Preview;
};

/** Blueprint subclasses can customize camera and ghost appearance. */
UCLASS(Blueprintable)
class ECHOOFCRASH_API AEchoGhostPawn : public APawn
{
 GENERATED_BODY()
public:
 AEchoGhostPawn();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UCameraComponent> Camera;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class USpringArmComponent> Boom;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class USkeletalMeshComponent> Spirit;
 UPROPERTY(BlueprintReadOnly) FVector SwimVelocity=FVector::ZeroVector;
 float SwimClock=0;
 float EmergenceApplied=0;
 UFUNCTION(BlueprintCallable) void ApplyLookInput(float X,float Y);
};

/** Campaign rules live in C++; editable clues/routes remain available to Blueprint. */
UCLASS(Blueprintable)
class ECHOOFCRASH_API AEchoCampaignDirector : public AActor
{
 GENERATED_BODY()
public:
 AEchoCampaignDirector();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 void TickSoundscape(float Dt);
 void TickInteractionAudio(float Dt);
 void SetSoundLoop(FName Name,float Gain,float Pitch=1.f);
 UPROPERTY(BlueprintReadOnly) TMap<FName,TObjectPtr<class UAudioComponent>> ActionAudio;
 int32 AudioRescueTarget=INDEX_NONE,AudioReachTarget=INDEX_NONE;
 float ReachSoundCooldown=0;
 void SoundEvent(FName Name,float Gain=.5f,FVector Location=FVector::ZeroVector,bool Spatial=false);
 UFUNCTION(BlueprintPure) FVector SpiritContactPoint() const;
 UFUNCTION(BlueprintCallable) void ReturnWithoutDeduction();
 UPROPERTY(BlueprintReadOnly) bool AwakeningDialoguePending=false;
 UPROPERTY(BlueprintReadOnly) TArray<FString> AudioEvents;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class UAudioComponent> MusicAudio;
 UPROPERTY() TObjectPtr<class UAudioComponent> PreviousMusicAudio;
 UPROPERTY() TMap<FName,TObjectPtr<class USoundBase>> AudioCache;
 TMap<TWeakObjectPtr<class UAudioComponent>,float> EffectVoices;
 FName MusicName;
 float MusicGain=0,MusicCrossfade=0,AudioDuck=1;
 int AudioPhase=-1,LastTimerSecond=-1;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign") int32 Chapter = 0;
 UPROPERTY(BlueprintReadOnly, Category="Campaign") EEchoPhase Phase = EEchoPhase::Introduction;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign") TArray<FEchoClue> Clues;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Driving") TArray<FVector> Route;
 UPROPERTY(BlueprintReadOnly) FVector Center;
 UPROPERTY(BlueprintReadOnly) float TimeLeft = 120;
 UPROPERTY(BlueprintReadOnly) float PhaseTime = 0;
 UPROPERTY(BlueprintReadOnly) float RescueProgress = 0;
 UPROPERTY(BlueprintReadOnly) float QTEValue = 0;
 UPROPERTY(BlueprintReadOnly) bool QTESuccess = false;
 UPROPERTY(BlueprintReadOnly) bool JournalOpen = false;
 UPROPERTY(BlueprintReadOnly) bool Paused = false;
 UPROPERTY(BlueprintReadOnly) bool CorrectDeduction = false;
 UPROPERTY(BlueprintReadOnly) int32 Focus = INDEX_NONE;
 UPROPERTY(BlueprintReadOnly) FString Notice;
 UPROPERTY(BlueprintReadOnly) FString Speaker;
 UPROPERTY(BlueprintReadOnly) FName DialogueID;
 UPROPERTY() TArray<FName> DialogueQueue;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class UAudioComponent> VoiceAudio;
 UPROPERTY(BlueprintReadOnly) int32 MenuPage=0;
 UPROPERTY(BlueprintReadOnly) int32 TutorialPage=0;
 UPROPERTY(BlueprintReadOnly) bool TutorialPracticeHit=false;
 UPROPERTY(BlueprintReadOnly) bool HasCheckpoint=false;
 bool LowTimeSpoken=false;
 UFUNCTION(BlueprintCallable) void MenuAction(FName Action);
 UFUNCTION(BlueprintCallable) void Say(FName ID);
 void Conversation(const TArray<FName>& IDs);
 void NextDialogue();
 void StopDialogue();
 void SetCursor(bool Visible);
 void SetPaused(bool Value);
 void BeginOpening();
 void EndOpening();
 void LoadPreferences();
 void SavePreferences();
 void TickVerification();
 void TickControlVerification();
 UPROPERTY() TArray<FString> NoticePages;
 UPROPERTY(BlueprintReadOnly) FString ChapterTitle;
 UPROPERTY(BlueprintReadOnly) FString Story;
 UPROPERTY(BlueprintReadOnly) TArray<FString> Answers;
 UFUNCTION(BlueprintCallable) void Advance();
 UFUNCTION(BlueprintCallable) void ResolveQTE(float Value);
 UFUNCTION(BlueprintCallable) void StartInvestigation();
 UFUNCTION(BlueprintCallable) bool Interact(int32 Index, float HeldSeconds);
 UFUNCTION(BlueprintCallable) void SubmitDeduction(int32 Answer);
 UFUNCTION(BlueprintCallable) void Retry();
 UFUNCTION(BlueprintCallable) void ContinueSavedJourney();
 UFUNCTION(BlueprintCallable) void SaveCheckpoint(int32 NextChapter);
 UFUNCTION(BlueprintPure) int32 EvidenceCount() const;
 UFUNCTION(BlueprintPure) int32 RescueCount() const;
 UFUNCTION(BlueprintPure) int32 EvidenceTotal() const;
 UFUNCTION(BlueprintPure) int32 RescueTotal() const;
 UFUNCTION(BlueprintPure) FString Hint() const;
 UFUNCTION(BlueprintPure) float InteractionReach(int32 Index) const;
 UFUNCTION(BlueprintPure) bool CanReachClue(int32 Index) const;
 UPROPERTY(BlueprintReadOnly) bool FocusReachable = false;
 float ScanTime = 0;
 UPROPERTY(BlueprintReadOnly) int32 HintTarget = INDEX_NONE;
 UPROPERTY(BlueprintReadOnly) int32 HintPenalty = 0;
 UPROPERTY(BlueprintReadOnly) int32 RawChapterScore = 0;
 UPROPERTY(BlueprintReadOnly) FVector InvestigationExtent = FVector(950,950,0);
 float HintCooldown = 0,BoundaryCooldown = 0;
 UPROPERTY() TObjectPtr<class AExponentialHeightFog> EchoFog;
 UPROPERTY() TArray<TObjectPtr<AActor>> SuppressedFog;
 UFUNCTION(BlueprintCallable) void RequestHint();
 void SetEchoAtmosphere(bool Enable);
 void BeginDissolve(int32 Index);
 void TickInvestigationEffects(float Dt);
 FVector ConstrainSpirit(FVector Position,FVector& Velocity);
 float Speed = 0;
 float Steering = 0;
 int32 RouteIndex = 1;
 int32 CorrectAnswer = 0;
 float NoticeTime = 0;
 float RescueCostFeedback = 0;
 float DepartureFadeAt = -1;
 // Incident-only streamed levels share the current city and never replace its camera.
 UPROPERTY() TArray<TObjectPtr<class ULevelStreaming>> PendingIncidents;
 bool SeamlessApproach = false;
 float ApproachWheelOffset = 0;
 UFUNCTION(BlueprintPure) FTransform JourneyRoadAt(float Distance) const;
 UFUNCTION(BlueprintPure) float JourneyRoadLength() const;
 void PreloadIncidents();
 float IdleTime = 0;
 float WheelAngle = 0;
 float FinishConfirmTime = 0;
 bool Scored = false;
 UPROPERTY() TObjectPtr<AActor> DriveCar;
 UPROPERTY() TObjectPtr<class ACameraActor> DriveCamera;
 UPROPERTY() TObjectPtr<class UPoseableMeshComponent> DriveMesh;
 UPROPERTY() TObjectPtr<class UAudioComponent> EngineAudio;
 UPROPERTY() TObjectPtr<class UAudioComponent> Atmosphere;
 UPROPERTY() TArray<TObjectPtr<AActor>> Tableau;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Driving") TArray<FVector> OpponentRoute;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Driving") float DriveDuration = 8.5f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Balance") FEchoJourneyRules Rules;
 UPROPERTY(BlueprintReadOnly) int32 ChapterScore = 0;
 UPROPERTY(BlueprintReadOnly) int32 ChapterStars = 0;
 UPROPERTY(BlueprintReadOnly) int32 MoodDelta = 0;
 UPROPERTY(BlueprintReadOnly) int32 MoodBefore = 60;
 UPROPERTY(BlueprintReadOnly) bool Arrest = false;
 UPROPERTY(BlueprintReadOnly) int32 InspectIndex = INDEX_NONE;
 UPROPERTY(BlueprintReadOnly) float DriveClock = 0;
 UPROPERTY(BlueprintReadOnly) bool QTEResolved = false;
 UPROPERTY() TObjectPtr<AActor> OpponentCar;
 UPROPERTY() TObjectPtr<class UPoseableMeshComponent> OpponentMesh;
 UPROPERTY() TObjectPtr<class UAudioComponent> OpponentAudio;
 UPROPERTY() TObjectPtr<class USplineComponent> PlayerSpline;
 UPROPERTY() TObjectPtr<class USplineComponent> OpponentSpline;
 UPROPERTY() TObjectPtr<class AEchoStoryScene> Cinema;
 FTransform PlayerImpact, OpponentImpact;
 FVector AvoidStart, AvoidForward, AvoidRight;
 float OpponentStartDistance = 0;
 float PlayerExitDistance = 0, OpponentExitDistance = 0;
 float InspectMouseX = 0, InspectMouseY = 0;
 void RollWheels(float Dt,float PlayerSpeed,float OtherSpeed);
 void UpdateCockpit(float Dt);
 UFUNCTION(BlueprintCallable) void ApplyCockpitLook(float X,float Y);
 void RestoreTableauPoses();
 FVector CockpitOffset=FVector(-7.55,-44.56,165);
 float CockpitYaw=0,CockpitPitch=0;
 uint64 LastCockpitInputFrame=MAX_uint64;
 float OtherWheelAngle=0;
 bool ApproachHint = false, RescueHint = false, SearchHint = false;
 int32 IdleHintStage = 0, LastStamp = -1;
 UFUNCTION(BlueprintCallable) void Inspect(int32 Index);
 UFUNCTION(BlueprintCallable) void SetBalance(EEchoBalance Preset);
 UFUNCTION(BlueprintCallable) void BeginImpact();
 UFUNCTION(BlueprintCallable) void BeginOutcome();
 UFUNCTION(BlueprintCallable) void BeginDeparture();
 UFUNCTION(BlueprintCallable) void BeginArrival();
 void BindClues();
 void TickPresentation(float Dt);
 void TickAvoidance(float Dt);
 void CompleteAvoidance();
 void ContinueRoad();
 void PreparePreviews();
 void PlayFeedback(const TCHAR* Asset);
 void ConfigureChapter();
 void BeginDrive();
 void TickDrive(float Dt);
 void Finish();
 void SetNotice(const FString& Text);
};

UCLASS()
class ECHOOFCRASH_API AEchoCampaignHUD : public AHUD
{
 GENERATED_BODY()
public:
 virtual void DrawHUD() override;
 virtual void NotifyHitBoxClick(FName BoxName) override;
 bool DrawShell(class AEchoCampaignDirector* D,float W,float H,float S);
 void Button(FName ID,const FString& Text,float X,float Y,float W,float H,bool Primary=false,bool Enabled=true);
 FName AudioHoverButton;
 double LastHoverSound=-1;
 void Frame(float X,float Y,float W,float H,FLinearColor Fill,FLinearColor Edge,float Radius=12);
 void SteeringWheel(float X,float Y,float Radius,float Marker,float Low,float High);
 void Centered(const FString& Text,float X,float Y,float Scale,FLinearColor Color);
 void Thumbnail(class UTexture* Texture,float X,float Y,float W,float H);
 void Label(const FString& Text, float X, float Y, float Scale=1, FLinearColor Color=FLinearColor::White);
 void Panel(float X,float Y,float W,float H,float Alpha=.86f);
 void DrawConversationAndInteraction(AEchoCampaignDirector* D,float W,float H,float Scale);
 // Runtime bounds also let the development checks verify the rendered layout.
 UPROPERTY(BlueprintReadOnly) FVector2D DialogueOrigin = FVector2D::ZeroVector;
 UPROPERTY(BlueprintReadOnly) FVector2D DialogueSize = FVector2D::ZeroVector;
 UPROPERTY(BlueprintReadOnly) FVector2D InteractionOrigin = FVector2D::ZeroVector;
 UPROPERTY(BlueprintReadOnly) FVector2D InteractionSize = FVector2D::ZeroVector;
 FString LastDialogue;
 float DialogueAppearedAt = 0;
 FVector2D MeasureText(const FString& Text,float Scale,bool Bold=false) const;
 void PaintText(const FString& Text,float X,float Y,float Scale,FLinearColor Color,bool Center=false,bool Bold=false);
 float FitText(const FString& Text,float Scale,float Width,float Height,bool Bold=false) const;
 TArray<FString> WrapText(const FString& Text,float Width,float Scale,bool Bold=false) const;
 float TextBlock(const FString& Text,float X,float Y,float Width,float Scale,FLinearColor Color,bool Center=true,bool Bold=false);
 void ContinueMarker(float X,float Y,float Scale);
 UPROPERTY() TObjectPtr<class UTexture2D> ContinueTexture;
 UPROPERTY(BlueprintReadWrite) bool AuditLayout=false;
 UFUNCTION(BlueprintCallable) FString GetLayoutAudit() const;
 struct FTextBounds {FString Text;FVector2D Origin,Size;};
 TArray<FTextBounds> TextBounds;
 FVector2D ContinueOrigin=FVector2D::ZeroVector,ContinueSize=FVector2D::ZeroVector;
};

UCLASS(Blueprintable)
class ECHOOFCRASH_API AEchoCampaignGameMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 AEchoCampaignGameMode();
 virtual void BeginPlay() override;
};
