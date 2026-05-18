// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NSPlayerCharacterBase.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UCharacterTrajectoryComponent;

UCLASS()
class NEOSANCTUM_API ANSPlayerCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ANSPlayerCharacterBase();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	TObjectPtr<USpringArmComponent> SpringArmComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	TObjectPtr<UCameraComponent> CameraComp;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UCharacterTrajectoryComponent> CharacterTrajectoryComp;
};
