#include "EchoCampaign.h"
#include "EchoPresentation.h"
#include "Components/SplineComponent.h"
#include "EchoGameInstance.h"
#include "EchoGameplayLibrary.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Sound/SoundBase.h"
#include "Animation/AnimSequence.h"
#include "Engine/Texture2D.h"

AEchoCampaignDirector::AEchoCampaignDirector(){PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("CampaignRoot"));}
void AEchoCampaignDirector::BeginPlay()
{
 Super::BeginPlay();
 if(ActorHasTag(TEXT("EchoIncidentTemplate"))){SetActorTickEnabled(false);for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->GetLevel()==GetLevel() && It->ActorHasTag(TEXT("EchoTableau")))It->SetActorHiddenInGame(true);return;}
 LoadPreferences();ConfigureChapter();HasCheckpoint=UGameplayStatics::DoesSaveGameExist(TEXT("EchoJourney"),0);
 // Imported editor camera props are not part of the accident tableau.
 for(TActorIterator<AActor> It(GetWorld());It;++It){TArray<UStaticMeshComponent*> Meshes;It->GetComponents(Meshes);for(auto Mesh:Meshes)if(Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetPathName().Contains(TEXT("/Engine/EditorMeshes/MatineeCam"))){Mesh->SetHiddenInGame(true);Mesh->SetVisibility(false);}}
 for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->GetLevel()==GetLevel() && It->ActorHasTag(TEXT("EchoTableau")))Tableau.Add(*It);
 if(auto GI=Cast<UEchoGameInstance>(GetGameInstance())){Rules=FEchoJourneyRules::Preset(GI->Balance);if(!GI->JourneyStarted){GI->Mood=Rules.StartingMood;GI->JourneyStarted=true;}MoodBefore=GI->Mood;GI->Chapter=Chapter;GI->BeginInvestigation(FName(*FString::Printf(TEXT("Chapter%d"),Chapter+1)));}
 if(auto PC=GetWorld()->GetFirstPlayerController()){PC->bShowMouseCursor=false;PC->bEnableClickEvents=true;PC->SetInputMode(FInputModeGameOnly());}
 RestoreTableauPoses();BindClues();
 PreloadIncidents();
 if(auto GI=Cast<UEchoGameInstance>(GetGameInstance())){if(GI->StartAtTutorial){GI->StartAtTutorial=false;Phase=EEchoPhase::Tutorial;SetCursor(true);}else if(Chapter==0 && !GI->FrontEndEntered){Phase=EEchoPhase::Title;SetCursor(true);}else SetCursor(true);}
 if(auto GI=Cast<UEchoGameInstance>(GetGameInstance()))if(GI->ContinueDriving){GI->ContinueDriving=false;GetWorldTimerManager().SetTimerForNextTick([this](){BeginDrive();Say(Chapter==1?TEXT("road_two"):TEXT("road_three"));});}
}
void AEchoCampaignDirector::ConfigureChapter()
{
 const TArray<FEchoClue> CustomClues=Clues; const TArray<FVector> CustomRoute=Route;
 Chapter=FMath::Clamp(Chapter,0,2); Clues.Empty(); Answers.Empty();
 auto Add=[this](const TCHAR* T,const TCHAR* Desc,FVector P,bool Rescue=false){FEchoClue C;C.Title=T;C.Description=Desc;C.Position=P;C.Rescue=Rescue;Clues.Add(C);};
 if(Chapter==0){
 Center=FVector(-9200,-580,150);ChapterTitle=TEXT("01 / A photograph left behind");
 Story=TEXT("A family journey. A stranger's grief. One moment before everything changes.");
 Add(TEXT("An open bottle"),TEXT("Alcohol spilled across the passenger seat. The driver had been drinking."),FVector(-9253,-390,176));
 Add(TEXT("Wedding photograph"),TEXT("A wedding photograph she keeps beside her. A happy memory, held onto through grief."),FVector(-9188,-345,150));
 Add(TEXT("The grieving driver"),TEXT("Her story does not have to end here."),FVector(-9248,-346,218),true);
 Add(TEXT("Your wife"),TEXT("Bring her back to the journey you promised each other."),FVector(-9252,-820,145),true);
 Add(TEXT("Your own body"),TEXT("There is still a way home."),FVector(-9165,-772,241),true);
 Answers={TEXT("Brake failure"),TEXT("Grief and alcohol impaired the driver"),TEXT("A planned attack")};CorrectAnswer=1;
 }else if(Chapter==1){
 Center=FVector(-450,13720,150);ChapterTitle=TEXT("02 / What they concealed");
 Story=TEXT("An embrace. Two concealed weapons. Look beyond the first explanation.");
 Add(TEXT("A concealed stone"),TEXT("A heavy, rounded stone clenched behind the boy's companion. His hidden grip reveals an intention to hurt her."),FVector(-315,13705,169));
 Add(TEXT("A hidden knife"),TEXT("The girl holds a blade low beside her seat. Both concealed weapons contradict the couple's affectionate pose."),FVector(-348,13666,117));
 Add(TEXT("The schoolgirl"),TEXT("Knowing the truth does not make a life disposable."),FVector(-268,13663,114),true);
 Add(TEXT("The schoolboy"),TEXT("Leave him the chance to answer for what happened."),FVector(-336,13687,103),true);
 Add(TEXT("Your wife"),TEXT("Find her through the suspended glass."),FVector(-683,13791,149),true);
 Add(TEXT("Your own body"),TEXT("Hold onto the life you can still return to."),FVector(-651,13698,241),true);
 Answers={TEXT("A deliberate attack caused the crash"),TEXT("A sudden storm"),TEXT("The family car ran out of fuel")};CorrectAnswer=0;
 }else{
 Center=FVector(4870,-350,150);ChapterTitle=TEXT("03 / The cost of a distraction");
 Story=TEXT("A meal voucher. A glance away from the road. Seconds nobody can take back.");
 Add(TEXT("Meal voucher"),TEXT("A voucher for two burgers and cola. The driver was reading it instead of watching the road."),FVector(4764,-301,225));
 Add(TEXT("Two hamburgers"),TEXT("The unwrapped meal was being eaten while the truck was moving."),FVector(4669,-314,184));
 Add(TEXT("Spilled cola"),TEXT("The tipped cup is still spilling beside the controls, frozen in the instant of the crash."),FVector(4718,-347,181));
 Add(TEXT("The delivery driver"),TEXT("One mistake should not be the end of his story."),FVector(4766,-372,173),true);
 Add(TEXT("Your wife"),TEXT("The final chance to bring her home."),FVector(5131,-339,194),true);
 Add(TEXT("Your own body"),TEXT("Choose to return."),FVector(5053,-262,260),true);
 Answers={TEXT("The truck was sabotaged"),TEXT("The driver intended to hit the family"),TEXT("Reading the meal voucher distracted the driver")};CorrectAnswer=2;
 }
 FVector Forward=Chapter==0?FVector(0,1,0):FVector(1,0,0);
 FVector End=Chapter==0?FVector(-9205,-1100,65):(Chapter==1?FVector(-1000,13730,65):FVector(5500,-310,65));
 Route={End-Forward*2800,End-Forward*1800,End-Forward*800,End};
 if(CustomClues.Num())Clues=CustomClues; if(CustomRoute.Num()>=2)Route=CustomRoute;
 if(auto PC=GetWorld()->GetFirstPlayerController()) if(auto P=PC->GetPawn()) {P->SetActorLocation(Center-Forward*700+FVector(0,0,230));PC->SetControlRotation((Center-P->GetActorLocation()).Rotation());}
}
int32 AEchoCampaignDirector::EvidenceCount() const {int32 N=0;for(auto&C:Clues)N+=!C.Rescue&&C.Complete;return N;}
int32 AEchoCampaignDirector::RescueCount() const {int32 N=0;for(auto&C:Clues)N+=C.Rescue&&C.Complete;return N;}
int32 AEchoCampaignDirector::EvidenceTotal() const {int32 N=0;for(auto&C:Clues)N+=!C.Rescue;return N;}
int32 AEchoCampaignDirector::RescueTotal() const {return Clues.Num()-EvidenceTotal();}
void AEchoCampaignDirector::SetNotice(const FString& T){StopDialogue();NoticePages.Empty();T.ParseIntoArray(NoticePages,TEXT(". "),true);if(NoticePages.Num()){Notice=NoticePages[0];NoticePages.RemoveAt(0);}else Notice=T;NoticeTime=FMath::Max(5.f,Notice.Len()*.065f);}
FString AEchoCampaignDirector::Hint() const
{
 if(EvidenceCount()==EvidenceTotal())return TEXT("All evidence found. Press F to reconstruct the cause; rescue anyone still waiting.");
 if(Chapter==0)return TEXT("Look inside the stranger's car: a bottle and a memory of someone she lost.");
 if(Chapter==1)return TEXT("Search around the students' car. The stone and the concealed blade tell one story.");
 return TEXT("Search the truck cab and the spilled meal. What took the driver's eyes off the road?");
}
void AEchoCampaignDirector::BeginDrive()
{
 const bool Keep=SeamlessApproach && IsValid(DriveCar) && IsValid(DriveCamera);
 SetEchoAtmosphere(false);SetCursor(false);if(!Keep && IsValid(DriveCar))DriveCar->Destroy();if(IsValid(OpponentCar))OpponentCar->Destroy();if(!Keep && IsValid(DriveCamera))DriveCamera->Destroy();if(!Keep && EngineAudio)EngineAudio->Stop();if(OpponentAudio)OpponentAudio->Stop();if(PlayerSpline)PlayerSpline->DestroyComponent();if(OpponentSpline)OpponentSpline->DestroyComponent();
 Phase=EEchoPhase::Driving;PhaseTime=0;DriveClock=0;QTEResolved=false;QTESuccess=false;OtherWheelAngle=0;
 ApproachWheelOffset=Keep?WheelAngle:0;
 if(!Keep){Speed=0;WheelAngle=0;CockpitYaw=0;CockpitPitch=0;}
 if(auto P=GetWorld()->GetFirstPlayerController()->GetPawn())P->SetActorHiddenInGame(true);
 RestoreTableauPoses();
 for(auto A:Tableau)if(A)A->SetActorHiddenInGame(true);
 auto SpawnCar=[this](bool Opponent){
  AActor* Source=nullptr;for(auto A:Tableau)if(A && A->ActorHasTag(Opponent?TEXT("EchoOpponentCar"):TEXT("EchoPlayerCar"))){Source=A;break;}
  auto A=GetWorld()->SpawnActor<AActor>();auto M=NewObject<UPoseableMeshComponent>(A);A->SetRootComponent(M);M->RegisterComponent();M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  if(Source){if(auto S=Source->FindComponentByClass<USkeletalMeshComponent>()){M->SetSkinnedAssetAndUpdate(S->GetSkeletalMeshAsset());for(int I=0;I<S->GetNumMaterials();++I)M->SetMaterial(I,S->GetMaterial(I));}A->SetActorScale3D(Source->GetActorScale3D());}
  else M->SetSkinnedAssetAndUpdate(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/PolygonTown/Meshes/Vehicles/SK_Veh_Pickup_01.SK_Veh_Pickup_01")));
  if(Source){for(auto Person:Tableau){if(!Person)continue;bool Occupant=Opponent?Person->ActorHasTag(TEXT("EchoCulprit")):(Person->ActorHasTag(TEXT("EchoFather"))||Person->ActorHasTag(TEXT("EchoWife")));if(!Occupant)continue;if(auto Body=Person->FindComponentByClass<USkeletalMeshComponent>()){
   if(!Opponent && Person->ActorHasTag(TEXT("EchoFather"))){
    auto Sampler=NewObject<USkeletalMeshComponent>(A,TEXT("DriverSampler"));Sampler->SetupAttachment(M);Sampler->RegisterComponent();Sampler->SetSkeletalMesh(Body->GetSkeletalMeshAsset());Sampler->SetCollisionEnabled(ECollisionEnabled::NoCollision);Sampler->SetVisibility(false);Sampler->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;Sampler->PlayAnimation(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/EchoCampaign/Animation/MixamoTown/MX_Driving.MX_Driving")),true);
    auto Driver=NewObject<UPoseableMeshComponent>(A,TEXT("CockpitDriver"));Driver->SetupAttachment(M);Driver->RegisterComponent();Driver->SetSkinnedAssetAndUpdate(Body->GetSkeletalMeshAsset());Driver->SetCollisionEnabled(ECollisionEnabled::NoCollision);Driver->SetRelativeLocation(FVector(-3,-44,64));Driver->SetRelativeRotation(FRotator(0,-90,0));Driver->SetRelativeScale3D(FVector(.90));continue;}

   auto Copy=NewObject<UPoseableMeshComponent>(A);Copy->SetupAttachment(M);Copy->RegisterComponent();Copy->SetSkinnedAssetAndUpdate(Body->GetSkeletalMeshAsset());Body->TickAnimation(0,false);Body->RefreshBoneTransforms();Copy->CopyPoseFromSkeletalComponent(Body);Copy->SetRelativeTransform(Body->GetComponentTransform().GetRelativeTransform(Source->GetActorTransform()));Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision);}}}
  A->Tags.Add(Opponent?TEXT("EchoMovingOpponent"):TEXT("EchoMovingPlayer"));if(Opponent){OpponentCar=A;OpponentMesh=M;}else{DriveCar=A;DriveMesh=M;}
 };
 if(!Keep)SpawnCar(false);SpawnCar(true);
 auto MakeSpline=[this](const TArray<FVector>& Points){auto S=NewObject<USplineComponent>(this);S->SetupAttachment(RootComponent);S->RegisterComponent();S->ClearSplinePoints();for(auto P:Points)S->AddSplinePoint(P,ESplineCoordinateSpace::World,false);S->UpdateSpline();return S;};
 PlayerSpline=MakeSpline(Route);OpponentSpline=MakeSpline(OpponentRoute);
 if(Keep)DriveDuration=PlayerSpline->GetSplineLength()/650.f;
 if(!Keep)EngineAudio=UGameplayStatics::SpawnSoundAttached(LoadObject<USoundBase>(nullptr,TEXT("/Game/EchoCampaign/Audio/EnginePickup.EnginePickup")),DriveMesh,NAME_None,FVector::ZeroVector,EAttachLocation::KeepRelativeOffset,true,.22);
 FString Engine=Chapter==0?TEXT("EngineSedan"):Chapter==1?TEXT("EngineSport"):TEXT("EngineTruck");
 OpponentAudio=UGameplayStatics::SpawnSoundAttached(LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/EchoCampaign/Audio/%s.%s"),*Engine,*Engine)),OpponentMesh,NAME_None,FVector::ZeroVector,EAttachLocation::KeepRelativeOffset,true,.18);
 if(!Keep){DriveCamera=GetWorld()->SpawnActor<ACameraActor>();DriveCamera->GetCameraComponent()->SetFieldOfView(90);GetWorld()->GetFirstPlayerController()->SetViewTarget(DriveCamera);}
 SavePreferences();TickDrive(0);
}
void AEchoCampaignDirector::TickDrive(float Dt)
{
 if(!DriveCar || !OpponentCar || !PlayerSpline || !OpponentSpline)return;
 DriveClock+=Dt*(Phase==EEchoPhase::QTE && !QTEResolved?.06f:1.f);
 auto Progress=[this](float T){return FMath::Clamp(SeamlessApproach?T/DriveDuration:(T-.8f*(1-FMath::Exp(-T/.8f)))/(DriveDuration-.8f*(1-FMath::Exp(-DriveDuration/.8f))),0.f,1.f);};
 float A=Progress(DriveClock);
 auto Move=[&](AActor* Car,UPoseableMeshComponent* Mesh,USplineComponent* Path){float Distance=A*Path->GetSplineLength();FVector P=Path->GetLocationAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World);FRotator R=Path->GetRotationAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World);R.Pitch=0;R.Roll=0;Car->SetActorLocationAndRotation(P,R);
  if(Mesh)for(int I=0;I<Mesh->GetNumBones();++I){FName Bone=Mesh->GetBoneName(I);if((Bone.ToString().Contains(TEXT("wheel"),ESearchCase::IgnoreCase)||Bone.ToString().Contains(TEXT("tyre"),ESearchCase::IgnoreCase))){Mesh->ResetBoneTransformByName(Bone);FRotator Base=Mesh->GetBoneRotationByName(Bone,EBoneSpaces::ComponentSpace);Mesh->SetBoneRotationByName(Bone,(FRotator(FMath::Fmod(Distance/36*180/PI+(Car==DriveCar?ApproachWheelOffset:0),360.f),0,0).Quaternion()*Base.Quaternion()).Rotator(),EBoneSpaces::ComponentSpace);}}
 };
 float PreviousYaw=DriveCar->GetActorRotation().Yaw;
 Move(DriveCar,DriveMesh,PlayerSpline);Move(OpponentCar,OpponentMesh,OpponentSpline);
 Steering=Dt>0?FMath::FindDeltaAngleDegrees(PreviousYaw,DriveCar->GetActorRotation().Yaw)/Dt*.45f:0;
 WheelAngle=FMath::Fmod(A*PlayerSpline->GetSplineLength()/36*180/PI+ApproachWheelOffset,360.f);
 Speed=SeamlessApproach?650.f:PlayerSpline->GetSplineLength()/(DriveDuration-.8f)*(1-FMath::Exp(-DriveClock/.8f));if(EngineAudio)EngineAudio->SetPitchMultiplier(.8f+Speed/1600);if(OpponentAudio)OpponentAudio->SetPitchMultiplier(.9f+Speed/2200);
 UpdateCockpit(Dt);
 if(Phase==EEchoPhase::Driving && !QTEResolved && DriveClock>=DriveDuration-(Chapter>0?.85f:.7f)){Phase=EEchoPhase::QTE;PhaseTime=0;}
 if(DriveClock>=DriveDuration)BeginImpact();
}
void AEchoCampaignDirector::ResolveQTE(float Value)
{
 if(Phase!=EEchoPhase::QTE || QTEResolved)return;
 const float Low[]={.4f,.2f,.7f},High[]={.7f,.4f,.8f};
 QTESuccess=UEchoGameplayLibrary::IsQTEHit(Value,Low[Chapter],High[Chapter]);QTEResolved=true;
 SoundEvent(QTESuccess?TEXT("QTESuccess"):TEXT("QTEFailure"),.32f);SetSoundLoop(TEXT("QTETension"),0);
 if(QTESuccess){Phase=EEchoPhase::Avoidance;PhaseTime=0;AvoidStart=DriveCar->GetActorLocation();AvoidForward=DriveCar->GetActorForwardVector();AvoidRight=DriveCar->GetActorRightVector();OpponentStartDistance=OpponentSpline->GetDistanceAlongSplineAtLocation(OpponentCar->GetActorLocation(),ESplineCoordinateSpace::World);PlayFeedback(TEXT("Brake"));}
 else{Phase=EEchoPhase::Driving;PhaseTime=0;}
}
void AEchoCampaignDirector::StartInvestigation()
{
 if(DriveCar)DriveCar->Destroy();if(OpponentCar)OpponentCar->Destroy();if(DriveCamera)DriveCamera->Destroy();
 for(auto A:Tableau)if(A)A->SetActorHiddenInGame(false);
 if(auto Pawn=GetWorld()->GetFirstPlayerController()->GetPawn())Pawn->SetActorHiddenInGame(false);
 if(EngineAudio)EngineAudio->Stop();if(OpponentAudio)OpponentAudio->Stop();
 Atmosphere=UGameplayStatics::SpawnSound2D(this,LoadObject<USoundBase>(nullptr,TEXT("/Game/EchoCampaign/Audio/Atmosphere.Atmosphere")),.2f);
 StopDialogue();Notice.Empty();AwakeningDialoguePending=true;Phase=EEchoPhase::Awakening;PhaseTime=0;TimeLeft=Rules.InvestigationSeconds+(Chapter==2?15:0);RescueProgress=0;SetEchoAtmosphere(true);
 RestoreTableauPoses();BindClues();PreparePreviews();
 auto PC=GetWorld()->GetFirstPlayerController();if(PC && PC->GetPawn()){auto P=PC->GetPawn();P->SetActorLocation(PlayerImpact.TransformPosition(CockpitOffset)+FVector(0,0,10));PC->SetControlRotation(FRotator(-5,PlayerImpact.Rotator().Yaw,0));if(auto Ghost=Cast<AEchoGhostPawn>(P)){Ghost->SwimVelocity=FVector::ZeroVector;Ghost->EmergenceApplied=0;}PC->SetViewTarget(P);}
 if(PC && PC->PlayerCameraManager)PC->PlayerCameraManager->StartCameraFade(1,0,2.f,FLinearColor::White,false,false);
 // Visual fade only: the authored veil/emergence sounds remain audible.
 // The first voice still waits until the white veil has fully recovered.
}
bool AEchoCampaignDirector::Interact(int32 Index,float HeldSeconds)
{
 if(Phase!=EEchoPhase::Investigation || Paused || JournalOpen || InspectIndex!=INDEX_NONE || TimeLeft<=0 || !FMath::IsFinite(HeldSeconds) || !Clues.IsValidIndex(Index) || Clues[Index].Complete)return false;
 if(!CanReachClue(Index))return false;
 auto&C=Clues[Index];if(C.Rescue && HeldSeconds<3)return false;
 if(!C.Rescue)SoundEvent(Chapter==0?(Index==0?TEXT("ClueGlass"):TEXT("ClueFrame")):Chapter==1?(Index==0?TEXT("ClueStone"):TEXT("ClueKnife")):Index==0?TEXT("Paper"):Index==2?TEXT("Paper"):TEXT("ClueFood"),.28f);
 C.Complete=true;if(C.Rescue){TimeLeft=FMath::Max(0.f,TimeLeft-Rules.RescueTimeCost);RescueCostFeedback=2.5f;SetSoundLoop(TEXT("RescueCharge"),0);AudioRescueTarget=INDEX_NONE;}BeginDissolve(Index);SoundEvent(TEXT("SoulDissolve"),.16f);if(HintTarget==Index)ScanTime=0;RescueProgress=0;IdleHintStage=0;IdleTime=0;PlayFeedback(C.Rescue?TEXT("Rescue"):TEXT("Evidence"));
 if(!C.Rescue && EvidenceCount()==EvidenceTotal())SoundEvent(TEXT("CluesComplete"),.23f);
 if(auto GI=Cast<UEchoGameInstance>(GetGameInstance())){if(C.Rescue)GI->RecordRescue(FName(*C.Title));else GI->CollectEvidence(FName(*C.Title));}

 if(C.Rescue)Say(C.Title.Contains(TEXT("wife"))?TEXT("rescue_wife"):C.Title.Contains(TEXT("own body"))?TEXT("rescue_self"):Chapter==1?TEXT("rescue_student"):TEXT("rescue_driver"));
 else{const TCHAR* IDs0[]={TEXT("evidence_bottle"),TEXT("evidence_photo")};const TCHAR* IDs1[]={TEXT("evidence_stone"),TEXT("evidence_knife")};const TCHAR* IDs2[]={TEXT("evidence_voucher"),TEXT("evidence_burgers"),TEXT("evidence_cola")};Say(Chapter==0?IDs0[Index]:Chapter==1?IDs1[Index]:IDs2[Index]);if(EvidenceCount()==EvidenceTotal())DialogueQueue.Add(TEXT("evidence_complete"));}return true;
}
void AEchoCampaignDirector::ReturnWithoutDeduction()
{
 if(Phase!=EEchoPhase::Deduction || EvidenceCount()==EvidenceTotal())return;
 CorrectDeduction=false;Finish();Say(TEXT("deduction_wrong"));
}
void AEchoCampaignDirector::SubmitDeduction(int32 Answer)
{
 if(Phase!=EEchoPhase::Deduction || EvidenceCount()!=EvidenceTotal() || Answer<0 || Answer>=Answers.Num())return;
 CorrectDeduction=Answer==CorrectAnswer && EvidenceCount()==EvidenceTotal();Finish();Say(CorrectDeduction?(Chapter==0?TEXT("deduction_one"):Chapter==1?TEXT("deduction_two"):TEXT("deduction_three")):TEXT("deduction_wrong"));
}
void AEchoCampaignDirector::Finish()
{
 Phase=EEchoPhase::Results;PhaseTime=0;JournalOpen=false;Paused=false;InspectIndex=INDEX_NONE;LastStamp=-1;
 if(Atmosphere)Atmosphere->FadeOut(1,0);
 if(!Scored){
 RawChapterScore=FEchoJourneyRules::Score(EvidenceCount(),EvidenceTotal(),RescueCount(),RescueTotal(),CorrectDeduction);ChapterScore=FMath::Max(0,RawChapterScore-HintPenalty);ChapterStars=FEchoJourneyRules::Stars(ChapterScore);
 Arrest=CorrectDeduction;for(auto& C:Clues)if(C.Rescue && C.SourceActor && C.SourceActor->ActorHasTag(TEXT("EchoCulprit")) && !C.Complete)Arrest=false;
 if(auto GI=Cast<UEchoGameInstance>(GetGameInstance())){MoodBefore=GI->Mood;GI->TotalSaved+=RescueCount();GI->TotalEvidence+=EvidenceCount();GI->TotalDeductions+=CorrectDeduction;GI->TotalScore+=ChapterScore;GI->Mood=FMath::Clamp(GI->Mood+Rules.MoodChange(RawChapterScore,EvidenceCount()==0 && RescueCount()==0),0,100);MoodDelta=GI->Mood-MoodBefore;}Scored=true;}
 SetCursor(true);
}
void AEchoCampaignDirector::Retry()
{
 if(auto GI=Cast<UEchoGameInstance>(GetGameInstance())){if(Scored){GI->TotalSaved-=RescueCount();GI->TotalEvidence-=EvidenceCount();GI->TotalDeductions-=CorrectDeduction;GI->TotalScore-=ChapterScore;GI->Mood=MoodBefore;if(QTESuccess)GI->Avoided=FMath::Max(0,GI->Avoided-1);}}
 UGameplayStatics::OpenLevel(this,FName(*FString::Printf(TEXT("/Game/EchoCampaign/Maps/Episode%02d"),Chapter+1)));
}
void AEchoCampaignDirector::Advance()
{
 if(Paused || (Phase==EEchoPhase::Awakening && AwakeningDialoguePending))return;
 if(Phase==EEchoPhase::Deduction){ReturnWithoutDeduction();return;}
 if(Phase==EEchoPhase::Title){MenuAction(TEXT("New"));return;}
 if(Phase==EEchoPhase::Tutorial){if(TutorialPage<2)++TutorialPage;else BeginOpening();return;}
 if(Phase==EEchoPhase::Opening){NextDialogue();return;}
 if(Phase==EEchoPhase::Introduction){BeginDrive();Say(Chapter==0?TEXT("road_one"):Chapter==1?TEXT("road_two"):TEXT("road_three"));}
 else if(Phase==EEchoPhase::Investigation || Phase==EEchoPhase::Awakening || Phase==EEchoPhase::Outcome || Phase==EEchoPhase::Arrival || Phase==EEchoPhase::Avoidance || Phase==EEchoPhase::Departure || Phase==EEchoPhase::Driving){NextDialogue();}
 else if(Phase==EEchoPhase::Results){if(PhaseTime<Clues.Num()*.48f+2)PhaseTime=Clues.Num()*.48f+2;else BeginOutcome();}
 else if(Phase==EEchoPhase::Finale)MenuAction(TEXT("New"));
}
void AEchoCampaignDirector::Tick(float Dt)
{
 Super::Tick(Dt);auto PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
 TickVerification();TickControlVerification();TickSoundscape(Dt);
 // Use real frame time for deadlines and holds; background editor throttling must not extend the timer.
 Dt=FMath::Max(0.f,Dt);
 if(PC->WasInputKeyJustPressed(EKeys::Escape)){if(InspectIndex!=INDEX_NONE){Inspect(INDEX_NONE);return;}if(JournalOpen){JournalOpen=false;SoundEvent(TEXT("UIBack"),.2f);SetCursor(false);if(VoiceAudio)VoiceAudio->SetPaused(false);return;}if(MenuPage){MenuPage=0;SoundEvent(TEXT("UIBack"),.2f);return;}if(Phase==EEchoPhase::Title)return;SetPaused(!Paused);return;}
 if(Phase==EEchoPhase::Introduction){if(PC->WasInputKeyJustPressed(EKeys::One))SetBalance(EEchoBalance::Story);if(PC->WasInputKeyJustPressed(EKeys::Two))SetBalance(EEchoBalance::Journey);if(PC->WasInputKeyJustPressed(EKeys::Three))SetBalance(EEchoBalance::Tension);}
 if(Paused){if(PC->WasInputKeyJustPressed(EKeys::R))MenuAction(TEXT("Restart"));return;}
 if(Phase==EEchoPhase::Opening && PC->WasInputKeyJustPressed(EKeys::SpaceBar)){EndOpening();return;}
 if((Phase==EEchoPhase::Title || Phase==EEchoPhase::Tutorial || Phase==EEchoPhase::Finale) && MenuPage)return;
 if(PC->WasInputKeyJustPressed(EKeys::R) && (Phase==EEchoPhase::Results || Phase==EEchoPhase::Introduction)){Retry();return;}
 if(PC->WasInputKeyJustPressed(EKeys::C) && Phase==EEchoPhase::Introduction){ContinueSavedJourney();return;}
 if(PC->WasInputKeyJustPressed(EKeys::Enter)){SoundEvent(TEXT("UIClick"),.18f);Advance();return;}
 if(Phase==EEchoPhase::Investigation && PC->WasInputKeyJustPressed(EKeys::Tab)){JournalOpen=!JournalOpen;SoundEvent(JournalOpen?TEXT("Paper"):TEXT("UIBack"),.25f);SetCursor(JournalOpen);if(VoiceAudio)VoiceAudio->SetPaused(JournalOpen);RescueProgress=0;}
 if(InspectIndex!=INDEX_NONE){TickPresentation(Dt);return;}
 if(Phase==EEchoPhase::Investigation){const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three};int E=0;for(int I=0;I<Clues.Num();++I)if(!Clues[I].Rescue){if(E<3 && PC->WasInputKeyJustPressed(Keys[E]) && Clues[I].Complete){Inspect(I);return;}++E;}}
 if(JournalOpen)return;
 if(Phase==EEchoPhase::Investigation || Phase==EEchoPhase::Awakening){bool Visible=Phase==EEchoPhase::Awakening || PC->IsInputKeyDown(EKeys::LeftAlt);if(PC->bShowMouseCursor!=Visible)SetCursor(Visible);}
 PhaseTime+=Dt;NoticeTime-=Dt;RescueCostFeedback=FMath::Max(0.f,RescueCostFeedback-Dt);FinishConfirmTime=FMath::Max(0.f,FinishConfirmTime-Dt);ScanTime=FMath::Max(0.f,ScanTime-Dt);if(NoticeTime<=0 && !Notice.IsEmpty())NextDialogue();
 if(Phase==EEchoPhase::Tutorial && TutorialPage==1){QTEValue=.5f+.5f*FMath::Sin(PhaseTime*3-PI/2);if(PC->WasInputKeyJustPressed(EKeys::SpaceBar)){TutorialPracticeHit=QTEValue>=.4f && QTEValue<=.7f;SoundEvent(TutorialPracticeHit?TEXT("QTESuccess"):TEXT("QTEFailure"),.3f);}}
 const bool WasDeparting=Phase==EEchoPhase::Departure;
 TickPresentation(Dt);
 // The connector already advanced this frame. Do not advance the new approach
 // a second time on the handoff frame, which would briefly double road speed.
 if(WasDeparting && Phase==EEchoPhase::Driving)return;
 if(Phase==EEchoPhase::Opening || Phase==EEchoPhase::Title || Phase==EEchoPhase::Tutorial || Phase==EEchoPhase::Impact || Phase==EEchoPhase::Awakening || Phase==EEchoPhase::Avoidance || Phase==EEchoPhase::Outcome || Phase==EEchoPhase::Arrival || Phase==EEchoPhase::Departure)return;
 if(Phase==EEchoPhase::Driving || Phase==EEchoPhase::QTE){
 if(Phase==EEchoPhase::QTE){QTEValue=.5f+.5f*FMath::Sin(PhaseTime*4.0f-PI/2);if(PC->WasInputKeyJustPressed(EKeys::SpaceBar)){ResolveQTE(QTEValue);return;}if(PhaseTime>3.5f){ResolveQTE(-1);return;}}
 float Remaining=Dt;while(Remaining>0 && (Phase==EEchoPhase::Driving || Phase==EEchoPhase::QTE)){float Step=FMath::Min(Remaining,1.f/60.f);TickDrive(Step);Remaining-=Step;}return;}
 if(Phase==EEchoPhase::Deduction){if(EvidenceCount()!=EvidenceTotal())return;if(PC->WasInputKeyJustPressed(EKeys::One))SubmitDeduction(0);if(PC->WasInputKeyJustPressed(EKeys::Two))SubmitDeduction(1);if(PC->WasInputKeyJustPressed(EKeys::Three))SubmitDeduction(2);return;}
 if(Phase!=EEchoPhase::Investigation)return;
 TimeLeft=FMath::Max(0.f,TimeLeft-Dt);IdleTime+=Dt;TickInvestigationEffects(Dt);
 if(TimeLeft<30 && !LowTimeSpoken && Notice.IsEmpty()){LowTimeSpoken=true;Say(TEXT("time_low"));}
 if(!ApproachHint && Notice.IsEmpty() && PC->GetPawn() && FVector::Dist2D(PC->GetPawn()->GetActorLocation(),OpponentImpact.GetLocation())<500){ApproachHint=true;Say(Chapter==0?TEXT("approach_one"):Chapter==1?TEXT("approach_two"):TEXT("approach_three"));}
 if(Notice.IsEmpty() && IdleTime>Rules.HintDelay && IdleHintStage==0){++IdleHintStage;Say(Chapter==0?TEXT("hint_one"):Chapter==1?TEXT("hint_two"):TEXT("hint_three"));}
 if(Notice.IsEmpty() && IdleTime>Rules.HintDelay+18 && IdleHintStage==1){++IdleHintStage;Say(TEXT("echo_hint_lesson"));}
 if(Focus!=INDEX_NONE && Clues[Focus].Rescue && !RescueHint && Notice.IsEmpty()){RescueHint=true;Say(TEXT("rescue_hint"));}
 if(Focus!=INDEX_NONE && !Clues[Focus].Rescue && !SearchHint && Notice.IsEmpty()){SearchHint=true;Say(TEXT("echo_search_lesson"));}
 if(PC->WasInputKeyJustPressed(EKeys::Q))RequestHint();
 int32 Previous=Focus;Focus=INDEX_NONE;float Best=FLT_MAX;
 if(auto P=PC->GetPawn())for(int32 I=0;I<Clues.Num();++I){if(Clues[I].Complete)continue;float Distance=FVector::Dist(Clues[I].Position,SpiritContactPoint());if(Distance<=InteractionReach(I)+40 && Distance<Best){Best=Distance;Focus=I;}}
 FocusReachable=CanReachClue(Focus);
 if(Previous!=Focus)RescueProgress=0;
 if(FocusReachable){if(Clues[Focus].Rescue){RescueProgress=UEchoGameplayLibrary::AdvanceRescueProgress(RescueProgress,Dt,PC->IsInputKeyDown(EKeys::E));if(RescueProgress>=3)Interact(Focus,RescueProgress);}else if(PC->WasInputKeyJustPressed(EKeys::E))Interact(Focus,0);}else RescueProgress=0;
 if(PC->WasInputKeyJustPressed(EKeys::F)){if(EvidenceCount()==EvidenceTotal()){if(RescueCount()<RescueTotal() && FinishConfirmTime<=0){FinishConfirmTime=4;SetNotice(TEXT("People are still waiting. Press F again within 4 seconds to leave them and finish."));}else{Phase=EEchoPhase::Deduction;SetCursor(true);RescueProgress=0;}}else SetNotice(TEXT("Find the remaining clues to confirm the cause. If time runs out, return with what you recovered."));}
 if(TimeLeft<=0){Phase=EEchoPhase::Deduction;SetCursor(true);RescueProgress=0;Say(EvidenceCount()==EvidenceTotal()?TEXT("time_out"):TEXT("time_out_incomplete"));}
}

