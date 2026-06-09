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
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StaticMeshComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGeometryCollectionComponent> GeometryCollectionComp;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UNSDestructibleAttributeSet> Attributes;
	
	UPROPERTY(EditAnywhere, Category = "Destruction")
	float InitialHealth = 100.f;

	/** 파괴 후 서버가 액터를 정리하기까지의 시간(잔해 표시 구간).
	 *  GC 의 Remove-On-Sleep 소멸 시간보다 약간 길게 잡는다. */
	UPROPERTY(EditAnywhere, Category = "Destruction")
	float DebrisLifetime = 9.f;

	/** 이 시간 이후 relevant 된 클라는 잔해 시뮬 없이 숨김만 처리한다
	 *  (정착된 잔해 위치는 비결정적이라 재현 불가하므로). */
	UPROPERTY(EditAnywhere, Category = "Destruction")
	float SettleApproxTime = 3.f;
	
	UPROPERTY(EditAnywhere, Category = "Destruction")
	FName DebrisCollisionProfile = TEXT("DestructedFragment");

	UPROPERTY(EditAnywhere, Category = "Destruction|Impact")
	float BreakImpulseStrength = 150.f;

	UPROPERTY(EditAnywhere, Category = "Destruction|Impact")
	float BreakImpulseRadius = 200.f;
	
	UPROPERTY(EditAnywhere, Category = "Destruction|Impact")
	float LinearPushStrength = 100.f;
	
	UPROPERTY(EditAnywhere, Category = "Destruction|Impact")
	float ImpulseOriginOffset = 80.f;
	
	UPROPERTY(ReplicatedUsing = OnRep_Destroyed)
	bool bDestroyed = false;

	/** 파괴된 서버 월드 타임. 늦게 relevant 된 클라가 경과 시간을 계산해 상태를 재구성. */
	UPROPERTY(Replicated)
	float DestroyServerTime = 0.f;
	
	/** 네트워크 전용 FVector, 마지막 파괴한 Actor위치 저장용*/
	UPROPERTY(Replicated)
	FVector_NetQuantize ImpactAnchor = FVector::ZeroVector;
	
	UFUNCTION()
	void OnRep_Destroyed();
	
	/** Health 변화 콜백(서버에서만 바인딩). 0 이 되면 파괴를 개시. */
	void HandleHealthChanged(const FOnAttributeChangeData& Data);

	/** 스태틱→GC 교체 + 분해/임팩트. 전 머신 공통으로 호출되는 비주얼 처리.
	 *  Elapsed = 파괴 이후 경과 시간(늦참 클라 분기에 사용). */
	void StartDestruction(float Elapsed);
	
	/**
	 * 서버에서 사망 시점에 호출되는 확장 훅. 베이스는 아무것도 안 한다.
	 * 배럴이 오버라이드해 방사형 데미지를 적용한다.
	 */
	virtual void OnServerDestroyed(const FVector& Origin) {}

private:
	/** 중복 실행 가드(서버 직접 호출 + OnRep 이 겹치지 않도록). */
	bool bDestructionStarted = false;
	
	UPROPERTY(EditAnywhere, Category = "Destruction|Debug")
	bool bShowImpulseDebug =  false;
};
