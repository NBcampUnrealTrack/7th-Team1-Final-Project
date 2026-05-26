// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "NSDestructibleObjectBase.generated.h"

class UAbilitySystemComponent;
class UNSDestructibleAttributeSet;
class UGeometryCollectionComponent;

/*
 * 파괴가능 오브젝트 베이스 클래스
 * 어트리뷰트 셋에 데미지를 받아 체력이 0이 될 경우 카오스 디스트럭션으로 파괴
 * 마지막 데미지를 받은 위치를 저장하여 해당 위치에서 파괴 시작
 * 서버에서 실행하여 클라이언트에 복제되는 형식
 **/
UCLASS()
class NEOSANCTUM_API ANSDestructibleObjectBase : public AActor , public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANSDestructibleObjectBase();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void TriggerDestruction();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDestruction(FVector HitLocation);

	UPROPERTY(VisibleAnywhere, Category="GAS")
	UAbilitySystemComponent* ASC;

	UPROPERTY(VisibleAnywhere, Category="GAS")
	UNSDestructibleAttributeSet* DestructibleAttrSet;

	UPROPERTY(VisibleAnywhere, Category="Destructible")
	UGeometryCollectionComponent* GCComp;

	UPROPERTY(EditAnywhere, Category="Destructible")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, Category="Destructible")
	float ImpulseStrength = 300000.f;

private:
	FVector LastHitLocation;
	bool bIsDestroyed = false;

	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnGEApplied(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);

};