AEchoCampaignGameMode::AEchoCampaignGameMode(){DefaultPawnClass=AEchoGhostPawn::StaticClass();HUDClass=AEchoCampaignHUD::StaticClass();}
void AEchoCampaignGameMode::BeginPlay(){Super::BeginPlay();}

void AEchoCampaignDirector::PlayFeedback(const TCHAR* Asset)
{
 SoundEvent(FName(Asset));
}
void AEchoCampaignDirector::SaveCheckpoint(int32 NextChapter)
{
 auto Save=Cast<UEchoCampaignSave>(UGameplayStatics::CreateSaveGameObject(UEchoCampaignSave::StaticClass()));
 auto GI=Cast<UEchoGameInstance>(GetGameInstance());if(!Save || !GI)return;
 Save->Chapter=NextChapter;Save->Balance=GI->Balance;Save->Mood=GI->Mood;if(NextChapter>0){Save->Mood=GI->Mood;Save->TotalScore=GI->TotalScore;Save->Avoided=GI->Avoided;Save->Saved=GI->TotalSaved;Save->Evidence=GI->TotalEvidence;Save->Deductions=GI->TotalDeductions;}
 UGameplayStatics::SaveGameToSlot(Save,TEXT("EchoJourney"),0);
}
void AEchoCampaignDirector::ContinueSavedJourney()
{
 if(Phase!=EEchoPhase::Introduction && Phase!=EEchoPhase::Title)return;
 auto Save=Cast<UEchoCampaignSave>(UGameplayStatics::LoadGameFromSlot(TEXT("EchoJourney"),0));
 if(!Save){SetNotice(TEXT("No saved journey yet. Press ENTER to begin."));return;}
 if(auto GI=Cast<UEchoGameInstance>(GetGameInstance())){GI->FrontEndEntered=true;GI->Mood=Save->Mood;GI->Balance=Save->Balance;GI->TotalScore=Save->TotalScore;GI->Avoided=Save->Avoided;GI->JourneyStarted=true;GI->TotalSaved=Save->Saved;GI->TotalEvidence=Save->Evidence;GI->TotalDeductions=Save->Deductions;}
 UGameplayStatics::OpenLevel(this,FName(*FString::Printf(TEXT("/Game/EchoCampaign/Maps/Episode%02d"),FMath::Clamp(Save->Chapter,0,2)+1)));
}


