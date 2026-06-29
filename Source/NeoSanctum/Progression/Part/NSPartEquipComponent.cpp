// Copyright 2026 One Team. All rights reserved.

#include "NSPartEquipComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Progression/Part/NSDroppedPart.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"
#include "NeoSanctum/Tag/NSGameplayTags_Part.h"

UNSPartEquipComponent::UNSPartEquipComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNSPartEquipComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNSPartEquipComponent, EquippedParts);
}

void UNSPartEquipComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 진행 중 비동기 로드 콜백이 파괴 후 호출되지 않도록 취소
	for (TPair<ENSPartSlot, TSharedPtr<FStreamableHandle>>& Pair : EffectLoadHandles)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->CancelHandle();
		}
	}
	for (TPair<ENSPartSlot, TSharedPtr<FStreamableHandle>>& Pair : AbilityLoadHandles)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->CancelHandle();
		}
	}
	EffectLoadHandles.Empty();
	AbilityLoadHandles.Empty();

	Super::EndPlay(EndPlayReason);
}

void UNSPartEquipComponent::EquipPart(const FNSPartData& NewPart, TOptional<FVector> DropLocationOverride)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, NewPart);
	if (!Def)
	{
		return;
	}

	const FPrimaryAssetId DefId = Def->GetPrimaryAssetId();
	const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(this, DefId);
	if (!Row)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipComp] EquipPart: row 없음 (DefId=%s)"), *DefId.ToString());
		return;
	}
	const ENSPartSlot Slot = Row->PartSlot;

	DropPartInSlot(Slot, DropLocationOverride);

	FNSPartData Stored = NewPart;
	Stored.Slot = Slot;

	if (FNSPartData* Existing = FindPart(Slot))
	{
		*Existing = Stored;
	}
	else
	{
		EquippedParts.Add(Stored);
	}

	ApplyPartEffect(Slot);
	GrantAbilities(Slot);

	OnPartChanged.Broadcast(Slot, Stored);
}

void UNSPartEquipComponent::ClearAll()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	for (const FNSPartData& Part : EquippedParts)
	{
		RemovePartEffects(Part.Slot);
	}
	EquippedParts.Empty();
}

void UNSPartEquipComponent::CopyRunStateFrom(const UNSPartEquipComponent* Source)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!Source)
	{
		return;
	}

	// 데이터만 이관, 핸들은 이전 ASC 기준이라 무효이므로 복사하지 않음 (ReapplyAll에서 새로 발급)
	EquippedParts = Source->EquippedParts;
}

void UNSPartEquipComponent::ReapplyAll()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	// 이전 ASC 기준 핸들 폐기 후 슬롯별 GE/GA 재발급
	ActiveGEHandles.Empty();
	GrantedAbilityHandlesBySlot.Empty();

	for (const FNSPartData& Part : EquippedParts)
	{
		ApplyPartEffect(Part.Slot);
		GrantAbilities(Part.Slot);
	}
}

bool UNSPartEquipComponent::HasEquippedPart(ENSPartSlot Slot) const
{
	return FindPart(Slot) != nullptr;
}

const FNSPartData* UNSPartEquipComponent::GetEquippedPart(ENSPartSlot Slot) const
{
	return FindPart(Slot);
}

FNSPartData* UNSPartEquipComponent::FindPart(ENSPartSlot Slot)
{
	return EquippedParts.FindByPredicate([Slot](const FNSPartData& P) { return P.Slot==Slot; });
}

const FNSPartData* UNSPartEquipComponent::FindPart(ENSPartSlot Slot) const
{
	return EquippedParts.FindByPredicate([Slot](const FNSPartData& P) {return P.Slot == Slot;});
}

// ================================================================
// 드롭 / 효과 제거
// ================================================================

void UNSPartEquipComponent::DropPartInSlot(ENSPartSlot Slot, TOptional<FVector> LocationOverride)
{
	FNSPartData* Existing = FindPart(Slot);
	if (!Existing)
	{
		return;
	}

	const FNSPartData Dropped = *Existing;

	RemovePartEffects(Slot);
	EquippedParts.RemoveAll([Slot](const FNSPartData& P) { return P.Slot==Slot; });

	// 파츠 교체할때 위치 변경
	const APlayerState* PS = Cast<APlayerState>(GetOwner());
	const APawn* Pawn = PS ? PS->GetPawn() : nullptr;
	const FVector Location = LocationOverride.IsSet() ? LocationOverride.GetValue()
		: (Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector);
	
	// 실제로 그 위치에 장착한 하츠 드롭
	SpawnDroppedPart(Dropped, Location);
}

