// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Data/Progression/Drop/NSHealDropTypes.h"
#include "NSLocalHealPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UDataTable;

/**
 * 클라 로컬 전용 회복 아이템 픽업
 * 
 */
UCLASS()
class NEOSANCTUM_API ANSLocalHealPickup : public AActor
{
	GENERATED_BODY()

public:
	ANSLocalHealPickup();
	
	void Initialize(const FNSHealSpawnEvent& Event, const UDataTable* HealPotionTable);
	int32 GetDropId() const { return DropId; }
	
	// ================================================================
	// 서버 검증 결과
	// ================================================================
	
	// 서버 검증 성공 -> 완전히 제거
	void ConfirmCollected();
	// 서버 검증 실패
	void RestoreVisual();
protected:
	// 포물선 발사 애니메이션용 Tick
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& Sweep);
	
private:
	// PotionTag를 RowName으로 변환해 HealPotionTable에서 회복 포션 행을 찾아 메시 비동기 로드 시작.
	void StartMeshLoad(const UDataTable* HealPotionTable);
	// 비동기 완료 콜백
	void OnMeshLoaded();
	// 만료 타이머 종료시 호출
	void HandleExpire();
	
	// 포물선 관련
	void StartDropLaunch(const FNSDropLaunchData& InLaunchData);
	void UpdateDropLaunch();
	void FinishDropLaunch();
	
	float GetServerWorldTimeSeconds() const;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> CollisionSphere;
	
	UPROPERTY(VisibleAnywhere) 
	TObjectPtr<UStaticMeshComponent> MeshComp;
	
	// 오버랩 판정용 스피어
	UPROPERTY(EditDefaultsOnly, Category="Heal")
	float CollisionRadius = 80.f;
	
	int32 DropId = INDEX_NONE;
	// 어떤 포션인지 식별하는 태그. HealPotionTable에서 회복 포션 행(메시/스케일)을 조회하는 키로 쓴다.
	FGameplayTag PotionTag;
	
	FSoftObjectPath PendingMeshPath;
	TSharedPtr<FStreamableHandle> MeshLoadHandle;
	
	FTimerHandle ExpireTimer;
	
	FNSDropLaunchData LaunchData;
	bool bIsLaunching = false;
	
	bool bCollectRequested = false;
};
