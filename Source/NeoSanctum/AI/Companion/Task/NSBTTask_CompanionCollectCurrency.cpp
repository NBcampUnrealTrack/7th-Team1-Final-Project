// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_CompanionCollectCurrency.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/System/Subsystem/NSCurrencyDropSubsystem.h"

UNSBTTask_CompanionCollectCurrency::UNSBTTask_CompanionCollectCurrency()
{
	NodeName = "재화 수집";
	
	TargetDropIdKey.AddIntFilter(
	this,
	GET_MEMBER_NAME_CHECKED(UNSBTTask_CompanionCollectCurrency, TargetDropIdKey));
}

EBTNodeResult::Type UNSBTTask_CompanionCollectCurrency::ExecuteTask(UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;
	
	int32 DropId = BB->GetValueAsInt(TargetDropIdKey.SelectedKeyName);
	if (DropId == INDEX_NONE) return EBTNodeResult::Failed;
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;
	
	ANSBaseCompanionAI* CompanionPawn = Cast<ANSBaseCompanionAI>(AIController->GetPawn());
	if (!CompanionPawn) return EBTNodeResult::Failed;
	
	FVector CompanionLocation = CompanionPawn->GetActorLocation();
	
	AActor* CompanionOwner = CompanionPawn->GetOwnerPlayer();
	if (!CompanionOwner) return EBTNodeResult::Failed;
	
	// OwnerPawn 가져오기
	APawn* OwnerPawn = Cast<APawn>(CompanionOwner);
	if (!OwnerPawn) return EBTNodeResult::Failed;
	
	// OwnerPlayerState 가져오기
	ANSPlayerState* OwnerPS = OwnerPawn->GetPlayerState<ANSPlayerState>();
	if (!OwnerPS) return EBTNodeResult::Failed;
	
	// 재화쪽 SubSystem 가져오기
	UNSCurrencyDropSubsystem* DropSubsystem = CompanionPawn->GetWorld()->GetSubsystem<UNSCurrencyDropSubsystem>();
	if (!DropSubsystem) return EBTNodeResult::Failed;
	
	if (DropSubsystem->TryCollectByCompanion(DropId,OwnerPS, CompanionLocation))
	{
		BB->SetValueAsInt(TargetDropIdKey.SelectedKeyName, INDEX_NONE);
		return EBTNodeResult::Succeeded;
	}
	else
	{
		return EBTNodeResult::Failed;
	}
}
