#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoJourneyRules.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoJourneyBalanceTest,"Echo.Journey.BalanceAndEndings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEchoJourneyBalanceTest::RunTest(const FString& Parameters)
{
 for(int C=0;C<3;++C){int E=C==2?3:2,R=C==1?4:3;TestEqual(TEXT("Every chapter has equal maximum score"),FEchoJourneyRules::Score(E,E,R,R,true),1000);TestEqual(TEXT("Rescue only preserves half the score"),FEchoJourneyRules::Score(0,E,R,R,false),500);TestEqual(TEXT("Evidence and deduction alone cannot earn five stars"),FEchoJourneyRules::Score(E,E,0,R,true),500);}
 TestEqual(TEXT("Empty case score"),FEchoJourneyRules::Score(0,2,0,3,false),0);
 const int Thresholds[]={0,349,350,599,600,799,800,949,950,1000};const int Stars[]={1,1,2,2,3,3,4,4,5,5};for(int I=0;I<10;++I)TestEqual(TEXT("Star boundary"),FEchoJourneyRules::Stars(Thresholds[I]),Stars[I]);
 auto R=FEchoJourneyRules::Preset(EEchoBalance::Journey);TestEqual(TEXT("An empty first standard case returns home"),R.StartingMood+R.MoodChange(0,true),0);TestEqual(TEXT("Perfect case improves mood"),R.MoodChange(1000,false),18);TestEqual(TEXT("Mixed case leaves recovery possible"),R.MoodChange(600,false),-2);
 TestEqual(TEXT("Three safe escapes reach joyful ending"),FEchoJourneyRules::Ending(R.StartingMood+3*R.AvoidanceMood),3);
 TestEqual(TEXT("Zero goes home"),FEchoJourneyRules::Ending(0),0);TestEqual(TEXT("One still reaches destination"),FEchoJourneyRules::Ending(1),1);TestEqual(TEXT("Calm lower threshold"),FEchoJourneyRules::Ending(35),2);TestEqual(TEXT("Joy lower threshold"),FEchoJourneyRules::Ending(75),3);
 TestTrue(TEXT("Story gives more time"),FEchoJourneyRules::Preset(EEchoBalance::Story).InvestigationSeconds>R.InvestigationSeconds);TestTrue(TEXT("Tension gives less time"),FEchoJourneyRules::Preset(EEchoBalance::Tension).InvestigationSeconds<R.InvestigationSeconds);
 TestTrue(TEXT("Story allows recovery from one empty case"),FEchoJourneyRules::Preset(EEchoBalance::Story).StartingMood>FEchoJourneyRules::Preset(EEchoBalance::Story).EmptyCaseLoss);
 return true;
}
#endif

