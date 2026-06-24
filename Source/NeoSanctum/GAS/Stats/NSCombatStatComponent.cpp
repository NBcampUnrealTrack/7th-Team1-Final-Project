// Copyright 2026 One Team. All rights reserved.


#include "NSCombatStatComponent.h"

#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Augment/NSAugmentInventoryComponent.h"


UNSCombatStatComponent::UNSCombatStatComponent()
{

	PrimaryComponentTick.bCanEverTick = false;
}

void UNSCombatStatComponent::BeginPlay()
{
	Super::BeginPlay();

	RebuildBaseStatCache();
	
	// Modifier DataTable은 먼저 원본 기준으로 캐싱
	RebuildAugmentSourceCache();
	
	// AugmentInventory 변경 이벤트를 받아 Active Modifier를 갱신
	BindAugmentInventory();
	
	// 이미 보유 중인 증강이 있다면 현재 상태 기준으로 한 번 갱신
	RebuildActiveModifierCache();
}

void UNSCombatStatComponent::RebuildBaseStatCache()
{
	CachedBaseStatsByAbility.Reset();
	
	if (!AbilityBaseStatTable)
	{
		NS_OBJ_LOG(LogNSGAS, Warning, "스킬 기본 스탯 DataTable이 설정되지 않았습니다.");
		
		return;
	}
	
	if (AbilityBaseStatTable->GetRowStruct() != FNSAbilityBaseStatRow::StaticStruct())
	{
		NS_OBJ_LOG(LogNSGAS, Warning,
			"스킬 기본 스탯 DataTable의 Row Struct가 올바르지 않습니다. Table={Table}",
			("Table", AbilityBaseStatTable->GetName())
		);
		
		return;
	}
	
	const FString ContextString = TEXT("AbilityBaseStatCache");
	
	for (const FName& RowName : AbilityBaseStatTable->GetRowNames())
	{
		const FNSAbilityBaseStatRow* Row = 
			AbilityBaseStatTable->FindRow<FNSAbilityBaseStatRow>(RowName, ContextString, false);
		
		if (!Row)
		{
			continue;
		}
		
		if (!Row->AbilityTag.IsValid() || !Row->StatTag.IsValid())
		{
			NS_OBJ_LOG(LogNSGAS, Warning,
				"유효하지 않은 스킬 스탯 Row입니다. RowName={RowName}, AbilityTag={AbilityTag}, StatTag={StatTag}",
				("RowName", RowName.ToString()),
				("AbilityTag", Row->AbilityTag.ToString()),
				("StatTag", Row->StatTag.ToString())
			);

			continue;
		}
		
		TMap<FGameplayTag, FNSCachedAbilityBaseStat>& StatMap = 
			CachedBaseStatsByAbility.FindOrAdd(Row->AbilityTag);
		
		if (StatMap.Contains(Row->StatTag))
		{
			// 같은 AbilityTag + StatTag 조합은 하나의 기본값만 허용
			NS_OBJ_LOG(LogNSGAS, Warning,
				"중복된 스킬 기본 스탯 Row입니다. RowName={RowName}, AbilityTag={AbilityTag}, StatTag={StatTag}",
				("RowName", RowName.ToString()),
				("AbilityTag", Row->AbilityTag.ToString()),
				("StatTag", Row->StatTag.ToString())
			);

			continue;
		}
		
		FNSCachedAbilityBaseStat CachedStat;
		CachedStat.BaseValue = Row->BaseValue;
		CachedStat.bModifiable = Row->bModifiable;
		
		StatMap.Add(Row->StatTag, CachedStat);
	}
	
	NS_OBJ_LOG(LogNSGAS, Log,
		"스킬 기본 스탯 캐시 생성 완료. AbilityCount={AbilityCount}",
		("AbilityCount", CachedBaseStatsByAbility.Num())
	);
}

bool UNSCombatStatComponent::TryGetBaseAbilityStat(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float& OutValue) const
{
	const TMap<FGameplayTag, FNSCachedAbilityBaseStat>* StatMap = CachedBaseStatsByAbility.Find(AbilityTag);
	
	if (!StatMap)
	{
		return false;
	}
	
	const FNSCachedAbilityBaseStat* CachedStat = StatMap->Find(StatTag);
	
	if (!CachedStat)
	{
		return false;
	}
	
	OutValue = CachedStat->BaseValue;
	return true;
}

