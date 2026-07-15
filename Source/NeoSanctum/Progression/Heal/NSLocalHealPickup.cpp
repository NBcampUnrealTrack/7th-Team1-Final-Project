// Copyright 2026 One Team. All rights reserved.


#include "NSLocalHealPickup.h"

#include "Components/SphereComponent.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"

ANSLocalHealPickup::ANSLocalHealPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	bReplicates = false;
	AActor::SetReplicateMovement(false);
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(CollisionRadius);
	
	// Player채널에만 반응하도록
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(NSCollisionChannels::Player, ECR_Overlap);
	
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(CollisionSphere);
	// 비주얼 전용이라 자체 충돌/오버랩은 필요 없음 —> 판정은 CollisionSphere가 담당
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetGenerateOverlapEvents(false);
	MeshComp->SetCanEverAffectNavigation(false);
}

void ANSLocalHealPickup::Initialize(const FNSHealSpawnEvent& Event, const UDataTable* HealPotionTable)
{
	DropId = Event.DropId;
	PotionTag = Event.PotionTag;
                                                                                                                                                
	SetActorLocation(Event.Location);
	CollisionSphere->SetSphereRadius(CollisionRadius);
	
	StartMeshLoad(HealPotionTable);
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ANSLocalHealPickup::OnSphereBeginOverlap);
	
	// 궤적 실행
	if (Event.LaunchData.IsValid())
	{
		StartDropLaunch(Event.LaunchData);
	} else
	{
		SetActorTickEnabled(true);
	}
	
	// 자동 사라지기
	if (Event.Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(ExpireTimer, this, &ANSLocalHealPickup::HandleExpire, Event.Duration, false);
	}
}

void ANSLocalHealPickup::ConfirmCollected()
{
	Destroy();
}

void ANSLocalHealPickup::RestoreVisual()
{
	bCollectRequested = false;
	SetActorHiddenInGame(false);
	
	// 발사중이 아닐 때만 충돌을 다시 켬
	if (!bIsLaunching)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

void ANSLocalHealPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	// 날아가는중 / 이미 수집요청 보냈으면 추가 오버랩 무시
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
	
	// 먼저 숨기고 충돌 꺼서 반응이 빠른것처럼 유도
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	PS->Server_CollectHeal(DropId);
}

void ANSLocalHealPickup::StartMeshLoad(const UDataTable* HealPotionTable)
{
	if (!HealPotionTable)
	{
		return;
	}

	// RowName 규약: 포션 태그의 전체 이름을 그대로 쓴다(예: "Reward.Potion.Heal.Large").
	static const FString Context(TEXT("ANSLocalHealPickup::StartMeshLoad"));
	const FNSHealPotionRow* Row = HealPotionTable->FindRow<FNSHealPotionRow>(
			PotionTag.GetTagName(), Context, false);

	if (!Row)
	{
		return;
	}
	
	SetActorScale3D(FVector(Row->Scale));

	// 이미 메모리에 로드되어 있다면(에디터 참조 등으로 상주 중인 경우) 즉시 적용하고 끝낸다.
	if (UStaticMesh* Mesh = Row->Mesh.Get())
	{
		MeshComp->SetStaticMesh(Mesh);
		
		const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
		MeshBaseRelativeZ = -(MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z);
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, MeshBaseRelativeZ));
		return;
	}

	PendingMeshPath = Row->Mesh.ToSoftObjectPath();
	if (PendingMeshPath.IsNull())
	{
		return;
	}

	// 동기 로드(LoadSynchronous)는 게임 스레드를 멈추게 하므로 금지 — 항상 비동기로 요청하고
	// 완료 콜백(OnMeshLoaded)에서 적용한다.
	MeshLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(PendingMeshPath,
			FStreamableDelegate::CreateUObject(this, &ANSLocalHealPickup::OnMeshLoaded));
}

void ANSLocalHealPickup::OnMeshLoaded()
{
	UStaticMesh* Mesh = Cast<UStaticMesh>(PendingMeshPath.ResolveObject());
	if (Mesh)
	{
		MeshComp->SetStaticMesh(Mesh);
		
		const FBoxSphereBounds MeshBounds = Mesh->GetBounds();                                                                                                                     
		MeshBaseRelativeZ = -(MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z);                                                                                                       
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, MeshBaseRelativeZ));
	}
}

void ANSLocalHealPickup::HandleExpire()
{
	Destroy();
}

void ANSLocalHealPickup::StartDropLaunch(const FNSDropLaunchData& InLaunchData)
{
	LaunchData = InLaunchData;

	const float ElapsedTime = FMath::Max(0.0f, GetServerWorldTimeSeconds() - LaunchData.StartServerTime);

	if (ElapsedTime >= LaunchData.FlightDuration)
	{
		SetActorLocation(LaunchData.TargetLocation);
		FinishDropLaunch();
		return;
	}

	bIsLaunching = true;
	// 날아가는 동안에는 주울 수 없어야 하므로 충돌을 꺼둠
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorLocation(LaunchData.StartLocation);
	SetActorTickEnabled(true);

	UpdateDropLaunch();
}

void ANSLocalHealPickup::UpdateDropLaunch()
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

void ANSLocalHealPickup::FinishDropLaunch()
{
	bIsLaunching = false;
	SetActorLocation(LaunchData.TargetLocation);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetActorTickEnabled(true);
}

float ANSLocalHealPickup::GetServerWorldTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}
	const AGameStateBase* GameState = World->GetGameState();
	return GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}


void ANSLocalHealPickup::BeginPlay()
{
	Super::BeginPlay();
}

void ANSLocalHealPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateDropLaunch();
	UpdateBobAnimation(DeltaTime);
}

void ANSLocalHealPickup::UpdateBobAnimation(float DeltaSeconds)
{
	if (!MeshComp)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// sin이 -1 ~ 1 자연스런 왕복곡선이라 사용
	const float BobOffsetZ = FMath::Sin(World->GetTimeSeconds() * BobSpeed) * BobAmplitude;
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, MeshBaseRelativeZ + BobOffsetZ));
}
