// Copyright 2026 One Team. All rights reserved.

/*
 * 작성자 : 최준혁
 *
 * 파일 생성일 : 26.07.15
 *
 * 클래스 개요 : 보스 포격 패턴 시스템에서 공유하는 enum과 struct 타입 모음
 * 대상 선정, 발수 계산, 착탄 위치, 폭발 타이밍, 반복 방지 정책의 데이터 표현을 담당
*/

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NSBossArtilleryTypes.generated.h"

// 보스 포격 패턴의 고정 식별자
UENUM(BlueprintType)
enum class ENSBossArtilleryPatternId : uint8
{
	// 유효하지 않은 포격 패턴 상태
	None UMETA(DisplayName = "None"),

	// 모든 전투 참여자를 견제하는 기본 포격 패턴
	PatternA_AllTargets UMETA(DisplayName = "Pattern A - All Targets"),

	// Threat 1위 대상을 집중 압박하는 포격 패턴
	PatternB_FocusedBarrage UMETA(DisplayName = "Pattern B - Focused Barrage"),

	// 플레이어 사이 공간을 공격해 진형 분산을 유도하는 포격 패턴
	PatternC_Separation UMETA(DisplayName = "Pattern C - Separation"),

	// 보스룸 중심에서 파동처럼 확산되는 포격 패턴
	PatternD_Wave UMETA(DisplayName = "Pattern D - Wave"),

	// 빠른 폭발과 지연 폭발을 섞어 복귀 행동을 견제하는 포격 패턴
	PatternE_OffBeat UMETA(DisplayName = "Pattern E - Off Beat")
};

// 포격 패턴이 기준 대상으로 삼을 대상
UENUM(BlueprintType)
enum class ENSBossArtilleryTargetMode : uint8
{
	// 유효한 대상 선정 방식이 없음
	None UMETA(DisplayName = "None"),

	// 보스 전투에 참여 중인 모든 유효 플레이어
	AllCombatants UMETA(DisplayName = "All Combatants"),

	// EnemyThreatComponent가 선택한 가장 위협적인 대상
	HighestThreat UMETA(DisplayName = "Highest Threat"),

	// 보스룸의 중심 위치를 기준점으로 사용
	ArenaCenter UMETA(DisplayName = "Arena Center"),

	// 보스 자신의 현재 위치를 기준점으로 사용
	BossLocation UMETA(DisplayName = "Boss Location"),

	// 플레이어 쌍 사이의 중간 지점을 기준점으로 사용
	BetweenCombatants UMETA(DisplayName = "Between Combatants")
};

// 포격 위치 생성 단계에서 사용할 기준점의 종류를 정의하는 enum
UENUM(BlueprintType)
enum class ENSBossArtilleryTargetPointType : uint8
{
	// 유효하지 않은 기준점 값을 나타내는 값
	None UMETA(DisplayName = "None"),

	// 단일 Actor 위치를 기준으로 사용하는 값
	Actor UMETA(DisplayName = "Actor"),

	// 두 Actor 사이의 중간 지점을 기준으로 사용하는 값
	PairMidpoint UMETA(DisplayName = "Pair Midpoint"),

	// 보스룸 중심 위치를 기준으로 사용하는 값
	ArenaCenter UMETA(DisplayName = "Arena Center"),

	// 보스 자신의 위치를 기준으로 사용하는 값
	BossLocation UMETA(DisplayName = "Boss Location")
};

// 포격 착탄 위치를 만들기 전에 수집되는 기준점 데이터
USTRUCT(BlueprintType)
struct FNSBossArtilleryTargetPoint
{
	GENERATED_BODY()

	// 이 기준점이 어떤 방식으로 만들어졌는지 나타냄
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Target")
	ENSBossArtilleryTargetPointType PointType = ENSBossArtilleryTargetPointType::None;

	// 단일 대상 또는 플레이어 쌍의 첫 번째 대상
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Target")
	TObjectPtr<AActor> PrimaryTarget = nullptr;

	// 플레이어 쌍의 두 번째 대상
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Target")
	TObjectPtr<AActor> SecondaryTarget = nullptr;

	// 포격 위치 생성의 기준이 되는 월드 위치
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Target")
	FVector Location = FVector::ZeroVector;

	// 보스 또는 기준점에서 대상 방향
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Target")
	FVector Direction = FVector::ForwardVector;

	// 플레이어 쌍 사이 거리 또는 기준점 보조 거리로 사용
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Target")
	float Distance = 0.0f;
};

// 포격 기준점 하나에 몇 발을 배정할지 나타내는 구조체
USTRUCT(BlueprintType)
struct FNSBossArtilleryShotAllocation
{
	GENERATED_BODY()