bool UNSCombatStatComponent::IsAbilityStatModifiable(
	const FGameplayTag& AbilityTag, const FGameplayTag& StatTag) const
{
	const TMap<FGameplayTag, FNSCachedAbilityBaseStat>* StatMap = CachedBaseStatsByAbility.Find(AbilityTag);
	
	if (!StatMap)
	{
		return false;
	}
	
	const FNSCachedAbilityBaseStat* CachedStat = StatMap->Find(StatTag);
	
	if (!CachedStat)
	{
		return false;
	}
	
	return CachedStat->bModifiable;
}

bool UNSCombatStatComponent::TryGetFinalAbilityStat(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float& OutValue) const
{
	float BaseValue = 0.0f;
	
	if (!TryGetBaseAbilityStat(AbilityTag, StatTag, BaseValue))
	{
		return false;
	}
	
	const TMap<FGameplayTag, FNSCombatStatModifierSum>* StatMap = ActiveModifiersByAbility.Find(AbilityTag);
	
	if (!StatMap)
	{
		OutValue = BaseValue;
		if (const FNSCombatStatModifierSum* TemporaryModifierSum = FindTemporaryModifierSum(AbilityTag, StatTag))
		{
			ApplyTemporaryModifierToFinalValue(*TemporaryModifierSum, OutValue);
		}
		return true;
	}
	
	const FNSCombatStatModifierSum* ModifierSum = StatMap->Find(StatTag);
	
	if (!ModifierSum)
	{
		OutValue = BaseValue;
		if (const FNSCombatStatModifierSum* TemporaryModifierSum = FindTemporaryModifierSum(AbilityTag, StatTag))
		{
			ApplyTemporaryModifierToFinalValue(*TemporaryModifierSum, OutValue);
		}
		return true;
	}
	
	// 최종 스탯은 Add 보정을 먼저 더한 뒤 Multiply 보정을 곱함
	OutValue = (BaseValue + ModifierSum->AddValue) * ModifierSum->MultiplyValue;
	if (const FNSCombatStatModifierSum* TemporaryModifierSum = FindTemporaryModifierSum(AbilityTag, StatTag))
	{
		ApplyTemporaryModifierToFinalValue(*TemporaryModifierSum, OutValue);
	}

	return true;
}

FGuid UNSCombatStatComponent::AddTemporaryCombatStatModifier(
	const FGameplayTag& TargetAbilityTag,
	const FGameplayTag& StatTag,
	ENSCombatStatModifierOperation Operation,
	float Value,
	float Duration)
{
	if (!TargetAbilityTag.IsValid() || !StatTag.IsValid() || Duration <= 0.0f)
	{
		return FGuid();
	}

	if (!IsAbilityStatModifiable(TargetAbilityTag, StatTag))
	{
		return FGuid();
	}

	if (Operation == ENSCombatStatModifierOperation::Multiply && Value <= 0.0f)
	{
		return FGuid();
	}

	// Duration 만료 시 자동 제거할 타이머 등록
	UWorld* World = GetWorld();
	if (!World)
	{
		return FGuid();
	}

	FNSTemporaryCombatStatModifier TemporaryModifier;
	TemporaryModifier.Handle = FGuid::NewGuid();
	TemporaryModifier.TargetAbilityTag = TargetAbilityTag;
	TemporaryModifier.StatTag = StatTag;
	TemporaryModifier.Operation = Operation;
	TemporaryModifier.Value = Value;

	const FGuid Handle = TemporaryModifier.Handle;
	World->GetTimerManager().SetTimer(
		TemporaryModifier.ExpireTimerHandle,
		FTimerDelegate::CreateUObject(this, &ThisClass::RemoveTemporaryCombatStatModifier, Handle),
		Duration,
		false
	);

	// Modifier 등록 후 최종 계산용 캐시 갱신
	TemporaryModifiersByHandle.Add(Handle, TemporaryModifier);
	RebuildTemporaryModifierCache();

	return Handle;
}

void UNSCombatStatComponent::RemoveTemporaryCombatStatModifier(FGuid Handle)
{
	FNSTemporaryCombatStatModifier RemovedModifier;
	if (!TemporaryModifiersByHandle.RemoveAndCopyValue(Handle, RemovedModifier))
	{
		return;
	}

	// 수동 제거 시 만료 타이머 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RemovedModifier.ExpireTimerHandle);
	}

	// 제거 후 최종 계산용 캐시 갱신
	RebuildTemporaryModifierCache();
}

