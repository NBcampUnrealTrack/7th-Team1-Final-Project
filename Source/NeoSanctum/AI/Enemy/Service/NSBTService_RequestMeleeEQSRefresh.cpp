#include "NSBTService_RequestMeleeEQSRefresh.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"

UNSBTService_RequestMeleeEQSRefresh::UNSBTService_RequestMeleeEQSRefresh()
{
	NodeName = TEXT("Request Melee EQS Refresh");

	bCreateNodeInstance = true;
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;

	Interval = 0.25f;
	RandomDeviation = 0.05f;

	TargetActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UNSBTService_RequestMeleeEQSRefresh, TargetActorKey),
		AActor::StaticClass());

	NeedsRefreshKey.AddBoolFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UNSBTService_RequestMeleeEQSRefresh, NeedsRefreshKey));

	HasLineOfSightKey.AddBoolFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UNSBTService_RequestMeleeEQSRefresh, HasLineOfSightKey));
}

void UNSBTService_RequestMeleeEQSRefresh::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BlackboardAsset);
		NeedsRefreshKey.ResolveSelectedKey(*BlackboardAsset);
		HasLineOfSightKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

void UNSBTService_RequestMeleeEQSRefresh::OnBecomeRelevant(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	bTrackingLostSight = false;
	LostSightStartTime = 0.0;
	LastSearchRequestTime = -1000.0;
}

void UNSBTService_RequestMeleeEQSRefresh::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	UWorld* World = OwnerComp.GetWorld();

	if (!AIController ||
		!AIController->HasAuthority() ||
		!Blackboard ||
		!World)
	{
		return;
	}

	AActor* TargetActor =
		Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));

	if (!IsValid(TargetActor))
	{
		Blackboard->SetValueAsBool(NeedsRefreshKey.SelectedKeyName, false);

		bTrackingLostSight = false;
		LostSightStartTime = 0.0;
		return;
	}

	APawn* Pawn = AIController->GetPawn();

	if (!Pawn)
	{
		Blackboard->SetValueAsBool(NeedsRefreshKey.SelectedKeyName, false);

		bTrackingLostSight = false;
		LostSightStartTime = 0.0;
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();

	const bool bPerceptionHasLineOfSight = Blackboard->GetValueAsBool(HasLineOfSightKey.SelectedKeyName);
	const bool bControllerHasLineOfSight = AIController->LineOfSightTo(TargetActor);
	const bool bHasLineOfSight = bPerceptionHasLineOfSight || bControllerHasLineOfSight;

	const float DistanceToTarget2D = FVector::Dist2D(
		Pawn->GetActorLocation(),
		TargetActor->GetActorLocation());

	const bool bTargetIsCloseEnough = DistanceToTarget2D <= DirectChaseDistance;

	// 타깃이 보이거나 충분히 가까우면 EQS 정찰을 하지 않고 직접 추적
	if (bHasLineOfSight || bTargetIsCloseEnough)
	{
		Blackboard->SetValueAsBool(NeedsRefreshKey.SelectedKeyName, false);

		bTrackingLostSight = false;
		LostSightStartTime = 0.0;
		return;
	}

	// 타깃이 보이지 않고, 직접 추적 거리 밖에 있는 상태
	if (!bTrackingLostSight)
	{
		bTrackingLostSight = true;
		LostSightStartTime = CurrentTime;

		Blackboard->SetValueAsBool(NeedsRefreshKey.SelectedKeyName, false);
		return;
	}

	const double LostSightElapsedTime = CurrentTime - LostSightStartTime;

	// 시야를 잃은 지 3초가 지나기 전까지는 EQS 정찰로 넘기지 않음
	if (LostSightElapsedTime < LostSightSearchDelay)
	{
		Blackboard->SetValueAsBool(NeedsRefreshKey.SelectedKeyName, false);
		return;
	}

	const bool bAlreadyNeedsRefresh = Blackboard->GetValueAsBool(NeedsRefreshKey.SelectedKeyName);

	if (bAlreadyNeedsRefresh)
	{
		return;
	}

	// 정찰 요청이 너무 자주 반복되지 않도록 간격을 둠
	if (CurrentTime - LastSearchRequestTime < SearchRequestInterval)
	{
		return;
	}

	Blackboard->SetValueAsBool(NeedsRefreshKey.SelectedKeyName, true);
	LastSearchRequestTime = CurrentTime;
}
