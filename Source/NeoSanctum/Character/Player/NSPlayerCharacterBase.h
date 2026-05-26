// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "NSPlayerCharacterBase.generated.h"

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

UCLASS()
class NEOSANCTUM_API ANSPlayerCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANSPlayerCharacterBase();
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* EventController) override;
	virtual void OnRep_PlayerState() override;
	
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	void InitializeFromCharacterData(const UNSCharacterData* InCharacterData);
	
	ANSWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }
	
public:
	UCharacterTrajectoryComponent* GetCharacterTrajectoryComponent() const { return CharacterTrajectoryComp; };
	UNSInputBinderComponent* GetInputBinderComponent() const { return InputBinderComp; }
	
protected:
	void InitializeAbilitySystem();
	void BindAttributeDelegates();
	
	void ApplyCharacterVisual();
	void ApplyInitialAttributeEffect();
	void GiveCharacterDataAbilities();
	void SpawnDefaultWeapon();
	
protected:
	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
	void ApplyMoveSpeedToCharacter(float MoveSpeed);
	
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
	
protected:
	// Motion Matching에서 사용하는 애니메이션 이동 예측 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UCharacterTrajectoryComponent> CharacterTrajectoryComp;
	
protected:
	// ASC 캐시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UNSAbilitySystemComponent> NSAbilitySystemComponent;
	
	// Player 전용 Attribute Set
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UNSPlayerAttributeSet> PlayerAttributeSet;
	
protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Character|Data")
	TObjectPtr<const UNSCharacterData> CharacterData;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<ANSWeaponBase> CurrentWeapon;

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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AActor> Weapon;
	
protected:
	// 캐릭터 데이터 테스트용 함수
	void LoadDebugCharacterDataAssets(UNSCharacterData* InCharacterData);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Debug")
	TSoftObjectPtr<UNSCharacterData> DebugCharacterData;
};
