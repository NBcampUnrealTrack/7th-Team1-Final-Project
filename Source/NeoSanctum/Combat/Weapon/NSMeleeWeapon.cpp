// Copyright 2026 One Team. All rights reserved.


#include "NSMeleeWeapon.h"

#include "Components/SceneComponent.h"

bool ANSMeleeWeapon::TryGetMeleeTraceSocketTransforms(TArray<FTransform>& OutTransforms) const
{
	OutTransforms.Reset();

	if (MeleeTraceSocketNames.IsEmpty())
	{
		return false;
	}

	OutTransforms.Reserve(MeleeTraceSocketNames.Num());

	for (const FName& SocketName : MeleeTraceSocketNames)
	{
		if (SocketName.IsNone())
		{
			continue;
		}

		FTransform SocketTransform;
		if (!TryGetMeleeTraceSocketTransform(SocketName, SocketTransform))
		{
			OutTransforms.Reset();
			return false;
		}

		OutTransforms.Add(SocketTransform);
	}

	return !OutTransforms.IsEmpty();
}

bool ANSMeleeWeapon::TryGetMeleeTraceSocketTransform(
	FName SocketName,
	FTransform& OutTransform) const
{
	if (SocketName.IsNone())
	{
		return false;
	}

	TArray<USceneComponent*> SceneComponents;
	GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (!IsValid(SceneComponent))
		{
			continue;
		}

		if (!MeleeTraceComponentName.IsNone() &&
			SceneComponent->GetFName() != MeleeTraceComponentName)
		{
			continue;
		}

		if (!SceneComponent->DoesSocketExist(SocketName))
		{
			continue;
		}

		OutTransform = SceneComponent->GetSocketTransform(SocketName, RTS_World);
		return true;
	}

	return false;
}
