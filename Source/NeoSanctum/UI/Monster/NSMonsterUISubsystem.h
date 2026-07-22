// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "NSMonsterUISubsystem.generated.h"

class APlayerController;
class INSMonsterUIHost;
class UNSBossMonsterPresenter;
class UNSNormalMonsterPresenter;

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 로컬 플레이어별 몬스터 상태 UI Presenter 수명을 관리하는 Subsystem입니다.
 * HUD Host를 등록받고 일반 몬스터/보스 Presenter를 소유합니다.
 */
UCLASS()
class NEOSANCTUM_API UNSMonsterUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	// PlayerController에서 몬스터 UI Subsystem을 조회하는 함수
	static UNSMonsterUISubsystem* Get(const APlayerController* PlayerController);

	// Subsystem 생성 시 Presenter를 준비하는 함수
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Subsystem 해제 시 Presenter와 Host 참조를 정리하는 함수
	virtual void Deinitialize() override;

	// HUD Root가 제공하는 몬스터 UI Host를 등록하는 함수
	void RegisterHUDHost(UObject* InHostObject);

	// 등록된 몬스터 UI Host를 해제하는 함수
	void UnregisterHUDHost(UObject* InHostObject);

	// 현재 등록된 몬스터 UI Host를 반환하는 함수
	INSMonsterUIHost* GetHUDHost() const;

	// 일반 몬스터 Presenter를 반환하는 함수
	UNSNormalMonsterPresenter* GetNormalMonsterPresenter() const { return NormalMonsterPresenter.Get(); }

	// 보스 몬스터 Presenter를 반환하는 함수
	UNSBossMonsterPresenter* GetBossMonsterPresenter() const { return BossMonsterPresenter.Get(); }

private:
	// HUD Root UObject를 약하게 보관하는 변수
	TWeakObjectPtr<UObject> HUDHostObject;

	// 일반 몬스터 상태 UI Presenter를 소유하는 변수
	UPROPERTY()
	TObjectPtr<UNSNormalMonsterPresenter> NormalMonsterPresenter;

	// 보스 몬스터 상태 UI Presenter를 소유하는 변수
	UPROPERTY()
	TObjectPtr<UNSBossMonsterPresenter> BossMonsterPresenter;
};
