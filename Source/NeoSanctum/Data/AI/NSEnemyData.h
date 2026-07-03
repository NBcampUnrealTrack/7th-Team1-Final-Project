// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NSEnemyData.generated.h"

class AActor;
class USkeletalMesh;
class UAnimInstance;
class UDataTable;
class UGameplayEffect;
class UGameplayAbility;
class UBehaviorTree;
class UStateTree;
class UEnvQuery;
class UMaterialInterface;


UENUM(BlueprintType)
enum class ENSEnemyMovementType : uint8
{
	Ground UMETA(DisplayName = "Ground"),
	Flying UMETA(DisplayName = "Flying"),
	Stationary UMETA(DisplayName = "Stationary")
};

UENUM(BlueprintType)
enum class ENSEnemyAttackType : uint8
{
	MeleeSweep UMETA(DisplayName = "Melee Sweep"),
	Projectile UMETA(DisplayName = "Projectile"),
	Hitscan UMETA(DisplayName = "Hitscan"),
	Area UMETA(DisplayName = "Area")
};

UENUM(BlueprintType)
enum class ENSEnemyRank : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Elite UMETA(DisplayName = "Elite"),
	Boss UMETA(DisplayName = "Boss")
};

UENUM(BlueprintType)
enum class ENSEnemyAimMode : uint8
{
	None UMETA(DisplayName = "None"),
	Target UMETA(DisplayName = "Target"),
	Predict UMETA(DisplayName = "Predict"),
	Ground UMETA(DisplayName = "Ground"),
	Forward UMETA(DisplayName = "Forward")
};

UENUM(BlueprintType)
enum class ENSBossLaserMode : uint8
{
	Straight UMETA(DisplayName = "Straight"),
	Radial UMETA(DisplayName = "Radial")
};

UENUM(BlueprintType)
enum class ENSEnemyBrainType : uint8
{
	BehaviorTree UMETA(DisplayName = "Behavior Tree"),
	StateTree UMETA(DisplayName = "State Tree")
};

UENUM(BlueprintType)
enum class ENSBossTargetPolicy : uint8
{
	PrimaryOnly UMETA(DisplayName = "Primary Only"),
	NearbyKnown UMETA(DisplayName = "Nearby Known"),
	AllKnown UMETA(DisplayName = "All Known"),
	RandomKnown UMETA(DisplayName = "Random Known")
};

UENUM(BlueprintType)
enum class ENSEnemyPartType : uint8
{
	SpawnedWeapon UMETA(DisplayName = "Spawned Weapon"),
	SpawnedPart UMETA(DisplayName = "Spawned Part"),
	IntegratedWeapon UMETA(DisplayName = "Integrated Weapon"),
	VisualOnly UMETA(DisplayName = "Visual Only")
};

UENUM(BlueprintType)
enum class ENSEnemyPartAttachRule : uint8
{
	KeepRelativeTransform UMETA(DisplayName = "KeepRelativeTransform"),
	KeepWorldTransform UMETA(DisplayName = "KeepWorldTransform"),
	SnapToTargetNotIncludingScale UMETA(DisplayName = "SnapToTargetNotIncludingScale"),
	SnapToTargetIncludingScale UMETA(DisplayName = "SnapToTargetIncludingScale")
};

USTRUCT(BlueprintType)
struct FNSMonsterAttributeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Defense = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BaseDamage = 50.0f;

	// 피격 경직 이벤트가 발생하는 게이지 최대치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1.0"))
	float MaxHitGauge = 100.0f;

	// 유효한 피격 한 번에 증가하는 게이지 수치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float HitGaugeGainPerHit = 15.0f;
};

USTRUCT(BlueprintType)
struct FNSEnemyAttackCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float MinRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float MaxRange = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	bool bRequireLineOfSight = true;
};

USTRUCT(BlueprintType)
struct FNSEnemyAttackRow : public FTableRowBase
{
	GENERATED_BODY()

	// 이 공격 Row를 사용할 몬스터 ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta=(Categories="Character.Enemy"))
	FGameplayTag EnemyId;

	// AttackList의 AttackId와 연결되는 공격 ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FName AttackId = NAME_None;

	// 공격 실행 GA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TSubclassOf<UGameplayAbility> AbilityClass;

	// 공격 분류
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	ENSEnemyAttackType AttackType = ENSEnemyAttackType::MeleeSweep;

	// 공격 거리, 시야 조건 등 사용 가능 조건
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FNSEnemyAttackCondition Condition;

	// MeleeSweep 공격에서 사용하는 Sweep 반경
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float MeleeTraceRadius = 8.0f;

	// 공격 사용 후 다시 사용할 수 있기까지의 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float Cooldown = 0.0f;

