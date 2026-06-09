// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "NSGameplayCueNotify_DamageFlash.generated.h"

/**
 * 데미지 플래시 GameplayCue.
 * MyTarget의 UNSDamageFlashComponent를 찾아 PlayFlash() 호출.
 * 상태는 컴포넌트가 보유하므로 이 클래스는 무상태(Static).
 */
UCLASS()
class NEOSANCTUM_API UNSGameplayCueNotify_DamageFlash : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
	
public:
	virtual bool OnExecute_Implementation(AActor* MyTarget,	const FGameplayCueParameters& Parameters) const override;
};
