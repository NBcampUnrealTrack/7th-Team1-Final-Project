// Copyright 2026 One Team. All rights reserved.

#include "NSPartEquipComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Tag/NSGameplayTags_Part.h"

UNSPartEquipComponent::UNSPartEquipComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNSPartEquipComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UNSPartEquipComponent, EquippedPart, COND_OwnerOnly);
}

// 파츠 장착 -> 기존 파츠 드롭 -> 새 파츠 장착(GE,GA적용)
void UNSPartEquipComponent::EquipPart(const FNSPartData& NewPart)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	APlayerState* PS = Cast<APlayerState>(GetOwner());
	APawn* Pawn = PS ? PS->GetPawn() : nullptr;
	FVector DropLocation = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;

	DropCurrentPart(DropLocation);

	EquippedPart = NewPart;
	ApplyPartEffect();
	GrantAbilities();
}

// 수치 리롤
void UNSPartEquipComponent::RerollStat()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	if (!HasEquippedPart())
	{
		return;
	}

	UNSPartDefinition* Def = GetEquippedDefinition();
	if (!Def || !Def->bCanReroll)
	{
		return;
	}

	EquippedPart.CurrentValue = RollValueForRarity(Def, EquippedPart.CurrentRarity);
	EquippedPart.RollCount++;

	RemoveActiveGE();
	ApplyPartEffect();
}

void UNSPartEquipComponent::UpgradeRarity()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	if (!HasEquippedPart())
	{
		return;
	}
	if (EquippedPart.CurrentRarity == ENSPartRarity::Legendary)
	{
		return;
	}

	if (FMath::FRand() > UpgradeSuccessChance)
	{
		return;
	}

	EquippedPart.CurrentRarity = static_cast<ENSPartRarity>(static_cast<uint8>(EquippedPart.CurrentRarity) + 1);

	UNSPartDefinition* Def = GetEquippedDefinition();
	if (Def)
	{
		EquippedPart.CurrentValue = RollValueForRarity(Def, EquippedPart.CurrentRarity);
	}

	RemoveActiveGE();
	ApplyPartEffect();
}

// TODO : 장착 파츠 드롭, 파츠 드롭 구현 해야함
void UNSPartEquipComponent::DropCurrentPart(const FVector& Location)
{
	if (!HasEquippedPart())
	{
		return;
	}

	RemoveActiveGE();
	RemoveGrantedAbilities();
	EquippedPart = FNSPartData{};
}

void UNSPartEquipComponent::ClearAll()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	RemoveActiveGE();
	RemoveGrantedAbilities();
	EquippedPart = FNSPartData{};
}

void UNSPartEquipComponent::RemoveActiveGE()
{
	if (!ActiveGEHandle.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	ASC->RemoveActiveGameplayEffect(ActiveGEHandle);
	ActiveGEHandle.Invalidate();
}

void UNSPartEquipComponent::RemoveGrantedAbilities()
{
	if (GrantedAbilityHandles.Num() == 0)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		GrantedAbilityHandles.Empty();
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}
	GrantedAbilityHandles.Empty();
}

// GA적용
void UNSPartEquipComponent::GrantAbilities()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	UNSPartDefinition* Def = GetEquippedDefinition();
	if (!Def || Def->GrantedAbilities.Num() == 0)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	// 데이터 서브 시스템에서 메모리에 올려놓아 동기 Get() 가능
	for (const TSoftClassPtr<UGameplayAbility>& AbilityClass : Def->GrantedAbilities)
	{
		TSubclassOf<UGameplayAbility> LoadedClass = AbilityClass.Get();
		if (!LoadedClass)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(LoadedClass, 1, INDEX_NONE, this);
		GrantedAbilityHandles.Add(ASC->GiveAbility(Spec));
	}
}

// 파츠 GE적용
void UNSPartEquipComponent::ApplyPartEffect()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	UNSPartDefinition* Def = GetEquippedDefinition();
	if (!Def || Def->EffectClass.IsNull())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	// 데이터 서브 시스템에서 메모리에 올려놓기 때문에 동기 Get() 가능 (바로 반환됨)
	TSubclassOf<UGameplayEffect> GEClass = Def->EffectClass.Get();
	if (!GEClass)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	Spec.Data->SetSetByCallerMagnitude(NSGameplayTags::Part_Value, EquippedPart.CurrentValue);
	ActiveGEHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
}

// 미리 지정해놓은 ValueRange내에서 리롤
float UNSPartEquipComponent::RollValueForRarity(const UNSPartDefinition* Def, ENSPartRarity Rarity) const
{
	const FNSPartValueRange* Range = Def->ValueRange.Find(Rarity);
	if (!Range)
	{
		return 0.f;
	}
	return FMath::RandRange(Range->Min, Range->Max);
}

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

UNSPartDefinition* UNSPartEquipComponent::GetEquippedDefinition() const
{
	if (EquippedPart.DefinitionPtr.IsNull())
	{
		return nullptr;
	}

	UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(this);
	if (!DataSS)
	{
		return EquippedPart.DefinitionPtr.Get();
	}

	const FPrimaryAssetId Id = UAssetManager::Get().GetPrimaryAssetIdForPath(EquippedPart.DefinitionPtr.ToSoftObjectPath());
	UNSPartDefinition* Cached = DataSS->GetData<UNSPartDefinition>(Id);

	// 캐시에 없으면(데이터 미등록 등) nullptr
	return Cached ? Cached : EquippedPart.DefinitionPtr.Get();
}

void UNSPartEquipComponent::OnRep_EquippedPart()
{
	OnPartChanged.Broadcast(EquippedPart);
}

// Server RPC
void UNSPartEquipComponent::ServerRequestEquip_Implementation(FNSPartData NewPart)
{
	EquipPart(NewPart);
}

void UNSPartEquipComponent::ServerRequestReroll_Implementation()
{
	RerollStat();
}

void UNSPartEquipComponent::ServerRequestUpgradeRarity_Implementation()
{
	UpgradeRarity();
}
