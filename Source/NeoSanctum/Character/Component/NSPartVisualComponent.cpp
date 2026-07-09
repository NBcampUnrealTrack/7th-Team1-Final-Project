// Copyright 2026 One Team. All rights reserved.

#include "NSPartVisualComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"

UNSPartVisualComponent::UNSPartVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSPartVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundEquipComp && PartChangedHandle.IsValid())
	{
		BoundEquipComp->OnPartChanged.Remove(PartChangedHandle);
		PartChangedHandle.Reset();
	}

	for (TPair<FGameplayTag, TSharedPtr<FStreamableHandle>>& Pair : MeshLoadHandles)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->CancelHandle();
		}
	}
	MeshLoadHandles.Empty();

	Super::EndPlay(EndPlayReason);
}

void UNSPartVisualComponent::BindToEquipComponent(UNSPartEquipComponent* EquipComp, USkeletalMeshComponent* LeaderMesh)
{
	if (!EquipComp || !LeaderMesh)
	{
		return;
	}

	// 중복 구독 방지
	if (BoundEquipComp == EquipComp)
	{
		return;
	}

	if (BoundEquipComp && PartChangedHandle.IsValid())
	{
		BoundEquipComp->OnPartChanged.Remove(PartChangedHandle);
	}

	LeaderMeshComp = LeaderMesh;
	BoundEquipComp = EquipComp;

	EnsureSlotComponents();
	// SetDefaultVisualParts가 이 함수보다 먼저 호출됐을 수도 있음 - 그때는 LeaderMeshComp가 없어서
	// 시각 전용 슬롯 컴포넌트를 못 만들었을 테니, 여기서 한 번 더 보장해줌
	for (const FNSDefaultVisualPartEntry& Entry : DefaultVisualParts)
	{
		EnsureSlotComponent(Entry.PartVisualTag);
	}

	PartChangedHandle = EquipComp->OnPartChanged.AddUObject(this, &UNSPartVisualComponent::HandlePartChanged);

	// 구독 전 이미 복제된 파츠가 있을 수 있음 - 여기선 PendingParts만 채우고, 실제 표시는 아래 RebuildAllSlotVisuals에서 한 번에 처리
	if (const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(GetOwner()))
	{
		for (const auto& Pair : DataSS->GetAllSlotRows())
		{
			if (const FNSPartData* Part = EquipComp->GetEquippedPart(Pair.Key))
			{
				PendingParts.Add(Pair.Key, *Part);
			}
		}
	}

	RebuildAllSlotVisuals();
}

void UNSPartVisualComponent::SetDefaultVisualParts(const TArray<FNSDefaultVisualPartEntry>& InDefaultVisualParts)
{
	DefaultVisualParts = InDefaultVisualParts;

	// 기본 파츠가 하나도 없으면 BaseLeaderMesh만 그대로 보임 - 의도한 fallback인지 빠뜨린 건지 로그로 남겨둠
	if (DefaultVisualParts.Num() == 0)
	{
		NS_ACTOR_LOG(GetOwner(), LogNS, Warning,
			"DefaultVisualParts가 비어있어 BaseLeaderMesh로만 표시됩니다. 신규 캐릭터라면 기본 파츠 값을 채워야 합니다.");
	}

	// 게임플레이 슬롯표(Arm/Body/Leg)에 없는 시각 전용 슬롯(Head, Hair 등)도 컴포넌트를 만들어둠
	for (const FNSDefaultVisualPartEntry& Entry : DefaultVisualParts)
	{
		EnsureSlotComponent(Entry.PartVisualTag);
	}

	RebuildAllSlotVisuals();
}

void UNSPartVisualComponent::EnsureSlotComponents()
{
	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(GetOwner());
	if (!DataSS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PartVisual] EnsureSlotComponents: DataSubsystem 없음"));
		return;
	}

	for (const auto& Pair : DataSS->GetAllSlotRows())
	{
		EnsureSlotComponent(Pair.Key);
	}
}

