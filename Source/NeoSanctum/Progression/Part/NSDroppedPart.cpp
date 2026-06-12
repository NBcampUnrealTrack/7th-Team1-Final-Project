// Copyright 2026 One Team. All rights reserved.

#include "NSDroppedPart.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"

ANSDroppedPart::ANSDroppedPart()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(SceneRoot);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANSDroppedPart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANSDroppedPart, StoredInstance);
}

void ANSDroppedPart::BeginPlay()
{
	Super::BeginPlay();

	SetupVisual();
}

void ANSDroppedPart::Initialize(const FNSPartData& InPart)
{
	if (!HasAuthority())
	{
		return;
	}
	StoredInstance = InPart;
	SetupVisual();
}

void ANSDroppedPart::TryPickup(APawn* InstigatorPawn)
{
	if (!HasAuthority())
	{
		return;
	}
	if (!InstigatorPawn)
	{
		return;
	}
	ANSPlayerState* PS = InstigatorPawn->GetPlayerState<ANSPlayerState>();
	if (!PS)
	{
		return;
	}
	
	UNSPartEquipComponent* EquipComp = PS->GetPartEquipComponent();
	if (!EquipComp)
	{
		return;
	}
	
	// 주운 파츠 위치에 기존 장착 파츠를 드랍 — 제자리 교체
	EquipComp->EquipPart(StoredInstance, GetActorLocation());
	Destroy();
}

void ANSDroppedPart::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (VisualLoadHandle.IsValid())
	{
		VisualLoadHandle->CancelHandle();
		VisualLoadHandle.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void ANSDroppedPart::OnRep_StoredInstance()
{
	SetupVisual();
}

void ANSDroppedPart::SetupVisual()
{
	if (!StoredInstance.IsValid())
	{
		return;
	}
	
	UNSPartDefinition* Def = StoredInstance.DefinitionPtr.Get();
	if (!Def)
	{
		const FSoftObjectPath DefPath = StoredInstance.DefinitionPtr.ToSoftObjectPath();
		if (DefPath.IsNull())
		{
			return;
		}
		VisualLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(DefPath,
			FStreamableDelegate::CreateUObject(this, &ANSDroppedPart::SetupVisual));
		return;
	}
	
	USkeletalMesh* Mesh = Def->PartMesh.Get();
	if (!Mesh)
	{
		const FSoftObjectPath MeshPath = Def->PartMesh.ToSoftObjectPath();
		if (MeshPath.IsNull())
		{
			return;
		}
		VisualLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MeshPath,
			FStreamableDelegate::CreateUObject(this, &ANSDroppedPart::SetupVisual));
		return;
	}
	
	MeshComp->SetSkeletalMesh(Mesh);

	// 파츠 메시는 피벗이 본 위치 기준이라 땅에 닿게 Z 보정
	const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
	const float BottomOffsetZ = MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z;
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -BottomOffsetZ));
}
