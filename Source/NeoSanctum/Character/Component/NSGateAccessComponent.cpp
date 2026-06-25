// Copyright 2026 One Team. All rights reserved.

#include "NSGateAccessComponent.h"

#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Interaction/Prop/NSEntranceGate.h"

UNSGateAccessComponent::UNSGateAccessComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSGateAccessComponent::InitializeGateAccess()
{
	UNSPlayerProgressComponent* Progress = GetProgressComponent();
	if (!Progress)
	{
		// PlayerState가 아직 없으면 다른 훅(PossessedBy/OnRep_PlayerState)에서 재시도됨
		return;
	}

	// 진행도 동기화/변경 시 갱신. 대상이 바뀐 경우에만 재바인딩.
	if (BoundProgress.Get() != Progress)
	{
		if (BoundProgress.IsValid() && ProgressChangedHandle.IsValid())
		{
			BoundProgress->OnProgressChanged.Remove(ProgressChangedHandle);
		}
		ProgressChangedHandle =
			Progress->OnProgressChanged.AddUObject(this, &UNSGateAccessComponent::RefreshAllGates);
		BoundProgress = Progress;
	}

	RefreshAllGates();
}

void UNSGateAccessComponent::RefreshAllGates()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	// 통과 처리는 권위(서버) 또는 소유 클라(예측)에서만 의미가 있음. 시뮬레이트 프록시는 제외.
	const bool bLocallyControlled = OwnerPawn->IsLocallyControlled();
	if (!OwnerPawn->HasAuthority() && !bLocallyControlled)
	{
		return;
	}

	const UNSPlayerProgressComponent* Progress = GetProgressComponent();
	if (!Progress)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UPrimitiveComponent* Capsule = nullptr;
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerPawn))
	{
		Capsule = OwnerCharacter->GetCapsuleComponent();
	}

	for (TActorIterator<ANSEntranceGate> It(World); It; ++It)
	{
		ANSEntranceGate* Gate = *It;
		if (!Gate)
		{
			continue;
		}

		const bool bUnlocked = Progress->IsNPCUnlocked(Gate->GetRequiredNPCId());

		// 해금한 폰만 이 문을 무시(통과). 서버=권위 폰, 소유 클라=로컬 폰.
		if (Capsule)
		{
			Capsule->IgnoreActorWhenMoving(Gate, bUnlocked);
		}

		// 외형은 로컬 시점에서만 (각 클라가 자기 진행도 색을 봄)
		if (bLocallyControlled)
		{
			Gate->SetLocalUnlockVisual(bUnlocked);
		}
	}
}

UNSPlayerProgressComponent* UNSGateAccessComponent::GetProgressComponent() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return nullptr;
	}

	const APlayerState* PS = OwnerPawn->GetPlayerState();
	if (!PS)
	{
		return nullptr;
	}

	return PS->FindComponentByClass<UNSPlayerProgressComponent>();
}
