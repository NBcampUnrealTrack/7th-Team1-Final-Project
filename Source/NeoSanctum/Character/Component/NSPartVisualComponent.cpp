// Copyright 2026 One Team. All rights reserved.

#include "NSPartVisualComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
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

	PartChangedHandle = EquipComp->OnPartChanged.AddUObject(this, &UNSPartVisualComponent::HandlePartChanged);

	// 구독 전 이미 복제된 파츠가 있을 수 있음
	if (const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(GetOwner()))
	{
		for (const auto& Pair : DataSS->GetAllSlotRows())
		{
			if (const FNSPartData* Part = EquipComp->GetEquippedPart(Pair.Key))
			{
				HandlePartChanged(Pair.Key, *Part);
			}
		}
	}
}

void UNSPartVisualComponent::EnsureSlotComponents()
{
	if (SlotMeshComps.Num() > 0 || !LeaderMeshComp)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(Owner);
	if (!DataSS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PartVisual] EnsureSlotComponents: DataSubsystem 없음"));
		return;
	}
	for (const auto& Pair : DataSS->GetAllSlotRows())
	{
		const FGameplayTag Slot = Pair.Key;
		USkeletalMeshComponent* MeshComp = NewObject<USkeletalMeshComponent>(Owner);
		MeshComp->SetupAttachment(LeaderMeshComp);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComp->RegisterComponent();
		// 메인 바디 본 트랜스폼을 그대로 따라감
		MeshComp->SetLeaderPoseComponent(LeaderMeshComp);

		SlotMeshComps.Add(Slot, MeshComp);
	}
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
	UpdateSlotVisual(Slot);
}

/**
 * 비동기 로드 비주얼 업데이트
 * 비동기 로드 중 
 * 비동기 콜백으로 자기자신을 넘겨서 최신파츠만 갱신될 수 있도록 해줌
 * @param Slot 슬롯 (레그, 바디, 암)
 */
void UNSPartVisualComponent::UpdateSlotVisual(FGameplayTag Slot)
{
	USkeletalMeshComponent* MeshComp = GetSlotMeshComp(Slot);
	if (!MeshComp)
	{
		return;
	}

	const FNSPartData* Part = PendingParts.Find(Slot);
	if (!Part || !Part->IsValid())
	{
		ClearSlotVisual(Slot);
		return;
	}

	
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
	if (!Def)
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

	USkeletalMesh* Mesh = Def->PartMesh.Get();
	if (!Mesh)
	{
		const FSoftObjectPath MeshPath = Def->PartMesh.ToSoftObjectPath();
		if (MeshPath.IsNull())
		{
			ClearSlotVisual(Slot);
			return;
		}
		MeshLoadHandles.Add(Slot, UAssetManager::GetStreamableManager().RequestAsyncLoad(
			MeshPath, FStreamableDelegate::CreateUObject(this, &UNSPartVisualComponent::UpdateSlotVisual, Slot)));
		return;
	}

	MeshComp->SetSkeletalMesh(Mesh);
	// 리더포즈 연결상태 재초기화 방지용 
	MeshComp->SetLeaderPoseComponent(LeaderMeshComp);
}

void UNSPartVisualComponent::ClearSlotVisual(FGameplayTag Slot)
{
	if (USkeletalMeshComponent* MeshComp = GetSlotMeshComp(Slot))
	{
		MeshComp->SetSkeletalMesh(nullptr);
	}
}

