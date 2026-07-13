// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NSMonsterUITypes.h"
#include "NSMonsterStatusViewModel.generated.h"

class UAbilitySystemComponent;
class UNSEnemyData;
struct FOnAttributeChangeData;

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 몬스터 ASC를 관찰해 UI 표시용 상태값을 계산하는 ViewModel입니다.
 * Widget에는 Current/Max가 아닌 Percent와 표시 정책만 전달합니다.
 */
UCLASS()
class NEOSANCTUM_API UNSMonsterStatusViewModel : public UObject
{
	GENERATED_BODY()

public:
	// 대상 몬스터 Actor와 ASC를 일반 몬스터 기본 정책으로 연결하는 함수
	bool Initialize(AActor* InTargetActor);

	// 대상 몬스터 Actor와 ASC를 지정한 표시 정책으로 연결하는 함수
	bool Initialize(AActor* InTargetActor, const FNSMonsterUIDisplayPolicy& InDisplayPolicy);

	// Attribute Delegate 구독을 해제하고 참조를 정리하는 함수
	void Shutdown();

	// 현재 UI 상태값을 반환하는 함수
	const FNSMonsterUIStatus& GetStatus() const { return CachedStatus; }

	// UI 상태 변경 시 Widget에 알리는 델리게이트 변수
	FNSMonsterUIStatusChanged OnStatusChanged;

private:
	// 표시 정책을 현재 UI 상태값에 적용하는 함수
	void ApplyDisplayPolicy();

	// 현재 Attribute 값을 읽어 UI 상태값을 갱신하는 함수
	void RefreshStatus();

	// Attribute 변경 Delegate를 구독하는 함수
	void BindAttributeDelegates();

	// Attribute 변경 Delegate 구독을 해제하는 함수
	void UnbindAttributeDelegates();

	// Attribute 변경 시 UI 상태값을 갱신하는 함수
	void HandleAttributeChanged(const FOnAttributeChangeData& Data);

	// Current/Max 값을 수치 텍스트로 변환하는 함수
	FText MakeValueText(float CurrentValue, float MaxValue) const;

private:
	// 현재 ViewModel이 사용할 표시 정책 변수
	FNSMonsterUIDisplayPolicy DisplayPolicy;

	// 상태 표시 대상 몬스터 Actor를 약하게 보관하는 변수
	TWeakObjectPtr<AActor> TargetActor;

	// 상태 표시 대상 ASC를 약하게 보관하는 변수
	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;

	// Widget에 전달할 캐시된 상태값 변수
	FNSMonsterUIStatus CachedStatus;

	// Health 변경 Delegate Handle 변수
	FDelegateHandle HealthChangedHandle;

	// MaxHealth 변경 Delegate Handle 변수
	FDelegateHandle MaxHealthChangedHandle;

	// Shield 변경 Delegate Handle 변수
	FDelegateHandle ShieldChangedHandle;

	// MaxShield 변경 Delegate Handle 변수
	FDelegateHandle MaxShieldChangedHandle;

	// HitGauge 변경 Delegate Handle 변수
	FDelegateHandle HitGaugeChangedHandle;

	// MaxHitGauge 변경 Delegate Handle 변수
	FDelegateHandle MaxHitGaugeChangedHandle;
};