void UNSPartEquipComponent::SpawnDroppedPart(const FNSPartData& Part, const FVector& Location)
{
	ANSDroppedPart::SpawnInWorld(GetWorld(), DroppedPartClass, Part, Location);
}

void UNSPartEquipComponent::RemovePartEffects(ENSPartSlot Slot)
{
	RemoveGEForSlot(Slot);
	RemoveAbilitiesForSlot(Slot);

	if (TSharedPtr<FStreamableHandle>* LoadHandle = EffectLoadHandles.Find(Slot))
	{
		if (LoadHandle->IsValid())
		{
			(*LoadHandle)->CancelHandle();
		}
		EffectLoadHandles.Remove(Slot);
	}
	if (TSharedPtr<FStreamableHandle>* LoadHandle = AbilityLoadHandles.Find(Slot))
	{
		if (LoadHandle->IsValid())
		{
			(*LoadHandle)->CancelHandle();
		}
		AbilityLoadHandles.Remove(Slot);
	}
}

void UNSPartEquipComponent::RemoveGEForSlot(ENSPartSlot Slot)
{
	FActiveGameplayEffectHandle* Handle = ActiveGEHandles.Find(Slot);
	if (!Handle || !Handle->IsValid())
	{
		ActiveGEHandles.Remove(Slot);
		return;
	}

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		ASC->RemoveActiveGameplayEffect(*Handle);
	}
	ActiveGEHandles.Remove(Slot);
}

void UNSPartEquipComponent::RemoveAbilitiesForSlot(ENSPartSlot Slot)
{
	TArray<FGameplayAbilitySpecHandle>* Handles = GrantedAbilityHandlesBySlot.Find(Slot);
	if (!Handles || Handles->Num() == 0)
	{
		GrantedAbilityHandlesBySlot.Remove(Slot);
		return;
	}

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		for (const FGameplayAbilitySpecHandle& Handle : *Handles)
		{
			if (Handle.IsValid())
			{
				ASC->ClearAbility(Handle);
			}
		}
	}
	GrantedAbilityHandlesBySlot.Remove(Slot);
}

// ================================================================
// GE 적용 및 로드
// ================================================================

void UNSPartEquipComponent::ApplyPartEffect(ENSPartSlot Slot)
{
	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}
	
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
	if (!Def || Def->EffectClass.IsNull())
	{
		return;
	}
	
	if (TSubclassOf<UGameplayEffect> Loaded = Def->EffectClass.Get())
	{
		Internal_ApplyGE(Slot, Loaded);
		return;
	}
	
	const FSoftObjectPath Path = Def->EffectClass.ToSoftObjectPath();
	TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Path, FStreamableDelegate::CreateUObject(this, &UNSPartEquipComponent::OnEffectLoaded, Slot));
	EffectLoadHandles.Add(Slot, Handle);
}

void UNSPartEquipComponent::Internal_ApplyGE(ENSPartSlot Slot, TSubclassOf<UGameplayEffect> GEClass)
{
	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	// 리롤같이 스텟변경이 있는경우 대비용으로 기존 GE먼저 제거
	RemoveGEForSlot(Slot);

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(NSGameplayTags::Part_Value, Part->CurrentValue);
	ActiveGEHandles.Add(Slot, ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data));
}

void UNSPartEquipComponent::OnEffectLoaded(ENSPartSlot Slot)
{
	EffectLoadHandles.Remove(Slot);
	
	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}
	
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
	if (!Def)
	{
		return;
	}
	
	TSubclassOf<UGameplayEffect> Loaded = Def->EffectClass.Get();
	if (!Loaded)
	{
		return;
	}
	Internal_ApplyGE(Slot, Loaded);
}

// ================================================================
// GA 부여 및 로드
// ================================================================

void UNSPartEquipComponent::GrantAbilities(ENSPartSlot Slot)
{
	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}
	
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
	if (!Def || Def->GrantedAbilities.Num() == 0)
	{
		return;
	}
	
	bool bAllLoaded = true;
	for (const TSoftClassPtr<UGameplayAbility>& AbilityClass : Def->GrantedAbilities)
	{
		if (!AbilityClass.Get())
		{
			bAllLoaded = false;
			break;
		}
	}
	if (bAllLoaded)
	{
		OnAbilitiesLoaded(Slot);
		return;
	}
	
	TArray<FSoftObjectPath> Paths;
	for (const TSoftClassPtr<UGameplayAbility>& AbilityClass : Def->GrantedAbilities)
	{
		if (!AbilityClass.IsNull())
		{
			Paths.Add(AbilityClass.ToSoftObjectPath());
		}
	}
	
	if (Paths.Num() == 0)
	{
		OnAbilitiesLoaded(Slot);
		return;
	}
	
	TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Paths, FStreamableDelegate::CreateUObject(this, &UNSPartEquipComponent::OnAbilitiesLoaded,
			Slot));
	AbilityLoadHandles.Add(Slot, Handle);
}

