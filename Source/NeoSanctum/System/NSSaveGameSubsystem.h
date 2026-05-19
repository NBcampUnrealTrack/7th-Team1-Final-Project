// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSSaveRepository.h"
#include "NSSaveGameSubsystem.generated.h"

class UNSPermanentSaveGame;

/**
 * 영구 데이터 저장 완료 콜백
 * @param bSuccess 저장 성공 여부
 */
DECLARE_DELEGATE_OneParam(FNSSaveComplete, bool);

/**
 * 영구 데이터 로드 완료 콜백
 * 로드 결과와 역직렬화된 세이브 오브젝트를 함께 전달
 * @param 로드 성공 여부
 * @param 로드된 영구 세이브 데이터 오브젝트 (실패시 nullptr)
 */
DECLARE_DELEGATE_TwoParams(FNSLoadPermanentComplete, bool, UNSPermanentSaveGame*);

// 게임 시작 시 영구 데이터 로드 완료 알림 외부에서는 여기서 데이터를 받아 처리
DECLARE_MULTICAST_DELEGATE_OneParam(FNSOnPermanentDataLoaded, UNSPermanentSaveGame*);

UCLASS()
class NEOSANCTUM_API UNSSaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void SavePermanent(UNSPermanentSaveGame* Data, FNSSaveComplete OnComplete);
	void LoadPermanent(FNSLoadPermanentComplete OnComplete);

	// 로드가 이미 완료된 경우 즉시 접근, 아직 로딩 중이면 nullptr
	UNSPermanentSaveGame* GetCachedPermanentData() const { return CachedData; }

	// 초기 로드 완료 시 브로드캐스트
	FNSOnPermanentDataLoaded OnPermanentDataLoaded;

private:
	// 구현한 인터페이스 객체를 담는 컨테이너
	UPROPERTY()
	TScriptInterface<INSSaveRepository> Repository;

	// 마지막으로 로드된 세이브 데이터를 메모리에 캐싱, 추후에 세이브파일을 읽지않고 이 캐시데이터를 사용하면 됨
	UPROPERTY()
	TObjectPtr<UNSPermanentSaveGame> CachedData;

};
