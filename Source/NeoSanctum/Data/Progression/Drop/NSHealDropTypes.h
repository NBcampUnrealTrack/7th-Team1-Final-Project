// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Data/Progression/Drop/NSDropLaunchData.h"
#include "NSHealDropTypes.generated.h"

class ANSPlayerState;
class UStaticMesh;

/**
 * 서버 드랍 레지스트리 (서버 전용, 복제안함)
 */
USTRUCT()
struct FNSHealDropEntry
{
	GENERATED_BODY()

	// 어떤 포션인지 식별하는 태그. 실제 회복%는 이 태그로 DT_HealPotion을 조회해 얻는다.
	UPROPERTY()
	FGameplayTag PotionTag;

	// 드랍 위치 (오버랩 거리 검증용)
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	// 회복 픽업 비주얼의 포물선 발사 정보
	UPROPERTY()
	FNSDropLaunchData LaunchData;

	// 만료 시간
	UPROPERTY()
	float ExpireTime = 0.f;

	// 이미 획득한 플레이어
	UPROPERTY()
	TSet<TWeakObjectPtr<ANSPlayerState>> CollectedPlayer;
};

/**
 * 네트워크로 클라까지 전송되는 회복 아이템 생성 정보
 * CollectedPlayer처럼 누가 획득했는지에 대한 정보가 없음
 */
USTRUCT()
struct FNSHealSpawnEvent
{
	GENERATED_BODY()

	// 드롭 식별 번호
	UPROPERTY()
	int32 DropId = INDEX_NONE;

	// 어떤 포션인지 식별하는 태그. 클라 픽업이 이 태그로 DT_HealPotion을 조회해
	// 메시/스케일(비주얼 티어)을 결정한다. 회복% 자체는 클라가 알 필요 없어 싣지 않는다.
	UPROPERTY()
	FGameplayTag PotionTag;

	// 드랍 위치, FVector_NetQuantize10 : 전송용 벡터 압축
	UPROPERTY()
	FVector_NetQuantize10 Location = FVector::ZeroVector;

	// 클라이언트에서 동일한 포물선 궤적을 재생하기 위한 서버 결정 발사 정보
	UPROPERTY()
	FNSDropLaunchData LaunchData;

	// 만료까지 남은 시간
	UPROPERTY()
	float Duration = 0.f;
};

/**
 * 회복 포션 정의 행
 * RowName에서 포션 태그의 전체 이름을 그대로 사용 -> ex) Reward.Potion.Heal.Large
 */
USTRUCT(BlueprintType)
struct FNSHealPotionRow : public FTableRowBase
{
	GENERATED_BODY()

	// MaxHealth 대비 회복 퍼센트 -> 서버가 수집 시점에 MaxHealth와 곱해 실제 회복량 산출
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 HealPercent = 0;

	// Soft 참조로 두고 픽업이 비동기 로드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UStaticMesh> Mesh;

	// 메시 크기 배율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Scale = 1.f;
};