// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"                                                                                      
#include "GameFramework/Actor.h"
#include "NeoSanctum/Data/Progression/Currency/NSCurrencyTypes.h"
#include "NSCurrencyReplicationProxy.generated.h"

class USceneComponent;
class ANSLocalCurrencyPickup;
class UNSCurrencyVisualData;

/**
 * 각 PlayerController가 소유하는 Onwer_Only 네트워크 전달 Actor
 * 서버 재화 시슽메이 이 Actor를 통해 해당 클라에만 픽업 생성/제거/복원 지시
 * 클라 측에서 로컬 픽업을 DropId별로 직접 관리
 */
UCLASS()
class NEOSANCTUM_API ANSCurrencyReplicationProxy : public AActor
{
	GENERATED_BODY()
public:
	ANSCurrencyReplicationProxy();
	// ================================================================
	// 서버 전용 송신
	// ================================================================
	void SendSpawnEvent(const FNSCurrencySpawnEvent& Event);
	// 수거 성공 or 만료 -> 로컬 픽업 제거
	void SendRemoveEvent(int32 DropId);
	// 검증 실패 -> 로컬 픽업 복원
	void SendRestoreEvent(int32 DropId);
	
private:
	UFUNCTION(Client, Reliable)
	void Client_SpawnCurrency(const FNSCurrencySpawnEvent& Event);
	
	UFUNCTION(Client, Reliable)
	void Client_RemoveCurrency(int32 DropId);
	
	UFUNCTION(Client, Reliable)
	void Client_RestoreCurrency(int32 DropId);
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;
	
	// 클라에서 스폰할 픽업 클래스
	UPROPERTY(EditDefaultsOnly, Category="Currency")
	TSubclassOf<ANSLocalCurrencyPickup> PickupClass;
	
	// 비주얼 데이터
	UPROPERTY(EditDefaultsOnly, Category="Currency")
	TObjectPtr<UNSCurrencyVisualData> VisualData;
	
	UPROPERTY()
	TMap<int32, TObjectPtr<ANSLocalCurrencyPickup>> ActivePickups;
};