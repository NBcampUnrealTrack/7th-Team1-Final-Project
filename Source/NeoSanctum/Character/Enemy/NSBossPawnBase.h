// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Character/Enemy/NSEnemyPawnBase.h"
#include "NSBossPawnBase.generated.h"

class UNSBossModeComponent;
class UNSBossTargetComponent;
class UNSEnemyCosmeticComponent;
class UNSBossArtilleryComponent;

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
	
	// Enemy Pawn의 코스메틱 이벤트 컴포넌트를 반환하는 함수
	UNSEnemyCosmeticComponent* GetCosmeticComponent() const { return CosmeticComponent; }
	
	// Boss의 포격 패턴 컴포넌트를 반환하는 함수
	UNSBossArtilleryComponent* GetBossArtilleryComponent() const { return BossArtilleryComponent; }

protected:
	// Boss 사망 시 Boss 전용 공격 타깃 상태를 함께 정리하는 함수
	virtual void ApplyDeadState() override;
	
	// Boss가 피격 경직에 진입할 때 타깃 방향으로 몸체를 정렬하는 함수
	virtual void HandleHitReactionStateChanged(bool bHitReacting) override;

	// 피격 경직 몽타주가 타깃 방향으로 재생되도록 Actor Yaw를 보정하는 함수
	void FaceCurrentTargetForHitReaction();
	
	// Boss Pawn에 적용할 Collision Profile 이름을 반환하는 함수
	virtual FName GetAliveCollisionProfileName() const override;

protected:
	// Boss의 현재 전투 Mode를 관리하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSBossModeComponent> BossModeComponent;

	// Boss의 공격별 다중 타깃 목록을 관리하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSBossTargetComponent> BossTargetComponent;
	
	// Enemy Pawn의 코스메틱 이벤트를 리슨서버 호스트와 클라이언트로 전달하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyCosmeticComponent> CosmeticComponent;
	
	// Boss의 포격 패턴 선택과 실행을 관리하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSBossArtilleryComponent> BossArtilleryComponent;
	
	// 피격 경직 진입 시 현재 타깃 방향으로 Actor를 돌릴지 결정하는 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|HitReaction")
	bool bFaceTargetOnHitReaction = true;
};
