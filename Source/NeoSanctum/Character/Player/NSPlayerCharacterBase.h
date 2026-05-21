// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NSPlayerCharacterBase.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UCharacterTrajectoryComponent;
class UNSInputBinderComponent;

UCLASS()
class NEOSANCTUM_API ANSPlayerCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ANSPlayerCharacterBase();
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	UCharacterTrajectoryComponent* GetCharacterTrajectoryComponent() const { return CharacterTrajectoryComp; };
	UNSInputBinderComponent* GetInputBinderComponent() const { return InputBinderComp; }
	
protected:
	void UpdateCameraFacingRotation(float DeltaSeconds);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	TObjectPtr<USpringArmComponent> SpringArmComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	TObjectPtr<UCameraComponent> CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UNSInputBinderComponent> InputBinderComp;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UCharacterTrajectoryComponent> CharacterTrajectoryComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rotation")
	bool bUseCameraFacingRotation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float CameraFacingTurnStartAngle = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float CameraFacingTurnStopAngle = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "0.0"))
	float CameraFacingRotationSpeed = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "0.0"))
	float CameraFacingMoveSpeedThreshold = 10.f;

	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	bool bIsCameraFacingRotationActive = false;
};
