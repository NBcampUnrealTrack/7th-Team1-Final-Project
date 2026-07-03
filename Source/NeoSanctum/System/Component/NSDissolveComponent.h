// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSDissolveComponent.generated.h"

DECLARE_DELEGATE(FNSOnDissolveComplete)

/*
 * 작성자 : 최준혁
 *
 * 파일 생성일 : 26.05.28
 *
 * 클래스 개요 : Actor가 가진 MeshComponent에 디졸브 머티리얼 값을 적용하는 공통 컴포넌트
 * 옵션이 켜진 경우 Owner에 Attach된 Actor의 MeshComponent까지 함께 처리
*/
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSDissolveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSDissolveComponent();

	UFUNCTION(BlueprintCallable, Category = "Utility|Visuals")
	void StartDissolve(bool bDestroyAfterDissolve = false);

	//(이용호 추가)
	void ResetDissolve();

	FNSOnDissolveComplete OnDissolveComplete;
	
protected:
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;
	
	// 디졸브 효과 지속 시간
	UPROPERTY(EditDefaultsOnly, Category = "Dissolve Settings")
	float DissolveDuration = 5.0f;

	// Owner에 Attach된 하위 Actor들의 Mesh까지 함께 디졸브할지 정하는 옵션
	UPROPERTY(EditDefaultsOnly, Category = "Dissolve Settings")
	bool bIncludeAttachedActors = false;

private:
	// Owner와 옵션에 따른 Attached Actor의 MeshComponent를 수집하는 함수
	void CollectDissolveMeshes(TArray<UMeshComponent*>& OutMeshes) const;

	// 지정 Actor가 가진 MeshComponent를 수집하는 함수
	void CollectMeshesFromActor(AActor* TargetActor, TArray<UMeshComponent*>& OutMeshes) const;

	void UpdateDissolveAlpha();

	FTimerHandle DissolveTimerHandle;
	
	float DissolveStartTime = 0.0f;
};
