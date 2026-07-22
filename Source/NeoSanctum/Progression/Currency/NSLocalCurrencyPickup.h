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

	// 착지 후 계속 틱을 돌며 MeshComp를 사인파로 위아래 움직임
	void UpdateBobAnimation(float DeltaSeconds);

	// 바운싱 애니메이션 진폭/속도
	UPROPERTY(BlueprintReadOnly, Category = "Currency|Visual")
	float BobAmplitude = 12.f;

	UPROPERTY(BlueprintReadOnly, Category = "Currency|Visual")
	float BobSpeed = 2.f;

	// 메시 로드 후 계산한 피벗 보정 Z값 —> 바운싱은 이 값 위에 오프셋을 더함
	float MeshBaseRelativeZ = 0.f;

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
