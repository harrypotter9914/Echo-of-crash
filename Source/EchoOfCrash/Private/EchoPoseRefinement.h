#pragma once
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"

namespace EchoPose
{
// Retargeted wrist extremes are too sharp on the Synty mesh. Ease them toward
// the mesh's own neutral wrist, preserving small gestures and forearm motion.
inline void SoftenWrists(UPoseableMeshComponent* Mesh)
{
 auto Asset=Cast<USkeletalMesh>(Mesh->GetSkinnedAsset());if(!Asset)return;
 const auto& Ref=Asset->GetRefSkeleton();
 for(FName Hand:{FName(TEXT("Hand_L")),FName(TEXT("Hand_R"))}){
  int32 Index=Ref.FindBoneIndex(Hand);if(Index==INDEX_NONE)continue;int32 Parent=Ref.GetParentIndex(Index);if(Parent==INDEX_NONE)continue;
  auto H=Mesh->GetBoneTransformByName(Hand,EBoneSpaces::ComponentSpace);
  FQuat P=Mesh->GetBoneTransformByName(Ref.GetBoneName(Parent),EBoneSpaces::ComponentSpace).GetRotation();
  FQuat Neutral=Ref.GetRefBonePose()[Index].GetRotation(),Local=P.Inverse()*H.GetRotation();
  float Angle=FMath::RadiansToDegrees(Neutral.AngularDistance(Local));
  if(Angle>25){float Soft=25+20*FMath::Tanh((Angle-25)/20);H.SetRotation(P*FQuat::Slerp(Neutral,Local,Soft/Angle));Mesh->SetBoneTransformByName(Hand,H,EBoneSpaces::ComponentSpace);}
 }
}

inline void RestingPalm(UPoseableMeshComponent* Mesh,bool Left,FVector Forward,float Alpha)
{
 FName Hand=Left?TEXT("Hand_L"):TEXT("Hand_R"),Index=Left?TEXT("indexFinger_01_l"):TEXT("indexFinger_01_r"),Middle=Left?TEXT("finger_01_l"):TEXT("finger_01_r");
 auto H=Mesh->GetBoneTransformByName(Hand,EBoneSpaces::WorldSpace);
 FVector I=Mesh->GetBoneLocationByName(Index,EBoneSpaces::WorldSpace),M=Mesh->GetBoneLocationByName(Middle,EBoneSpaces::WorldSpace);
 FVector Along=((I+M)*.5-H.GetLocation()).GetSafeNormal(),Across=(I-M).GetSafeNormal();
 if(Along.IsNearlyZero() || Across.IsNearlyZero())return;
 FQuat Basis=FRotationMatrix::MakeFromXY(Along,Across).ToQuat();
 FQuat Target=FRotationMatrix::MakeFromXY(-FVector::UpVector+Forward*.10f,Forward).ToQuat();
 H.SetRotation(FQuat::Slerp(H.GetRotation(),Target*Basis.Inverse()*H.GetRotation(),Alpha));Mesh->SetBoneTransformByName(Hand,H,EBoneSpaces::WorldSpace);
 SoftenWrists(Mesh);
}
}
