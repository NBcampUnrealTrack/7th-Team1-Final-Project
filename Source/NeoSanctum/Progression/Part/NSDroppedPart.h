// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NSDroppedPart.generated.h"

class USkeletalMeshComponent;

/**
 * 바닥에 드랍된 파츠 액터
 * 픽업 처리는 서버 권한(TryPickup)에서만 수행 — 추후 상호작용 시스템이 호출
 * 재화로도 가능하다면
 */
UCLASS()
class NEOSANCTUM_API ANSDroppedPart : public AActor
{
	GENERATED_BODY()

public:
	ANSDroppedPart();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버에서 스폰 직후 1회 호출 — 파츠 데이터 세팅 및 비주얼 적용
	UFUNCTION(BlueprintCallable, Category = "Part") 
	void Initialize(const FNSPartData& InPart);

	// 서버 권한에서만 실행. 상호작용 시스템이 인터랙터 폰을 넘겨 호출
	UFUNCTION(BlueprintCallable, Category = "Part")
	void TryPickup(APawn* InstigatorPawn);

	const FNSPartData& GetStoredPart() const { return StoredInstance; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_StoredInstance();

	// DefinitionPtr -> PartMesh 비동기 로드 후 MeshComp에 세팅 (로드 완료 시 재진입)
	void SetupVisual();

protected:
	// ===== 테스트용 임시 코드 — 몬스터 드랍 연동 후 삭제 =====
	// 레벨 배치 액터의 디테일 패널에서 지정 시 BeginPlay에서 자동 Initialize
	UPROPERTY(EditAnywhere, Category = "Part|Debug")
	TSoftObjectPtr<UNSPartDefinition> DebugDefinition;

	UPROPERTY(EditAnywhere, Category = "Part|Debug")
	ENSPartRarity DebugRarity = ENSPartRarity::Common;

	UPROPERTY(EditAnywhere, Category = "Part|Debug")
	float DebugValue = 15.f;
	// ===== 테스트용 임시 코드 끝 =====

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Part")
	TObjectPtr<USkeletalMeshComponent> MeshComp;

	UPROPERTY(ReplicatedUsing = OnRep_StoredInstance, BlueprintReadOnly, Category = "Part")
	FNSPartData StoredInstance;

private:
	// 진행 중인 비주얼(Definition/PartMesh) 비동기 로드 핸들
	TSharedPtr<FStreamableHandle> VisualLoadHandle;
};