	// 같은 우선순위 후보 중 이 공격이 선택될 상대 가중치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	// 공격 선택 시 먼저 비교할 우선순위. 높은 값이 먼저 후보가 됨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	int32 Priority = 0;

	// 공격 판정 전에 플레이어에게 보여줄 경고 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack", meta = (ClampMin = "0.0"))
	float WarnTime = 0.0f;

	// 공격 종료 후 다음 패턴으로 넘어가기 전 대기 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack", meta = (ClampMin = "0.0"))
	float RecoverTime = 0.0f;

	// 기본 데미지에 곱할 공격별 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack", meta = (ClampMin = "0.0"))
	float DamageScale = 1.0f;

	// 이 공격에서 사용할 조준 방식
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack")
	ENSEnemyAimMode AimMode = ENSEnemyAimMode::Target;

	// 레이저 공격에서 사용할 발사 모드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack")
	ENSBossLaserMode LaserMode = ENSBossLaserMode::Straight;

	// 이 공격을 사용할 수 있는 Boss Mode 목록. 비어 있으면 모든 Mode에서 사용할 수 있음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack", meta=(Categories="State.Enemy"))
	FGameplayTagContainer AllowedModeTags;

	// 공격 중 이동을 허용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack")
	bool bMove = false;

	// 공격 중 몸체 회전을 허용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack")
	bool bTurnBody = false;

	// 공격 중 상체 회전을 허용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack")
	bool bTurnUpper = false;

	// 공격 중 식별된 무기 회전을 허용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack")
	bool bTurnWeapon = false;

	// 공격 중 허용되는 좌우 조준 제한 각도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack", meta = (ClampMin = "0.0"))
	float YawLimit = 0.0f;

	// 공격 중 허용되는 상하 조준 제한 각도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack", meta = (ClampMin = "0.0"))
	float PitchLimit = 0.0f;

	// 타깃 지정 정책
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Target")
	ENSBossTargetPolicy TargetPolicy = ENSBossTargetPolicy::PrimaryOnly;

	// 최대 타깃 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Target", meta = (ClampMin = "1"))
	int32 MaxTargets = 1;

	// 타깃 탐색 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Target", meta = (ClampMin = "0.0"))
	float TargetSearchRadius = 6000.0f;

	// 현재 타깃 포함 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Target")
	bool bIncludePrimaryTarget = true;
};

USTRUCT(BlueprintType)
struct FNSEnemyAttackValue
{
	GENERATED_BODY()

	// 값을 덮어쓸 공격 ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	FName AttackId = NAME_None;

	// 해당 공격에 적용할 Override 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase", meta = (ClampMin = "0.0"))
	float Value = 0.0f;
};

USTRUCT(BlueprintType)
struct FNSEnemyPhaseRow : public FTableRowBase
{
	GENERATED_BODY()
	// 이 Phase Row를 사용할 몬스터 ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase", meta=(Categories="Character.Enemy"))
	FGameplayTag EnemyId;

	// 페이즈를 식별하는 ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	FName PhaseId = NAME_None;

	// 이 페이즈로 진입하는 체력 비율. 1.0은 100%를 의미
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HPThreshold = 1.0f;

	// 이 페이즈에서 사용할 수 있는 공격 ID 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	TArray<FName> AttackIds;

	// 이 페이즈에 진입할 때 실행할 Gameplay Ability
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	TSubclassOf<UGameplayAbility> TransitionGA;

	// 현재 페이즈를 표시하기 위해 부여할 GameplayTag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	FGameplayTag PhaseTag;

	// 페이즈 전환 중 일반 패턴 선택을 막을지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	bool bLockPattern = false;

	// 이 페이즈에서 특정 공격의 Weight를 덮어쓸 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	TArray<FNSEnemyAttackValue> WeightOverrides;

	// 이 페이즈에서 특정 공격의 Cooldown을 덮어쓸 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	TArray<FNSEnemyAttackValue> CooldownOverrides;
};

USTRUCT(BlueprintType)
struct FNSEnemyPartRow : public FTableRowBase
{
	GENERATED_BODY()

	// 이 파츠 Row를 사용할 몬스터 ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (Categories = "Character.Enemy"))
	FGameplayTag EnemyId;

	// 같은 몬스터 안에서 파츠를 식별하는 ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FName PartId = NAME_None;

	// 파츠의 사용 방식
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	ENSEnemyPartType PartType = ENSEnemyPartType::IntegratedWeapon;

	// SpawnedWeapon 또는 SpawnedPart 타입에서 스폰할 Actor 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	TSubclassOf<AActor> ActorClass;

	// 스폰한 Actor를 부착하거나 IntegratedWeapon 기준으로 사용할 메시 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FName AttachSocket = NAME_None;

	// 파츠 부착 시 사용할 Transform 적용 방식
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	ENSEnemyPartAttachRule AttachRule = ENSEnemyPartAttachRule::KeepRelativeTransform;

