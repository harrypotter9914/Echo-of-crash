#include "EchoGameInstance.h"

void UEchoGameInstance::BeginInvestigation(FName AccidentId)
{
    CurrentAccident = AccidentId;
    CollectedEvidence.Reset();
    RescuedPeople.Reset();
    OnInvestigationChanged.Broadcast();
}

bool UEchoGameInstance::CollectEvidence(FName EvidenceId)
{
    if (EvidenceId.IsNone() || CollectedEvidence.Contains(EvidenceId)) return false;
    CollectedEvidence.Add(EvidenceId);
    OnInvestigationChanged.Broadcast();
    return true;
}

bool UEchoGameInstance::RecordRescue(FName PersonId)
{
    if (PersonId.IsNone() || RescuedPeople.Contains(PersonId)) return false;
    RescuedPeople.Add(PersonId);
    OnInvestigationChanged.Broadcast();
    return true;
}
