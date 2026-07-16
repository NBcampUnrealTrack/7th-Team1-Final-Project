// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "NeoSanctum/Combat/Component/Artillery/NSBossArtilleryComponent.h"
#include "GA_EnemyAttackBombard.generated.h"

class UMaterialInterface;
class ANSBossAIController;
class ANSAreaWarningPlaneActor;
class UNSEnemyPartComponent;
class UNSBossTargetComponent;
class UNSEnemyThreatComponent;
struct FNSEnemyAttackRow;

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.06
 * 
 * 클래스 개요 : TitanWalker의 포격 공격을 처리하는 Gameplay Ability
 * AttackMontage의 GameplayEvent 시점에 BombardData 기준 착탄 지점을 생성하고,
 * ImpactDelay 이후 ImpactRadius 범위 데미지를 적용
*/
UCLASS()
class NEOSANCTUM_API UGA_EnemyAttackBombard : public UGA_EnemyAttackBase
{
	GENERATED_BODY()

public:
	UGA_EnemyAttackBombard();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	virtual void InitializeAttack() override;
	virtual void HandleAttackEvent(const FGameplayEventData& Payload) override;
	virtual void HandleAttackMontageCompleted() override;

private:
	// 현재 Avatar의 CurrentAttackRow를 반환하는 함수
	const FNSEnemyAttackRow* GetCurrentAttackRow() const;

	// 현재 Avatar의 BossArtilleryComponent를 반환하는 함수
	UNSBossArtilleryComponent* GetBossArtilleryComponent() const;

	// 현재 Avatar의 BossTargetComponent를 반환하는 함수
	UNSBossTargetComponent* GetBossTargetComponent() const;

	// 현재 Avatar의 EnemyThreatComponent를 반환하는 함수
	UNSEnemyThreatComponent* GetThreatComponent() const;

	// 기존 보스 타깃 정보로 포격 컴포넌트의 전투 참여자 목록을 갱신하는 함수
	void SyncArtilleryCombatants(UNSBossArtilleryComponent& ArtilleryComponent) const;

	// BossTargetComponent와 ThreatComponent에서 포격 전투 참여자 후보를 수집하는 함수
	void CollectArtilleryCombatants(TArray<AActor*>& OutCombatants) const;

	// AnimNotify GameplayEvent 이후 포격 패턴을 시작하는 함수
	void StartBombardVolley();

	void HandleArtilleryExecutionFinished(int32 ExecutionId);
	void UnbindArtilleryFinishedDelegate();

	// 몽타주와 예약된 착탄이 모두 끝났을 때 Ability를 종료하는 함수
	void TryFinishBombardAbility();

private:
	// 현재 Ability에서 사용할 AttackRow
	const FNSEnemyAttackRow* CachedAttackRow = nullptr;

	// 포격 패턴 실행 요청이 시작됐는지 저장하는 변수
	bool bArtilleryStarted = false;

	// AttackMontage 재생이 끝났는지 저장하는 변수
	bool bMontageCompleted = false;

	// 포격 컴포넌트의 실행이 모두 끝났는지 저장하는 변수
	bool bArtilleryCompleted = false;

	// 현재 Ability가 기다리고 있는 포격 실행 ID를 저장하는 변수
	int32 ActiveArtilleryExecutionId = 0;

	// 현재 Ability가 실행을 요청한 포격 컴포넌트를 약하게 참조하는 변수
	TWeakObjectPtr<UNSBossArtilleryComponent> ActiveArtilleryComponent;

	// 포격 완료 델리게이트 구독 해제에 사용할 핸들을 저장하는 변수
	FDelegateHandle ArtilleryFinishedDelegateHandle;

protected:
	virtual void PrepareForAttackMontage() override;

private:
	// 포격 준비 코스메틱 이벤트를 클라이언트에 전송하는 함수
	void SendBombardPrepareCosmeticEvent() const;

	// 포격 코스메틱 이벤트를 CosmeticComponent로 전달하는 함수
	void SendBombardCosmeticEvent(const FNSCosmeticEventNetData& EventData, bool bReliable) const;
};