	// AttachSocket 기준으로 적용할 상대 Transform
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FTransform RelativeTransform = FTransform::Identity;

	// 이 파츠를 사용할 공격 ID 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Attack")
	TArray<FName> AttackIds;

	// 투사체, 총구 이펙트가 시작되는 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Socket")
	FName MuzzleSocket = NAME_None;

	// GameplayCue 이펙트를 붙일 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Socket")
	FName CueSocket = NAME_None;

	// Hitscan, Laser 판정의 시작점으로 사용할 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Socket")
	FName TraceSocket = NAME_None;

	// Melee Sweep 판정의 시작점으로 사용할 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Socket")
	FName TraceStartSocket = NAME_None;

	// Melee Sweep 판정의 끝점으로 사용할 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Socket")
	FName TraceEndSocket = NAME_None;

	// LeftHand IK 위치로 사용할 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|IK")
	FName LeftHandIKSocket = NAME_None;

	// 장착 상태에서도 LeftHand IK를 사용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|IK")
	bool bUseLeftHandIKWhileEquipped = false;

	// AnimBP 또는 Control Rig에서 회전시킬 조준 본 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Aim")
	FName AimBone = NAME_None;

	// Control Rig에서 사용할 조준 컨트롤 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Aim")
	FName AimControl = NAME_None;

	// 파츠가 좌우로 회전할 수 있는 최대 각도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Aim", meta = (ClampMin = "0.0"))
	float YawLimit = 0.0f;

	// 파츠가 위아래로 회전할 수 있는 최대 각도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Aim", meta = (ClampMin = "0.0"))
	float PitchLimit = 0.0f;

	// 조준 목표를 따라갈 때 사용하는 보간 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Aim", meta = (ClampMin = "0.0"))
	float AimSpeed = 10.0f;
};

USTRUCT(BlueprintType)
struct FNSEnemyMaterialDefinition
{
	GENERATED_BODY()

	// 런타임 머티리얼을 적용할 스켈레탈 메시의 머티리얼 슬롯 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	FName MaterialSlotName = NAME_None;

	// 해당 슬롯에 MID를 만들기 전에 적용할 초기 머티리얼
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> InitialMaterial;

	// 해당 몬스터 종류의 기본 외형 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	FLinearColor MonsterTint = FLinearColor::White;
};

/**
 * Enemy 초기화 시 필요한 PrimaryDataAsset 입니다.
 */
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSEnemyData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 몬스터 고유 식별자
	UPROPERTY(EditDefaultsOnly, Category = "Identity", meta=(Categories="Character.Enemy"))
	FGameplayTag EnemyId;

	// 몬스터 설명
	UPROPERTY(EditDefaultsOnly, Category = "Identity", meta = (MultiLine = true))
	FString Description;

	// 이동 분류
	UPROPERTY(EditDefaultsOnly, Category = "Classification")
	ENSEnemyMovementType MovementType = ENSEnemyMovementType::Ground;

	// 몬스터 등급
	UPROPERTY(EditDefaultsOnly, Category = "Classification")
	ENSEnemyRank EnemyRank = ENSEnemyRank::Normal;

	// 스켈레탈 메시
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	// 캐릭터 애니메이션 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TSubclassOf<UAnimInstance> AnimClass;

	// 몬스터 크기 배율
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FVector DrawScale = FVector(1.0f, 1.0f, 1.0f);

	// 몬스터의 슬롯별 초기 머티리얼과 기본 외형 색상 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TArray<FNSEnemyMaterialDefinition> MaterialDefinitions;

	// 파츠, 장착형 무기, 메시 일체형 무기 포인트 정보를 읽을 DataTable
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UDataTable> PartsTable;

	// 초기 스탯
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TObjectPtr<UDataTable> AttributeInitData;

	// 기본 GE
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> DefaultEffects;

	// 기본 GA
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	// 피격 게이지 최대치 도달 시 실행할 몬스터 전용 경직 Ability
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> HitReactionAbilityClass;

	// 체력이 0에 도달했을 때 실행할 몬스터 전용 사망 Ability
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> DeathAbilityClass;

	// 공격별 거리, 쿨타임, 가중치 등 수치를 읽을 DataTable
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UDataTable> AttackTable;

	// 페이즈별 사용 가능한 공격 목록과 페이즈 전환 정보를 읽을 DataTable
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UDataTable> PhaseTable;

	// 몬스터 AI 실행 방식
	UPROPERTY(EditDefaultsOnly, Category = "AI Config")
	ENSEnemyBrainType BrainType = ENSEnemyBrainType::BehaviorTree;

	UPROPERTY(EditDefaultsOnly, Category = "AI Config",
		meta = (EditCondition = "BrainType == ENSEnemyBrainType::BehaviorTree", EditConditionHides))
	TObjectPtr<UBehaviorTree> BehaviorTree;

	UPROPERTY(EditDefaultsOnly, Category = "AI Config",
		meta = (EditCondition = "BrainType == ENSEnemyBrainType::StateTree", EditConditionHides))
	TObjectPtr<UStateTree> StateTree;

	UPROPERTY(EditDefaultsOnly, Category = "AI Config",
		meta = (EditCondition = "EnemyRank != ENSEnemyRank::Boss", EditConditionHides))
	TObjectPtr<UEnvQuery> EQSQuery;

	// OtherEnemies Context에 포함할 같은 타깃 추적 몬스터의 최대 반경
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Config|Melee EQS",
		meta = (ClampMin = "0.0", EditCondition = "EnemyRank != ENSEnemyRank::Boss", EditConditionHides))
	float MeleeEQSNeighborRadius = 1000.0f;