void UNSCombatStatComponent::BindAugmentInventory()
{
	ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(GetOwner());
	
	if (!NSPlayerState)
	{
		return;
	}
	
	UNSAugmentInventoryComponent* AugmentInventory = NSPlayerState->GetAugmentInventory();
	
	if (!AugmentInventory)
	{
		return;
	}
	
	CachedAugmentInventory = AugmentInventory;
	
	// 재초기화 상황에서 중복 바인딩을 방지
	AugmentInventory->OnInventoryChanged.RemoveDynamic(
		this, &UNSCombatStatComponent::HandleAugmentInventoryChanged);
	
	AugmentInventory->OnInventoryChanged.AddDynamic(
		this, &UNSCombatStatComponent::HandleAugmentInventoryChanged);
}

void UNSCombatStatComponent::HandleAugmentInventoryChanged()
{
	// 증강 보유 상태가 바뀌면 최종 계산용 Modifier만 다시 만듬
	RebuildActiveModifierCache();
}

void UNSCombatStatComponent::RebuildAugmentSourceCache()
{
	CachedModifierRowsByDefId.Reset();
	
	if (!AugmentDefinitionTable)
	{
		NS_OBJ_LOG(LogNSGAS, Warning, "증강 효과 정의 DataTable이 설정되지 않았습니다.");
		return;
	}
	
	if (AugmentDefinitionTable->GetRowStruct() != FNSAugmentDefinitionRow::StaticStruct())
	{
		NS_OBJ_LOG(LogNSGAS, Warning,
			"증강 효과 정의 DataTable의 Row Struct가 올바르지 않습니다. Table={Table}",
			("Table", AugmentDefinitionTable->GetName())
		);
		return;
	}
	
	const FString ContextString = "AugmentModifierCache";
	
	for (const FName& RowName : AugmentDefinitionTable->GetRowNames())
	{
		const FNSAugmentDefinitionRow* Row =
			AugmentDefinitionTable->FindRow<FNSAugmentDefinitionRow>(RowName, ContextString, false);
		
		if (!Row || !Row->bEnabled)
		{
			continue;
		}
		
		if (!Row->AugmentTag.IsValid() 
			|| !Row->OwnerCharacterTag.IsValid()
			|| Row->Definition.IsNull()
			|| !Row->TargetAbilityTag.IsValid() 
			|| !Row->StatTag.IsValid())
		{
			NS_OBJ_LOG(LogNSGAS, Warning,
				"유효하지 않은 증강 효과 정의 Row입니다. RowName={RowName}, AugmentTag={AugmentTag}",
				("RowName", RowName.ToString()),
				("AugmentTag", Row->AugmentTag.ToString())
			);
			continue;
		}
		
		if (Row->Operation == ENSCombatStatModifierOperation::Multiply && Row->ValuePerStack <= 0.0f)
		{
			NS_OBJ_LOG(LogNSGAS, Warning,
				"유효하지 않은 증강 Multiply Modifier Row입니다. RowName={RowName}, Value={Value}",
				("RowName", RowName.ToString()),
				("Value", Row->ValuePerStack)
			);
			continue;
		}
		
		// Inventory는 DefId를 저장하므로 Definition SoftPtr의 에셋 이름을 같은 DefId로 변환.
		const FPrimaryAssetId DefId(UNSDataSubsystem::AugmentAssetType, FName(*Row->Definition.GetAssetName()));
		
		if (!DefId.IsValid())
		{
			NS_OBJ_LOG(LogNSGAS, Warning,
				"증강 Definition에서 유효한 DefId를 만들지 못했습니다. RowName={RowName}",
				("RowName", RowName.ToString())
			);
			continue;
		}
		
		CachedModifierRowsByDefId.FindOrAdd(DefId).Add(*Row);
	}
	
	NS_OBJ_LOG(LogNSGAS, Log,
		"증강 CombatStat Modifier 캐시 생성 완료. DefinitionCount={DefinitionCount}",
		("DefinitionCount", CachedModifierRowsByDefId.Num())
	);
}

