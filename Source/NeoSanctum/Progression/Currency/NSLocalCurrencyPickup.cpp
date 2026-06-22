// Copyright 2026 One Team. All rights reserved.

#include "NSLocalCurrencyPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Progression/Currency/NSCurrencyVisualData.h"

ANSLocalCurrencyPickup::ANSLocalCurrencyPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false;
	AActor::SetReplicateMovement(false);
	
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(CollisionRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(NSCollisionChannels::Player, ECR_Overlap);

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
	
	if (Event.LaunchData.IsValid())
	{
		StartDropLaunch(Event.LaunchData);
	}
	
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
	
	if (!bIsLaunching)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

void ANSLocalCurrencyPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	UpdateDropLaunch();
}

void ANSLocalCurrencyPickup::StartDropLaunch(const FNSDropLaunchData& InLaunchData)
{
	LaunchData = InLaunchData;
	
	// 늦게 접속한 클라이언트도 서버 시작 시간을 기준으로 현재 궤적 위치를 계산
	const float ElapsedTime = FMath::Max(0.0f, GetServerWorldTimeSeconds() - LaunchData.StartServerTime);
	
	if (ElapsedTime >= LaunchData.FlightDuration)
	{
		SetActorLocation(LaunchData.TargetLocation);
		FinishDropLaunch();
		return;
	}
	
	bIsLaunching = true;
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorLocation(LaunchData.StartLocation);
	SetActorTickEnabled(true);
	
	UpdateDropLaunch();
}

void ANSLocalCurrencyPickup::UpdateDropLaunch()
{
	if (!bIsLaunching)
	{
		return;
	}
	
	const float ElapsedTime = FMath::Max(0.0f, GetServerWorldTimeSeconds() - LaunchData.StartServerTime);
	
	const float Alpha = FMath::Clamp(
		ElapsedTime / LaunchData.FlightDuration,
		0.0f,
		1.0f
	);
	
	const FVector StartLocation = LaunchData.StartLocation;
	const FVector TargetLocation = LaunchData.TargetLocation;
	
	FVector CurrentLocation = FMath::Lerp(StartLocation, TargetLocation, Alpha);
	
	CurrentLocation.Z += 4.0f * LaunchData.ArcHeight * Alpha * (1.0f - Alpha);
	
	SetActorLocation(CurrentLocation);
	
	if (Alpha >= 1.0f)
	{
		FinishDropLaunch();
	}
}

void ANSLocalCurrencyPickup::FinishDropLaunch()
{
	bIsLaunching = false;
	SetActorLocation(LaunchData.TargetLocation);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetActorTickEnabled(false);
}

float ANSLocalCurrencyPickup::GetServerWorldTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}
	
	const AGameStateBase* GameState = World->GetGameState();
	return GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
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
	UStaticMesh* Mesh = Cast<UStaticMesh>(PendingMeshPath.ResolveObject());
	if (Mesh)
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
	if (bIsLaunching || bCollectRequested)
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

