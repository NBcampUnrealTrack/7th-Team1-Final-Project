// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSBossTargetComponent.generated.h"

class UNSEnemyThreatComponent;
struct FNSEnemyAttackRow;

/**
 * 작성자: 최준혁
 * 
 * 파일 생성일: 26.07.02
 * 
 * 클래스 개요: Boss가 공격에 사용할 다중 타깃 목록을 구성하고 보관하는 컴포넌트입니다.
 * PrimaryTarget은 대표 타깃이고, CurrentAttackTargets는 이번 공격에서 실제로 노릴 타깃 목록입니다.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSBossTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSBossTargetComponent();

	// 저장된 공격 타깃 목록을 초기화하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Target")
	void ResetTargets();

	// 지정된 조건에 맞는 공격 타깃 목록을 구성하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Target")
	void BuildAttackTargets(
		AActor* PrimaryTarget,
		const FNSEnemyAttackRow& AttackRow);

	// 현재 공격 타깃 목록을 반환하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Target")
	void GetCurrentAttackTargets(TArray<AActor*>& OutTargets) const;

	// 지정된 Index의 공격 타깃을 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Target")
	AActor* GetAttackTarget(int32 Index) const;

	// 현재 공격 타깃 목록의 첫 번째 유효 타깃을 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Target")
	AActor* GetPrimaryAttackTarget() const;

	// 현재 공격 타깃 수를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Target")
	int32 GetAttackTargetCount() const;

private:
	// AttackRow의 TargetPolicy에 따라 후보 타깃을 수집하는 함수
	void CollectCandidatesByPolicy(
		AActor* PrimaryTarget,
		const FNSEnemyAttackRow& AttackRow,
		TArray<AActor*>& OutCandidates) const;

	// PrimaryTarget 1명만 후보로 추가하는 함수
	void CollectPrimaryOnlyTargets(
		AActor* PrimaryTarget,
		TArray<AActor*>& OutCandidates) const;

	// PrimaryTarget 주변의 Threat 후보를 수집하는 함수
	void CollectNearbyKnownTargets(
		AActor* PrimaryTarget,
		const FNSEnemyAttackRow& AttackRow,
		TArray<AActor*>& OutCandidates) const;

	// ThreatComponent에 기록된 모든 후보 타깃을 수집하는 함수
	void CollectAllKnownTargets(
		AActor* PrimaryTarget,
		const FNSEnemyAttackRow& AttackRow,
		TArray<AActor*>& OutCandidates) const;

	// 후보 타깃을 무작위 순서로 섞는 함수
	void ShuffleCandidates(TArray<AActor*>& Candidates) const;

	// 후보 타깃을 PrimaryTarget 우선, 이후 거리 기준으로 정렬하는 함수
	void SortCandidates(AActor* PrimaryTarget, TArray<AActor*>& Candidates) const;

	// 대상이 체력 데이터를 갖고 있고 살아 있는지 확인하는 함수
	bool IsValidLivingTarget(const AActor* Target) const;

	// Owner Pawn을 반환하는 함수
	APawn* GetOwnerPawn() const;

	// Owner의 ThreatComponent를 반환하는 함수
	UNSEnemyThreatComponent* GetThreatComponent() const;

private:
	// 이번 공격에서 실제로 사용할 타깃 목록
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> CurrentAttackTargets;
};
