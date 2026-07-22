// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "NSDeathSpectatorPawn.generated.h"

class UCameraComponent;
class UNSInputBinderComponent;
class USphereComponent;
class ANSPlayerCharacterBase;

UCLASS()
class NEOSANCTUM_API ANSDeathSpectatorPawn : public APawn
{
	GENERATED_BODY()
	
public:
	ANSDeathSpectatorPawn();
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 관전자 Pawn이 따라갈 Actor 설정
	void SetFollowTarget(AActor* NewFollowTarget);
	// 서버에서 확정한 관전 대상을 복제 상태로 저장
	void SetSpectatorTarget(ANSPlayerCharacterBase* NewSpectatorTarget);
	ANSPlayerCharacterBase* GetSpectatorTarget() const { return SpectatorTarget; }
	// 복제나 ClientRestart 이후 관전 ViewTarget을 다시 적용
	void RefreshSpectatorTargetView();

	UNSInputBinderComponent* GetInputBinderComponent() const { return InputBinderComp; }
	UCameraComponent* GetCameraComponent() const { return CameraComp; }
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spectator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRootComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spectator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNSInputBinderComponent> InputBinderComp;
	
	// 던전 룸 가시성 계산용 바운드
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spectator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> RoomBoundsComp;

	// Room streaming 유지를 위해 따라갈 관전 대상
	UPROPERTY(Transient)
	TObjectPtr<AActor> FollowTarget;
	
	// 관전 대상 주변을 따라가는 보간 속도
	UPROPERTY(EditDefaultsOnly, Category = "Spectator|Interpolation", meta = (ClampMin = "0.0"))
	float FollowInterpSpeed = 12.f;
	
	// 관전 대상 위치에서 떨어져 있을 거리
	UPROPERTY(EditDefaultsOnly, Category = "Spectator|Interpolation")
	FVector FollowOffset = FVector(0.f, 0.f, 120.f);

	UPROPERTY(EditDefaultsOnly, Category = "Spectator|Camera", meta = (ClampMin = "0.0"))
	float CameraLocationInterpSpeed = 18.f;

	UPROPERTY(EditDefaultsOnly, Category = "Spectator|Camera", meta = (ClampMin = "0.0"))
	float CameraRotationInterpSpeed = 24.f;

	UPROPERTY(EditDefaultsOnly, Category = "Spectator|Camera", meta = (ClampMin = "0.0"))
	float CameraFOVInterpSpeed = 24.f;

	// 서버에서 확정한 관전 대상
	UPROPERTY(ReplicatedUsing = OnRep_SpectatorTarget)
	TObjectPtr<ANSPlayerCharacterBase> SpectatorTarget;

	// 같은 대상을 다시 확정해도 클라이언트 적용을 보장하기 위한 갱신 번호
	UPROPERTY(ReplicatedUsing = OnRep_SpectatorTargetRevision)
	int32 SpectatorTargetRevision = 0;

	UPROPERTY(Transient)
	TObjectPtr<ANSPlayerCharacterBase> LastAppliedSpectatorTarget;

	UPROPERTY(Transient)
	bool bHasSmoothedCameraPOV = false;

	UPROPERTY(Transient)
	FVector SmoothedCameraLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	FRotator SmoothedCameraRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	float SmoothedCameraFOV = 90.f;

	UFUNCTION()
	void OnRep_SpectatorTarget();

	UFUNCTION()
	void OnRep_SpectatorTargetRevision();

	// 로컬 컨트롤러에서 관전 대상을 ViewTarget으로 적용
	void ApplySpectatorTargetView();
};
