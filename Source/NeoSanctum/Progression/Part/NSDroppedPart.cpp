// Copyright 2026 One Team. All rights reserved.

#include "NSDroppedPart.h"

#include "Components/SphereComponent.h"
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
	
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(150.f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(CollisionSphere);
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
	
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ANSDroppedPart::OnSphereBeginOverlap);
	CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &ANSDroppedPart::OnSphereEndOverlap);
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
	
	EquipComp->EquipPart(StoredInstance);
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
	
	USkeletalMesh* Mesh = Def->DropMesh.Get();
	if (!Mesh)
	{
		const FSoftObjectPath MeshPath = Def->DropMesh.ToSoftObjectPath();
		if (MeshPath.IsNull())
		{
			return;
		}
		VisualLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MeshPath,
			FStreamableDelegate::CreateUObject(this, &ANSDroppedPart::SetupVisual));
		return;
	}
	
	MeshComp->SetSkeletalMesh(Mesh);
}

void ANSDroppedPart::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}
	OnPickupRangeEntered(Pawn);
}

void ANSDroppedPart::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}
	OnPickupRangeExited(Pawn);
}
