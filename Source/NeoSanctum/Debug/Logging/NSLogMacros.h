// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NSLogCategories.h"
#include "NSLogContext.h"

#include "Logging/StructuredLog.h"

/**
 * NeoSanctum 공통 로그 매크로입니다.
 *
 * 기본 사용법:
 *
 * NS_LOG(LogNS, Log, "Initialize complete");
 *
 * NS_OBJ_LOG(LogNSGAS, Log,
 * 	"Input processed. Held={Held}",
 * 	("Held", InputHeldSpecHandles.Num()));
 *
 * NS_ACTOR_LOG(AvatarActor, LogNSGAS, Log,
 * 	"TargetDataReady. Count={Count}",
 * 	("Count", TargetDataHandle.Num()));
 *
 * 작성 규칙:
 * - Format 문자열에는 TEXT()를 붙이지 않습니다. UE_LOGFMT 형식에 맞춰 일반 문자열로 작성합니다.
 * - 추가 값은 ("Name", Value) 형태로 넘깁니다.
 * - 팀 컨벤션상 Error / Fatal 대신 Log 또는 Warning을 사용합니다.
 * - 프레임마다 많이 호출되는 곳에서는 로그 스팸에 주의합니다.
 *
 * 매크로 선택 기준:
 * - NS_LOG      : static 함수, 전역 함수, 단순 시스템 로그.
 * - NS_OBJ_LOG  : UObject 멤버 함수에서 this 이름까지 보고 싶을 때.
 * - NS_NET_LOG  : UObject / UWorld 기준으로 NetMode만 보고 싶을 때.
 * - NS_ACTOR_LOG: Actor의 NetMode + Role + Actor 이름까지 보고 싶을 때.
 *
 * 주의:
 * - 매크로는 namespace로 감쌀 수 없으므로 이 헤더를 include하면 아래 매크로들이 전역으로 노출됩니다.
 * - 매크로 노출을 더 엄격하게 관리하고 싶다면 NSActorLog.h, NSNetLog.h처럼 기능별 헤더로 분리합니다.
 */

/**
 * 정적 함수 / 전역 함수 / this가 의미 없는 곳에서 사용합니다.
 *
 * 출력 예:
 * [FunctionName] Message
 */
#define NS_LOG(Category, Verbosity, Format, ...) \
	do \
	{ \
		UE_LOGFMT(Category, Verbosity, "[{Function}] " Format, \
			("Function", FString(__FUNCTION__)), ##__VA_ARGS__); \
	} \
	while (false)

/**
 * UObject 멤버 함수에서 사용합니다.
 *
 * 전제:
 * - this가 UObject 계열이어야 합니다.
 * - 일반 C++ 타입에서는 NS_LOG를 사용합니다.
 *
 * 출력 예:
 * [GA_RangerAutoFire_C_0][FunctionName] Message
 */
#define NS_OBJ_LOG(Category, Verbosity, Format, ...) \
	do \
	{ \
		UE_LOGFMT(Category, Verbosity, "[{Instance}][{Function}] " Format, \
			("Instance", NSLog::GetObjectNameString(this)), \
			("Function", FString(__FUNCTION__)), ##__VA_ARGS__); \
	} \
	while (false)

/**
 * UObject 또는 UWorld 기준으로 NetMode를 함께 출력합니다.
 *
 * 사용하기 좋은 곳:
 * - Subsystem
 * - Component
 * - GameplayAbility
 * - Manager 계열 UObject
 *
 * 출력 예:
 * [Client01][FunctionName] Message
 */
#define NS_NET_LOG(WorldContextObject, Category, Verbosity, Format, ...) \
	do \
	{ \
		UE_LOGFMT(Category, Verbosity, "[{NetMode}][{Function}] " Format, \
			("NetMode", NSLog::GetNetModeString(WorldContextObject)), \
			("Function", FString(__FUNCTION__)), ##__VA_ARGS__); \
	} \
	while (false)

/**
 * Actor 기준으로 NetMode, LocalRole / RemoteRole, Actor 이름을 함께 출력합니다.
 *
 * 사용하기 좋은 곳:
 * - GAS Ability에서 AvatarActor 기준 로그를 찍을 때
 * - TargetData 송수신 흐름을 확인할 때
 * - GameplayCue 중복 실행 여부를 확인할 때
 * - 무기 / 캐릭터 복제 흐름을 확인할 때
 *
 * 출력 예:
 * [ListenServer][Authority / SimulatedProxy][BP_Ranger_C_0][FunctionName] Message
 */
#define NS_ACTOR_LOG(Actor, Category, Verbosity, Format, ...) \
	do \
	{ \
		const AActor* LogActor = (Actor); \
		UE_LOGFMT(Category, Verbosity, "[{NetMode}][{Role}][{Actor}][{Function}] " Format, \
			("NetMode", NSLog::GetNetModeString(LogActor)), \
			("Role", NSLog::GetRoleString(LogActor)), \
			("Actor", NSLog::GetActorNameString(LogActor)), \
			("Function", FString(__FUNCTION__)), ##__VA_ARGS__); \
	} \
	while (false)
