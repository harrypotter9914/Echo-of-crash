void AEchoStoryScene::TickPoliceAftermath(float Dt)
{
 if(!PoliceCar || People.Num()<3)return;
 const int Count=People.Num()-2;
 const float Talk=12.8f,CuffEnd=Talk+Count*5.f,EscortStart=CuffEnd+.6f,EscortEnd=EscortStart+5.2f;
 const float BoardStart=EscortEnd+.5f,BoardEnd=BoardStart+Count*9.f,ReturnStart=BoardEnd+.4f,EntryStart=ReturnStart+2.5f;
 const float LeaveStart=EntryStart+6.1f,Reaction=LeaveStart+4.8f;
 if(DialogueBeat==0 && !Director->Notice.IsEmpty())Clock=FMath::Min(Clock,Talk+2.5f);
 if(DialogueBeat==1 && !Director->Notice.IsEmpty())Clock=FMath::Min(Clock,Reaction+7.5f);
 float T=Clock-2.5f;AftermathTime=T;
 auto Officer=People.Last();auto Hero=People[0];
 FVector HeroP=Origin-Forward*300-Right*350;
 auto GroupP=[&](int J){return Origin-Forward*600-Right*(350+J*120);};
 auto CarPoint=[&](FVector V){return PoliceCar->GetActorTransform().TransformPosition(V);};
 auto Ease=[](float A,float B,float Value){return FMath::SmoothStep(A,B,Value);};
 if(T<0){AftermathBeat=TEXT("Reality");for(int I=0;I<People.Num()-1;++I){Pose(People[I],0,PerformanceClock);GroundPerformance(People[I],People[I]->GetOwner()->GetActorLocation(),0);}Shot(Origin-Forward*850-Right*650+FVector(0,0,360),Origin+Forward*100+FVector(0,0,90),Dt);return;}
 float Approach=Ease(0,3,T),LeaveTime=FMath::Max(0.f,T-LeaveStart);
 float LeaveDistance=LeaveTime<2.5f?100*LeaveTime*LeaveTime:625+500*(LeaveTime-2.5f);
 FVector CarP=Origin+Forward*FMath::Lerp(-2600.f,-1050.f,Approach),CarFacing=Forward;
 if(LeaveTime>0){
  auto S=[](float U){U=FMath::Clamp(U,0.f,1.f);return U*U*U*(10+U*(-15+6*U));};
  auto DS=[](float U){return U>0 && U<1?30*U*U*(1-U)*(1-U):0.f;};
  // The grounded, eastbound chapter-three truck spans farther across this lane.
  // Pass its rear at Y=-870 and wait until the entire sedan clears its front.
  const float Bypass=Director->Chapter==2?620.f:550.f;
  float Side=-Bypass*S(LeaveDistance/650),Slope=-Bypass/650*DS(LeaveDistance/650);
  if(Director->Chapter==1){
   // Pass the grounded incident in the clear opposite lane, then take the
   // junction's southbound exit. Continuing east would enter the waterfront.
   float TurnAt=-250-(Origin.X-1050),Radius=500,Arc=Radius*PI*.5f;
   if(LeaveDistance>TurnAt){float D=LeaveDistance-TurnAt,A=FMath::Min(D/Radius,PI*.5f);CarP=FVector(-250+Radius*FMath::Sin(A),Origin.Y-550-Radius*(1-FMath::Cos(A)),0);CarFacing=FVector(FMath::Cos(A),-FMath::Sin(A),0);if(D>Arc)CarP+=CarFacing*(D-Arc);}
   else{CarP=Origin+Forward*(-1050+LeaveDistance)+Right*Side;CarFacing=(Forward+Right*Slope).GetSafeNormal();}
  }else{
   const float ReturnAt=Director->Chapter==2?2250.f:1900.f;
   Side+=Bypass*S((LeaveDistance-ReturnAt)/1100);Slope+=Bypass/1100*DS((LeaveDistance-ReturnAt)/1100);
   CarP=Origin+Forward*(-1050+LeaveDistance)+Right*Side;CarFacing=(Forward+Right*Slope).GetSafeNormal();
  }
 }
 PoliceCar->SetActorLocationAndRotation(CarP,CarFacing.Rotation());
 if(!RealityRevealed){RealityRevealed=true;PoliceCar->SetActorHiddenInGame(false);for(auto M:People)M->GetOwner()->SetActorHiddenInGame(false);Siren=UGameplayStatics::SpawnSound2D(this,LoadObject<USoundBase>(nullptr,TEXT("/Game/EchoCampaign/Audio/PoliceArrival.PoliceArrival")),.15);}
 if(Siren && T>=3 && T-Dt<3)Siren->FadeOut(.6f,0);
 if(auto M=PoliceCar->FindComponentByClass<UPoseableMeshComponent>())for(int I=0;I<M->GetNumBones();++I){FName B=M->GetBoneName(I);if(B.ToString().Contains(TEXT("wheel"),ESearchCase::IgnoreCase)){M->ResetBoneTransformByName(B);auto R=M->GetBoneRotationByName(B,EBoneSpaces::ComponentSpace);R.Pitch+=(1550*Approach+LeaveDistance)/36*180/PI;M->SetBoneRotationByName(B,R,EBoneSpaces::ComponentSpace);}}
 if(DialogueBeat<0 && T>=Talk){DialogueBeat=0;Clock=Talk+2.5f;T=Talk;Director->Conversation({TEXT("officer_arrest"),Director->Chapter==0?FName(TEXT("driver_remorse")):Director->Chapter==1?FName(TEXT("student_protest")):FName(TEXT("trucker_remorse"))});if(Director->Chapter==1)Director->DialogueQueue.Insert(TEXT("student_regret"),1);}
 if(DialogueBeat==0 && T>=Reaction){DialogueBeat=1;Director->Say(Director->ChapterStars>=4?TEXT("hero_relief"):TEXT("hero_partial"));}
 CuffedCount=FMath::Clamp(FMath::FloorToInt((T-Talk-3.8f)/5)+1,0,Count);
 CelebrationStarted=T>=Reaction && Director->ChapterStars>=4;
 AftermathBeat=T<3?TEXT("Parking"):T<8.8f?TEXT("Exiting"):T<Talk?TEXT("Approaching"):T<=Talk && !Director->Notice.IsEmpty()?TEXT("Dialogue"):T<CuffEnd?TEXT("Cuffing"):T<EscortStart?TEXT("Secured"):T<BoardStart?TEXT("Escort"):T<BoardEnd?TEXT("Boarding"):T<EntryStart?TEXT("OfficerReturn"):T<LeaveStart?TEXT("OfficerEntry"):T<Reaction?TEXT("Departing"):TEXT("Reaction");
 Hero->GetOwner()->SetActorLocationAndRotation(HeroP,FRotator(0,Forward.Rotation().Yaw+90,0));Pose(Hero,CelebrationStarted?2:T>=Reaction && !Director->Notice.IsEmpty()?6:0,PerformanceClock);GroundPerformance(Hero,HeroP,0);

 float FrontOpen=T<8.8f?ExitDoor(T-3):T>=EntryStart?EntryDoor(T-EntryStart):0;
 int BoardIndex=FMath::Clamp(FMath::FloorToInt((T-BoardStart)/9.f),0,Count-1);float BoardTime=T-BoardStart-BoardIndex*9.f,BoardExtra=BoardIndex>0?.5f:0;
 float RearOpen=T>=BoardStart && T<BoardEnd?Ease(.35f,1.2f,BoardTime)*(1-Ease(6.4f+BoardExtra,7.4f+BoardExtra,BoardTime)):0;
 PoliceDoor(PoliceCar,TEXT("FrontL"),FrontOpen);PoliceDoor(PoliceCar,TEXT("RearL"),RearOpen);DoorAudio(TEXT("PoliceFront"),FrontOpen,PoliceCar->GetActorLocation());DoorAudio(TEXT("PoliceRear"),RearOpen,PoliceCar->GetActorLocation());
 auto BoundHands=[&](UPoseableMeshComponent* M,FVector Facing,float Alpha){FVector Side=FVector::CrossProduct(FVector::UpVector,Facing),Pelvis=M->GetBoneLocationByName(TEXT("pelvis"),EBoneSpaces::WorldSpace),Center=Pelvis+Facing*29+FVector(0,0,7);
  ReachHand(M,true,FMath::Lerp(M->GetBoneLocationByName(TEXT("Hand_L"),EBoneSpaces::WorldSpace),Center-Side*8,Alpha));ReachHand(M,false,FMath::Lerp(M->GetBoneLocationByName(TEXT("Hand_R"),EBoneSpaces::WorldSpace),Center+Side*8,Alpha));EchoPose::SoftenWrists(M);
 };
 for(int J=0;J<Count;++J){auto M=People[J+1];float Local=T-Talk-J*5,Enter=T-BoardStart-J*9.f;FVector P=GroupP(J),Facing=Forward;
  FVector Wait=CarPoint(FVector(-170-J*110,-235-J*105,0));
  if(T<EscortStart){M->GetOwner()->SetActorLocationAndRotation(P,FRotator(0,Forward.Rotation().Yaw-90,0));bool Talking=Director->DialogueID==TEXT("driver_remorse") || Director->DialogueID==TEXT("trucker_remorse") || (Director->DialogueID==TEXT("student_protest") && M->GetSkinnedAsset()->GetName().Contains(TEXT("SchoolBoy"))) || (Director->DialogueID==TEXT("student_regret") && M->GetSkinnedAsset()->GetName().Contains(TEXT("SchoolGirl")));Pose(M,Talking && T<=Talk?6:0,PerformanceClock);GroundPerformance(M,P,0);}
  else if(Enter<0){float U=Ease(EscortStart,EscortEnd,T);P=FMath::Lerp(P,Wait,U);Facing=(Wait-GroupP(J)).GetSafeNormal();M->GetOwner()->SetActorLocationAndRotation(P,FRotator(0,Facing.Rotation().Yaw-90,0));Pose(M,T<EscortEnd?1:0,PerformanceClock);GroundPerformance(M,P,0);}
  else {
   // The officer opens the rear door. Bound passengers use the entry body clip;
   // their hands stay together, instead of reaching for a handle through the cuff.
   float Delay=J>0?2.5f:2.f,Entry=FMath::Clamp(Enter-Delay,0.f,5.65f);
   // Sedan cushions sit lower than the pickup used by the source motion.
   // The first passenger slides across the bench before the second boards.
   float Across=Count>1 && J==0?86*Ease(3.8f,5.6f,Entry):0;
   P=CarPoint(FVector(-103,-205+Across,-27*Ease(2.2f,4.2f,Entry)));
   if(Enter<Delay){FVector End=P;P=FMath::Lerp(Wait,End,Ease(.9f,Delay,Enter));Facing=(End-Wait).GetSafeNormal();M->GetOwner()->SetActorLocationAndRotation(P,FRotator(0,Facing.Rotation().Yaw-90,0));Pose(M,Enter>.9f?1:0,PerformanceClock);GroundPerformance(M,P,0);}
   else {M->GetOwner()->SetActorLocationAndRotation(P,FRotator(0,PoliceCar->GetActorRotation().Yaw,0));Pose(M,13,PerformanceClock,Entry);}
   PoliceFootwell(M,PoliceCar,Ease(2.6f,4.2f,Entry));
   if(Enter>=Delay)Facing=FMath::Lerp(PoliceCar->GetActorRightVector(),PoliceCar->GetActorForwardVector(),Ease(.8f,3.f,Entry)).GetSafeNormal();
  }
  float Restrain=Ease(.65f,1.5f,Local);BoundHands(M,Facing,Restrain);
  if(Cuffs.IsValidIndex(J))DrawCuffs(Cuffs[J],M,Ease(1.6f,3.4f,Local),Local>=1.5f);
 }

 FVector OfficerP,OfficerFacing;int Gesture=0;bool Ground=true;
 FVector TalkP=GroupP(0)+Forward*75,ExitEnd=CarPoint(FVector(17,-208,0));
 if(T<3){OfficerP=CarPoint(FVector(0,-40,5));OfficerFacing=Forward;Gesture=15;Ground=false;}
 else if(T<8.8f){float E=T-3,Crouch=24*Ease(0,.45f,E)*(1-Ease(2.7f,3.7f,E));OfficerP=CarPoint(FVector(0,-44,5*(1-Ease(.9f,2.7f,E))-Crouch));OfficerFacing=Forward;Gesture=14;Ground=false;}
 else if(T<Talk){float U=Ease(8.8f,Talk,T);FVector Bend=Origin-Forward*525-Right*140;
  OfficerP=FMath::Square(1-U)*ExitEnd+2*(1-U)*U*Bend+U*U*TalkP;
  FVector Tangent=((Bend-ExitEnd)*(1-U)+(TalkP-Bend)*U).GetSafeNormal();OfficerFacing=FMath::Lerp(Tangent,-Forward,Ease(.88f,1,U)).GetSafeNormal();Gesture=1;}
 else if(T<EscortStart){int J=FMath::Clamp(FMath::FloorToInt((T-Talk)/5),0,Count-1);float Local=T-Talk-J*5;FVector From=GroupP(FMath::Max(0,J-1))+Forward*75,To=GroupP(J)+Forward*75;OfficerP=FMath::Lerp(From,To,Ease(0,.85f,Local));OfficerFacing=-Forward;Gesture=T==Talk && Director->DialogueID==TEXT("officer_arrest")?6:J>0 && Local<.85f?1:0;}
 else if(T<BoardStart){FVector From=GroupP(Count-1)+Forward*75,To=CarPoint(FVector(-82,-136,0));
  FVector B=Origin-Forward*420-Right*720,C=Origin-Forward*800-Right*600;float U=Ease(EscortStart,EscortEnd,T),V=1-U;
  OfficerP=V*V*V*From+3*V*V*U*B+3*V*U*U*C+U*U*U*To;OfficerFacing=(V*V*(B-From)+2*V*U*(C-B)+U*U*(To-C)).GetSafeNormal();Gesture=T<EscortEnd?1:0;}
 else if(T<ReturnStart){FVector Handle=FVector(-7,-94,86)+FRotator(0,74*RearOpen,0).RotateVector(FVector(-95,-2,0));FVector Close=CarPoint(FVector(Handle.X+20,Handle.Y-40,0));
  FVector AtHandle=Close,Safe=CarPoint(FVector(50,-245,0));
  float Away=Ease(1.1f,1.9f,BoardTime)*(1-Ease(6.f+BoardExtra,6.9f+BoardExtra,BoardTime));
  bool MovingAway=BoardTime>1.1f && BoardTime<1.9f,MovingBack=BoardTime>6+BoardExtra && BoardTime<6.9f+BoardExtra;
  OfficerP=FMath::Lerp(AtHandle,Safe,Away);OfficerFacing=MovingAway?(Safe-AtHandle).GetSafeNormal():MovingBack?(AtHandle-Safe).GetSafeNormal():Right;Gesture=MovingAway || MovingBack?1:0;}
 else if(T<EntryStart){OfficerP=FMath::Lerp(CarPoint(FVector(-82,-136,0)),CarPoint(FVector(12,-202,0)),Ease(ReturnStart,EntryStart,T));OfficerFacing=(Forward*94-Right*66).GetSafeNormal();Gesture=1;}
 else {OfficerP=CarPoint(FVector(12,-202,-25.5f*Ease(2.2f,4.2f,T-EntryStart)));OfficerFacing=PoliceCar->GetActorRightVector();Gesture=13;Ground=false;}
 auto OA=Officer->GetOwner();OA->SetActorLocationAndRotation(OfficerP,FRotator(0,OfficerFacing.Rotation().Yaw-90,0));
 Pose(Officer,Gesture,PerformanceClock,Gesture==14?T-3:Gesture==13?FMath::Min(T-EntryStart,5.65f):-1);
 if(Ground)GroundPerformance(Officer,OfficerP,0);
 if(T>=3 && T<8.8f)PoliceFootwell(Officer,PoliceCar,Ease(.15f,.6f,T-3),true);
 if(T>=EntryStart)PoliceFootwell(Officer,PoliceCar,Ease(2.6f,4.2f,T-EntryStart));
 if(T>=3 && T<8.8f)PoliceDoorHand(Officer,PoliceCar,false,FrontOpen,Ease(.3f,.6f,T-3)*(1-Ease(4.6f,4.9f,T-3)));
 if(T>=Talk && T<CuffEnd){int J=FMath::Clamp(FMath::FloorToInt((T-Talk)/5),0,Count-1);float L=T-Talk-J*5,Weight=Ease(.9f,1.7f,L)*(1-Ease(3.7f,4.6f,L));auto Suspect=People[J+1];
  for(bool Left:{true,false}){FName H=Left?TEXT("Hand_L"):TEXT("Hand_R"),Other=Left?TEXT("Hand_R"):TEXT("Hand_L");FVector Target=Suspect->GetBoneLocationByName(Other,EBoneSpaces::WorldSpace)+FVector(0,0,2);ReachHand(Officer,Left,FMath::Lerp(Officer->GetBoneLocationByName(H,EBoneSpaces::WorldSpace),Target,Weight));}EchoPose::SoftenWrists(Officer);
 }
 if(T>=BoardStart && T<BoardEnd){float OpenReach=Ease(.15f,.4f,BoardTime)*(1-Ease(1.1f,1.6f,BoardTime)),CloseReach=Ease(6+BoardExtra,6.5f+BoardExtra,BoardTime)*(1-Ease(7.4f+BoardExtra,7.8f+BoardExtra,BoardTime));PoliceDoorHand(Officer,PoliceCar,true,RearOpen,FMath::Max(OpenReach,CloseReach));}
 if(T>=EntryStart && T<EntryStart+5.8f)PoliceDoorHand(Officer,PoliceCar,false,FrontOpen,Ease(.65f,1.f,T-EntryStart)*(1-Ease(4.5f,4.8f,T-EntryStart)));

 if(T<3)Shot(PoliceCar->GetActorLocation()+Forward*280-Right*650+FVector(0,0,300),PoliceCar->GetActorLocation()+FVector(0,0,95),Dt);
 else if(T<9)Shot(PoliceCar->GetActorLocation()+Forward*220-Right*470+FVector(0,0,185),PoliceCar->GetActorLocation()-Right*70+FVector(0,0,92),Dt);
 else if(T<Talk)Shot(Origin-Forward*900-Right*750+FVector(0,0,230),GroupP(0)+FVector(0,0,100),Dt);
 else if(T<CuffEnd){int J=FMath::Clamp(FMath::FloorToInt((T-Talk)/5),0,Count-1);Shot(GroupP(J)+Forward*240-Right*230+FVector(0,0,165),GroupP(J)+Forward*34+FVector(0,0,105),Dt);}
 else if(T<EscortEnd){
  // The narrow truck intersection has a lamp and facade on the near side.
  // Cut to the open road side for this wide shot; do not fly through the cast.
  if(Director->Chapter==2){if(T-Dt<CuffEnd)ShotInitialized=false;Shot(Origin-Forward*1430+Right*550+FVector(0,0,650),Origin-Forward*830-Right*200+FVector(0,0,100),Dt);}
  else Shot(Origin-Forward*900-Right*1100+FVector(0,0,360),Origin-Forward*900-Right*330+FVector(0,0,90),Dt);
 }
 else if(T<EntryStart){if(Director->Chapter==2 && T-Dt<EscortEnd)ShotInitialized=false;Shot(PoliceCar->GetActorLocation()+Forward*100-Right*540+FVector(0,0,195),PoliceCar->GetActorLocation()-Forward*70-Right*125+FVector(0,0,90),Dt);}
 else if(T<LeaveStart)Shot(PoliceCar->GetActorLocation()+Forward*220-Right*470+FVector(0,0,185),PoliceCar->GetActorLocation()-Right*70+FVector(0,0,92),Dt);
 else if(T<Reaction)Shot(Origin-Forward*850-Right*650+FVector(0,0,280),PoliceCar->GetActorLocation()+FVector(0,0,95),Dt);
 else Shot(HeroP-Forward*360-Right*240+FVector(0,0,210),HeroP+FVector(0,0,110),Dt);
 if(T>Reaction+5 && Director->Notice.IsEmpty()){Director->BeginDeparture();SetActorTickEnabled(false);}
}