void UNSPartVisualComponent::EnsureSlotComponent(FGameplayTag Slot)
{
	if (!LeaderMeshComp || SlotMeshComps.Contains(Slot))
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = NewObject<USkeletalMeshComponent>(Owner);
	MeshComp->SetupAttachment(LeaderMeshComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->RegisterComponent();
	// 메인 바디 본 트랜스폼을 그대로 따라감
	MeshComp->SetLeaderPoseComponent(LeaderMeshComp);

	SlotMeshComps.Add(Slot, MeshComp);
}

USkeletalMeshComponent* UNSPartVisualComponent::GetSlotMeshComp(FGameplayTag Slot) const
{
	const TObjectPtr<USkeletalMeshComponent>* Found = SlotMeshComps.Find(Slot);
	return Found ? *Found : nullptr;
}


void UNSPartVisualComponent::HandlePartChanged(FGameplayTag Slot, const FNSPartData& Part)
{
	// 비동기 로드 중 교체 대비용
	PendingParts.Add(Slot, Part);
	// 이 파츠의 VisualTag가 다른 시각 슬롯에도 영향을 줄 수 있어서 전체 재계산
	RebuildAllSlotVisuals();
}

void UNSPartVisualComponent::RebuildAllSlotVisuals()
{
	for (const TPair<FGameplayTag, TObjectPtr<USkeletalMeshComponent>>& Pair : SlotMeshComps)
	{
		UpdateSlotVisual(Pair.Key);
	}
}

/**
 * 슬롯 하나의 최종 소스를 계산해 적용
 * 1) 이 슬롯이 게임플레이 슬롯 자신의 자리면 장착 파츠 확인 (VisualTag 있으면 다른 자리에서 보여주니 여긴 비움)
 * 2) 이 슬롯이 시각 슬롯이면 VisualTag로 이 자리를 차지하겠다는 장착 파츠가 있는지 확인
 * 3) 없으면 CharacterData 기본 파츠로 대체
 * 비동기 로드 콜백으로 자기 자신을 넘겨서 로드 완료 시점에도 최신 상태를 다시 계산하게 함
 * @param Slot 슬롯 (게임플레이 슬롯 + 시각 전용 슬롯)
 */
void UNSPartVisualComponent::UpdateSlotVisual(FGameplayTag Slot)
{
	USkeletalMeshComponent* MeshComp = GetSlotMeshComp(Slot);
	if (!MeshComp)
	{
		return;
	}

	const FNSPartData* Part = PendingParts.Find(Slot);
	if (Part && Part->IsValid())
	{
		UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
		if (!IsValid(Def))
		{
			const FSoftObjectPath DefPath = Part->DefinitionPtr.ToSoftObjectPath();
			if (DefPath.IsNull())
			{
				return;
			}

			MeshLoadHandles.Add(Slot, UAssetManager::GetStreamableManager().RequestAsyncLoad(
				DefPath, FStreamableDelegate::CreateUObject(this, &UNSPartVisualComponent::UpdateSlotVisual, Slot)));
			return;
		}

		const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(this, Def->GetPrimaryAssetId());
		if (Row && Row->VisualTag.IsValid())
		{
			// 이 파츠는 VisualTag 시각 슬롯 쪽에서 표시되니, 원래 게임플레이 슬롯 자리는 비워둠(중복 표시 방지)
			ClearSlotVisual(Slot);
			return;
		}

		// VisualTag 미설정 - 게임플레이 슬롯 자리에 그대로 표시. 장착 파츠라 색상 오버라이드는 없음.
		ApplySlotMesh(Slot, MeshComp, Def->PartMesh, TSoftObjectPtr<UMaterialInterface>());
		return;
	}

	// 시각 슬롯이라면, 이 자리를 차지하겠다는 장착 파츠가 있는지 확인
	if (const FNSPartData* MappedPart = FindEquippedPartForVisualSlot(Slot))
	{
		UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *MappedPart);
		if (!IsValid(Def))
		{
			const FSoftObjectPath DefPath = MappedPart->DefinitionPtr.ToSoftObjectPath();
			if (DefPath.IsNull())
			{
				return;
			}

			MeshLoadHandles.Add(Slot, UAssetManager::GetStreamableManager().RequestAsyncLoad(
				DefPath, FStreamableDelegate::CreateUObject(this, &UNSPartVisualComponent::UpdateSlotVisual, Slot)));
			return;
		}

		// 장착 파츠는 자기 원래 머터리얼을 그대로 씀, 색상 오버라이드는 없음
		ApplySlotMesh(Slot, MeshComp, Def->PartMesh, TSoftObjectPtr<UMaterialInterface>());
		return;
	}

	// 장착으로 채워지지 않은 슬롯은 CharacterData 기본 파츠로 대체
	if (const FNSDefaultVisualPartEntry* DefaultEntry = FindDefaultEntry(Slot))
	{
		ApplySlotMesh(Slot, MeshComp, DefaultEntry->PartMesh, DefaultEntry->MaterialOverride);
		return;
	}

	ClearSlotVisual(Slot);
}

