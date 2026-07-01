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
	
	if (TryResolveAbilityBaseStatTableFromDataSubsystem())
	{
		RebuildBaseStatCache();
	}
	else if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		if (!DataSubsystem->IsCommonReady())
		{
			// CommonData가 비동기로 아직 준비되지 않은 경우 완료 시점에 기본 스탯 캐시를 생성.
			DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &ThisClass::HandleCommonDataReady);
			DataSubsystem->OnCommonDataReady.AddDynamic(this, &ThisClass::HandleCommonDataReady);
		}
		else
		{
			NS_OBJ_LOG(LogNSGAS, Warning, "CommonData는 준비됐지만 스킬 기본 스탯 DataTable을 찾지 못했습니다.");
		}
	}
	else
	{
		NS_OBJ_LOG(LogNSGAS, Warning, "DataSubsystem을 찾지 못해 스킬 기본 스탯 DataTable을 조회할 수 없습니다.");
	}
	
	if (TryResolveAugmentDefinitionTableFromDataSubsystem())
	{
		RebuildAugmentSourceCache();
	}
	else if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		if (DataSubsystem->IsRunReady())
		{
			RebuildAugmentSourceCache();
		}
		else
		{
			// 인런 데이터가 아직 준비되지 않은 경우 완료 시점에 증강 Modifier 캐시를 생성.
			DataSubsystem->OnRunGameDataReady.RemoveDynamic(this, &ThisClass::HandleRunGameDataReady);
			DataSubsystem->OnRunGameDataReady.AddDynamic(this, &ThisClass::HandleRunGameDataReady);
		}
	}
	else
	{
		RebuildAugmentSourceCache();
	}
	
	// AugmentInventory 변경 이벤트를 받아 Active Modifier를 갱신
	BindAugmentInventory();
	
	// 이미 보유 중인 증강이 있다면 현재 상태 기준으로 한 번 갱신
	RebuildActiveModifierCache();
}

void UNSCombatStatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &ThisClass::HandleCommonDataReady);
		DataSubsystem->OnRunGameDataReady.RemoveDynamic(this, &ThisClass::HandleRunGameDataReady);
	}
	
	Super::EndPlay(EndPlayReason);
}

bool UNSCombatStatComponent::TryResolveAbilityBaseStatTableFromDataSubsystem()
{
	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem || !DataSubsystem->IsCommonReady())
	{
		return false;
	}

	UDataTable* LoadedTable = DataSubsystem->GetCommonAbilityBaseStatTable();
	if (!IsValid(LoadedTable))
	{
		return false;
	}

	AbilityBaseStatTable = LoadedTable;
	return true;
}

void UNSCombatStatComponent::HandleCommonDataReady()
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &ThisClass::HandleCommonDataReady);
	}
	
	TryResolveAbilityBaseStatTableFromDataSubsystem();
	RebuildBaseStatCache();
}

bool UNSCombatStatComponent::TryResolveAugmentDefinitionTableFromDataSubsystem()
{
	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		return AugmentDefinitionTable != nullptr;
	}
	
	if (UDataTable* LoadedTable = DataSubsystem->GetCurrentAugmentDefinitionTable())
	{
		AugmentDefinitionTable = LoadedTable;
		return true;
	}
	
	return DataSubsystem->IsRunReady() && AugmentDefinitionTable != nullptr;
}

void UNSCombatStatComponent::HandleRunGameDataReady()
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnRunGameDataReady.RemoveDynamic(this, &ThisClass::HandleRunGameDataReady);
	}
	
	TryResolveAugmentDefinitionTableFromDataSubsystem();
	RebuildAugmentSourceCache();
	RebuildActiveModifierCache();
}

void UNSCombatStatComponent::RebuildBaseStatCache()
{
	CachedBaseStatsByAbility.Reset();
	
	if (!AbilityBaseStatTable)
	{
		NS_OBJ_LOG(LogNSGAS, Warning, "CommonDataConfig의 스킬 기본 스탯 DataTable이 로드되지 않았습니다.");
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
	
	// 증강과 Temporary Buff의 Add / Multiply%를 먼저 합산한 뒤 최종 배율을 한 번만 적용.
	FNSCombatStatModifierSum CombinedModifierSum;
	
	if (const TMap<FGameplayTag, FNSCombatStatModifierSum>* StatMap =
		ActiveModifiersByAbility.Find(AbilityTag))
	{
		if (const FNSCombatStatModifierSum* ActiveModifierSum = StatMap->Find(StatTag))
		{
			CombinedModifierSum.AddValue += ActiveModifierSum->AddValue;
			CombinedModifierSum.MultiplyPercent += ActiveModifierSum->MultiplyPercent;
		}
	}
	
	if (const FNSCombatStatModifierSum* TemporaryModifierSum =
		FindTemporaryModifierSum(AbilityTag, StatTag))
	{
		CombinedModifierSum.AddValue += TemporaryModifierSum->AddValue;
		CombinedModifierSum.MultiplyPercent += TemporaryModifierSum->MultiplyPercent;
	}
	
	const float Multiplier = 1.0f + (CombinedModifierSum.MultiplyPercent * 0.01f);
	OutValue = (BaseValue + CombinedModifierSum.AddValue) * Multiplier;
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

	if (Operation == ENSCombatStatModifierOperation::Multiply && Value <= -100.0f)
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
	
	const FString ContextString = TEXT("AugmentModifierCache");
	
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
		
		if (Row->Operation == ENSCombatStatModifierOperation::Multiply)
		{
			const float MaxStackMultiplier =
				NSAugment::CalculateStackedMultiplyPercent(Row->ValuePerStack, Row->MaxStack);

			if (MaxStackMultiplier <= 0.0f)
			{
				NS_OBJ_LOG(LogNSGAS, Warning,
					"증강 Multiply Modifier의 최대 스택 배율이 0 이하입니다. RowName={RowName}, Value={Value}, MaxStack={MaxStack}, FinalMultiplier={FinalMultiplier}",
					("RowName", RowName.ToString()),
					("Value", Row->ValuePerStack),
					("MaxStack", Row->MaxStack),
					("FinalMultiplier", MaxStackMultiplier)
				);
				continue;
			}
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
		ModifierSum.MultiplyPercent += ModifierRow.ValuePerStack * static_cast<float>(Stacks);
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
			ModifierSum.MultiplyPercent += TemporaryModifier.Value;
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
