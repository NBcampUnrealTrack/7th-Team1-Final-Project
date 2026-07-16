// Copyright 2026 One Team. All rights reserved.


#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NeoSanctum/Combat/Component/Artillery/NSBossArtilleryTypes.h"
#include "NSBossArtilleryPatternData.generated.h"

/*
 * 작성자 : 최준혁
 *
 * 파일 생성일 : 26.07.15
 *
 * 클래스 개요 : 보스 포격 패턴 하나를 에디터에서 설정할 수 있는 DataAsset
 * 패턴 선택 가중치, 대상 선정, 발수 계산, 착탄 위치, 폭발 타이밍, 피해 설정을 보관
*/
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSBossArtilleryPatternData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// AssetManager와 디버그에서 이 포격 패턴을 식별하기 위한 PrimaryAssetId를 반환하는 함수
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	// 에디터에서 포격 패턴 설정의 필수값과 모순을 검증하는 함수
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

public:
	// A~E 중 이 DataAsset이 나타내는 포격 패턴의 고정 식별자
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	ENSBossArtilleryPatternId PatternId = ENSBossArtilleryPatternId::None;

	// 에디터와 디버그 로그에서 읽기 쉽게 표시할 패턴 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	// 패턴의 의도와 튜닝 메모를 남기기 위한 설명
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = true))
	FText Description;

	// 패턴 선택 확률과 반복 방지 정책을 정의하는 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artillery")
	FNSBossArtillerySelectionData SelectionData;

	// 포격 기준 대상 또는 기준 위치를 어떻게 수집할지 정의하는 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artillery")
	FNSBossArtilleryTargetData TargetData;

	// 이 패턴에서 생성할 포탄 수를 어떻게 계산할지 정의하는 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artillery")
	FNSBossArtilleryShotBudgetData ShotBudgetData;

	// 포탄의 착탄 위치를 어떻게 배치할지 정의하는 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artillery")
	FNSBossArtilleryPlacementData PlacementData;

	// 포탄의 경고 시간과 폭발 시간을 어떻게 배치할지 정의하는 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artillery")
	FNSBossArtilleryTimingData TimingData;

	// 포탄 폭발 시 서버 피해 판정에 사용할 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artillery")
	FNSBossArtilleryDamageData DamageData;

	// 개발 중 포격 위치와 선택 결과를 확인하기 위한 디버그 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artillery")
	FNSBossArtilleryDebugData DebugData;
};