	// 포탄을 배정받을 포격 기준점
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Shot Budget")
	FNSBossArtilleryTargetPoint TargetPoint;

	// 이 기준점에 배정된 포탄 수
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Shot Budget")
	int32 ShotCount = 0;

	// 전체 포격 실행 안에서 이 기준점의 첫 포탄 인덱스
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Shot Budget")
	int32 FirstShotIndex = 0;
};

// 포격 패턴의 총 포탄 수를 계산하는 방식
UENUM(BlueprintType)
enum class ENSBossArtilleryShotBudgetMode : uint8
{
	// 유효한 발수 계산 방식이 없음
	None UMETA(DisplayName = "None"),

	// 유효 타깃 수에 타깃당 발수를 곱해 총 포탄 수를 계산
	PerTarget UMETA(DisplayName = "Per Target"),

	// 타깃 수와 무관하게 고정된 전체 포탄 수를 사용
	FixedTotal UMETA(DisplayName = "Fixed Total"),

	// 링 개수와 링당 포탄 수를 곱해 아레나 포격용 총 포탄 수를 계산
	PerRing UMETA(DisplayName = "Per Ring"),

	// 플레이어 쌍 또는 분산 기준점 개수에 따라 총 포탄 수를 계산
	BetweenCombatants UMETA(DisplayName = "Between Combatants")
};

// 착탄 위치를 생성하는 방식
UENUM(BlueprintType)
enum class ENSBossArtilleryPlacementMode : uint8
{
	// 유효한 착탄 위치 생성 방식이 없음
	None UMETA(DisplayName = "None"),

	// 타깃의 현재 위치를 착탄 위치로 사용
	TargetCurrent UMETA(DisplayName = "Target Current"),

	// 타깃의 현재 속도를 이용해 예측 위치를 착탄 위치로 사용
	TargetPrediction UMETA(DisplayName = "Target Prediction"),

	// 타깃 주변 반경 안에 랜덤 착탄 위치를 생성
	RandomAroundTarget UMETA(DisplayName = "Random Around Target"),

	// 타깃 주변 좁은 반경 안에 군집 착탄 위치를 생성
	ClusterAroundTarget UMETA(DisplayName = "Cluster Around Target"),

	// 두 플레이어 사이의 중간 지점을 착탄 위치로 사용
	BetweenTargets UMETA(DisplayName = "Between Targets"),

	// 기준점 주변에 링 형태의 착탄 위치를 생성
	Ring UMETA(DisplayName = "Ring"),

	// 여러 링 또는 거리 단계로 파동형 착탄 위치를 생성
	WaveRings UMETA(DisplayName = "Wave Rings"),

	// 타깃의 이동 방향 앞쪽을 막는 착탄 위치를 생성
	EscapeRouteBlock UMETA(DisplayName = "Escape Route Block")
};

// 포탄별 폭발 시간을 생성하는 방식
UENUM(BlueprintType)
enum class ENSBossArtilleryTimingMode : uint8
{
	// 유효한 폭발 시간 생성 방식이 없음
	None UMETA(DisplayName = "None"),

	// 포탄 순서에 따라 일정 간격으로 순차 폭발
	Sequential UMETA(DisplayName = "Sequential"),

	// 설정된 시간 범위 안에서 랜덤하게 폭발 시간을 배치
	RandomScatter UMETA(DisplayName = "Random Scatter"),

	// 거의 같은 시점에 폭발하되 작은 편차만 적용
	Simultaneous UMETA(DisplayName = "Simultaneous"),

	// 몇 발씩 묶어서 버스트 단위로 폭발
	Burst UMETA(DisplayName = "Burst"),

	// 기준점과의 거리에 따라 파동처럼 순차 폭발
	Wave UMETA(DisplayName = "Wave"),

	// 빠른 폭발과 지연 폭발을 섞어 엇박을 만듦
	OffBeat UMETA(DisplayName = "Off Beat")
};

// 최근 사용 패턴에 적용할 반복 방지 정책
UENUM(BlueprintType)
enum class ENSBossArtilleryRepeatPolicy : uint8
{
	// 최근 사용 기록과 무관하게 기본 가중치만 사용
	None UMETA(DisplayName = "None"),

	// 최근 사용된 정도에 따라 선택 가중치만 낮춤
	SoftWeightPenalty UMETA(DisplayName = "Soft Weight Penalty"),

	// 직전에 사용한 패턴은 한 번 선택에서 제외
	BlockImmediateRepeat UMETA(DisplayName = "Block Immediate Repeat"),

	// 지정된 사용 횟수 동안 패턴을 선택 후보에서 제외
	HardCooldownUses UMETA(DisplayName = "Hard Cooldown Uses")
};

