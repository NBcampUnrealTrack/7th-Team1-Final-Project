// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "NSDeathSpectatorPawn.generated.h"

class UCameraComponent;
class UNSInputBinderComponent;
class UNSSpectatorViewComponent;

UCLASS()
class NEOSANCTUM_API ANSDeathSpectatorPawn : public APawn
{
	GENERATED_BODY()
	
public:
	ANSDeathSpectatorPawn();
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	void SetSpectatorView(UNSSpectatorViewComponent* NewSpectatorView);

	UNSInputBinderComponent* GetInputBinderComponent() const { return InputBinderComp; }
	UCameraComponent* GetCameraComponent() const { return CameraComp; }
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spectator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRootComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spectator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNSInputBinderComponent> InputBinderComp;

	// 타겟의 SpectatorViewComponent 캐시
	UPROPERTY(Transient)
	TObjectPtr<UNSSpectatorViewComponent> TargetSpectatorView;
	
	// 카메라 위치를 따라가는 보간 속도
	UPROPERTY(EditDefaultsOnly, Category = "Spectator|Interpolation", meta = (ClampMin = "0.0"))
	float LocationInterpSpeed = 18.f;
	
	// 카메라 회전을 따라가는 보간 속도
	UPROPERTY(EditDefaultsOnly, Category = "Spectator|Interpolation", meta = (ClampMin = "0.0"))
	float RotationInterpSpeed = 18.f;
	
	// 시야 각을 따라가는 보간 속도
	UPROPERTY(EditDefaultsOnly, Category = "Spectator|Interpolation", meta = (ClampMin = "0.0"))
	float FOVInterpSpeed = 12.f;
	
	// 대상 전환 시에 즉시 대상의 카메라 정보로 이동할지 여부
	UPROPERTY(Transient)
	bool bSnapToTargetPOV = false;
};
