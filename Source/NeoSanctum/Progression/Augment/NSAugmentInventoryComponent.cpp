// Copyright 2026 One Team. All rights reserved.

#include "NSAugmentInventoryComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Augment/NSAugmentDefinition.h"
#include "NeoSanctum/Debug/Logging/NSLogCategories.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/GAS/Stats/NSCombatStatAttributeMapping.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"


namespace
{
	bool TryGetAttributeSetByCallerTag(const FNSAugmentDefinitionRow& Row, FGameplayTag& OutSetByCallerTag)
	{
		const FNSCombatStatAttributeMapping* Mapping = NSCombatStatAttribute::FindMapping(Row.StatTag);
		if (!Mapping)
		{
			return false;
		}

		switch (Row.Operation)
		{
		case ENSCombatStatModifierOperation::Add:
			OutSetByCallerTag = Mapping->AddSetByCallerTag;
			return OutSetByCallerTag.IsValid();

		case ENSCombatStatModifierOperation::Multiply:
			OutSetByCallerTag = Mapping->MultiplySetByCallerTag;
			return OutSetByCallerTag.IsValid();

		default:
			return false;
		}
	}

	// StackEffect 재적용 전후 Max Attribute 증가분만 Current Attribute에 반영하기 위한 스냅샷.
	struct FNSAugmentMaxDeltaSnapshot
	{
		bool bShouldAdjust = false;
		// Health처럼 0에서 보정되면 부활처럼 보일 수 있는 Attribute는 현재값 보정을 건너뜀.
		bool bSkipIfOldCurrentIsZero = false;

		FGameplayAttribute CurrentAttribute;
		FGameplayAttribute MaxAttribute;

		float OldCurrentValue = 0.0f;
		float OldMaxValue = 0.0f;
	};

	bool HasDefinitionRowWithStatTag(
		const TArray<FNSAugmentDefinitionRow>& DefinitionRows,
		const FGameplayTag StatTag)
	{
		return DefinitionRows.ContainsByPredicate(
			[StatTag](const FNSAugmentDefinitionRow& Row)
			{
				return Row.StatTag == StatTag;
			}
		);
	}

	FNSAugmentMaxDeltaSnapshot CaptureMaxDeltaSnapshot(
		UAbilitySystemComponent* ASC,
		const TArray<FNSAugmentDefinitionRow>& DefinitionRows,
		const FGameplayTag SourceStatTag,
		const FGameplayAttribute CurrentAttribute,
		const FGameplayAttribute MaxAttribute,
		const bool bSkipIfOldCurrentIsZero = false)
	{
		FNSAugmentMaxDeltaSnapshot Snapshot;
		Snapshot.bShouldAdjust = HasDefinitionRowWithStatTag(DefinitionRows, SourceStatTag);
		Snapshot.bSkipIfOldCurrentIsZero = bSkipIfOldCurrentIsZero;
		Snapshot.CurrentAttribute = CurrentAttribute;
		Snapshot.MaxAttribute = MaxAttribute;

		if (!ASC || !Snapshot.bShouldAdjust)
		{
			return Snapshot;
		}

		Snapshot.OldCurrentValue = ASC->GetNumericAttribute(CurrentAttribute);
		Snapshot.OldMaxValue = ASC->GetNumericAttribute(MaxAttribute);

		return Snapshot;
	}

	void ApplyMaxDeltaSnapshot(
		UAbilitySystemComponent* ASC,
		const FNSAugmentMaxDeltaSnapshot& Snapshot)
	{
		if (!ASC || !Snapshot.bShouldAdjust)
		{
			return;
		}

		if (Snapshot.bSkipIfOldCurrentIsZero && Snapshot.OldCurrentValue <= 0.0f)
		{
			return;
		}

		const float NewMaxValue = ASC->GetNumericAttribute(Snapshot.MaxAttribute);
		const float MaxDelta = NewMaxValue - Snapshot.OldMaxValue;

		float NewCurrentValue = Snapshot.OldCurrentValue;

		if (MaxDelta > KINDA_SMALL_NUMBER)
		{
			// Max가 증가한 경우에는 증가분 만큼 Current도 같이 올림.
			NewCurrentValue += MaxDelta;
		}

		// Max가 감소했거나 기존 Current가 새 Max보다 큰 경우에는 새 Max로 클램프.
		NewCurrentValue = FMath::Clamp(NewCurrentValue, 0.0f, NewMaxValue);

		ASC->SetNumericAttributeBase(Snapshot.CurrentAttribute, NewCurrentValue);
	}
}

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