public:
	// EnemyId와 일치하는 공격 Row를 AttackTable에서 미리 캐싱하는 함수
	void CacheAttackRows() const;

	// EnemyId와 일치하는 Phase Row를 PhaseTable에서 미리 캐싱하는 함수
	void CachePhaseRows() const;

	// EnemyId와 일치하는 Part Row를 PartsTable에서 미리 캐싱하는 함수
	void CachePartRows() const;

	// AttackTable, PhaseTable, PartsTable에서 캐싱한 Row를 모두 초기화하는 함수
	void InvalidateCachedRows() const;

	// 현재 EnemyId와 일치하는 공격 Row 목록을 반환하는 함수
	const TArray<const FNSEnemyAttackRow*>& GetAttackRows() const;

	// 현재 EnemyId와 일치하는 Phase Row 목록을 반환하는 함수
	const TArray<const FNSEnemyPhaseRow*>& GetPhaseRows() const;

	// 현재 EnemyId와 일치하는 Part Row 목록을 반환하는 함수
	const TArray<const FNSEnemyPartRow*>& GetPartRows() const;

	// 현재 EnemyData가 사용할 Part Row를 하나 이상 가지고 있는지 확인하는 함수. fallback 용도
	bool HasPartRows() const;

	// PartId와 일치하는 Part Row를 반환하는 함수
	const FNSEnemyPartRow* FindPartRowById(FName PartId) const;

	// AttackId를 사용하는 Part Row 목록을 반환하는 함수
	void GetPartRowsByAttackId(FName AttackId, TArray<const FNSEnemyPartRow*>& OutPartRows) const;

#if WITH_EDITOR
	// 에디터에서 EnemyData가 수정되면 캐시된 Row를 초기화하는 함수
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// 현재 체력 비율에 맞는 Phase Row를 반환하는 함수. Phase Row가 없으면 nullptr를 반환
	const FNSEnemyPhaseRow* FindPhaseRowByHealthRatio(float HealthRatio) const;

	// 현재 체력 비율 기준으로 AttackId가 사용 가능한지 확인하는 함수. Phase Row가 없으면 모든 공격을 허용함
	bool IsAttackAllowedByPhase(FName AttackId, float HealthRatio) const;

private:
	// AttackTable에서 현재 EnemyId와 일치하는 Row만 모아둔 캐시
	mutable TArray<const FNSEnemyAttackRow*> CachedAttackRows;

	// PhaseTable에서 현재 EnemyId와 일치하는 Row만 모아둔 캐시
	mutable TArray<const FNSEnemyPhaseRow*> CachedPhaseRows;

	// PartsTable에서 현재 EnemyId와 일치하는 Row만 모아둔 캐시
	mutable TArray<const FNSEnemyPartRow*> CachedPartRows;

	// AttackTable 캐시가 초기화되었는지 여부
	mutable bool bAttackRowsCached = false;

	// PhaseTable 캐시가 초기화되었는지 여부
	mutable bool bPhaseRowsCached = false;

	// PartsTable 캐시가 초기화되었는지 여부
	mutable bool bPartRowsCached = false;

public:
	// 현재 페이즈 기준으로 공격 Weight Override를 반영한 최종 Weight를 반환하는 함수
	float GetPhaseAttackWeight(const FNSEnemyAttackRow& AttackRow, float HealthRatio) const;

	// 현재 페이즈 기준으로 공격 Cooldown Override를 반영한 최종 Cooldown을 반환하는 함수
	float GetPhaseAttackCooldown(const FNSEnemyAttackRow& AttackRow, float HealthRatio) const;

private:
	// Phase Override 목록에서 AttackId와 일치하는 값을 찾고, 없으면 기본값을 반환하는 함수
	float GetPhaseAttackOverrideValue(
		const TArray<FNSEnemyAttackValue>& Overrides,
		FName AttackId,
		float DefaultValue) const;
};
