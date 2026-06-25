// Copyright 2026 One Team. All rights reserved.

#include "NSAugmentInventoryComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Augment/NSAugmentDefinition.h"
#include "NeoSanctum/Tag/NSGameplayTags_Augment.h"

UNSAugmentInventoryComponent::UNSAugmentInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSAugmentInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UNSAugmentInventoryComponent, Owned, COND_OwnerOnly);
}

void UNSAugmentInventoryComponent::ApplyAugment(const FPrimaryAssetId& DefId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	// DefId로 증강 값을 가져와야 함
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data || !Data->IsRunReady())
	{
		return;
	}

	UNSAugmentDefinition* Def = Data->GetData<UNSAugmentDefinition>(DefId);
	if (!Def)
	{
		return;
	}
	
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}
	
	FNSAugmentInstance* Existing = Owned.FindByPredicate(
		[&DefId](const FNSAugmentInstance& Instance) 
		{ return Instance.DefId == DefId; });
	
	// 증강이 존재하면
	if (Existing)
	{
		Existing->Stacks++;
		// Common / Rare / Epic / Legendary(수치강화)
		ApplyStackEffect(*Existing, Def, ASC);
	} else
	{
		const bool bIsHaveLegendarySlot = Def->Rarity == ENSAugmentRarity::Legendary && !Def->GrantedAbilityClass.IsNull();
		
		FNSAugmentInstance NewInstance;
		NewInstance.DefId = DefId;
		NewInstance.Rarity = Def->Rarity;
		NewInstance.Stacks = 1;
		NewInstance.bCountsAsLegendarySlot = bIsHaveLegendarySlot;
		// Common / Rare / Epic / Legendary(수치강화)
		ApplyStackEffect(NewInstance, Def, ASC);
		// Legendary 기믹 GA
		GrantMechanicAbility(NewInstance, Def, ASC);
		Owned.Add(NewInstance);
	}
	OnInventoryChanged.Broadcast();
}

void UNSAugmentInventoryComponent::ApplyStackEffect(FNSAugmentInstance& Inst, UNSAugmentDefinition* Def, UAbilitySystemComponent* ASC)
{
	if (Def->StackEffectClass.IsNull())
	{
		return;
	}

	// 처음 골랐을때는 무시, 중복 증강을 또 고른경우 핸들을 제거하고 아래에서 다시 적용
	if (Inst.EffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(Inst.EffectHandle);
		Inst.EffectHandle.Invalidate();
	}
	
	// InRunData 번들로 미리 로드되어 DataSubsystem 캐시에 상주하므로 동기 로드 없이 .Get()으로 조회
	UClass* GEClass = Def->StackEffectClass.Get();
	if (!GEClass)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GEClass, 1.f, ContextHandle);

	if (!SpecHandle.IsValid())
	{
		return;
	}
	
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Augment_SetByCaller_Stack, static_cast<float>(Inst.Stacks));
	Inst.EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void UNSAugmentInventoryComponent::GrantMechanicAbility(FNSAugmentInstance& Inst, UNSAugmentDefinition* Def, UAbilitySystemComponent* ASC)
{
	if (Def->GrantedAbilityClass.IsNull())
	{
		return;
	}
	
	// InRunData 번들로 미리 로드되어 DataSubsystem 캐시에 상주하므로 동기 로드 없이 .Get()으로 조회
	UClass* GAClass = Def->GrantedAbilityClass.Get();
	if (!GAClass)
	{
		return;
	}

	TSubclassOf<UGameplayAbility> AbilityClass = GAClass;
	Inst.AbilityHandle = ASC->GiveAbility((FGameplayAbilitySpec(AbilityClass, 1,INDEX_NONE, this)));
}
	
void UNSAugmentInventoryComponent::ClearAll()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	UAbilitySystemComponent* ASC = GetOwnerASC();
	for (FNSAugmentInstance& Inst : Owned)
	{
		if (ASC && Inst.EffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Inst.EffectHandle);
		}
		if (ASC && Inst.AbilityHandle.IsValid())
		{
			ASC->ClearAbility(Inst.AbilityHandle);
		}
	}
	Owned.Reset();
	OnInventoryChanged.Broadcast();
}

void UNSAugmentInventoryComponent::CopyRunStateFrom(const UNSAugmentInventoryComponent* Source)
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
	Owned.Reset();
	for (const FNSAugmentInstance& SourceInst : Source->Owned)
	{
		FNSAugmentInstance NewInstance;
		NewInstance.DefId = SourceInst.DefId;
		NewInstance.Rarity = SourceInst.Rarity;
		NewInstance.Stacks = SourceInst.Stacks;
		NewInstance.bCountsAsLegendarySlot = SourceInst.bCountsAsLegendarySlot;
		Owned.Add(NewInstance);
	}
}

void UNSAugmentInventoryComponent::ReapplyAll()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data || !Data->IsRunReady())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	for (FNSAugmentInstance& Inst : Owned)
	{
		UNSAugmentDefinition* Def = Data->GetData<UNSAugmentDefinition>(Inst.DefId);
		if (!Def)
		{
			continue;
		}

		// 새 ASC 기준 핸들로 갱신, 누적 Stacks는 SetByCaller로 한 번에 적용
		Inst.EffectHandle.Invalidate();
		Inst.AbilityHandle = FGameplayAbilitySpecHandle();
		ApplyStackEffect(Inst, Def, ASC);
		GrantMechanicAbility(Inst, Def, ASC);
	}

	OnInventoryChanged.Broadcast();
}

void UNSAugmentInventoryComponent::OnRep_Owned()
{
	// 복제되었을때 어떤 트리거 -> 가지고 있는 증강 아이콘 갱신 -> 델리게이트로 브로드캐스트
	OnInventoryChanged.Broadcast();
}

int32 UNSAugmentInventoryComponent::GetStackCount(const FPrimaryAssetId& DefId) const
{
	const FNSAugmentInstance* Found = Owned.FindByPredicate(
		[&DefId](const FNSAugmentInstance& Instance) { return Instance.DefId == DefId; });
	return Found ? Found->Stacks : 0;
}

UAbilitySystemComponent* UNSAugmentInventoryComponent::GetOwnerASC() const
{
	if (IAbilitySystemInterface* ASInterface = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return ASInterface->GetAbilitySystemComponent();
	}
	return nullptr;
}

int32 UNSAugmentInventoryComponent::GetLegendaryCount() const
{
	int32 LegendaryCount = 0;
	for (const FNSAugmentInstance& Own : Owned)
	{
		if (Own.bCountsAsLegendarySlot)
		{
			++LegendaryCount;
		}
	}
	return LegendaryCount;
}


