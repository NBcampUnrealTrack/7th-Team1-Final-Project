// Copyright 2026 One Team. All rights reserved.

#include "NSEnemyVisualMaterialApplier.h"

#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Data/AI/NSEnemyVisualParameterTypes.h"

void FNSEnemyVisualMaterialApplier::ApplyEnemyVisualMaterials(
	const UObject* WorldContextObject,
	USkeletalMeshComponent* MeshComponent,
	const UNSEnemyData* EnemyData,
	TArray<TObjectPtr<UMaterialInstanceDynamic>>& OutRuntimeMaterials,
	TArray<UMaterialInstanceDynamic*>& OutFlashTargetMaterials)
{
	OutRuntimeMaterials.Reset();
	OutFlashTargetMaterials.Reset();

	if (!MeshComponent || !EnemyData)
	{
		return;
	}

	TArray<const FNSEnemyVisualParameterRow*> EnemyVisualParameterRows;

	// 현재 스테이지의 EnemyVisualParameterTable에서 이 EnemyId에 해당하는 Row를 수집하는 변수
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(WorldContextObject);
	if (DataSubsystem)
	{
		DataSubsystem->GetCurrentEnemyVisualParameterRows(
			EnemyData->EnemyId,
			EnemyVisualParameterRows);
	}

	for (const FNSEnemyMaterialDefinition& Definition : EnemyData->MaterialDefinitions)
	{
		// 현재 MaterialDefinition이 가리키는 메시 머티리얼 슬롯 인덱스를 저장하는 변수
		const int32 MaterialIndex = MeshComponent->GetMaterialIndex(Definition.MaterialSlotName);

		if (MaterialIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy material slot not found. EnemyId=%s, Slot=%s"),
			       *EnemyData->EnemyId.ToString(),
			       *Definition.MaterialSlotName.ToString());
			continue;
		}

		// MID의 부모로 사용할 기본 머티리얼 또는 MI를 저장하는 변수
		UMaterialInterface* InitialMaterial = Definition.InitialMaterial
			                                      ? Definition.InitialMaterial.Get()
			                                      : MeshComponent->GetMaterial(MaterialIndex);

		if (!InitialMaterial)
		{
			continue;
		}

		MeshComponent->SetMaterial(MaterialIndex, InitialMaterial);

		// 런타임에서 파라미터를 변경하기 위해 생성하는 MID 변수
		UMaterialInstanceDynamic* MID =
			MeshComponent->CreateDynamicMaterialInstance(MaterialIndex, InitialMaterial);

		if (!MID)
		{
			continue;
		}

		InitializeCommonRuntimeParameters(MID);

		// 기존 MonsterTint 기반 머티리얼과의 호환성을 유지하기 위해 기본 틴트를 적용하는 변수
		MID->SetVectorParameterValue(TEXT("MonsterTint"), Definition.MonsterTint);

		// 현재 슬롯에 적용할 스테이지 외형 파라미터 Row를 저장하는 변수
		const FNSEnemyVisualParameterRow* ParameterRow =
			FindParameterRowForSlot(EnemyVisualParameterRows, Definition.MaterialSlotName);

		if (ParameterRow)
		{
			ApplyParameterRowToMID(MID, *ParameterRow);
		}

		OutRuntimeMaterials.Add(MID);
		OutFlashTargetMaterials.Add(MID);
	}
}

const FNSEnemyVisualParameterRow* FNSEnemyVisualMaterialApplier::FindParameterRowForSlot(
	const TArray<const FNSEnemyVisualParameterRow*>& ParameterRows,
	FName MaterialSlotName)
{
	// MaterialSlotName이 None인 전체 슬롯 적용 Row를 저장하는 변수
	const FNSEnemyVisualParameterRow* GlobalRow = nullptr;

	// MaterialSlotName이 정확히 일치하는 슬롯 전용 Row를 저장하는 변수
	const FNSEnemyVisualParameterRow* ExactSlotRow = nullptr;

	for (const FNSEnemyVisualParameterRow* Row : ParameterRows)
	{
		if (!Row || !Row->bEnabled)
		{
			continue;
		}

		if (Row->MaterialSlotName.IsNone())
		{
			if (!GlobalRow)
			{
				GlobalRow = Row;
			}

			continue;
		}

		if (Row->MaterialSlotName == MaterialSlotName)
		{
			ExactSlotRow = Row;
			break;
		}
	}

	return ExactSlotRow ? ExactSlotRow : GlobalRow;
}

void FNSEnemyVisualMaterialApplier::ApplyParameterRowToMID(
	UMaterialInstanceDynamic* MID,
	const FNSEnemyVisualParameterRow& ParameterRow)
{
	if (!MID)
	{
		return;
	}

	MID->SetVectorParameterValue(TEXT("Color"), ParameterRow.Color);
	MID->SetVectorParameterValue(TEXT("Dust"), ParameterRow.Dust);
	MID->SetVectorParameterValue(TEXT("Emiss_Color"), ParameterRow.EmissColor);
}

void FNSEnemyVisualMaterialApplier::InitializeCommonRuntimeParameters(UMaterialInstanceDynamic* MID)
{
	if (!MID)
	{
		return;
	}

	MID->SetScalarParameterValue(TEXT("HitFlashAmount"), 0.0f);
	MID->SetScalarParameterValue(TEXT("DissolveMask"), -1.0f);
}
