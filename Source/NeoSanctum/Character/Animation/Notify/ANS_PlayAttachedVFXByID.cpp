// Copyright 2026 One Team. All rights reserved.

#include "ANS_PlayAttachedVFXByID.h"

#include "NiagaraComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSVFXSubsystem.h"

void UANS_PlayAttachedVFXByID::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!IsValid(MeshComp) || VFXID.IsNone())
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FName ResolvedSocketName = NAME_None;
	USceneComponent* AttachComponent = FindAttachComponent(MeshComp, ResolvedSocketName);
	if (!IsValid(AttachComponent))
	{
		return;
	}

	UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(MeshComp);
	if (!IsValid(VFXSubsystem))
	{
		return;
	}

	// 같은 Mesh에서 중복 실행 중인 VFX 정리
	StopVFX(MeshComp);

	UNiagaraComponent* NiagaraComponent = VFXSubsystem->SpawnVFXAttached(
		VFXID,
		AttachComponent,
		ResolvedSocketName,
		LocationOffset,
		RotationOffset,
		ScaleMultiplier,
		true);

	if (IsValid(NiagaraComponent))
	{
		ActiveVFXComponents.Add(MeshComp, NiagaraComponent);
	}
}

void UANS_PlayAttachedVFXByID::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	StopVFX(MeshComp);
}

FString UANS_PlayAttachedVFXByID::GetNotifyName_Implementation() const
{
	if (VFXID.IsNone())
	{
		return TEXT("NS Play Attached VFX By ID");
	}

	return FString::Printf(TEXT("Attached VFX: %s"), *VFXID.ToString());
}

USceneComponent* UANS_PlayAttachedVFXByID::FindAttachComponent(
	USkeletalMeshComponent* MeshComp,
	FName& OutSocketName) const
{
	OutSocketName = NAME_None;

	if (!IsValid(MeshComp))
	{
		return nullptr;
	}

	// Notify가 발생한 MeshComp를 우선적으로 탐색함
	if (bSearchNotifyMesh)
	{
		if (CheckComponent(MeshComp, true, OutSocketName) ||
			CheckComponent(MeshComp, false, OutSocketName))
		{
			return MeshComp;
		}
	}

	AActor* Owner = MeshComp->GetOwner();

	// Owner Actor의 컴포넌트 순회하며 탐색
	if (bSearchOwnerComponents)
	{
		if (USceneComponent* OwnerComponent = FindAttachComponentInActor(Owner, true, OutSocketName))
		{
			return OwnerComponent;
		}
		if (USceneComponent* OwnerComponent = FindAttachComponentInActor(Owner, false, OutSocketName))
		{
			return OwnerComponent;
		}
	}

	// Owner에 Attach된 Actor들의 컴포넌트를 탐색
	if (bSearchAttachedActors && IsValid(Owner))
	{
		TArray<AActor*> AttachedActors;
		Owner->GetAttachedActors(AttachedActors);

		for (AActor* AttachedActor : AttachedActors)
		{
			if (USceneComponent* AttachedComponent =
				FindAttachComponentInActor(AttachedActor, true, OutSocketName))
			{
				return AttachedComponent;
			}
		}

		for (AActor* AttachedActor : AttachedActors)
		{
			if (USceneComponent* AttachedComponent =
				FindAttachComponentInActor(AttachedActor, false, OutSocketName))
			{
				return AttachedComponent;
			}
		}
	}

	// 전부 실패하면 Notify Mesh에 붙히는걸로 fallback
	return bFallbackToNotifyMesh ? MeshComp : nullptr;
}

USceneComponent* UANS_PlayAttachedVFXByID::FindAttachComponentInActor(
	AActor* Actor,
	const bool bRequirePreferredComponent,
	FName& OutSocketName) const
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	TArray<USceneComponent*> SceneComponents;
	Actor->GetComponents<USceneComponent>(SceneComponents);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (CheckComponent(SceneComponent, bRequirePreferredComponent, OutSocketName))
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

bool UANS_PlayAttachedVFXByID::CheckComponent(
	USceneComponent* SceneComponent,
	const bool bRequirePreferredComponent,
	FName& OutSocketName) const
{
	if (!IsValid(SceneComponent))
	{
		return false;
	}

	// PreferredComponentName 지정 시 해당 이름이 있다면 가장 우선적으로 선택
	if (bRequirePreferredComponent &&
		(PreferredComponentName.IsNone() || SceneComponent->GetFName() != PreferredComponentName))
	{
		return false;
	}

	// SocketName 지정 시 해당 소켓 보유 여부 확인
	if (!SocketName.IsNone())
	{
		if (!SceneComponent->DoesSocketExist(SocketName))
		{
			return false;
		}

		OutSocketName = SocketName;
		return true;
	}

	OutSocketName = NAME_None;
	return true;
}

void UANS_PlayAttachedVFXByID::StopVFX(USkeletalMeshComponent* MeshComp)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	TWeakObjectPtr<UNiagaraComponent>* FoundComponent = ActiveVFXComponents.Find(MeshComp);
	if (!FoundComponent)
	{
		return;
	}

	if (UNiagaraComponent* NiagaraComponent = FoundComponent->Get())
	{
		// NotifyState 종료 시 VFX 비활성화
		NiagaraComponent->Deactivate();
	}

	ActiveVFXComponents.Remove(MeshComp);
}
