// Copyright 2026 One Team. All rights reserved.

#include "NSLogContext.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/Object.h"

namespace NSLog
{
	FString GetNetModeString(const UWorld* World)
	{
		// 생성자나 해제 중인 객체처럼 World가 없는 시점에도 로그 함수가 호출될 수 있습니다.
		if (!IsValid(World))
		{
			return TEXT("None");
		}

		switch (World->GetNetMode())
		{
		case NM_Standalone:
			return TEXT("Standalone");

		case NM_DedicatedServer:
			return TEXT("DedicatedServer");

		case NM_ListenServer:
			return TEXT("ListenServer");

		case NM_Client:
#if WITH_EDITOR
			// PIE 다중 클라이언트 테스트에서 어떤 클라이언트의 로그인지 구분하기 위함입니다.
			return FString::Printf(TEXT("Client%02d"), UE::GetPlayInEditorID());
#else
			return TEXT("Client");
#endif

		default:
			return TEXT("Unknown");
		}
	}

	FString GetNetModeString(const UObject* WorldContextObject)
	{
		// UObject가 유효하지 않으면 GetWorld()를 호출할 수 없습니다.
		if (!IsValid(WorldContextObject))
		{
			return TEXT("None");
		}

		return GetNetModeString(WorldContextObject->GetWorld());
	}

	FString GetRoleString(const AActor* Actor)
	{
		// Role은 Actor에만 존재합니다.
		if (!IsValid(Actor))
		{
			return TEXT("None");
		}

		const UEnum* NetRoleEnum = StaticEnum<ENetRole>();

		if (!IsValid(NetRoleEnum))
		{
			return TEXT("Unknown");
		}

		const FString LocalRoleString = NetRoleEnum->GetNameStringByValue(static_cast<int64>(Actor->GetLocalRole()));
		const FString RemoteRoleString = NetRoleEnum->GetNameStringByValue(static_cast<int64>(Actor->GetRemoteRole()));

		// 출력 예: Authority / SimulatedProxy 또는 AutonomousProxy / Authority
		return FString::Printf(TEXT("%s / %s"), *LocalRoleString, *RemoteRoleString);
	}

	FString GetObjectNameString(const UObject* Object)
	{
		return GetNameSafe(Object);
	}

	FString GetActorNameString(const AActor* Actor)
	{
		return GetNameSafe(Actor);
	}
}
