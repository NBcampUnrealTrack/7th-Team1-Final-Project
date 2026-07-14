// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSWaypointMarkerComponent.generated.h"

class UTexture2D;

/**
 * 웨이포인트 마커를 표시할 액터에 붙이는 컴포넌트
 * bMarkerActive를 서버가 켜고 끄면 리플리케이션으로 모든 클라에 반영되고,
 * 각 클라는 로컬 NSWaypointSubsystem에 등록/해제
 */
UCLASS(ClassGroup = (NeoSanctum), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSWaypointMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSWaypointMarkerComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버 권한에서 마커 표시 여부 토글
	UFUNCTION(BlueprintCallable, Category = "Waypoint")
	void SetMarkerActive(bool bNewActive);

	/**
	 * 로컬(비복제) 마커 표시 토글
	 * 플레이어별 안내(아웃런 콘솔/NPC)나 비복제 액터(보스 진입 볼륨)처럼
	 * 리플리케이션을 타면 안 되는 경우에 사용. 호출한 머신에서만 반영됨
	 */
	UFUNCTION(BlueprintCallable, Category = "Waypoint")
	void SetMarkerActiveLocal(bool bNewActive);

	// 마커 아이콘
	const TSoftObjectPtr<UTexture2D>& GetMarkerIcon() const { return MarkerIcon; }

	// 마커 표시 월드 위치 = 소유 액터 위치 + 오프셋
	FVector GetMarkerWorldLocation() const;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnRep_MarkerActive();

	// 현재 bMarkerActive 상태에 맞춰 로컬 서브시스템에 등록/해제
	void UpdateRegistration();

	// 마커 아이콘 텍스처
	UPROPERTY(EditAnywhere, Category = "Waypoint")
	TSoftObjectPtr<UTexture2D> MarkerIcon;

	// 액터 원점 기준 마커 표시 오프셋
	UPROPERTY(EditAnywhere, Category = "Waypoint")
	FVector WorldOffset = FVector(0.f, 0.f, 50.f);

	// 마커 표시 여부, 서버가 원본이고 OnRep으로 각 클라가 등록 상태를 갱신
	UPROPERTY(ReplicatedUsing = OnRep_MarkerActive, EditAnywhere, Category = "Waypoint")
	bool bMarkerActive = true;

	// 로컬 전용 표시 플래그 (복제 안 됨), 최종 표시는 bMarkerActive와 OR 판정
	bool bMarkerActiveLocal = false;
};
