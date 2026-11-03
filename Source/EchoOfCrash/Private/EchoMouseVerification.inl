// Exercise device-axis events through PlayerInput and the normal camera ticks.
// In particular, do not test only ApplyLookInput: that missed the double scaling.
static int LookCase=0, Sent=0;
auto GI=Cast<UEchoGameInstance>(GetGameInstance());if(!GI)return;
struct FLookCase {const TCHAR* Name;float Sens;bool Spirit;int Samples;int Lock;float Sign;};
static const FLookCase Cases[]={
 {TEXT("cockpit raw mouse at 100 percent"),1,false,1,0,1},
 {TEXT("cockpit raw mouse at 200 percent"),2,false,1,0,1},
 {TEXT("cockpit movement split across frames"),1,false,4,0,1},
 {TEXT("spirit raw mouse at 100 percent"),1,true,1,0,1},
 {TEXT("spirit raw mouse at 200 percent"),2,true,1,0,1},
 {TEXT("spirit movement split across frames"),1,true,4,0,1},
 {TEXT("spirit raw mouse at 20 percent"),.2f,true,1,0,1},
 {TEXT("spirit raw mouse at 400 percent"),4,true,1,0,1},
 {TEXT("negative mouse axes turn left and down"),1,true,1,0,-1},
 {TEXT("paused spirit ignores mouse look"),1,true,1,1,1},
 {TEXT("journal blocks mouse look"),1,true,1,2,1},
 {TEXT("awakening still locks mouse look"),1,true,1,3,1},
 {TEXT("paused cockpit ignores mouse look"),1,false,1,1,1}
};
if(Stage==-1){
 MenuAction(TEXT("Settings"));
 GI->LookSensitivity=4;MenuAction(TEXT("SettingPlus2"));Check(TEXT("settings maximum is 400 percent"),FMath::IsNearlyEqual(GI->LookSensitivity,4.f));
 GI->LookSensitivity=.2f;MenuAction(TEXT("SettingMinus2"));Check(TEXT("settings minimum is 20 percent"),FMath::IsNearlyEqual(GI->LookSensitivity,.2f));
 GI->LookSensitivity=2.3f;MenuAction(TEXT("SettingPlus2"));GI->LookSensitivity=1;GI->PreferencesLoaded=false;LoadPreferences();
 Check(TEXT("sensitivity above old maximum survives save and reload"),FMath::IsNearlyEqual(GI->LookSensitivity,2.4f));
 MenuAction(TEXT("ResetMouse"));Check(TEXT("reset button restores 100 percent"),GI->LookSensitivity==1);
 MenuPage=0;Phase=EEchoPhase::Introduction;BeginDrive();StopDialogue();Stage=0;
}
else if(Stage==0){
 if(LookCase>=UE_ARRAY_COUNT(Cases)){
  SetPaused(true);MenuAction(TEXT("Settings"));GI->LookSensitivity=1;SavePreferences();Stamp=Now;Stage=3;return;
 }
 const auto& C=Cases[LookCase];
 Paused=false;JournalOpen=false;InspectIndex=INDEX_NONE;StopDialogue();SetCursor(false);
 Phase=C.Spirit?EEchoPhase::Investigation:EEchoPhase::Driving;PhaseTime=0;TimeLeft=150;
 GI->LookSensitivity=C.Sens;CockpitYaw=CockpitPitch=0;PC->SetControlRotation(FRotator::ZeroRotator);
 P->SwimVelocity=FVector::ZeroVector;P->SetActorLocation(Center+FVector(0,0,300));
 if(C.Lock==1)SetPaused(true);
 if(C.Lock==2){JournalOpen=true;SetCursor(true);}
 if(C.Lock==3){Phase=EEchoPhase::Awakening;AwakeningDialoguePending=true;}
 Sent=0;Stage=1;
}
else if(Stage==1){
 const auto& C=Cases[LookCase];
 if(Sent<C.Samples){
  PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::MouseX,20.f*C.Sign/C.Samples,GetWorld()->GetDeltaSeconds(),1,FPlatformTime::Cycles64()));
  PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::MouseY,10.f*C.Sign/C.Samples,GetWorld()->GetDeltaSeconds(),1,FPlatformTime::Cycles64()));
  ++Sent;
 }else{Stamp=Now;Stage=2;}
}
else if(Stage==2 && Now-Stamp>.2){
 const auto& C=Cases[LookCase];const auto R=PC->GetControlRotation();
 const float Yaw=C.Spirit?FRotator::NormalizeAxis(R.Yaw):CockpitYaw;
 const float Pitch=C.Spirit?FRotator::NormalizeAxis(R.Pitch):CockpitPitch;
 const float Expected=C.Lock?0.f:3.6f*C.Sens*C.Sign;
 Check(C.Name,FMath::IsNearlyEqual(Yaw,Expected,.03f) && FMath::IsNearlyEqual(Pitch,Expected*.5f,.03f));
 UE_LOG(LogTemp,Display,TEXT("ECHO_MOUSE_TEST %s yaw=%.4f pitch=%.4f expectedYaw=%.4f"),C.Name,Yaw,Pitch,Expected);
 ++LookCase;Stage=0;
}
else if(Stage==3 && Now-Stamp>.3){
 FString Dir=FPaths::ProjectSavedDir()/TEXT("Verification");
 FScreenshotRequest::RequestScreenshot(Dir/TEXT("mouse-settings.png"),false,false);
 Finish();
}
