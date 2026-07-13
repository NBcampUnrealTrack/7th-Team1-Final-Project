// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NSBossMonsterPresenter.generated.h"

class ULocalPlayer;

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 보스 몬스터 상태 UI의 런타임 수명을 관리하는 Presenter입니다.
 * 이후 다중 보스 위젯과 ViewModel을 보스 수에 맞게 관리합니다.
 * Presenter: 이벤트와 ViewModel/Widget을 연결해서 화면 표현을 관리하는 객체
 */
UCLASS()
class NEOSANCTUM_API UNSBossMonsterPresenter : public UObject
{
	GENERATED_BODY()

public:
	// 로컬 플레이어 기준 보스 Presenter를 초기화하는 함수
	void Initialize(ULocalPlayer* InLocalPlayer);

	// 보스 Presenter가 보유한 런타임 상태를 해제하는 함수
	void Shutdown();

private:
	// Presenter가 속한 로컬 플레이어를 약하게 보관하는 변수
	TWeakObjectPtr<ULocalPlayer> OwningLocalPlayer;
};