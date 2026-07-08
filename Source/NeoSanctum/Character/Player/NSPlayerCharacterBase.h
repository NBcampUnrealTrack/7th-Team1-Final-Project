// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffectTypes.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "NSPlayerCharacterBase.generated.h"

class ANSCompanionDroneAI;
class UNSCompanionDefinition;
class ANSBaseDroneAI;
class ANSWeaponBase;
class UNSCharacterData;
struct FOnAttributeChangeData;
class UGameplayAbility;
class UGameplayEffect;
class UNSPlayerAttributeSet;
class UNSAbilitySystemComponent;
class USpringArmComponent;
class UCameraComponent;
class UCharacterTrajectoryComponent;
class UNSInputBinderComponent;
class UNSPartVisualComponent;
class UNSInteractionComponent;
class UNSMeleeAttackReservationComponent;
class UNSGateAccessComponent;
class UNSPlayerAttackFeedbackComponent;
class UNSPlayerHitTakenFeedbackComponent;
class UNSHitReactionComponent;
class UNSDamageFlashComponent;
class UNSMinimapIconComponent;
struct FMinimalViewInfo;

// 관전자 ViewTarget 계산에 필요한 최소 카메라 상태
USTRUCT()
struct FNSReplicatedSpectatorCameraState
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHasValidData = false;

	UPROPERTY()
	FRotator ViewRotation = FRotator::ZeroRotator;

	UPROPERTY()
	float FOV = 90.f;
};

UCLASS()
class NEOSANCTUM_API ANSPlayerCharacterBase : public ACharacter, public IAbilitySystemInterface, 
											  public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ANSPlayerCharacterBase();
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* EventController) override;
	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// 원격 관전자 시점에서 복제된 카메라 회전 사용
	virtual FRotator GetViewRotation() const override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
	
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	
public:
	// 캐릭터 데이터를 변경하는 경우에 호출할 API로 기능할 함수 (UI에서 사용한다면 이 함수를 호출)
	UFUNCTION(BlueprintCallable, Category = "Character|Data")
	void ChangeCharacterData(UNSCharacterData* InCharacterData);
	
	// 캐릭터 데이터를 변경하도록 요청하는 Server RPC
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Character|Data")
	void Server_ChangeCharacterData(UNSCharacterData* InCharacterData);

	UCharacterTrajectoryComponent* GetCharacterTrajectoryComponent() const { return CharacterTrajectoryComp; }
	UNSInputBinderComponent* GetInputBinderComponent() const { return InputBinderComp; }
	ANSWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }
	// PlayerAttackFeedbackComponent Getter
	UNSPlayerAttackFeedbackComponent* GetPlayerAttackFeedbackComponent() const { return PlayerAttackFeedbackComp; }
	UNSPlayerHitTakenFeedbackComponent* GetPlayerHitTakenFeedbackComponent() const { return PlayerHitTakenFeedbackComp; }
	UNSMinimapIconComponent* GetMinimapIconComponent() const { return MinimapIconComponent; }

	// 캐릭터 데이터에 등록된 반응형 GE를 상황 태그 기준으로 적용
	void ApplyReactiveGameplayEffect(const FGameplayTag& TriggerTag);
	
	bool TryGetAimTraceStartLocation(FVector& OutLocation) const;
	
public:
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(static_cast<uint8>(ETeamId::Player)); }
	
	// 저장된 장착 파츠를 현재 폰에 적용
	void ApplyEquippedPart();

	// 공용 업그레이드 레벨을 Attribute에 재적용. 캐릭터 초기화 시뿐만 아니라 구매 직후에도 호출.
	void ApplyCommonUpgradeAttributeEffect();
	
protected:
	void InitializeAbilitySystem();
	void BindAttributeDelegates();

	// Possess후 PartEquipComponent에 VisualComp 연결
	void BindPartVisual();
	
	void InitializeFromCharacterData(const UNSCharacterData* InCharacterData);
	void ApplyCurrentCharacterData();
	
	void ApplyCharacterVisual();
	void ApplyInitialAttributeEffect();
	void ApplyDefaultGameplayEffects();
	
	void GiveCharacterDataAbilities();
	void SpawnDefaultWeapon();
	ANSWeaponBase* SpawnWeapon(TSubclassOf<ANSWeaponBase> WeaponClass, FName AttachSocketName);
	
	// 캐릭터 데이터를 런타임 중에 제거 : 캐릭터 데이터를 적용하는 상황에서 초기에 호출함
	void ClearCharacterDataRuntimeState();
	
protected:
	UFUNCTION()
	void OnRep_CurrentCharacterData();
	
	UFUNCTION()
	void OnRep_CurrentWeapon();

	UFUNCTION()
	void OnRep_CurrentLeftHandWeapon();
	
	// 사망 연출을 클라이언트에서 한 번 복제해야함.
	UFUNCTION()
	void OnRep_DeathPresentationStarted();
	
protected:
	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
	void ApplyMoveSpeedToCharacter(float MoveSpeed);
	
protected:
	void HandleOutOfHealth();
	void Die();
	void ApplyDeathState();
	void StartDeathRagdoll();
	
