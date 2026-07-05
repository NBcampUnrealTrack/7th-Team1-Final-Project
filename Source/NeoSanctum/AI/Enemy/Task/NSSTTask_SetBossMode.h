// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "NSSTTask_SetBossMode.generated.h"

class UNSBossModeComponent;

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.05
 * 
 * 클래스 개요 : Boss StateTree 상태 진입 시 BossModeComponent의 현재 ModeTag를 변경하는 Task
*/
USTRUCT(meta = (DisplayName = "Set Boss Mode", Category = "NeoSanctum|Boss"))
struct NEOSANCTUM_API FNSSTTask_SetBossMode : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FNSSTTask_SetBossMode();

	// StateTree 상태 진입 시 BossModeComponent에 적용할 ModeTag
	UPROPERTY(EditAnywhere, Category = "Config", meta = (Categories = "State.Enemy"))
	FGameplayTag ModeTag;

	// StateTree 상태 종료 시 현재 ModeTag가 ModeTag와 같으면 제거할지 여부
	UPROPERTY(EditAnywhere, Category = "Config")
	bool bClearModeOnExit = false;

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

private:
	// StateTree Context에서 BossModeComponent를 찾는 함수
	UNSBossModeComponent* ResolveModeComponent(FStateTreeExecutionContext& Context) const;
};
