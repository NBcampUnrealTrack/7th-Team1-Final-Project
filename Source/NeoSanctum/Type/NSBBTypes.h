// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.04
 * 
 * 파일 개요 : Enemy AI Blackboard Key 이름을 역할별 namespace로 관리하는 타입 파일
 * Controller, Behavior Tree Task, StateTree Task가 동일한 Key 이름을 사용하도록 고정
*/
namespace NSBB
{
	// 타깃 탐지, 공격 대상, 시야 정보를 저장하는 Blackboard Key 모음
	namespace Target
	{
		// 현재 AI가 대표 전투 대상으로 추적하는 Actor 키 이름
		extern NEOSANCTUM_API const FName TargetActor;

		// 현재 공격이 실제로 조준하거나 타격할 Actor 키 이름
		extern NEOSANCTUM_API const FName AttackActor;

		// 현재 타깃의 마지막 감지 위치 키 이름
		extern NEOSANCTUM_API const FName TargetLastKnownLocation;

		// 현재 타깃에게 직접 시야가 닿는지 저장하는 Bool 키 이름
		extern NEOSANCTUM_API const FName HasTargetLineOfSight;
	}

	// 공격 가능 여부, 공격 실행 상태, 피격 경직 상태를 저장하는 Blackboard Key 모음
	namespace Combat
	{
		// 현재 AI가 공격을 시작할 수 있는지 저장하는 Bool 키 이름
		extern NEOSANCTUM_API const FName CanAttack;

		// 현재 AI가 공격 Ability를 실행 중인지 저장하는 Bool 키 이름
		extern NEOSANCTUM_API const FName IsAttacking;

		// 현재 AI가 피격 경직, 그로기, 무력화 상태인지 저장하는 Bool 키 이름
		extern NEOSANCTUM_API const FName IsHitReacting;
	}

	// Enemy Phase 상태를 저장하는 Blackboard Key 모음
	namespace Phase
	{
		// Phase 전환 중 일반 공격 패턴 선택을 막는 Bool 키 이름
		extern NEOSANCTUM_API const FName PhasePatternLocked;

		// 현재 적용 중인 PhaseId를 저장하는 Name 키 이름
		extern NEOSANCTUM_API const FName CurrentPhaseId;
	}

	// 일반 몬스터 이동, 후퇴, 순찰 상태를 저장하는 Blackboard Key 모음
	namespace Movement
	{
		// 현재 일반 몬스터가 후퇴해야 하는지 저장하는 Bool 키 이름
		extern NEOSANCTUM_API const FName ShouldRetreat;

		// 현재 일반 몬스터가 이동할 후퇴 위치를 저장하는 Vector 키 이름
		extern NEOSANCTUM_API const FName RetreatLocation;

		// 순찰 BTTask가 선택한 순찰 위치를 저장하는 Vector 키 이름
		extern NEOSANCTUM_API const FName PatrolLocation;
	}

	// 일반 근접 몬스터 예약, 접근, EQS 상태를 저장하는 Blackboard Key 모음
	namespace Melee
	{
		// 현재 몬스터가 근접 공격 예약을 보유 중인지 저장하는 Bool 키 이름
		extern NEOSANCTUM_API const FName HasAttackReservation;

		// 현재 몬스터가 근접 타깃에게 접근할 수 있는지 저장하는 Bool 키 이름
		extern NEOSANCTUM_API const FName CanApproachTarget;

		// 근접 EQS Query Asset을 저장하는 Object 키 이름
		extern NEOSANCTUM_API const FName EQSQuery;

		// 근접 EQS가 선택한 접근 위치를 저장하는 Vector 키 이름
		extern NEOSANCTUM_API const FName ApproachLocation;

		// 근접 EQS를 다시 실행해야 하는지 저장하는 Bool 키 이름
		extern NEOSANCTUM_API const FName EQSNeedsRefresh;
	}

	// Boss 전용 상태를 저장하는 Blackboard Key 모음
	namespace Boss
	{
		// 현재 Boss ModeTag 이름을 저장하는 Name 키 이름
		extern NEOSANCTUM_API const FName CurrentModeTag;
	}
}
