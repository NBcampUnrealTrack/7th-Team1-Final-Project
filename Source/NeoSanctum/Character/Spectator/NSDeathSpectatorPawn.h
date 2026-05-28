// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "NSDeathSpectatorPawn.generated.h"

class UCameraComponent;
class UNSInputBinderComponent;

UCLASS()
class NEOSANCTUM_API ANSDeathSpectatorPawn : public APawn
{
	GENERATED_BODY()
	
public:
	ANSDeathSpectatorPawn();
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	UNSInputBinderComponent* GetInputBinderComponent() const { return InputBinderComp; }
	UCameraComponent* GetCameraComponent() const { return CameraComp; }
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spectator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRootComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spectator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNSInputBinderComponent> InputBinderComp;
};
