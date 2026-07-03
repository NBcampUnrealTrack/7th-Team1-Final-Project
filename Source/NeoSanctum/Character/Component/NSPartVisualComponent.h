// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSPartVisualComponent.generated.h"

class USkeletalMeshComponent;
class UNSPartEquipComponent;

/**
 * 캐릭터에 부착되는 파츠 시각 컴포넌트
 * 슬롯별 스켈레탈 메시를 메인 바디(리더)에 Leader Pose로 연결해 파츠를 시각 장착
 * 데이터/GAS는 PlayerState의 UNSPartEquipComponent가 담당 — 여기는 비주얼 전용
 */
UCLASS(ClassGroup=(NEOSANCTUM), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSPartVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSPartVisualComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 캐릭터가 PlayerState를 얻은 시점에 호출 — EquipComponent 구독 + 슬롯 메시 생성/리더포즈 연결
	void BindToEquipComponent(UNSPartEquipComponent* EquipComp, USkeletalMeshComponent* LeaderMesh);

private:
	// 슬롯별 USkeletalMeshComponent를 런타임 생성 후 리더포즈 연결 (1회)
	void EnsureSlotComponents();
	USkeletalMeshComponent* GetSlotMeshComp(FGameplayTag Slot) const;

	// OnPartChanged 콜백 — 최신 데이터 보관 후 비주얼 갱신
	void HandlePartChanged(FGameplayTag Slot, const FNSPartData& Part);

	// PendingParts[Slot] 기준으로 메시 세팅. 미로드 시 비동기 로드 후 재진입 (NSDroppedPart와 동일 패턴)
	void UpdateSlotVisual(FGameplayTag Slot);
	void ClearSlotVisual(FGameplayTag Slot);

private:
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> LeaderMeshComp;

	UPROPERTY(Transient)
	TObjectPtr<UNSPartEquipComponent> BoundEquipComp;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<USkeletalMeshComponent>> SlotMeshComps;

	// 비동기 로드 중 교체 대비 —> 슬롯별 최신 파츠 데이터
	TMap<FGameplayTag, FNSPartData> PendingParts;

	// 진행 중인 메시(Definition/PartMesh) 비동기 로드 핸들 (슬롯별)
	TMap<FGameplayTag, TSharedPtr<FStreamableHandle>> MeshLoadHandles;

	FDelegateHandle PartChangedHandle;
};
