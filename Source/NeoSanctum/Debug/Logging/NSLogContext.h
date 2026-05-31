// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "Containers/UnrealString.h"

class AActor;
class UObject;
class UWorld;

/**
 * 로그 매크로에서 사용하는 문자열 변환 유틸입니다.
 *
 * IWYU 의도:
 * - 이 헤더에서는 AActor / UObject / UWorld를 전방 선언만 합니다.
 * - Engine/World.h, GameFramework/Actor.h 같은 무거운 include는 .cpp로 숨깁니다.
 * - 로그를 쓰는 파일이 로그 때문에 불필요한 엔진 헤더를 끌고 오지 않게 하기 위함입니다.
 *
 * 사용 규칙:
 * - 일반적으로 팀원이 직접 호출하기보다는 NS_LOG 계열 매크로를 통해 사용합니다.
 * - 로그 문자열 포맷을 바꾸고 싶다면 매크로를 먼저 확인합니다.
 * - NetMode / Role 변환 규칙을 바꾸고 싶다면 이 파일을 확인합니다.
 */
namespace NSLog
{
	/**
	 * World의 NetMode를 사람이 읽기 쉬운 문자열로 변환합니다.
	 *
	 * 반환 예:
	 * - Standalone
	 * - ListenServer
	 * - DedicatedServer
	 * - Client01, Client02
	 */
	FString GetNetModeString(const UWorld* World);

	/**
	 * UObject에서 World를 얻어 NetMode 문자열을 반환합니다.
	 *
	 * 사용 예:
	 * - Actor
	 * - ActorComponent
	 * - GameplayAbility
	 * - Subsystem
	 */
	FString GetNetModeString(const UObject* WorldContextObject);

	/**
	 * Actor의 LocalRole / RemoteRole을 함께 표시합니다.
	 *
	 * GAS나 복제 디버깅에서는 Authority / AutonomousProxy / SimulatedProxy 구분이 중요합니다.
	 * Actor가 아닌 UObject 계열에서는 이 함수 대신 GetNetModeString을 사용합니다.
	 */
	FString GetRoleString(const AActor* Actor);

	// UObject 이름을 안전하게 반환합니다.
	FString GetObjectNameString(const UObject* Object);

	// Actor 이름을 안전하게 반환합니다.
	FString GetActorNameString(const AActor* Actor);
}
