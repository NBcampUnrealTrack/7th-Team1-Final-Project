// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Character/Enemy/NSEnemyPawnBase.h"
#include "NSBossPawnBase.generated.h"

class UNSBossModeComponent;
class UNSBossTargetComponent;

/*
 * 작성자 : 최준혁
 *
 * 파일 생성일 : 26.07.02
 * 
 * 클래스 개요 : Boss Pawn이 공유하는 공통 기반 클래스
 * ANSEnemyPawnBase의 Enemy 공통 기능을 사용하고, Boss Mode와 Boss 다중 타깃 기능을 추가
*/
UCLASS(Abstract)
class NEOSANCTUM_API ANSBossPawnBase : public ANSEnemyPawnBase
{
	GENERATED_BODY()

public:
	ANSBossPawnBase();

	virtual void BeginPlay() override;

	// Boss의 ModeComponent를 반환하는 함수
	UNSBossModeComponent* GetBossModeComponent() const { return BossModeComponent; }

	// Boss의 다중 타깃 컴포넌트를 반환하는 함수
	UNSBossTargetComponent* GetBossTargetComponent() const { return BossTargetComponent; }

protected:
	// Boss 사망 시 Boss 전용 공격 타깃 상태를 함께 정리하는 함수
	virtual void ApplyDeadState() override;

protected:
	// Boss의 현재 전투 Mode를 관리하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSBossModeComponent> BossModeComponent;

	// Boss의 공격별 다중 타깃 목록을 관리하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSBossTargetComponent> BossTargetComponent;
};
