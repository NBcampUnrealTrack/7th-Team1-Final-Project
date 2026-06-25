// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Data/Progression/Currency/NSCurrencyTypes.h"
#include "NSLocalCurrencyPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNSCurrencyVisualData;

UCLASS()
class NEOSANCTUM_API ANSLocalCurrencyPickup : public AActor
{
	GENERATED_BODY()

public:
	ANSLocalCurrencyPickup();
	
	void Initialize(const FNSCurrencySpawnEvent& Event, const UNSCurrencyVisualData* VisualData);
	
	int32 GetDropId() const {return DropId;}
	
	// ================================================================
	// 서버 검증 결과 라우팅
	// ================================================================
	
	// 검증 결과 성공 -> 제거
	void ConfirmCollected();
	// 검증 실패 -> 다시 보이게
	void RestoreVisual();
protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
				UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	
private:
	void StartMeshLoad(const UNSCurrencyVisualData* VisualData);
	void OnMeshLoaded();
	void HandleExpire();
	void StartDropLaunch(const FNSDropLaunchData& InLaunchData);
	void UpdateDropLaunch();
	void FinishDropLaunch();
	float GetServerWorldTimeSeconds() const;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> CollisionSphere;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComp;
	
	UPROPERTY(EditDefaultsOnly, Category="Currency")
	float CollisionRadius = 80.f;
	
	int32 DropId = INDEX_NONE;
	FGameplayTag CurrencyType;
	ENSCurrencyGrade Grade = ENSCurrencyGrade::None;
	
	FSoftObjectPath PendingMeshPath;
	TSharedPtr<FStreamableHandle> MeshLoadHandle;
	FTimerHandle ExpireTimer;
	
	FNSDropLaunchData LaunchData;
	bool bIsLaunching = false;
	
	bool bCollectRequested = false;
};
