// Copyright 2026 One Team. All rights reserved.


#include "NSBTService_ComputeKitePoint.h"

#include "BehaviorTree/BlackboardData.h"

UNSBTService_ComputeKitePoint::UNSBTService_ComputeKitePoint()
{
	NodeName = "Compute Kite Point";
	bNotifyTick = true;
	
	TargetActorKey.AddVectorFilter(
		this, GET_MEMBER_NAME_CHECKED(UNSBTService_ComputeKitePoint, TargetActorKey));
}

void UNSBTService_ComputeKitePoint::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if ()
}

void UNSBTService_ComputeKitePoint::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
}