void UNSCombatStatComponent::RebuildActiveModifierCache()
{
	ActiveModifiersByAbility.Reset();
	
	UNSAugmentInventoryComponent* AugmentInventory = CachedAugmentInventory.Get();
	
	if (!AugmentInventory)
	{
		return;
	}
	
	for (const FNSAugmentInstance& OwnedAugment : AugmentInventory->GetOwned())
	{
		const TArray<FNSAugmentDefinitionRow>* ModifierRows =
			CachedModifierRowsByDefId.Find(OwnedAugment.DefId);
		
		if (!ModifierRows)
		{
			continue;
		}
		
		for (const FNSAugmentDefinitionRow& ModifierRow : *ModifierRows)
		{
			// 보유 증강의 현재 스택 수를 반영해 활성 Modifier를 누적
			ApplyModifierRow(ModifierRow, OwnedAugment.Stacks);
		}
	}
	
	NS_OBJ_LOG(LogNSGAS, Log,
		"CombatStat Active Modifier 캐시 갱신 완료. AbilityCount={AbilityCount}",
		("AbilityCount", ActiveModifiersByAbility.Num())
	);
}

void UNSCombatStatComponent::ApplyModifierRow(const FNSAugmentDefinitionRow& ModifierRow, int32 Stacks)
{
	if (Stacks <= 0)
	{
		return;
	}
	
	if (!IsAbilityStatModifiable(ModifierRow.TargetAbilityTag, ModifierRow.StatTag))
	{
		return;
	}
	
	TMap<FGameplayTag, FNSCombatStatModifierSum>& StatMap =
		ActiveModifiersByAbility.FindOrAdd(ModifierRow.TargetAbilityTag);
	
	FNSCombatStatModifierSum& ModifierSum = StatMap.FindOrAdd(ModifierRow.StatTag);

	switch (ModifierRow.Operation)
	{
	case ENSCombatStatModifierOperation::Add:
		// Add는 스택 수만큼 단순 누적
		ModifierSum.AddValue += ModifierRow.ValuePerStack * static_cast<float>(Stacks);
		break;
		
	case ENSCombatStatModifierOperation::Multiply:
		// Multiply는 스택마다 같은 배율을 반복 적용
		ModifierSum.MultiplyValue *= FMath::Pow(ModifierRow.ValuePerStack, static_cast<float>(Stacks));
		break;
		
	default:
		break;
	}
}

void UNSCombatStatComponent::RebuildTemporaryModifierCache()
{
	TemporaryModifiersByAbility.Reset();

	// 활성화된 TemporaryModifier(즉, 버프)를 최종 계산용 캐시에 누적
	for (const TPair<FGuid, FNSTemporaryCombatStatModifier>& TemporaryModifierPair : TemporaryModifiersByHandle)
	{
		const FNSTemporaryCombatStatModifier& TemporaryModifier = TemporaryModifierPair.Value;
		TMap<FGameplayTag, FNSCombatStatModifierSum>& StatMap =
			TemporaryModifiersByAbility.FindOrAdd(TemporaryModifier.TargetAbilityTag);

		FNSCombatStatModifierSum& ModifierSum = StatMap.FindOrAdd(TemporaryModifier.StatTag);

		switch (TemporaryModifier.Operation)
		{
		case ENSCombatStatModifierOperation::Add:
			ModifierSum.AddValue += TemporaryModifier.Value;
			break;
		case ENSCombatStatModifierOperation::Multiply:
			ModifierSum.MultiplyValue *= TemporaryModifier.Value;
			break;
		default:
			break;
		}
	}
}

const FNSCombatStatModifierSum* UNSCombatStatComponent::FindTemporaryModifierSum(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag) const
{
	const TMap<FGameplayTag, FNSCombatStatModifierSum>* StatMap = TemporaryModifiersByAbility.Find(AbilityTag);

	if (!StatMap)
	{
		return nullptr;
	}

	return StatMap->Find(StatTag);
}

void UNSCombatStatComponent::ApplyTemporaryModifierToFinalValue(
	const FNSCombatStatModifierSum& ModifierSum,
	float& InOutValue) const
{
	// 기존 Final 값 위에 TemporaryModifier 보정 적용
	InOutValue = (InOutValue + ModifierSum.AddValue) * ModifierSum.MultiplyValue;
}
