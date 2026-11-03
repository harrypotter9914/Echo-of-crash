#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoGameplayLibrary.h"
#include "EchoGameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoGameplayTest, "Echo.Gameplay.QTEAndInvestigation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEchoGameplayTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Inclusive lower boundary"), UEchoGameplayLibrary::IsQTEHit(0.4f, 0.4f, 0.7f));
    TestFalse(TEXT("Exclusive lower boundary"), UEchoGameplayLibrary::IsQTEHit(0.4f, 0.4f, 0.7f, false));
    TestFalse(TEXT("Outside QTE window"), UEchoGameplayLibrary::IsQTEHit(0.8f, 0.4f, 0.7f));
    TestFalse(TEXT("Invalid window"), UEchoGameplayLibrary::IsQTEHit(0.5f, 0.7f, 0.4f));
    TestEqual(TEXT("Rescue completion is capped"), UEchoGameplayLibrary::AdvanceRescueProgress(2.9f, 0.5f, true), 3.0f);
    TestEqual(TEXT("Releasing cancels rescue"), UEchoGameplayLibrary::AdvanceRescueProgress(2.9f, 0.1f, false), 0.0f);
    UEchoGameInstance* State = NewObject<UEchoGameInstance>();
    State->BeginInvestigation(TEXT("AccidentOne"));
    TestTrue(TEXT("First evidence"), State->CollectEvidence(TEXT("Bottle")));
    TestFalse(TEXT("No duplicate evidence"), State->CollectEvidence(TEXT("Bottle")));
    TestFalse(TEXT("No empty evidence"), State->CollectEvidence(NAME_None));
    TestTrue(TEXT("First rescue"), State->RecordRescue(TEXT("Driver")));
    TestFalse(TEXT("No duplicate rescue"), State->RecordRescue(TEXT("Driver")));
    State->BeginInvestigation(TEXT("AccidentTwo"));
    TestEqual(TEXT("New accident clears evidence"), State->CollectedEvidence.Num(), 0);
    TestEqual(TEXT("New accident clears rescues"), State->RescuedPeople.Num(), 0);
    return true;
}
#endif
