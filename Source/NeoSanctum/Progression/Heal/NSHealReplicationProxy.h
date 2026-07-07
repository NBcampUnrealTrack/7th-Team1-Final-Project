// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Actor.h"
#include "NeoSanctum/Data/Progression/Drop/NSHealDropTypes.h"
#include "NSHealReplicationProxy.generated.h"

class USceneComponent;
class ANSLocalHealPickup;
class UDataTable;

/**                                                                                                                                         
 * 각 PlayerController가 소유하는 Owner_Only 네트워크 전달 Actor
 * 서버 회복 드랍 시스템이 이 Actor를 통해 해당 클라에만 픽업 생성/제거/복원 지시
 * 클라 측에서 로컬 픽업을 DropId별로 직접 관리
 */
UCLASS()
class NEOSANCTUM_API ANSHealReplicationProxy : public AActor
{
	GENERATED_BODY()

public:
	ANSHealReplicationProxy();
	// ================================================================
	// 서버 전용 송신
	// ================================================================
	
	// 새 회복 아이템 등록되었을때 (RegisterDrop) 호출
	void SendSpawnEvent(const FNSHealSpawnEvent& Event);
	// 수거 성공 or 만료 -> 로컬 픽업 제거
	void SendRemoveEvent(int32 DropId);
	// 검증 실패 -> 로컬 픽업 복원
	void SendRestoreEvent(int32 DropId);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	// 실제 네트워크 전송을 담당하는 Client RPC 3종, Reliable로 손실시에도 재전송되도록
	UFUNCTION(Client, Reliable)
	void Client_SpawnHeal(const FNSHealSpawnEvent& Event);
	
	UFUNCTION(Client, Reliable)
	void Client_RemoveHeal(int32 DropId);
	
	UFUNCTION(Client, Reliable)
	void Client_RestoreHeal(int32 DropId);

	// 이 Actor자체는 순수 데이터 전달용이지만 AActor는 RootComponent가 있어야 Transform을 가질 수 있어 빈 SceneComponent를 둠
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;
	
	// 클라에 실제로 스폰할 로컬 픽업 액터 클래스, BP서브 클래스로 메시/충돌 반경 커스텀 가능
	UPROPERTY(EditDefaultsOnly, Category="Heal")
	TSubclassOf<ANSLocalHealPickup> PickupClass;
	
	// 회복 포션 정의 테이블
	UPROPERTY(EditDefaultsOnly, Category="Heal")
	TSoftObjectPtr<UDataTable> HealPotionTable;

	UPROPERTY()
	TObjectPtr<UDataTable> LoadedHealPotionTable;

	TSharedPtr<FStreamableHandle> HealPotionTableLoadHandle;
	// DropId -> 로컬 픽업 액터 매핑, 중복 스폰 방지 및 어떤 픽업 대상으로 해야하는지 찾기 위해 필요
	UPROPERTY()
	TMap<int32, TObjectPtr<ANSLocalHealPickup>> ActivePickups;
	
};