protected:
	// 카메라 컨트롤 방향 기준 캐릭터 회전 보간, 현재 Tick()에서 함
	void UpdateCameraFacingRotation(float DeltaSeconds);
	// 관전자에게 전달할 카메라 상태 주기적 갱신
	void UpdateSpectatorCameraState(float DeltaSeconds);
	// 회전이나 FOV가 충분히 변했을 때만 카메라 상태 전송
	bool ShouldSendSpectatorCameraState(const FNSReplicatedSpectatorCameraState& NewCameraState) const;

	UFUNCTION(Server, Unreliable)
	void Server_UpdateSpectatorCameraState(const FNSReplicatedSpectatorCameraState& NewCameraState);

protected:
	// Spring Arm 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	TObjectPtr<USpringArmComponent> SpringArmComp;
	
	// 카메라 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	TObjectPtr<UCameraComponent> CameraComp;

	// Input 바인딩 하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UNSInputBinderComponent> InputBinderComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parts")
	TObjectPtr<UNSPartVisualComponent> PartVisualComp;

	// 상호작용 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UNSInteractionComponent> InteractionComp;

	// 거점 입장 게이트 접근(폰별 통과 + 로컬 외형) 처리 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UNSGateAccessComponent> GateAccessComp;

	// 플레이어가 공격으로 만든 히트 결과 피드백 처리
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<UNSPlayerAttackFeedbackComponent> PlayerAttackFeedbackComp;

	// 플레이어가 피해를 받았을 때의 로컬 피격 피드백 처리
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<UNSPlayerHitTakenFeedbackComponent> PlayerHitTakenFeedbackComp;

	// 피격 위치에 월드 리액션 GameplayCue를 재생
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<UNSHitReactionComponent> HitReactionComponent;

	// Shield 피격 시 캐릭터 머티리얼 플래시를 재생
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<UNSDamageFlashComponent> DamageFlashComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	TObjectPtr<UNSMinimapIconComponent> MinimapIconComponent;
	
protected:
	// Motion Matching에서 사용하는 애니메이션 이동 예측 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UCharacterTrajectoryComponent> CharacterTrajectoryComp;
	
protected:
	// ASC 캐시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UNSAbilitySystemComponent> NSAbilitySystemComponent;
	
	// Player 전용 Attribute Set
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UNSPlayerAttributeSet> PlayerAttributeSet;
	
protected:
	// 몬스터 근접 공격 예약 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UNSMeleeAttackReservationComponent> MeleeAttackReservationComp;
	
protected:
	// 현재 캐릭터 데이터
	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentCharacterData, BlueprintReadOnly, Category = "Character|Data")
	TObjectPtr<const UNSCharacterData> CurrentCharacterData;
	
	// 현재 캐릭터 무기
	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentWeapon, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<ANSWeaponBase> CurrentWeapon;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentLeftHandWeapon, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<ANSWeaponBase> CurrentLeftHandWeapon;
	
	// AbilitySpecHandle
	UPROPERTY(Transient)
	TArray<FGameplayAbilitySpecHandle> CharacterDataAbilityHandles;

	// EffectHandle
	UPROPERTY(Transient)
	TArray<FActiveGameplayEffectHandle> CharacterDataEffectHandles;

	// 공용 업그레이드 이펙트 핸들 (재구매 시 이전 것을 제거하고 재적용하기 위해 별도 추적)
	UPROPERTY(Transient)
	FActiveGameplayEffectHandle CommonUpgradeEffectHandle;

protected:
	// 카메라 방향 캐릭터 회전 설정들
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rotation")
	bool bUseCameraFacingRotation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float CameraFacingTurnStartAngle = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float CameraFacingTurnStopAngle = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "0.0"))
	float CameraFacingRotationSpeed = 160.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "0.0"))
	float CameraFacingMoveSpeedThreshold = 15.f;

	// 카메라 방향 회전 진행 상태관리
	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	bool bIsCameraFacingRotationActive = false;

	// 관전자가 ViewTarget으로 볼 때 사용할 카메라 상태
	UPROPERTY(Replicated)
	FNSReplicatedSpectatorCameraState SpectatorCameraState;

	UPROPERTY(Transient)
	FNSReplicatedSpectatorCameraState LastSentSpectatorCameraState;

	// 카메라 상태 전송 주기 계산용 누적 시간
	UPROPERTY(Transient)
	float SpectatorCameraStateSendElapsed = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spectator|Camera", meta = (ClampMin = "0.01"))
	float SpectatorCameraStateSendInterval = 0.066f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spectator|Camera", meta = (ClampMin = "0.0"))
	float SpectatorCameraRotationThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spectator|Camera", meta = (ClampMin = "0.0"))
	float SpectatorCameraFOVThreshold = 0.1f;
	
	//드론 스폰관련 Test
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Companion|AI")
	TSubclassOf<ANSBaseDroneAI> DroneAIClass;
	
private:
	// 사망 상태에 따라 연출을 실행할 때 클라이언트에 복제하는 시점을 조정하기 위한 bool 변수
	UPROPERTY(ReplicatedUsing = OnRep_DeathPresentationStarted)
	bool bDeathPresentationStarted = false;
	
	// @민재
protected:
	void TryInitializeCompanion();
	
	void SpawnCompanion(const UNSCompanionDefinition* Definition);
	
	void HandleCompanionDataReady();
	
protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<ANSCompanionDroneAI> CompanionAI;
	
	bool bBoundToCompanionDataReady = false;
	
};
