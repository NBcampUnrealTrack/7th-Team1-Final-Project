// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NSMonsterUIData.generated.h"

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 몬스터 종류별 상태 UI 표시 정책을 정의하는 데이터테이블 Row입니다.
 * RowName은 EnemyData의 EnemyId GameplayTag 이름과 동일하게 맞춰 사용합니다.
 */
USTRUCT(BlueprintType)
struct FNSMonsterUIData : public FTableRowBase
{
	GENERATED_BODY()

	// 이 Row가 적용될 몬스터 EnemyId를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI", meta=(Categories="Character.Enemy"))
	FGameplayTag EnemyId;

	// UI에 표시할 몬스터 이름을 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI")
	FText DisplayName;

	// 이름 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI|Display")
	bool bShowName = false;

	// 체력바 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI|Display")
	bool bShowHealth = true;

	// 체력 수치 텍스트 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI|Display")
	bool bShowHealthText = false;

	// 실드바 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI|Display")
	bool bShowShield = false;

	// 실드 수치 텍스트 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI|Display")
	bool bShowShieldText = false;

	// 피격 게이지 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI|Display")
	bool bShowHitGauge = true;

	// 피격 게이지 수치 텍스트 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI|Display")
	bool bShowHitGaugeText = false;

	// 일반 몬스터 UI에서 벽 뒤 Occlusion Trace를 사용할지 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI|Normal")
	bool bUseOcclusionTrace = true;

	// 일반 몬스터 UI 표시 유지 시간을 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI|Normal", meta = (ClampMin = "0.1"))
	float RevealDurationSeconds = 3.0f;

	// 일반 몬스터 UI 최대 표시 거리를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI|Normal", meta = (ClampMin = "0.0"))
	float MaxDisplayDistance = 3500.0f;
};
