// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSWeaponBase.generated.h"

UCLASS(Abstract)
class NEOSANCTUM_API ANSWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	ANSWeaponBase();
	
	FName GetAttachSocketName() const;
	
	// 자식 클래스에서 재정의
	virtual bool TryGetAttackOriginTransform(FTransform& OutTransform) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> WeaponRoot;
	
	// 캐릭터의 어느 소캣에 붙일지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName AttachSocketName = TEXT("");
};
