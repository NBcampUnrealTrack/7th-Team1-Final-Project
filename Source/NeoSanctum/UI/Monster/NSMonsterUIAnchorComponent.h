// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "NSMonsterUIAnchorComponent.generated.h"

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 몬스터 상태 UI가 따라갈 월드 기준 위치를 제공하는 SceneComponent입니다.
 * Presenter나 Widget을 알지 않고, 에디터에서 위치만 조정할 수 있는 UI 기준점 역할만 담당합니다.
 */
UCLASS(ClassGroup = (NS), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSMonsterUIAnchorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	// Tick 없이 몬스터 UI 앵커 컴포넌트를 초기화하는 함수
	UNSMonsterUIAnchorComponent();

	// 몬스터 UI가 사용할 월드 기준 위치를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "NS|MonsterUI")
	FVector GetAnchorLocation() const;
};
