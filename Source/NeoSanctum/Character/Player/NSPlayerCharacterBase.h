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

class ANSBaseCompanionAI;
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
class UNSSpectatorViewComponent;
class UNSPartVisualComponent;

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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
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
	UNSSpectatorViewComponent* GetSpectatorViewComponent() const { return SpectatorViewComp; }
	ANSWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }
	bool TryGetAimTraceStartLocation(FVector& OutLocation) const;
	
public:
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(static_cast<uint8>(ETeamId::Player)); }
	
protected:
	void InitializeAbilitySystem();
	void BindAttributeDelegates();

	// Possess후 PartEquipComponent에 VisualComp 연결
	void BindPartVisual();
	
	void InitializeFromCharacterData(const UNSCharacterData* InCharacterData);
	void ApplyCurrentCharacterData();
	void LoadCharacterDataAssets(const UNSCharacterData* InCharacterData);
	
	void ApplyCharacterVisual();
	void ApplyInitialAttributeEffect();
	void ApplyDefaultGameplayEffects();
	
	void GiveCharacterDataAbilities();
	void SpawnDefaultWeapon();
	
	// 캐릭터 데이터를 런타임 중에 제거 : 캐릭터 데이터를 적용하는 상황에서 초기에 호출함
	void ClearCharacterDataRuntimeState();
	
protected:
	UFUNCTION()
	void OnRep_CurrentCharacterData();
	
	UFUNCTION()
	void OnRep_CurrentWeapon();
	
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spectator")
	TObjectPtr<UNSSpectatorViewComponent> SpectatorViewComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parts")
	TObjectPtr<UNSPartVisualComponent> PartVisualComp;
	
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
	// 현재 캐릭터 데이터
	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentCharacterData, BlueprintReadOnly, Category = "Character|Data")
	TObjectPtr<const UNSCharacterData> CurrentCharacterData;
	
	// 현재 캐릭터 무기
	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentWeapon, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<ANSWeaponBase> CurrentWeapon;
	
	// AbilitySpecHandle
	UPROPERTY(Transient)
	TArray<FGameplayAbilitySpecHandle> CharacterDataAbilityHandles;
	
	// EffectHandle
	UPROPERTY(Transient)
	TArray<FActiveGameplayEffectHandle> CharacterDataEffectHandles;

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
	
	//드론 스폰관련 Test
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Companion|AI")
	TSubclassOf<ANSBaseCompanionAI> DroneAIClass;
	
private:
	// 사망 상태에 따라 연출을 실행할 때 클라이언트에 복제하는 시점을 조정하기 위한 bool 변수
	UPROPERTY(ReplicatedUsing = OnRep_DeathPresentationStarted)
	bool bDeathPresentationStarted = false;
};
