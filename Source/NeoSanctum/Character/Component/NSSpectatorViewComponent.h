// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSSpectatorViewComponent.generated.h"

class UCameraComponent;

// 보낼 카메라 정보 구조체
USTRUCT(BlueprintType)
struct FNSReplicatedSpectatorPOV
{
	GENERATED_BODY()

	// 위치
	UPROPERTY(BlueprintReadOnly)
	FVector_NetQuantize Location = FVector::ZeroVector;
	
	// 회전
	UPROPERTY(BlueprintReadOnly)
	FRotator Rotation = FRotator::ZeroRotator;
	
	// 시야 각 : 시야 각을 안보내면 관전자 화면이 플레이어가 보는 화면확대 정도와 달라서 어색함
	UPROPERTY(BlueprintReadOnly)
	float FOV = 90.f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSSpectatorViewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSSpectatorViewComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void SetSourceCamera(UCameraComponent* NewSourceCamera);
	const FNSReplicatedSpectatorPOV& GetReplicatedPOV() const { return ReplicatedPOV; }

private:
	void UpdateSpectatorPOV();
	FNSReplicatedSpectatorPOV CaptureCurrentPOV() const;

	UFUNCTION(Server, Unreliable)
	void Server_UpdateSpectatorPOV(const FNSReplicatedSpectatorPOV& NewPOV);

private:
	// 관전자에게 정보를 보낼 카메라 캐시
	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> SourceCamera;

	// 관전자에게 보낼 카메라 정보
	UPROPERTY(Replicated)
	FNSReplicatedSpectatorPOV ReplicatedPOV;
	
	// 타이머를 통해 얼마나 자주 카메라 정보를 보낼지 결정 : 0.033f가 약 30Hz라고 해서 기본값으로 설정함
	UPROPERTY(EditDefaultsOnly, Category = "Spectator", meta = (ClampMin = "0.01"))
	float UpdateInterval = 0.033f;

	FTimerHandle UpdateTimerHandle;
};
