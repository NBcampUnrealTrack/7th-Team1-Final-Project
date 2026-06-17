// Copyright 2026 One Team. All rights reserved.

#include "NSLocalCurrencyPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Progression/Currency/NSCurrencyVisualData.h"

ANSLocalCurrencyPickup::ANSLocalCurrencyPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	AActor::SetReplicateMovement(false);
	
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(CollisionRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(CollisionSphere);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetGenerateOverlapEvents(false);
}

void ANSLocalCurrencyPickup::Initialize(const FNSCurrencySpawnEvent& Event, const UNSCurrencyVisualData* VisualData)
{
	DropId = Event.DropId;
	CurrencyType = Event.CurrencyType;
	Grade = Event.Grade;
	
	SetActorLocation(Event.Location);
	CollisionSphere->SetSphereRadius(CollisionRadius);
	
	StartMeshLoad(VisualData);
	
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ANSLocalCurrencyPickup::OnSphereBeginOverlap);
	
	if (Event.Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(ExpireTimer, this, &ANSLocalCurrencyPickup::HandleExpire, Event.Duration, false);
	}
}

void ANSLocalCurrencyPickup::ConfirmCollected()
{
	Destroy();
}

void ANSLocalCurrencyPickup::RestoreVisual()
{
	bCollectRequested = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
}

void ANSLocalCurrencyPickup::StartMeshLoad(const UNSCurrencyVisualData* VisualData)
{
	if (!VisualData)
	{
		return;
	}
	const FNSCurrencyVisualRow* Row = VisualData->FindVisual(CurrencyType, Grade);
	if (!Row)
	{
		return;
	}
	
	SetActorScale3D(FVector(Row->Scale));
	
	if (UStaticMesh* Mesh = Row->Mesh.Get())
	{
		MeshComp->SetStaticMesh(Mesh);
		return;
	}
	
	PendingMeshPath = Row->Mesh.ToSoftObjectPath();
	if (PendingMeshPath.IsNull())
	{
		return;
	}
	
	MeshLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(PendingMeshPath,
		FStreamableDelegate::CreateUObject(this, &ANSLocalCurrencyPickup::OnMeshLoaded));
}

void ANSLocalCurrencyPickup::OnMeshLoaded()
{
	if (UStaticMesh* Mesh = Cast<UStaticMesh>(PendingMeshPath.ResolveObject()))
	{
		MeshComp->SetStaticMesh(Mesh);
	}
}

void ANSLocalCurrencyPickup::HandleExpire()
{
	Destroy();
}

void ANSLocalCurrencyPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	if (bCollectRequested)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	ANSPlayerState* PS = Pawn->GetPlayerState<ANSPlayerState>();
	if (!PS)
	{
		return;
	}

	bCollectRequested = true;

	// 즉각 UX -> 먼저 숨기고 서버 검증 요청
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	
	PS->Server_CollectCurrency(DropId);
}

void ANSLocalCurrencyPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MeshLoadHandle.IsValid())
	{
		MeshLoadHandle->CancelHandle();
		MeshLoadHandle.Reset();
	}
	GetWorldTimerManager().ClearTimer(ExpireTimer);
	Super::EndPlay(EndPlayReason);
}

