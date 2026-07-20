// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSPetUpgradeBridgeSubsystem.generated.h"

struct FNSPetUpgradeQueryMessage;
struct FNSPetUpgradeRequestMessage;
struct FNSPetUpgradeSelectRequestMessage;

/**
 *  펫 강화 UI와 진행도 시스템 사이의 GMS메시지를 중계
 *  UI는 진행도 시스템과 CompanionDefinition을 직접 참조하지 않음
 */
UCLASS()
class NEOSANCTUM_API UNSPetUpgradeBridgeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	//GMS 조회 및 강화 요청 채널 구독
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	//등록한 GMS 리스너 해제
	virtual void Deinitialize() override;
	
	
	
private:
	//펫 강화 화면 조회 요청
	void HandleQueryMessage(FGameplayTag Channel, const FNSPetUpgradeQueryMessage& Message);
	
	//특정 펫 강화 노드의 강화 요청을 처리
	void HandleUpgradeRequestMessage(FGameplayTag Channel, const FNSPetUpgradeRequestMessage& Message);
	
	//드론 선택 요청 처리
	void HandleSelectRequestMessage(FGameplayTag Channel, const FNSPetUpgradeSelectRequestMessage& Message);
private:
	//펫 강화 상태 조회 요청 리스너
	FGameplayMessageListenerHandle QueryListenerHandle;
	
	//펫 강화 실행 요청 리스너
	FGameplayMessageListenerHandle UpgradeRequestListenerHandle;
	
	//드론 선택 요청 리스너
	FGameplayMessageListenerHandle SelectRequestListenerHandle;
};
