// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NSEnemyVisualParameterTypes.generated.h"

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.15
 *
 * 클래스 개요: 스테이지별 몬스터 머티리얼 파라미터 오버라이드에 사용하는 DataTable Row 타입입니다.
 * EnemyId와 MaterialSlotName을 기준으로 Color, Dust, Emiss_Color 벡터 파라미터를 적용합니다.
 */
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSEnemyVisualParameterRow : public FTableRowBase
{
	GENERATED_BODY()

	// 이 외형 파라미터 Row를 사용할 몬스터 ID를 지정하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Visual", meta = (Categories = "Character.Enemy"))
	FGameplayTag EnemyId;

	// 이 외형 파라미터를 적용할 머티리얼 슬롯 이름을 지정하는 변수. None이면 해당 EnemyData의 모든 MaterialDefinitions 슬롯에 적용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Visual")
	FName MaterialSlotName = NAME_None;

	// 이 Row를 런타임 외형 적용 대상으로 사용할지 결정하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Visual")
	bool bEnabled = true;

	// 머티리얼의 Color 벡터 파라미터에 적용할 값을 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Visual|Parameter")
	FLinearColor Color = FLinearColor::White;

	// 머티리얼의 Dust 벡터 파라미터에 적용할 값을 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Visual|Parameter")
	FLinearColor Dust = FLinearColor::Black;

	// 머티리얼의 Emiss_Color 벡터 파라미터에 적용할 값을 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Visual|Parameter")
	FLinearColor EmissColor = FLinearColor::White;

#if WITH_EDITOR
	// EnemyVisualParameter Row의 필수 입력값을 검증하는 함수
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
