// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSProjectileVisual.generated.h"

/**
 * 클라이언트에서만 생성되는 투사체의 시각 표현
 * 충돌, 데미지, 네트워크 복제 하지 않음
 */
UCLASS()
class NEOSANCTUM_API ANSProjectileVisual : public AActor
{
	GENERATED_BODY()

public:
	ANSProjectileVisual();

	// 시각 탄환의 위치와 진행 방향을 갱신
	void SetVisualTransform(
		const FVector& Location,
		const FVector& Direction);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;
};
