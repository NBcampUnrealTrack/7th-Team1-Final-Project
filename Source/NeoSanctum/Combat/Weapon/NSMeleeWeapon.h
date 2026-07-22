// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSWeaponBase.h"
#include "NSMeleeWeapon.generated.h"

/**
 * 근접 무기 공통 베이스
 */
UCLASS()
class NEOSANCTUM_API ANSMeleeWeapon : public ANSWeaponBase
{
	GENERATED_BODY()

public:
	// 근접 공격 판정에 사용할 소켓들의 월드 Transform을 반환
	bool TryGetMeleeTraceSocketTransforms(TArray<FTransform>& OutTransforms) const;

protected:
	// 비어 있으면 모든 SceneComponent에서 판정 소켓을 검색
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Melee")
	FName MeleeTraceComponentName = NAME_None;

	// 근접 공격 궤적 판정에 사용할 소켓 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Melee")
	TArray<FName> MeleeTraceSocketNames = {
		TEXT("TrailBase"),
		TEXT("TrailMid"),
		TEXT("TrailTip")
	};

private:
	bool TryGetMeleeTraceSocketTransform(FName SocketName, FTransform& OutTransform) const;
};
