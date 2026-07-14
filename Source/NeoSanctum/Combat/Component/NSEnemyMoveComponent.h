// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSEnemyMoveComponent.generated.h"

class AAIController;
class ANSEnemyCharacterBase;
class UNSEnemyData;

struct FNSRetreatResult
{
	bool bShouldRetreat = false;
	bool bHasLocation = false;
	FVector Location = FVector::ZeroVector;
};

// 타깃 이동 위치 해석 결과 타입을 나타내는 열거형
UENUM(BlueprintType)
enum class ENSTargetMoveResolveType : uint8
{
	Actual UMETA(DisplayName = "Actual"),
	GroundProjected UMETA(DisplayName = "Ground Projected"),
	NearbyProjected UMETA(DisplayName = "Nearby Projected"),
	LastReachable UMETA(DisplayName = "Last Reachable"),
	Invalid UMETA(DisplayName = "Invalid")
};

// 타깃 이동 위치 해석 결과를 전달하는 구조체
USTRUCT(BlueprintType)
struct FNSResolvedTargetMoveResult
{
	GENERATED_BODY()

	// 타깃의 실제 월드 위치를 나타내는 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector ActualLocation = FVector::ZeroVector;

	// AI가 실제로 MoveTo에 사용할 위치를 나타내는 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector MoveLocation = FVector::ZeroVector;

	// 이동 위치가 어떤 방식으로 해석됐는지 나타내는 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ENSTargetMoveResolveType ResolveType = ENSTargetMoveResolveType::Invalid;

	// MoveLocation이 유효한 이동 위치인지 나타내는 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasMoveLocation = false;

	// 타깃이 현재 공중 상태인지 나타내는 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bTargetAirborne = false;

	// 몬스터가 공중 타깃 아래 이동 지점에 도착했는지 나타내는 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bArrivedBelowAirborneTarget = false;
};

