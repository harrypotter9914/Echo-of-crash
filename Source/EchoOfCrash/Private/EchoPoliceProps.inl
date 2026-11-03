// Hinged derivative retains the original police car's skeleton and rotating wheels.
static AActor* CinematicPolice(UWorld* World)
{
 auto A=World->SpawnActor<AActor>();A->Tags.Add(TEXT("EchoCinematicPolice"));
 auto Body=NewObject<UPoseableMeshComponent>(A);A->SetRootComponent(Body);Body->RegisterComponent();Body->SetSkinnedAssetAndUpdate(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/EchoCampaign/Vehicles/SK_PoliceBodyV15.SK_PoliceBodyV15")));Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 for(const TCHAR* Name:{TEXT("FrontL"),TEXT("FrontR"),TEXT("RearL"),TEXT("RearR")}){
  FString N(Name);auto Door=NewObject<UStaticMeshComponent>(A,FName(*N));Door->SetupAttachment(Body);Door->RegisterComponent();
  Door->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/EchoCampaign/Vehicles/SM_PoliceDoor%s.SM_PoliceDoor%s"),Name,Name)));
  Door->SetRelativeLocation(FVector(N.StartsWith(TEXT("Front"))?108:-7,N.EndsWith(TEXT("L"))?-94:94,86));Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 }
 return A;
}
static void PoliceDoor(AActor* Car,FName Name,float Open)
{
 TArray<UStaticMeshComponent*> Parts;Car->GetComponents(Parts);for(auto P:Parts)if(P->GetFName()==Name)P->SetRelativeRotation(FRotator(0,(Name.ToString().EndsWith(TEXT("L"))?74:-74)*Open,0));
}
static void PoliceDoorHand(UPoseableMeshComponent* M,AActor* Car,bool Rear,float Open,float Weight)
{
 if(Weight<=0)return;FVector Handle=FVector(Rear?-7:108,-94,86)+FRotator(0,74*Open,0).RotateVector(FVector(-95,-2,0));
 FVector Target=Car->GetActorTransform().TransformPosition(Handle);
 ReachHand(M,true,FMath::Lerp(M->GetBoneLocationByName(TEXT("Hand_L"),EBoneSpaces::WorldSpace),Target,Weight));EchoPose::SoftenWrists(M);
}
static void PoliceFootwell(UPoseableMeshComponent* M,AActor* Car,float Weight,bool Exiting=false)
{
 if(Weight<=0)return;
 // Fold the legs onto the sedan floor after lowering onto its cushion. Moving
 // the whole pickup entry pose down alone pushes shoes through the floorpan.
 for(bool Left:{true,false}){
  FName Upper=Left?TEXT("thigh_l"):TEXT("thigh_r"),Lower=Left?TEXT("calf_l"):TEXT("calf_r"),Foot=Left?TEXT("foot_l"):TEXT("foot_r");
  FTransform U=M->GetBoneTransformByName(Upper,EBoneSpaces::WorldSpace),L=M->GetBoneTransformByName(Lower,EBoneSpaces::WorldSpace),H=M->GetBoneTransformByName(Foot,EBoneSpaces::WorldSpace);
  FVector A=U.GetLocation(),B=L.GetLocation(),C=H.GetLocation(),Target=C;
  FVector Local=Car->GetActorTransform().InverseTransformPosition(C);
  float Floor=Car->GetActorLocation().Z+(Exiting && FMath::Abs(Local.Y)>91?5:44);
  Target.Z=FMath::Lerp(C.Z,Exiting?FMath::Max(C.Z,Floor):Floor,Weight);
  FVector D=(Target-A).GetSafeNormal();float L1=FVector::Distance(A,B),L2=FVector::Distance(B,C),Distance=FMath::Clamp(float(FVector::Distance(A,Target)),FMath::Abs(L1-L2)+.01f,L1+L2-.01f);
  Target=A+D*Distance;float Along=(L1*L1-L2*L2+Distance*Distance)/(2*Distance),Height=FMath::Sqrt(FMath::Max(0.f,L1*L1-Along*Along));
  FVector Bend=FVector::UpVector+Car->GetActorForwardVector()*.3f;Bend=(Bend-D*FVector::DotProduct(Bend,D)).GetSafeNormal();FVector Joint=A+D*Along+Bend*Height;
  U.SetRotation(FQuat::FindBetweenNormals((B-A).GetSafeNormal(),(Joint-A).GetSafeNormal())*U.GetRotation());L.SetRotation(FQuat::FindBetweenNormals((C-B).GetSafeNormal(),(Target-Joint).GetSafeNormal())*L.GetRotation());L.SetLocation(Joint);H.SetLocation(Target);
  M->SetBoneTransformByName(Upper,U,EBoneSpaces::WorldSpace);M->SetBoneTransformByName(Lower,L,EBoneSpaces::WorldSpace);M->SetBoneTransformByName(Foot,H,EBoneSpaces::WorldSpace);
 }
}
static AActor* MakeCuffs(UWorld* World)
{
 auto A=World->SpawnActor<AActor>();auto Root=NewObject<USceneComponent>(A);A->SetRootComponent(Root);Root->RegisterComponent();A->Tags.Add(TEXT("EchoHandcuffs"));
 for(int I=0;I<39;++I){auto M=NewObject<UStaticMeshComponent>(A,*FString::Printf(TEXT("Cuff%d"),I));M->SetupAttachment(Root);M->RegisterComponent();M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/EchoCampaign/Art/M_CuffsV15.M_CuffsV15")));}
 A->SetActorHiddenInGame(true);return A;
}
static void DrawCuffs(AActor* A,UPoseableMeshComponent* Person,float Closed,bool Visible)
{
 A->SetActorHiddenInGame(!Visible);if(!Visible)return;
 TArray<UStaticMeshComponent*> Parts;A->GetComponents(Parts);
 FVector Hands[2];
 auto Segment=[](UStaticMeshComponent* M,FVector P,FVector Q){M->SetWorldLocation((P+Q)*.5f);M->SetWorldRotation(FRotationMatrix::MakeFromZ(Q-P).Rotator());M->SetWorldScale3D(FVector(.012,.012,FVector::Distance(P,Q)/100));};
 for(int Side=0;Side<2;++Side){FName Hand=Side==0?TEXT("Hand_L"):TEXT("Hand_R"),Arm=Side==0?TEXT("lowerarm_l"):TEXT("lowerarm_r");FVector H=Person->GetBoneLocationByName(Hand,EBoneSpaces::WorldSpace),Axis=(H-Person->GetBoneLocationByName(Arm,EBoneSpaces::WorldSpace)).GetSafeNormal();H-=Axis*1.5f;Hands[Side]=H;
  FVector U=FVector::CrossProduct(Axis,FVector::UpVector).GetSafeNormal(),V=FVector::CrossProduct(Axis,U);if(U.IsNearlyZero()){U=FVector::RightVector;V=FVector::ForwardVector;}
  float Arc=FMath::Lerp(1.45f*PI,2*PI,Closed);
  for(int J=0;J<18;++J){float R=4.1f,A0=J*Arc/18,A1=(J+1)*Arc/18;Segment(Parts[Side*18+J],H+R*(U*FMath::Cos(A0)+V*FMath::Sin(A0)),H+R*(U*FMath::Cos(A1)+V*FMath::Sin(A1)));}
 }
 for(int I=0;I<3;++I){FVector P=FMath::Lerp(Hands[0],Hands[1],(I+.12f)/3),Q=FMath::Lerp(Hands[0],Hands[1],(I+.88f)/3);Segment(Parts[36+I],P,Q);}
}
