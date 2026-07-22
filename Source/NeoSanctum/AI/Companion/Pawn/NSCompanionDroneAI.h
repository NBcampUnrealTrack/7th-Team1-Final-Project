// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/AI/Base/NSBaseDroneAI.h"
#include "NeoSanctum/AI/Companion/State/NSCompanionTypes.h"
#include "NeoSanctum/Core/Interface/NSPlayerAttackFeedbackSourceInterface.h"
#include "NSCompanionDroneAI.generated.h"

class UNSCompanionAttributeSet;

UCLASS()
class NEOSANCTUM_API ANSCompanionDroneAI : public ANSBaseDroneAI,
											public INSPlayerAttackFeedbackSourceInterface
{
	GENERATED_BODY()

public:
	ANSCompanionDroneAI();

	virtual void PossessedBy(AController* NewController) override;

	// 드론 공격은 플레이어 히트/킬 피드백(사운드·크로스헤어 마커)을 발생시키지 않음 (터렛과 동일)
	virtual bool ShouldTriggerPlayerAttackFeedback() const override { return false; }
	
protected:
	// @민재 : 베이스의 데이터 초기화 훅 오버라이드. Definition 유효성 확인 후 적용
	virtual void InitializeFromData() override;

public:
	// @민재 : 컴패니언 상태 세팅 (Follow / Idle / Attack 등)
	void SetCurrentState(
		ECompanionState NewState);

private:
	ECompanionState CurrentState = ECompanionState::Follow;
	
	// Collect 진입 시 끈 콜리전을 이탈 시 되돌리기 위한 캐시
	ECollisionEnabled::Type CachedCollisionEnabled = ECollisionEnabled::QueryOnly;

#pragma region Upgrades
public:
	// @민재 : 지정된 노드 태그의 스탯 업그레이드를 새 레벨로 갱신
	void ApplyStatUpgrade(
		FGameplayTag NodeTag,
		int32 NewLevel);

protected:
	UPROPERTY()
	TMap<FGameplayTag, FActiveGameplayEffectHandle> StatUpgradeHandles;

#pragma endregion

#pragma region CachedData
public:
	// @민재 : 이 컴패니언의 소유 플레이어 지정 (스폰 파이프라인에서 호출)
	void SetOwnerPlayer(
		AActor* Actor);                                  // 소유 플레이어 (nullptr 허용: 해제)

	AActor* GetOwnerPlayer() const { return OwnerPlayer; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OwnerPlayer")
	TObjectPtr<AActor> OwnerPlayer;

#pragma endregion

#pragma region OwnerTeleport
public:
	// @민재 : 오너와의 거리 체크. 하드 리쉬 초과 또는 스턱 감지 시 텔포
	UFUNCTION()
	void CheckDistanceToOwner();

	// @민재 : 오너 위치로 즉시 이동. 로코모션 상태도 함께 리셋
	UFUNCTION()
	void TeleportToOwner();

protected:
	FTimerHandle CheckDistanceToOwnerTimer;

	UPROPERTY(EditDefaultsOnly, Category="DroneAI")
	float MaxDistance = 500.f;

	UPROPERTY(EditAnywhere, Category="DroneAI|Leash")
	float TeleportBackOffset = 150.f;
	
private:
	UPROPERTY(EditAnywhere, Category="DroneAI|Leash")
	float HardLeashDistance = 4000.f;

	UPROPERTY(EditAnywhere, Category="DroneAI|Leash")
	float StuckRecoverTime = 2.0f;

	float TimeBeyondLeash = 0.f;
	float PrevDistSqToOwner = -1.f;

	static constexpr float CheckInterval = 0.25f;

#pragma endregion

#pragma region CurrencyVacuum
private:
	// @민재 : 근처 재화 자동 수집. 타이머로 주기 호출
	void VacuumNearbyCurrency();

private:
	UPROPERTY(EditAnywhere, Category="DroneAI|Currency", meta=(AllowPrivateAccess=true))
	float CurrencyVacuumRadius = 250.f;

	// [추가] 재화 수집(Collect) 상태일 때 하강할 목표 고도. 낮을수록 지상에 밀착.
	UPROPERTY(EditAnywhere, Category="DroneAI|Currency", meta=(AllowPrivateAccess=true))
	float CollectAltitude = 60.f;

	// [추가] 평상시 유지 고도 캐시. Collect 이탈 시 이 값으로 복귀한다.
	// PossessedBy에서 로코모션 컴포넌트의 실제 설정값으로 갱신됨.
	float DefaultAltitude = 300.f;
	
	FTimerHandle CurrencyVacuumTimer;

#pragma endregion

#pragma region CompanionGAS
public:
	FORCEINLINE UNSCompanionAttributeSet* GetCompanionAttributeSet() const { return CompanionAttributeSet; }

protected:
	// @민재 : 컴패니언 업그레이드 스탯이 담기는 어트리뷰트 세트 (체력 없음)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UNSCompanionAttributeSet> CompanionAttributeSet;

#pragma endregion
};