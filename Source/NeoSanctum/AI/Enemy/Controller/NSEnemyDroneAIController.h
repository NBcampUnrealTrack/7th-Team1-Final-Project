// Copyright 2026 One Team. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "NSEnemyControllerBase.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "NSEnemyDroneAIController.generated.h"

class UAIPerceptionComponent;
class UNSEnemyThreatComponent;
class UNSEnemyAttackComponent;
class UNSEnemyTargetComponent;
class UNSEnemyStateComponent;
struct FNSEnemyAttackRow;

UCLASS()
class NEOSANCTUM_API ANSEnemyDroneAIController : public ANSEnemyControllerBase
{
    GENERATED_BODY()

public:
    ANSEnemyDroneAIController();

    virtual void Tick(float DeltaTime) override;

    virtual FGenericTeamId GetGenericTeamId() const override
    {
        return FGenericTeamId(static_cast<uint8>(ETeamId::Enemy));
    }
    virtual ETeamAttitude::Type GetTeamAttitudeTo(const AActor& Other) const;

    // ── 비행 공격 태스크(NSBTTask_FlyingExecuteAbility)가 호출 (4번 드론 분기) ──
    const FNSEnemyAttackRow* GetAttackRowByDistance();
    void RecordAttackUsed(const FNSEnemyAttackRow& AttackRow);

    // 현재 전투 타겟 (Tick 회전 동기화 / 카이팅 서비스에서 사용)
    AActor* GetCurrentTargetActor() const;

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
    // 타겟
    void UpdateTargetSelection();
    void UpdateCurrentTargetBlackboard();
    void ResetTargetingState();

    // 공격 선택
    const FNSEnemyAttackRow* FindAttackRowByDistance(bool bSelectWeightedAttack);
    bool CanUseAnyAttackByDistance();

    // 상태 판정
    bool IsDroneAIBlocked() const;
    bool IsValidLivingTarget(const AActor* Target) const;

    // 블랙보드
    void InitBBState();
    void SetCanAttackBB(bool bCanAttack);
    void SetIsAttackingBB(bool bIsAttacking);
    void ClearAttackState();
    void ClearTargetBB(bool bClearCanAttack);

    // 컴포넌트 접근
    UNSEnemyThreatComponent* GetEnemyThreatComponent() const;
    UNSEnemyAttackComponent* GetEnemyAttackComponent() const;
    UNSEnemyTargetComponent* GetEnemyTargetComponent() const;
    UNSEnemyStateComponent* GetEnemyStateComponent() const;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

    // 블랙보드 키 (보스와 동일 이름으로 맞춰 BT 재사용)
    FName TargetActorKey = TEXT("TargetActor");
    FName TargetLastKnownLocationKey = TEXT("TargetLastKnownLocation");
    FName HasTargetLineOfSightKey = TEXT("bHasTargetLineOfSight");
    FName CanAttackKey = TEXT("bCanAttack");
    FName IsAttackingKey = TEXT("bIsAttacking");
};