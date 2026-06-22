// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NeoSanctum/Interaction/Core/NSInteractable.h"
#include "NeoSanctum/Data/Progression/Drop/NSDropLaunchData.h"
#include "NSDroppedPart.generated.h"

class USkeletalMeshComponent;
class USphereComponent;

/**
 * 바닥에 드랍된 파츠 액터
 * 픽업 처리는 서버 권한(TryPickup)에서만 수행 — 추후 상호작용 시스템이 호출
 * 재화로도 가능하다면
 */
UCLASS()
class NEOSANCTUM_API ANSDroppedPart : public AActor, public INSInteractable
{
	GENERATED_BODY()

public:
	ANSDroppedPart();
	
	// 월드 좌표에 파츠를 스폰, 장착 교체/몬스터 드랍 공용 진입점
	static ANSDroppedPart* SpawnInWorld(UWorld* World, TSubclassOf<ANSDroppedPart> Class,
		const FNSPartData& Part, const FVector& Location);
	
	// 발사 시작점에서 생성한 뒤 TargetLocation까지 포물선으로 이동하는 파츠 스폰 경로
	static ANSDroppedPart* SpawnInWorld(
		UWorld* World,
		TSubclassOf<ANSDroppedPart> Class,
		const FNSPartData& Part,
		const FNSDropLaunchData& InLaunchData
	);

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
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	void OnRep_StoredInstance();

	// DefinitionPtr -> PartMesh 비동기 로드 후 MeshComp에 세팅 (로드 완료 시 재진입)
	void SetupVisual();
	
	void StartDropLaunch(const FNSDropLaunchData& InLaunchData);
	void UpdateDropLaunch();
	void FinishDropLaunch();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Part")
	TObjectPtr<USkeletalMeshComponent> MeshComp;

	UPROPERTY(ReplicatedUsing = OnRep_StoredInstance, BlueprintReadOnly, Category = "Part")
	FNSPartData StoredInstance;

private:
	FNSDropLaunchData LaunchData;
	float LaunchStartWorldTime = 0.0f;
	bool bIsLaunching = false;
	
	// 진행 중인 비주얼(Definition/PartMesh) 비동기 로드 핸들
	TSharedPtr<FStreamableHandle> VisualLoadHandle;
	
// ================================================================
// 데이터 접근 API
// ================================================================
public:
	// 상호작용 인터페이스 구현부
	virtual bool CanInteract_Implementation(APlayerController* Interactor) const override;
	virtual bool OnInteract_Implementation(APlayerController* Interactor) override;
	virtual FText GetPromptText_Implementation() const override;
	virtual TSoftObjectPtr<UTexture2D> GetPromptIcon_Implementation() const override;
	virtual int32 GetPromptRarityIndex_Implementation() const override;
	virtual FVector GetPromptWorldLocation_Implementation() const override;
protected:
	// 상호작용 감지용 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Part")
	TObjectPtr<USphereComponent> DetectionCollision;

	// 프롬프트 위젯이 뜰 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Part")
	TObjectPtr<USceneComponent> PromptAnchor;

	// 서버 줍기 재검증 시 허용 거리(변조 방지)
	UPROPERTY(EditAnywhere, Category = "Part")
	float InteractRadius = 100.f;

	// 프롬프트 액션 문구
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FText PromptText = FText::FromString(TEXT("줍기"));
	
};
