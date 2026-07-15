// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Combat/Cosmetic/NSCosmeticEventHandler.h"
#include "NSCosmeticHandler_Bombard.generated.h"

class ANSAreaWarningPlaneActor;
class ANSAreaWarningInstancedActor;

UCLASS(Blueprintable, BlueprintType)
class NEOSANCTUM_API UNSCosmeticHandler_Bombard : public UNSCosmeticEventHandler
{
	GENERATED_BODY()

public:
	// Bombard Handler가 처리할 EventTag 목록을 반환하는 함수
	virtual void GetHandledEventTags(TArray<FGameplayTag>& OutEventTags) const override;

	// Bombard 코스메틱 이벤트를 처리하는 함수
	virtual void HandleEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) override;

private:
	// 포격 준비 사운드 ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Bombard")
	FName PrepareSoundID = FName(TEXT("Monster_TitanWalker_Bombard_Prepare"));

	// 포격 발사 사운드 ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Bombard")
	FName LaunchSoundID = FName(TEXT("Monster_TitanWalker_Bombard_Launch"));

	// 포격 폭발 사운드 ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Bombard")
	FName ImpactSoundID = FName(TEXT("Monster_TitanWalker_Bombard_Explosion"));

	// 포격 발사 VFX ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Bombard")
	FName LaunchVFXID = FName(TEXT("Monster_TitanWalker_Bombard_Launch"));

	// 포격 폭발 VFX ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Bombard")
	FName ExplosionVFXID = FName(TEXT("Monster_TitanWalker_Bombard_Explosion"));

	// 폭발 VFX 기본 반경을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Bombard", meta = (ClampMin = "1.0"))
	float ExplosionBaseRadius = 100.0f;

	// 경고 Plane Actor 클래스를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Bombard|Warning")
	TSubclassOf<ANSAreaWarningPlaneActor> WarningPlaneClass;

	// 대량 포격 경고를 Instanced Mesh로 표시할 Actor 클래스를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Bombard|Warning")
	TSubclassOf<ANSAreaWarningInstancedActor> InstancedWarningClass;

	// Instanced Warning으로 전환할 최소 경고 위치 수를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Bombard|Warning", meta = (ClampMin = "1"))
	int32 InstancedWarningMinPointCount = 1;
	
	// Batch 경고에서 Instanced Warning Actor가 없을 때 기존 Plane Actor fallback을 허용할지 정하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Bombard|Warning")
	bool bAllowPlaneFallbackForBatchedWarning = false;

	// 경고 Plane 표시 시간을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Bombard|Warning", meta = (ClampMin = "0.0"))
	float WarningPlaneDuration = 1.0f;

	// 경고 Plane 스폰 Z 보정값을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Bombard|Warning")
	float WarningPlaneZOffset = 1.0f;

	// 포격 준비 코스메틱을 재생하는 함수
	void HandlePrepareEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) const;

	// 포격 발사 코스메틱을 재생하는 함수
	void HandleLaunchEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) const;

	// 포격 경고 Plane을 생성하는 함수
	void HandleWarningEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) const;

	// 포격 착탄 코스메틱을 재생하는 함수
	void HandleImpactEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) const;
};
