// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Combat/Cosmetic/NSCosmeticEventHandler.h"
#include "NSCosmeticHandler_MachineGun.generated.h"

UCLASS(Blueprintable, BlueprintType)
class NEOSANCTUM_API UNSCosmeticHandler_MachineGun : public UNSCosmeticEventHandler
{
	GENERATED_BODY()

public:
	// MachineGun Handler가 처리할 EventTag 목록을 반환하는 함수
	virtual void GetHandledEventTags(TArray<FGameplayTag>& OutEventTags) const override;

	// MachineGun 발사 코스메틱 이벤트를 처리하는 함수
	virtual void HandleEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) override;

private:
	// 머신건 발사 사운드 DataTable Row ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|MachineGun")
	FName FireSoundID = FName(TEXT("Monster_TitanWalker_MachineGun_Fire"));

	// 머신건 발사 VFX DataTable Row ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|MachineGun")
	FName FireVFXID = NAME_None;

	// 머신건 발사 VFX 재생 배율을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|MachineGun", meta = (ClampMin = "0.01"))
	float FireVFXScaleMultiplier = 1.0f;
};