/**
 * 일반 Enemy의 후퇴 위치 계산과 전투 중 회전 방향 제어를 담당하는 컴포넌트입니다.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyMoveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyMoveComponent();

	// 현재 타겟과 거리 기준으로 후퇴 여부와 후퇴 위치를 계산하는 함수
	FNSRetreatResult UpdateRetreat(
		AActor* TargetActor,
		bool bWasRetreating,
		bool bHasCurrentLocation,
		const FVector& CurrentLocation);

	// 후퇴 상태를 초기화하는 함수
	void ClearRetreat();

	// 공격/후퇴/준비 상태에 따라 이동 회전 또는 타겟 바라보기를 적용하는 함수
	void ApplyFacing(AAIController* Controller, AActor* TargetActor, AActor* AimActor, bool bFaceTarget);

	// 현재 거리에서 사용 가능한 공격이 있는지 확인하는 함수
	bool IsWithinAttackRange(AActor* TargetActor) const;

protected:
	// 후퇴 종료 거리 보정값. 경계에서 전진/후퇴가 반복되는 것을 줄임
	UPROPERTY(EditDefaultsOnly, Category = "Move|Retreat")
	float RetreatExitBuffer = 100.0f;

	// 새 후퇴 지점을 계산할 때 기본으로 떨어질 거리
	UPROPERTY(EditDefaultsOnly, Category = "Move|Retreat")
	float RetreatStepDistance = 250.0f;

	// 현재 후퇴 지점에 도착했다고 판단할 거리
	UPROPERTY(EditDefaultsOnly, Category = "Move|Retreat")
	float RetreatAcceptanceRadius = 75.0f;

private:
	ANSEnemyCharacterBase* GetOwnerEnemy() const;
	const UNSEnemyData* GetEnemyData() const;
	float GetMinAttackRange() const;

#pragma region NavMesh 밖 혹은 끼었을 때 텔레포트

public:
	// NavMesh 이탈 또는 이동 정체 상태를 감시하고 필요 시 복구 텔레포트를 수행하는 함수
	void UpdateNavigationRecovery(AAIController* Controller, float DeltaTime);

	// NavMesh 복구 감시 상태를 초기화하는 함수
	void ResetNavigationRecovery();

protected:
	// NavMesh 복구 텔레포트를 수행하기 전까지 대기할 시간을 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Recovery")
	float NavRecoveryDelay = 5.0f;

	// 몬스터가 이동했다고 판단할 최소 2D 이동 거리를 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Recovery")
	float StuckMoveTolerance = 25.0f;

	// 몬스터가 거의 정지했다고 판단할 최대 2D 속도를 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Recovery")
	float StuckVelocityTolerance = 10.0f;

	// 현재 위치가 NavMesh 투영 위치에서 벗어났다고 판단할 허용 거리를 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Recovery")
	float NavOutsideTolerance = 150.0f;

	// 복구 텔레포트 위치를 탐색할 반경을 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Recovery")
	float RecoverySearchRadius = 700.0f;

	// 현재 위치를 NavMesh에 투영할 때 사용할 탐색 범위를 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Recovery")
	FVector NavProjectionExtent = FVector(300.0f, 300.0f, 500.0f);

private:
	// NavMesh 이탈 또는 이동 정체 상태가 지속된 시간을 누적하는 변수
	float NavRecoveryElapsed = 0.0f;

	// 마지막 감시 위치가 기록되어 있는지 나타내는 변수
	bool bHasLastObservedLocation = false;

	// 이동 정체 여부를 판단하기 위해 마지막으로 기록한 위치를 나타내는 변수
	FVector LastObservedLocation = FVector::ZeroVector;

	// 마지막 유효 NavMesh 위치가 기록되어 있는지 나타내는 변수
	bool bHasLastValidNavLocation = false;

	// 복구 위치 탐색 기준으로 사용할 마지막 유효 NavMesh 위치를 나타내는 변수
	FVector LastValidNavLocation = FVector::ZeroVector;
#pragma endregion

#pragma region 플레이어 체공 시에도 플레이어 위치 추적

public:
	// 현재 타깃 Actor를 NavMesh 기반 이동 위치로 해석하는 함수
	FNSResolvedTargetMoveResult ResolveTargetMoveLocation(
		AActor* TargetActor,
		AAIController* Controller);

	// 타깃 이동 위치 해석 캐시와 마지막 유효 위치를 초기화하는 함수
	void ResetTargetMoveResolveState();

protected:
	// 타깃 이동 위치 해석 기능을 사용할지 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Target Resolve")
	bool bEnableTargetMoveResolve = true;

	// 타깃 아래 지면을 탐색할 때 사용할 Trace Channel을 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Target Resolve")
	TEnumAsByte<ECollisionChannel> TargetGroundTraceChannel = ECC_WorldStatic;

	// 타깃 위치에서 아래 방향으로 지면을 탐색할 최대 거리를 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Target Resolve", meta = (ClampMin = "0.0"))
	float TargetGroundTraceDistance = 3000.0f;

	// 타깃 아래 지면 탐색 Sweep 반경을 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Target Resolve", meta = (ClampMin = "0.0"))
	float TargetGroundSweepRadius = 45.0f;

	// 타깃 바로 아래 위치가 실패했을 때 주변 후보를 탐색할 반경을 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Target Resolve", meta = (ClampMin = "0.0"))
	float NearbySearchRadius = 350.0f;

	// 타깃 주변 이동 후보 위치를 몇 개 생성할지 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Target Resolve", meta = (ClampMin = "1"))
	int32 NearbySampleCount = 8;

	// 지면 후보 위치를 NavMesh에 투영할 때 사용할 탐색 범위를 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Target Resolve")
	FVector TargetNavProjectionExtent = FVector(150.0f, 150.0f, 500.0f);

	// 후보 위치까지 실제 경로 도달 가능성을 검사할지 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Target Resolve")
	bool bCheckTargetMoveReachability = true;

	// 후보 위치 경로 도달 가능성 검사를 다시 수행할 최소 간격을 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Target Resolve", meta = (ClampMin = "0.05"))
	float ReachabilityCheckInterval = 0.35f;

	// 공중 타깃 아래 위치에 도착했다고 판단할 2D 거리를 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Target Resolve", meta = (ClampMin = "0.0"))
	float ArriveBelowDistance = 140.0f;

	// 타깃을 공중 대응 대상으로 볼 최소 Z축 높이 차이를 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Target Resolve", meta = (ClampMin = "0.0"))
	float MinimumAirborneHeight = 180.0f;

private:
	// 타깃 Actor가 현재 공중 상태인지 확인하는 함수
	bool IsTargetAirborne(const AActor* TargetActor) const;

	// 지정 위치 아래의 지면 충돌 위치를 탐색하는 함수
	bool TraceGroundLocation(
		const AActor* TargetActor,
		const FVector& SourceLocation,
		FVector& OutGroundLocation) const;

	// 지정 위치를 NavMesh 위 위치로 투영하는 함수
	bool ProjectToNavigation(
		const FVector& SourceLocation,
		FVector& OutNavLocation) const;

	// 타깃 주변에서 도달 가능한 이동 후보 위치를 탐색하는 함수
	bool FindNearbyReachableLocation(
		const AActor* TargetActor,
		const FVector& TargetLocation,
		AAIController* Controller,
		FVector& OutLocation);

	// 지정 후보 위치까지 현재 몬스터가 도달 가능한지 확인하는 함수
	bool IsReachableMoveLocation(
		AAIController* Controller,
		const FVector& CandidateLocation);

	// 마지막으로 도달 가능했던 이동 위치를 기록하는 함수
	void MarkReachableLocation(const FVector& Location);

	// 마지막으로 도달 가능했던 타깃 이동 위치가 있는지 나타내는 변수
	bool bHasLastReachableMoveLocation = false;

	// 마지막으로 도달 가능했던 타깃 이동 위치를 저장하는 변수
	FVector LastReachableMoveLocation = FVector::ZeroVector;

	// 경로 도달 가능성 검사 캐시가 유효한지 나타내는 변수
	bool bHasReachabilityCache = false;

	// 마지막으로 경로 도달 가능성을 검사한 후보 위치를 저장하는 변수
	FVector CachedReachabilityLocation = FVector::ZeroVector;

	// 마지막 경로 도달 가능성 검사 결과를 저장하는 변수
	bool bCachedReachabilityResult = false;

	// 마지막으로 경로 도달 가능성 검사를 수행한 월드 시간을 저장하는 변수
	double LastReachabilityCheckTime = -1000.0;
#pragma endregion
};