bool UNSAugmentInventoryComponent::TryFindDefinitionRows(
	UNSDataSubsystem* Data,
	const FPrimaryAssetId& DefId, 
	TArray<FNSAugmentDefinitionRow>& OutRows) const
{
	OutRows.Reset();
	
	const UDataTable* AugmentDefinitionTable =
		Data ? Data->GetCurrentAugmentDefinitionTable() : nullptr;
	
	if (!IsValid(AugmentDefinitionTable) || !DefId.IsValid())
	{
		return false;
	}
	
	if (AugmentDefinitionTable->GetRowStruct() != FNSAugmentDefinitionRow::StaticStruct())
	{
		NS_OBJ_LOG(LogNS, Warning,
			"증강 효과 정의 DataTable의 Row Struct가 올바르지 않습니다. Table={Table}",
			("Table", AugmentDefinitionTable->GetName())
		);
		return false;
	}
	
	const FString ContextString = TEXT("AugmentInventoryDefinitionLookup");
	
	for (const FName& RowName : AugmentDefinitionTable->GetRowNames())
	{
		const FNSAugmentDefinitionRow* Row =
			AugmentDefinitionTable->FindRow<FNSAugmentDefinitionRow>(RowName, ContextString, false);
		
		if (!Row || !Row->bEnabled || Row->Definition.IsNull())
		{
			continue;
		}
		
		const FPrimaryAssetId RowDefId(UNSDataSubsystem::AugmentAssetType, FName(*Row->Definition.GetAssetName()));
		
		if (RowDefId == DefId)
		{
			OutRows.Add(*Row);
		}
	}
	
	if (!OutRows.IsEmpty())
	{
		return true;
	}
	
	NS_OBJ_LOG(LogNS, Warning,
		"보유 증강에 대응하는 정의 Row를 찾지 못했습니다. DefId={DefId}",
		("DefId", DefId.ToString())
	);
	
	return false;
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
	
	TArray<FNSAugmentDefinitionRow> DefinitionRows;
	if (!TryFindDefinitionRows(Data, DefId, DefinitionRows))
	{
		return;
	}
	
	const FNSAugmentDefinitionRow& RepresentativeRow = DefinitionRows[0];
	
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
		ApplyStackEffect(*Existing, DefinitionRows, ASC, true);
	} else
	{
		FNSAugmentInstance NewInstance;
		NewInstance.DefId = DefId;
		NewInstance.Rarity = RepresentativeRow.Rarity;
		NewInstance.Stacks = 1;
		NewInstance.bCountsAsLegendarySlot = RepresentativeRow.bCountAsLegendarySlot;
		// Common / Rare / Epic / Legendary(수치강화)
		ApplyStackEffect(NewInstance, DefinitionRows, ASC, true);
		// Legendary 기믹 GA
		GrantMechanicAbility(NewInstance, Def, ASC);
		Owned.Add(NewInstance);
	}
	OnInventoryChanged.Broadcast();
}

