// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "NSPlayerWorldStatusSubsystem.generated.h"

class APlayerController;
class INSPlayerWorldStatusHost;
class UNSPlayerWorldStatusPresenter;

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 로컬 플레이어별 플레이어 월드 상태 UI Presenter 수명을 관리하는 Subsystem입니다.
 * HUD Host를 등록받고 플레이어 월드 상태 Presenter를 소유합니다.
 */
UCLASS()
class NEOSANCTUM_API UNSPlayerWorldStatusSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	// PlayerController에서 플레이어 월드 상태 Subsystem을 조회하는 함수
	static UNSPlayerWorldStatusSubsystem* Get(const APlayerController* PlayerController);

	// Subsystem 생성 시 Presenter를 준비하는 함수
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Subsystem 해제 시 Presenter와 Host 참조를 정리하는 함수
	virtual void Deinitialize() override;

	// HUD Root가 제공하는 플레이어 월드 상태 Host를 등록하는 함수
	void RegisterHUDHost(UObject* InHostObject);

	// 등록된 플레이어 월드 상태 Host를 해제하는 함수
	void UnregisterHUDHost(UObject* InHostObject);

	// 현재 등록된 플레이어 월드 상태 Host를 반환하는 함수
	INSPlayerWorldStatusHost* GetHUDHost() const;

	// 플레이어 월드 상태 Presenter를 반환하는 함수
	UNSPlayerWorldStatusPresenter* GetPresenter() const { return Presenter.Get(); }

private:
	// HUD Root UObject를 약하게 보관하는 변수
	TWeakObjectPtr<UObject> HUDHostObject;

	// 플레이어 월드 상태 UI Presenter를 소유하는 변수
	UPROPERTY()
	TObjectPtr<UNSPlayerWorldStatusPresenter> Presenter;
};