// 포격 패턴 선택과 반복 방지에 필요한 설정값
USTRUCT(BlueprintType)
struct FNSBossArtillerySelectionData
{
	GENERATED_BODY()

	// 이 패턴이 후보에 있을 때 적용되는 기본 선택 가중치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection", meta = (ClampMin = "0.0"))
	float BaseWeight = 100.0f;

	// 이 패턴이 최근에 사용됐을 때 반복을 억제하는 정책
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
	ENSBossArtilleryRepeatPolicy RepeatPolicy = ENSBossArtilleryRepeatPolicy::SoftWeightPenalty;

	// 최근 사용 이력의 위치별 가중치 배율. 0번은 직전 사용 패턴에 적용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
	TArray<float> RecentUseWeightMultipliers = {0.2f, 0.5f, 0.8f};

	// HardCooldownUses 정책에서 이 패턴을 몇 번의 선택 동안 막을지 정함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection", meta = (ClampMin = "0"))
	int32 HardCooldownUseCount = 0;

	// 현재 패턴이 전투 난이도 예산에서 차지하는 위험도 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection", meta = (ClampMin = "0"))
	int32 DangerScore = 1;

	// 이 패턴을 사용할 수 있는 보스 페이즈 태그 목록. 비어 있으면 모든 페이즈에서 허용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection", meta = (Categories = "State.Enemy"))
	FGameplayTagContainer AllowedPhaseTags;
};

// 포격 패턴의 대상 수집에 필요한 설정값
USTRUCT(BlueprintType)
struct FNSBossArtilleryTargetData
{
	GENERATED_BODY()

	// 이 패턴이 사용할 기준 대상 수집 방식
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target")
	ENSBossArtilleryTargetMode TargetMode = ENSBossArtilleryTargetMode::AllCombatants;

	// 이 패턴이 실행되기 위해 필요한 최소 전투 참여자 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target", meta = (ClampMin = "0"))
	int32 MinCombatantCount = 1;

	// 이 패턴이 허용하는 최대 전투 참여자 수. 0이면 제한하지 않음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target", meta = (ClampMin = "0"))
	int32 MaxCombatantCount = 0;

	// BetweenCombatants 방식에서 사용할 플레이어 쌍의 최대 개수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target|Pairs", meta = (ClampMin = "0"))
	int32 MaxPairCount = 3;

	// BetweenCombatants 방식에서 너무 가까운 쌍을 제외하기 위한 최소 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target|Pairs", meta = (ClampMin = "0.0"))
	float MinPairDistance = 0.0f;

	// BetweenCombatants 방식에서 너무 멀리 떨어진 쌍을 제외하기 위한 최대 거리. 0이면 제한하지 않음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target|Pairs", meta = (ClampMin = "0.0"))
	float MaxPairDistance = 1500.0f;
};

// 포격 패턴의 포탄 수 계산에 필요한 설정값
USTRUCT(BlueprintType)
struct FNSBossArtilleryShotBudgetData
{
	GENERATED_BODY()

	// 이 패턴의 총 포탄 수를 계산하는 방식
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shot Budget")
	ENSBossArtilleryShotBudgetMode BudgetMode = ENSBossArtilleryShotBudgetMode::PerTarget;

	// PerTarget 방식에서 유효 타깃 한 명에게 배정할 포탄 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shot Budget", meta = (ClampMin = "1"))
	int32 ShotsPerTarget = 3;

	// FixedTotal 방식에서 사용할 고정 전체 포탄 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shot Budget", meta = (ClampMin = "1"))
	int32 FixedTotalShots = 8;

	// PerRing 방식에서 생성할 링 개수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shot Budget|Ring", meta = (ClampMin = "1"))
	int32 RingCount = 3;

	// PerRing 방식에서 링 하나에 배치할 포탄 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shot Budget|Ring", meta = (ClampMin = "1"))
	int32 ShotsPerRing = 8;

	// BetweenCombatants 방식에서 기준점 하나당 생성할 포탄 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shot Budget|Pairs", meta = (ClampMin = "1"))
	int32 ShotsPerPair = 1;

	// 최종 생성 가능한 전체 포탄 수의 상한
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shot Budget", meta = (ClampMin = "1"))
	int32 MaxTotalShots = 12;
};

// 포격 패턴의 착탄 위치 생성에 필요한 설정값
USTRUCT(BlueprintType)
struct FNSBossArtilleryPlacementData
{
	GENERATED_BODY()

	// 이 패턴이 사용할 착탄 위치 생성 방식
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	ENSBossArtilleryPlacementMode PlacementMode = ENSBossArtilleryPlacementMode::RandomAroundTarget;

	// TargetPrediction 방식에서 타깃 속도를 몇 초 앞까지 예측할지 정함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Prediction", meta = (ClampMin = "0.0"))
	float PredictionTime = 0.5f;

