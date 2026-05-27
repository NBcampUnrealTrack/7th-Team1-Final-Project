// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "NSPlayerController.generated.h"

UCLASS()
class NEOSANCTUM_API ANSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ANSPlayerController();
	
	// 게임 시작 요청
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RequestStartRun();
	
	void ExitSpectatorAndRespawn();
	
public:
	// 사망 관전자 상태로 진입 요청 : 캐릭터의 사망 로직에서 요청하도록 되어있음
	void RequestEnterDeathSpectatorMode();

private:
	// 실제로 사망 관전자 상태로 진입
	void EnterDeathSpectatorMode();
	// 진입 타이머 초기화 헬퍼
	void ClearDeathSpectatorModeTimer();
	
private:
	const FGameplayTagContainer& GetGameplayInputModeTags() const { return GameplayInputModeTags; }
	const FGameplayTagContainer& GetDeathSpectatorInputModeTags() const { return DeathSpectatorInputModeTags; }

private:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyRespawn();
	
	//PlayerState의 Attribute 변경을 HUD와 연결
	void BindAttributeToHUD();
	
	//현재 Attribute 값을 한번 읽어서 HUD초기값으로 반영
	void UpdateHUDHealthAndShield();
	
	//체력 변경시 갱신
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	
	//최대 체력 변경시 갱신
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	
	//실드 변경시 갱신
	void OnShieldChanged(const FOnAttributeChangeData& Data);
	
	//최대 실드 변경시 갱신
	void OnMaxShieldChanged(const FOnAttributeChangeData& Data);

private:
	// 기본적인 Gameplay 상태일 때의 Input Mode 태그 목록
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTagContainer GameplayInputModeTags;
	
	// 사망 시 Input Mode 태그 목록
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTagContainer DeathSpectatorInputModeTags;
	
private:
	// 사망 후 몇 초 뒤에 Death Spectator 모드로 진입할지 결정
	UPROPERTY(EditDefaultsOnly, Category = "Spectator", meta = (ClampMin = "0.0"))
	float DeathSpectatorModeDelay = 2.0f;

	FTimerHandle DeathSpectatorModeTimerHandle;
	
protected:
	virtual void BeginPlay() override;
	virtual void ClientRestart_Implementation(class APawn* NewPawn) override;
};