void UNSAugmentInventoryComponent::ApplyStackEffect(
	FNSAugmentInstance& Inst,
	const TArray<FNSAugmentDefinitionRow>& DefinitionRows,
	UAbilitySystemComponent* ASC,
	bool bAdjustCurrentByMaxDelta)
{
	// 스킬 수치만 바꾸는 증강은 기존 CombatStat Modifier 흐름만 사용하고 Attribute GE는 적용하지 않음.
	const bool bHasAttributeEffectRow = DefinitionRows.ContainsByPredicate(
		[](const FNSAugmentDefinitionRow& Row)
		{
			return NSCombatStatAttribute::FindMapping(Row.StatTag) != nullptr;
		}
	);

	if (!bHasAttributeEffectRow)
	{
		if (Inst.EffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Inst.EffectHandle);
			Inst.EffectHandle.Invalidate();
		}

		return;
	}

	if (!SharedAttributeStackEffectClass)
	{
		NS_OBJ_LOG(LogNS, Warning,
			"공용 어트리뷰트 스택 이펙트 클래스가 설정되어 있지 않습니다. DefId={DefId}",
			("DefId", Inst.DefId.ToString())
		);
		return;
	}

	FNSAugmentMaxDeltaSnapshot HealthSnapshot;
	FNSAugmentMaxDeltaSnapshot ShieldSnapshot;
	FNSAugmentMaxDeltaSnapshot AmmoSnapshot;

	// 새 증강 획득/스택 증가 시에만 Max 증가분을 Current에 더하기 위해 적용 전 값을 저장.
	// ReapplyAll()에서는 false로 호출해 스테이지 이동 중 의도치 않은 회복/탄약 회복을 막음.
	if (bAdjustCurrentByMaxDelta)
	{
		HealthSnapshot = CaptureMaxDeltaSnapshot(
			ASC,
			DefinitionRows,
			NSGameplayTags::CombatStat_MaxHealth,
			UNSBaseAttributeSet::GetHealthAttribute(),
			UNSBaseAttributeSet::GetMaxHealthAttribute(),
			true
		);
	
		ShieldSnapshot = CaptureMaxDeltaSnapshot(
			ASC,
			DefinitionRows,
			NSGameplayTags::CombatStat_MaxShield,
			UNSPlayerAttributeSet::GetShieldAttribute(),
			UNSPlayerAttributeSet::GetMaxShieldAttribute()
		);

		AmmoSnapshot = CaptureMaxDeltaSnapshot(
			ASC,
			DefinitionRows,
			NSGameplayTags::CombatStat_MaxAmmo,
			UNSPlayerAttributeSet::GetAmmoAttribute(),
			UNSPlayerAttributeSet::GetMaxAmmoAttribute()
		);
	}
	
	// 처음 골랐을때는 무시, 중복 증강을 또 고른경우 핸들을 제거하고 아래에서 다시 적용
	if (Inst.EffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(Inst.EffectHandle);
		Inst.EffectHandle.Invalidate();
	}
	
	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(SharedAttributeStackEffectClass, 1.0f, ContextHandle);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	NSCombatStatAttribute::InitializeNeutralSetByCallers(SpecHandle);
	
	bool bAppliedAnyPayload = false;

	for (const FNSAugmentDefinitionRow& Row : DefinitionRows)
	{
		const bool bHasAttributeMapping = NSCombatStatAttribute::FindMapping(Row.StatTag) != nullptr;

		FGameplayTag SetByCallerTag;
		if (!TryGetAttributeSetByCallerTag(Row, SetByCallerTag))
		{
			if (bHasAttributeMapping)
			{
				NS_OBJ_LOG(LogNS, Warning,
					"증강 어트리뷰트 행에 지원하지 않는 연산이 설정되어 있습니다. DefId={DefId}, StatTag={StatTag}, Operation={Operation}",
					("DefId", Inst.DefId.ToString()),
					("StatTag", Row.StatTag.ToString()),
					("Operation", StaticEnum<ENSCombatStatModifierOperation>()->GetNameStringByValue(static_cast<int64>(Row.Operation)))
				);
			}
			continue;
		}

		float Magnitude = 0.0f;
		if (!TryCalculateStackEffectMagnitude(Row, Inst.Stacks, Magnitude))
		{
			NS_OBJ_LOG(LogNS, Warning,
				"증강 어트리뷰트 적용값 계산에 실패했습니다. DefId={DefId}, StatTag={StatTag}, Stacks={Stacks}",
				("DefId", Inst.DefId.ToString()),
				("StatTag", Row.StatTag.ToString()),
				("Stacks", Inst.Stacks)
			);
			continue;
		}

		SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, Magnitude);
		bAppliedAnyPayload = true;
	}

	if (!bAppliedAnyPayload)
	{
		return;
	}

	Inst.EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	
	if (bAdjustCurrentByMaxDelta)
	{
		// 새 StackEffect 적용으로 증가한 Max 값만큼 Health / Shield / Ammo 현재값을 보정.
		ApplyMaxDeltaSnapshot(ASC, HealthSnapshot);
		ApplyMaxDeltaSnapshot(ASC, ShieldSnapshot);
		ApplyMaxDeltaSnapshot(ASC, AmmoSnapshot);
	}
}

bool UNSAugmentInventoryComponent::TryCalculateStackEffectMagnitude(
	const FNSAugmentDefinitionRow& DefinitionRow,
	int32 Stacks, 
	float& OutMagnitude)
{
	if (Stacks <= 0)
	{
		return false;
	}

	switch (DefinitionRow.Operation)
	{
	case ENSCombatStatModifierOperation::Add:
		OutMagnitude = DefinitionRow.ValuePerStack * static_cast<float>(Stacks);
		return true;
			
	case ENSCombatStatModifierOperation::Multiply:
		OutMagnitude = NSAugment::CalculateStackedMultiplyPercent(DefinitionRow.ValuePerStack, Stacks);
		return OutMagnitude > 0.0f;
			
	default:
		return false;
	}
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
		TArray<FNSAugmentDefinitionRow> DefinitionRows;
		if (!TryFindDefinitionRows(Data, Inst.DefId, DefinitionRows))
		{
			continue;
		}
		
		ApplyStackEffect(Inst, DefinitionRows, ASC, false);
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

bool UNSAugmentInventoryComponent::IsAttributeStatTag(const FGameplayTag& StatTag)
{
	return NSCombatStatAttribute::FindMapping(StatTag) != nullptr;
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