void UNSPartEquipComponent::OnAbilitiesLoaded(ENSPartSlot Slot)
{
	AbilityLoadHandles.Remove(Slot);
	
	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}
	
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
	if (!Def)
	{
		return;
	}
	
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}
	
	RemoveAbilitiesForSlot(Slot);
	
	TArray<FGameplayAbilitySpecHandle>& Handles = GrantedAbilityHandlesBySlot.FindOrAdd(Slot);
	for (const TSoftClassPtr<UGameplayAbility>& AbilityClass : Def->GrantedAbilities)
	{
		TSubclassOf<UGameplayAbility> Loaded = AbilityClass.Get();
		if (!Loaded)
		{
			continue;
		}
		FGameplayAbilitySpec Spec(Loaded, 1, INDEX_NONE, this);
		Handles.Add(ASC->GiveAbility(Spec));
	}
}

// ================================================================
// 리롤 / 등급업
// ================================================================

void UNSPartEquipComponent::RerollStat(ENSPartSlot Slot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}

	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
	const FPrimaryAssetId DefId = Def ? Def->GetPrimaryAssetId() : FPrimaryAssetId();
	const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(this, DefId);
	if (!Def || !Row || !Row->bCanReroll)
	{
		return;
	}
	
	Part->CurrentValue = RollValueForRarity(Def, Part->CurrentRarity);
	// TODO : 추후에 카운트에 따라 비용 증가시 사용
	Part->RollCount++;
	
	ApplyPartEffect(Slot);
	OnPartChanged.Broadcast(Slot, *Part);
}

void UNSPartEquipComponent::UpgradeRarity(ENSPartSlot Slot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}
	if (Part->CurrentRarity == ENSPartRarity::Legendary)
	{
		return;
	}
	if (FMath::FRand() > UpgradeSuccessChance)
	{
		return;
	}
	
	Part->CurrentRarity = static_cast<ENSPartRarity>(static_cast<uint8>(Part->CurrentRarity) + 1);
	
	if (UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part))
	{
		Part->CurrentValue = RollValueForRarity(Def, Part->CurrentRarity);
	}
	
	ApplyPartEffect(Slot);
	OnPartChanged.Broadcast(Slot, *Part);
}

float UNSPartEquipComponent::RollValueForRarity(const UNSPartDefinition* Def, ENSPartRarity Rarity) const
{
	if (!Def)
	{
		return 0.f;
	}

	const FPrimaryAssetId DefId = Def->GetPrimaryAssetId();
	const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(this, DefId);
	if (!Row)
	{
		return 0.f;
	}

	const FNSPartValueRange* Range = Row->ValueRange.Find(Rarity);
	return Range ? FMath::RandRange(Range->Min, Range->Max) : 0.f;
}

// ================================================================
// 조회 헬퍼
// ================================================================

UAbilitySystemComponent* UNSPartEquipComponent::GetOwnerASC() const
{
	APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (!PS)
	{
		return nullptr;
	}
	
	APawn* Pawn = PS->GetPawn();
	if (!Pawn)
	{
		return nullptr;
	}
	
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
	if (!ASI)
	{
		return nullptr;
	}
	return ASI->GetAbilitySystemComponent();
}

// ================================================================
// 리플리케이션 콜백
// ================================================================

void UNSPartEquipComponent::OnRep_EquippedParts()
{
	UE_LOG(LogTemp, Warning, TEXT("[EquipComp] OnRep_EquippedParts: 배열 크기=%d"), EquippedParts.Num());
	for (const FNSPartData& Part : EquippedParts)
	{
		OnPartChanged.Broadcast(Part.Slot, Part);
	}
}

// ================================================================
// Server RPC
// ================================================================

void UNSPartEquipComponent::Server_RequestEquip_Implementation(FNSPartData NewPart)
{
	EquipPart(NewPart);
}

void UNSPartEquipComponent::Server_RequestReroll_Implementation(ENSPartSlot Slot)
{
	RerollStat(Slot);
}

void UNSPartEquipComponent::Server_RequestUpgradeRarity_Implementation(ENSPartSlot Slot)
{
	UpgradeRarity(Slot);
}

void UNSPartEquipComponent::Server_RequestPickup_Implementation(ANSDroppedPart* TargetPart)
{
	if (!TargetPart)
	{
		return;
	}

	const APlayerState* PS = Cast<APlayerState>(GetOwner());
	APawn* Pawn = PS ? PS->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	TargetPart->TryPickup(Pawn);
}
