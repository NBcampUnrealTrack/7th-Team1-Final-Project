// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NSBaseDroneDefinition.h"
#include "Engine/DataAsset.h"
#include "NSCompanionDefinition.generated.h"

class UGameplayEffect;

USTRUCT(BlueprintType)
struct FNSCompanionUpgradeNode
{
	GENERATED_BODY()
	
	// 태그로 노드 확인
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag NodeTag;
	
	// 노드 최대 레벨
	UPROPERTY(EditDefaultsOnly)
	int32 MaxLevel = 5;
	
	// 적용할 effect
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> UpgradeEffect;
	
	// SetByCaller로 넘길 태그
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag SetByCallerTag;
	
	// 레벨당 증가량
	UPROPERTY(EditDefaultsOnly)
	float MagnitudePerLevel = 0.f;
	
	// 전 드론 공유 노드(재화 탐지 범위)면 true. 기본 false = 드론별 독립
	UPROPERTY(EditDefaultsOnly)
	bool bSharedAcrossDrones = false;
	
	// UI에 표시할 노드 아이콘
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UTexture2D> Icon;
	
	// 0→1레벨 = BaseCost, 이후 레벨당 CostPerLevel 가산
	UPROPERTY(EditDefaultsOnly)
	int64 BaseCost = 0;

	UPROPERTY(EditDefaultsOnly)
	int64 CostPerLevel = 0;
	
	UPROPERTY(EditDefaultsOnly)
	FText DisplayName;   // "드론 공격력 증가"
	
	UPROPERTY(EditDefaultsOnly) 
	FText Description;    // "공격 드론의 공격력이 증가합니다"
};

UCLASS(BlueprintType)
class NEOSANCTUM_API UNSCompanionDefinition : public UNSBaseDroneDefinition
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag CompanionTag;

	// 선택 UI 표시 이름
	UPROPERTY(EditDefaultsOnly)
	FText DisplayName;

	// 선택 UI 아이콘
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	// 해금 전제조건: 선행 컴패니언 태그
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag RequiredCompanionTag;

	// 해금 전제조건: 필요 업그레이드 수
	UPROPERTY(EditDefaultsOnly)
	int32 RequiredUpgradeCount = 5;

	// 이 컴패니언이 가진 업그레이드 노드들
	UPROPERTY(EditDefaultsOnly)
	TArray<FNSCompanionUpgradeNode> UpgradeNodes;
	
	// 최초 해금 시 소모 재화
	UPROPERTY(EditDefaultsOnly)
	int64 UnlockCost = 0;
	
	UPROPERTY(EditDefaultsOnly) 
	FText Description;
};
