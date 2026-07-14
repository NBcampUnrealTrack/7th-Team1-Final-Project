// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NSPlayerWorldStatusHost.generated.h"

class UPanelWidget;

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 플레이어 월드 상태 UI가 배치될 HUD 레이어를 제공하는 인터페이스입니다.
 * 구현체는 플레이어 상태 데이터는 알지 않고, 위젯이 붙을 화면 컨테이너만 제공합니다.
 */
UINTERFACE(MinimalAPI)
class UNSPlayerWorldStatusHost : public UInterface
{
	GENERATED_BODY()
};

class NEOSANCTUM_API INSPlayerWorldStatusHost
{
	GENERATED_BODY()

public:
	// 플레이어 월드 상태 위젯을 배치할 패널을 반환하는 함수
	virtual UPanelWidget* GetPlayerWorldStatusLayer() const = 0;
};