void UNSPartVisualComponent::ApplySlotMesh(
	FGameplayTag Slot,
	USkeletalMeshComponent* MeshComp,
	const TSoftObjectPtr<USkeletalMesh>& MeshPtr,
	const TSoftObjectPtr<UMaterialInterface>& MaterialOverride)
{
	USkeletalMesh* Mesh = MeshPtr.Get();
	if (!Mesh)
	{
		const FSoftObjectPath MeshPath = MeshPtr.ToSoftObjectPath();
		if (MeshPath.IsNull())
		{
			ClearSlotVisual(Slot);
			return;
		}
		MeshLoadHandles.Add(Slot, UAssetManager::GetStreamableManager().RequestAsyncLoad(
			MeshPath, FStreamableDelegate::CreateUObject(
				this,
				&UNSPartVisualComponent::UpdateSlotVisual,
				Slot
			))
		);
		return;
	}

	MeshComp->SetSkeletalMesh(Mesh);
	// 리더포즈 연결상태 재초기화 방지용
	MeshComp->SetLeaderPoseComponent(LeaderMeshComp);

	// 이전에 걸려있던 색상 오버라이드를 먼저 지우고, 이번 소스가 오버라이드를 갖고 있으면 그때만 다시 얹음
	// (Equipped <-> Default 전환 시 이전 색상이 남는 잔상 방지)
	MeshComp->EmptyOverrideMaterials();
	if (UMaterialInterface* Material = MaterialOverride.Get())
	{
		MeshComp->SetMaterial(0, Material);
	}
}

void UNSPartVisualComponent::ClearSlotVisual(FGameplayTag Slot)
{
	if (USkeletalMeshComponent* MeshComp = GetSlotMeshComp(Slot))
	{
		MeshComp->SetSkeletalMesh(nullptr);
	}
}

const FNSDefaultVisualPartEntry* UNSPartVisualComponent::FindDefaultEntry(FGameplayTag Slot) const
{
	return DefaultVisualParts.FindByPredicate(
		[Slot](const FNSDefaultVisualPartEntry& Entry) { return Entry.PartVisualTag == Slot; });
}

const FNSPartData* UNSPartVisualComponent::FindEquippedPartForVisualSlot(FGameplayTag VisualSlot) const
{
	for (const TPair<FGameplayTag, FNSPartData>& Pair : PendingParts)
	{
		if (!Pair.Value.IsValid())
		{
			continue;
		}

		UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, Pair.Value);
		if (!IsValid(Def))
		{
			continue;
		}

		const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(this, Def->GetPrimaryAssetId());

		if (Row && Row->VisualTag == VisualSlot)
		{
			return &Pair.Value;
		}
	}

	return nullptr;
}
