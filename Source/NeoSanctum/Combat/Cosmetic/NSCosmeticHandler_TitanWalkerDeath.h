// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Combat/Cosmetic/NSCosmeticEventHandler.h"
#include "NSCosmeticHandler_TitanWalkerDeath.generated.h"

/*
 * 작성자 : 최준혁
 *
 * 파일 생성일 : 26.07.18
 *
 * 클래스 개요 : TitanWalker 사망 폭발 코스메틱을 처리하는 Handler
 * 사망 이벤트 수신 시 기존 Bone 위치를 기준으로 NS_Explosion VFX를 시간차 재생
*/

// TitanWalker 사망 폭발 1회 재생 정보를 저장하는 구조체
USTRUCT(BlueprintType)
struct FNSTitanWalkerDeathExplosionStep
{
	GENERATED_BODY()

	// 폭발 위치 기준으로 사용할 Bone 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Explosion")
	FName BoneName = NAME_None;

	// 사망 이벤트 이후 해당 폭발을 재생할 지연 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Explosion", meta = (ClampMin = "0.0"))
	float Delay = 0.0f;

	// Bone 위치에 추가로 더할 월드 위치 오프셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Explosion")
	FVector LocationOffset = FVector::ZeroVector;

	// 해당 폭발에만 적용할 VFX 스케일 배율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Explosion", meta = (ClampMin = "0.0"))
	float ScaleMultiplier = 1.0f;
};

UCLASS(Blueprintable, BlueprintType)
class NEOSANCTUM_API UNSCosmeticHandler_TitanWalkerDeath : public UNSCosmeticEventHandler
{
	GENERATED_BODY()

public:
	// Handler가 처리할 TitanWalker 사망 코스메틱 EventTag를 반환하는 함수
	virtual void GetHandledEventTags(TArray<FGameplayTag>& OutEventTags) const override;

	// TitanWalker 사망 코스메틱 이벤트를 받아 폭발 타이머를 등록하는 함수
	virtual void HandleEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) override;

	// Handler 파괴 시 남아 있는 폭발 타이머를 정리하는 함수
	virtual void BeginDestroy() override;

private:
	// 사망 폭발에 사용할 VFX DataTable Row 이름
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|TitanWalker Death")
	FName ExplosionVFXID = FName(TEXT("NS_Explosion"));

	// Bone별 사망 폭발 재생 순서와 타이밍 목록
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|TitanWalker Death")
	TArray<FNSTitanWalkerDeathExplosionStep> ExplosionSteps;

	// 모든 사망 폭발에 공통으로 적용할 VFX 스케일 배율
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|TitanWalker Death", meta = (ClampMin = "0.0"))
	float GlobalScaleMultiplier = 1.0f;

	// 지연 재생 중인 폭발 타이머 핸들 목록
	TArray<FTimerHandle> ActiveTimerHandles;

	// 지정된 Bone 위치에 사망 폭발 VFX를 1회 재생하는 함수
	void PlayExplosionStep(TWeakObjectPtr<AActor> WeakOwnerActor, FNSTitanWalkerDeathExplosionStep Step) const;

	// OwnerActor에서 Enemy Mesh를 찾아 반환하는 함수
	USkeletalMeshComponent* ResolveEnemyMesh(AActor* OwnerActor) const;

	// 등록된 사망 폭발 타이머를 모두 해제하는 함수
	void ClearActiveTimers();
};
