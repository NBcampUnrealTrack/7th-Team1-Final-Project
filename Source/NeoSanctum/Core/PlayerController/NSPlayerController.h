// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffectTypes.h"
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
	
protected:
	virtual void BeginPlay() override;
};
