// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
class UNSEnemyData;
class USkeletalMeshComponent;
struct FNSEnemyVisualParameterRow;

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.15
 *
 * 클래스 개요: EnemyData의 기본 머티리얼 정의와 현재 LevelConfig의 EnemyVisualParameterTable을 합성해 몬스터 MID를 생성하는 헬퍼입니다.
 * Character 기반 몬스터와 Pawn 기반 몬스터가 같은 외형 파라미터 적용 로직을 공유하도록 분리했습니다.
 */
struct NEOSANCTUM_API FNSEnemyVisualMaterialApplier
{
	// EnemyData와 현재 스테이지 외형 테이블을 바탕으로 메시 슬롯별 MID를 생성하고 파라미터를 적용하는 함수
	static void ApplyEnemyVisualMaterials(
		const UObject* WorldContextObject,
		USkeletalMeshComponent* MeshComponent,
		const UNSEnemyData* EnemyData,
		TArray<TObjectPtr<UMaterialInstanceDynamic>>& OutRuntimeMaterials,
		TArray<UMaterialInstanceDynamic*>& OutFlashTargetMaterials);

private:
	// 현재 스테이지 외형 Row 중 지정한 슬롯에 적용할 Row를 찾는 함수
	static const FNSEnemyVisualParameterRow* FindParameterRowForSlot(
		const TArray<const FNSEnemyVisualParameterRow*>& ParameterRows,
		FName MaterialSlotName);

	// MID에 EnemyVisualParameter Row 값을 적용하는 함수
	static void ApplyParameterRowToMID(
		UMaterialInstanceDynamic* MID,
		const FNSEnemyVisualParameterRow& ParameterRow);

	// MID에 피격/디졸브 등 공통 런타임 파라미터의 초기값을 적용하는 함수
	static void InitializeCommonRuntimeParameters(UMaterialInstanceDynamic* MID);
};
