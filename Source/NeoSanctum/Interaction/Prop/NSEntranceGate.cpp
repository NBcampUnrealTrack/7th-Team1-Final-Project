// Copyright 2026 One Team. All rights reserved.

#include "NSEntranceGate.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInterface.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"

ANSEntranceGate::ANSEntranceGate()
{
	PrimaryActorTick.bCanEverTick = false;
	// 서버에 1개만 존재하지만 상태는 복제하지 않는다(외형=로컬, 통과=폰별 처리).
	bReplicates = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
	GateMesh->SetupAttachment(Root);

	// 기본 상태 = 잠김. 플레이어 캡슐만 막고 나머지는 무시(발사체/적 이동 방해 X).
	// 캡슐은 WorldStatic을 기본 Block 하므로, 게이트는 WorldStatic 오브젝트로 두고 Player 채널을 Block.
	GateMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GateMesh->SetCollisionObjectType(ECC_WorldStatic);
	GateMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	GateMesh->SetCollisionResponseToChannel(NSCollisionChannels::Player, ECR_Block);
}

void ANSEntranceGate::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 플레이어 진행도 기준으로 초기 외형 결정(미준비면 Lock 폴백).
	// 이후 변경은 UNSGateAccessComponent의 OnProgressChanged 푸시로 갱신됨.
	SetLocalUnlockVisual(IsUnlockedForLocalPlayer());
}

void ANSEntranceGate::SetLocalUnlockVisual(bool bUnlocked)
{
	ApplyMaterial(bUnlocked ? UnlockMaterial : LockMaterial);
}

void ANSEntranceGate::ApplyMaterial(UMaterialInterface* Material)
{
	if (!GateMesh || !Material)
	{
		return;
	}

	const int32 NumMaterials = GateMesh->GetNumMaterials();
	for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
	{
		GateMesh->SetMaterial(SlotIndex, Material);
	}
}

bool ANSEntranceGate::IsUnlockedForLocalPlayer() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (!GameInstance)
	{
		return false;
	}

	const APlayerController* LocalPC = GameInstance->GetFirstLocalPlayerController(GetWorld());
	if (!LocalPC || !LocalPC->PlayerState)
	{
		return false;
	}

	const UNSPlayerProgressComponent* Progress =
		LocalPC->PlayerState->FindComponentByClass<UNSPlayerProgressComponent>();
	return Progress ? Progress->IsNPCUnlocked(RequiredNPCId) : false;
}
