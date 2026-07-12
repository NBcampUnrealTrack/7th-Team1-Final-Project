// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSEnemyPawnBase.h"
#include "NSEnemyDrone.generated.h"

class UNSFlyingLocomotionComponent;

UCLASS()
class NEOSANCTUM_API ANSEnemyDrone : public ANSEnemyPawnBase
{
	GENERATED_BODY()

public:
	ANSEnemyDrone();

protected:
	virtual void BeginPlay() override;
	virtual UNSFlyingLocomotionComponent* GetFlyingLocomotion() const override;

public:
	// 풀 대기 상태인지 반환
	bool IsInPool() const;

	// 풀에서 꺼내 재활성화 (위치/회전 재배치 + 상태 리셋)
	void PrepareForReuse(const FVector& Location, const FRotator& Rotation);

	// 풀로 반환하기 위해 비활성화
	void DeactivateForPool();
	
	// 서버에서만 값 변경 (컨트롤러 Tick에서 호출)
	void SetManeuvering(bool bNewManeuvering);
	
	// 이동 중(카이팅/배회) 여부. ABP 블렌드 판단용 복제 플래그
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsManeuvering() const { return bIsManeuvering; }
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void ApplyDeadState() override;
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess))
	TObjectPtr<UNSFlyingLocomotionComponent> FlyingLocomotionComponent;
	
	UPROPERTY(Replicated)
	bool bIsManeuvering = false;
};
