#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NSPetUpgradeMessageTypes.generated.h"

class UTexture2D;
struct FNSCompanionStatEntry;

/**
 * 펫 강화 화면이 현재 펫의 전체 강화 상태를 요청할 때 사용하는 메시지입니다.
 * UI는 CompanionTag만 전달하며 실제 데이터 조회는 Bridge가 처리합니다.
 */

USTRUCT(BlueprintType)
struct FNSCompanionStatEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText  Name;   // 예: AttackDamage
	UPROPERTY(BlueprintReadOnly) float  Value = 0.f;
};

USTRUCT(BlueprintType)
struct FNSPetUpgradeQueryMessage
{
	GENERATED_BODY()
	
	//요청과 응답을 연결하기위한 고유 식별자
	UPROPERTY(BlueprintReadOnly)
	FGuid RequestId;
	
	//조회할 펫 태그, 비어있으면 현재 선택된 펫 사용
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag CompanionTag;
};

/**
 * 플레이어가 특정 펫 강화 노드의 강화를 요청할 때 사용하는 메시지입니다.
 * UI는 펫과 노드를 GameplayTag로만 식별하고 실제 강화 처리는 알지 못합니다.
 */

USTRUCT(BlueprintType)
struct FNSPetUpgradeRequestMessage
{
	GENERATED_BODY()
	
	//강화결과를 요청자와 연결하기위한 고유 식별자
	UPROPERTY(BlueprintReadOnly)
	FGuid RequestId;
	
	//강화 노드를 소유한 펫 Definition 태그
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag CompanionTag;
	
	//강화를 요청한 노드 태그
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag NodeTag;
};

/**
 * 펫 강화 트리의 노드 하나를 화면에 표시하기 위한 ViewData입니다.
 * Bridge가 CompanionDefinition과 ProgressionSubsystem의 값을 조합하여 생성합니다.
 */

USTRUCT(BlueprintType)
struct FNSPetUpgradeNodeViewData
{
	GENERATED_BODY()
	
	//이 노드를 소유한 펫 Definition 태그
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag CompanionTag;
	
	//강화 노드를 식별하는 태그
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag NodeTag;

	//잠금, 강화 가능, 최대 레벨 등의 UI 상태 태그
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag StateTag;

	//UI에 표시할 노드 이름
	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	//UI에 표시할 강화 효과 설명
	UPROPERTY(BlueprintReadOnly)
	FText Description;

	//UI에 표시할 노드 아이콘
	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	//스탯 노드가 아니라 드론 선택 노드인지 여부 (true면 클릭 시 선택 변경)
	UPROPERTY(BlueprintReadOnly)
	bool bIsDroneSelectNode = false;
	
	//저장 데이터에서 조회한 현재 강화 레벨
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentLevel = 0;

	//CompanionDefinition에서 조회한 최대 강화 레벨
	UPROPERTY(BlueprintReadOnly)
	int32 MaxLevel = 0;

	//다음 강화에 필요한 비용, 무료 강화라면 0
	UPROPERTY(BlueprintReadOnly)
	int64 UpgradeCost = 0;
	
	// 스탯 노드: 레벨당 증가량 (MagnitudePerLevel)
	UPROPERTY(BlueprintReadOnly)
	float IncreasePerLevel = 0.f;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FNSCompanionStatEntry> DroneStats;
};

/**
 * 펫 강화 화면 전체를 갱신하기 위한 상태 Snapshot 메시지입니다.
 * 현재 재화와 펫에 속한 모든 강화 노드의 표시 데이터를 전달합니다.
 */

USTRUCT(BlueprintType)
struct FNSPetUpgradeSnapshotMessage
{
	GENERATED_BODY()

	//Query 메시지와 연결되는 요청 식별자
	UPROPERTY(BlueprintReadOnly)
	FGuid RequestId;

	//현재 선택된 펫 태그
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag CompanionTag;

	//현재 보유한 공용 재화
	UPROPERTY(BlueprintReadOnly)
	int64 CurrentCurrency = 0;
	
	//화면에 표시할 전체 강화 노드 목록
	UPROPERTY(BlueprintReadOnly)
	TArray<FNSPetUpgradeNodeViewData> Nodes;
	
	//현재 선택된 드론의 표시 이름
	UPROPERTY(BlueprintReadOnly)
	FText SelectedDisplayName;

	//현재 선택된 드론의 아이콘
	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> SelectedIcon;
};

/**
 * 특정 강화 요청의 성공 또는 실패 결과를 UI에 전달하는 메시지입니다.
 * 실패한 경우 FailureTag를 이용해 UI가 적절한 안내 문구를 표시합니다.
 */

USTRUCT(BlueprintType)
struct FNSPetUpgradeResultMessage
{
	GENERATED_BODY()

	//Upgrade Request와 연결되는 요청 식별자
	UPROPERTY(BlueprintReadOnly)
	FGuid RequestId;

	//강화 대상 펫 Definition 태그
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag CompanionTag;

	//강화를 시도한 노드 태그
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag NodeTag;

	//실패 원인을 나타내는 태그, 성공한 경우 비어 있음
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag FailureTag;

	//강화 성공 여부
	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;
};

/**
 * 플레이어가 드론(종류) 선택 변경을 요청할 때 사용하는 메시지입니다.
 */
USTRUCT(BlueprintType)
struct FNSPetUpgradeSelectRequestMessage
{
	GENERATED_BODY()

	//결과를 요청자와 연결하기 위한 고유 식별자
	UPROPERTY(BlueprintReadOnly)
	FGuid RequestId;

	//선택하려는 드론 태그
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag CompanionTag;
};