	// RandomAroundTarget 방식에서 타깃 주변에 위치를 흩뿌릴 반경
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Scatter", meta = (ClampMin = "0.0"))
	float ScatterRadius = 400.0f;

	// ClusterAroundTarget 방식에서 포탄을 밀집시킬 반경
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Cluster", meta = (ClampMin = "0.0"))
	float ClusterRadius = 250.0f;

	// Ring 또는 WaveRings 방식에서 첫 번째 링의 반경
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Ring", meta = (ClampMin = "0.0"))
	float RingStartRadius = 500.0f;

	// WaveRings 방식에서 링 사이에 추가할 반경 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Ring", meta = (ClampMin = "0.0"))
	float RingSpacing = 450.0f;

	// EscapeRouteBlock 방식에서 타깃 이동 방향 앞쪽으로 얼마나 떨어진 곳을 막을지 정함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Block", meta = (ClampMin = "0.0"))
	float ForwardBlockDistance = 450.0f;

	// EscapeRouteBlock 방식에서 이동 방향 좌우로 배치할 보조 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Block", meta = (ClampMin = "0.0"))
	float SideBlockOffset = 250.0f;

	// 너무 가까운 착탄 위치가 중복 생성되는 것을 막기 위한 최소 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.0"))
	float MinImpactLocationDistance = 150.0f;
};

// 포격 패턴의 폭발 타이밍 생성에 필요한 설정값
USTRUCT(BlueprintType)
struct FNSBossArtilleryTimingData
{
	GENERATED_BODY()

	// 이 패턴이 사용할 폭발 시간 생성 방식
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing")
	ENSBossArtilleryTimingMode TimingMode = ENSBossArtilleryTimingMode::Burst;

	// 경고 표시 후 첫 폭발까지 보장할 기본 대기 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0"))
	float WarningDuration = 1.2f;

	// Sequential 방식에서 포탄 사이에 적용할 시간 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing|Sequential", meta = (ClampMin = "0.0"))
	float SequentialInterval = 0.2f;

	// RandomScatter 방식에서 추가 폭발 지연의 최소값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing|Random", meta = (ClampMin = "0.0"))
	float MinRandomDelay = 0.0f;

	// RandomScatter 방식에서 추가 폭발 지연의 최대값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing|Random", meta = (ClampMin = "0.0"))
	float MaxRandomDelay = 0.8f;

	// Simultaneous 방식에서 동시 폭발처럼 보이게 유지할 작은 시간 편차
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing|Simultaneous", meta = (ClampMin = "0.0"))
	float SimultaneousJitter = 0.08f;

	// Burst 방식에서 한 묶음에 포함할 포탄 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing|Burst", meta = (ClampMin = "1"))
	int32 ShotsPerBurst = 3;

	// Burst 방식에서 묶음 사이에 적용할 시간 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing|Burst", meta = (ClampMin = "0.0"))
	float BurstInterval = 0.5f;

	// Burst 방식에서 같은 묶음 안의 포탄에 적용할 작은 시간 편차
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing|Burst", meta = (ClampMin = "0.0"))
	float IntraBurstJitter = 0.08f;

	// Wave 방식에서 거리 차이를 시간 차이로 변환하는 속도 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing|Wave", meta = (ClampMin = "1.0"))
	float WaveSpeed = 900.0f;

	// OffBeat 방식에서 포탄 순서별로 적용할 추가 지연 시간 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing|OffBeat")
	TArray<float> OffBeatExtraDelays = {0.0f, 0.15f, 0.8f};
};

// 포격 패턴의 피해 판정에 필요한 설정값
USTRUCT(BlueprintType)
struct FNSBossArtilleryDamageData
{
	GENERATED_BODY()

	// 포탄 폭발 시 피해를 적용할 반경
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float DamageRadius = 300.0f;

	// 보스 기본 피해량 또는 공격 Row 피해량에 곱할 배율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float DamageScale = 1.0f;

	// 벽이나 장애물 뒤의 대상을 피해에서 제외할지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	bool bRequireLineOfSight = false;

	// 같은 포격 실행 안에서 같은 대상에게 여러 번 피해를 허용할지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	bool bAllowMultipleHitsPerTarget = true;
};

// 포격 패턴 개발 중 시각화와 로그에 사용할 설정값
USTRUCT(BlueprintType)
struct FNSBossArtilleryDebugData
{
	GENERATED_BODY()

	// 에디터나 개발 빌드에서 착탄 위치와 반경을 디버그 표시할지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug")
	bool bDrawDebug = false;

	// 디버그 도형과 문자열을 화면에 유지할 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug", meta = (ClampMin = "0.0"))
	float DrawTime = 2.0f;
};
