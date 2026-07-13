// Copyright 2026 One Team. All rights reserved.

#include "NSDroppedPart.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSVFXSubsystem.h"
#include "NeoSanctum/System/Subsystem/NSDroppedPartRegistrySubsystem.h"
#include "TimerManager.h"

ANSDroppedPart::ANSDroppedPart()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	
	// 서버가 갱신한 포물선 위치를 클라이언트에도 복제
	SetReplicateMovement(true);
	
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(SceneRoot);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	DetectionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionCollision"));
	DetectionCollision->SetupAttachment(SceneRoot);
	DetectionCollision->SetSphereRadius(InteractRadius);
	DetectionCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	PromptAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("PromptAnchor"));
	PromptAnchor->SetupAttachment(SceneRoot);
	PromptAnchor->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
}

void ANSDroppedPart::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터에서 InteractRadius를 바꾸면 스피어 반경도 즉시 반영
	if (DetectionCollision)
	{
		DetectionCollision->SetSphereRadius(InteractRadius);
	}
}

bool ANSDroppedPart::CanInteract_Implementation(APlayerController* Interactor) const
{
	return StoredInstance.IsValid();
}

bool ANSDroppedPart::OnInteract_Implementation(APlayerController* Interactor)
{
	if (!Interactor)
	{
		return false;
	}

	APlayerState* PS = Interactor->PlayerState;
	if (!PS)
	{
		return false;
	}

	UNSPartEquipComponent* EquipComp = PS->FindComponentByClass<UNSPartEquipComponent>();
	if (!EquipComp)
	{
		return false;
	}

	// 클라에서 실행되므로 Server RPC 경유 —> 실제 줍기는 서버 TryPickup
	EquipComp->Server_RequestPickup(this);
	return true;
}

FText ANSDroppedPart::GetPromptText_Implementation() const
{
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, StoredInstance);
	if (IsValid(Def))
	{
		return Def->PartName;
	}
	// Definition 미로드 시 기존 문구로 폴백
	return PromptText;
}

TSoftObjectPtr<UTexture2D> ANSDroppedPart::GetPromptIcon_Implementation() const
{
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, StoredInstance);
	if (IsValid(Def))
	{
		return Def->Icon;
	}
	return nullptr;
}

int32 ANSDroppedPart::GetPromptRarityIndex_Implementation() const
{
	return static_cast<int32>(StoredInstance.CurrentRarity);
}

FVector ANSDroppedPart::GetPromptWorldLocation_Implementation() const
{
	if (PromptAnchor)
	{
		return PromptAnchor->GetComponentLocation();
	}
	return GetActorLocation();
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

	// 겹침 방지용 로직, 서버 전용
	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UNSDroppedPartRegistrySubsystem* Registry = World->GetSubsystem<UNSDroppedPartRegistrySubsystem>())
			{
				Registry->RegisterDrop(this);
			}
		}
	}
}

void ANSDroppedPart::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsLaunching)
	{
		UpdateDropLaunch();
	}
	else
	{
		UpdateBobAnimation(DeltaSeconds);
	}
}

ANSDroppedPart* ANSDroppedPart::SpawnInWorld(UWorld* World, TSubclassOf<ANSDroppedPart> Class,
                                             const FNSPartData& Part, const FVector& Location)
{
	if (!World)
	{
		return nullptr;
	}

	if (!Class)
	{
		Class = ANSDroppedPart::StaticClass();
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, Location);
	ANSDroppedPart* Dropped = World->SpawnActorDeferred<ANSDroppedPart>(
		Class, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Dropped)
	{
		return nullptr;
	}

	Dropped->Initialize(Part);
	Dropped->FinishSpawning(SpawnTransform);
	return Dropped;
}

void ANSDroppedPart::StartDropLaunch(const FNSDropLaunchData& InLaunchData)
{
	if (!HasAuthority() || !InLaunchData.IsValid())
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	LaunchData = InLaunchData;
	LaunchStartWorldTime = World->GetTimeSeconds();
	bIsLaunching = true;
	
	SetActorLocation(LaunchData.StartLocation);
	SetActorTickEnabled(true);
	ForceNetUpdate();
}

void ANSDroppedPart::UpdateDropLaunch()
{
	if (!HasAuthority() || !bIsLaunching)
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		FinishDropLaunch();
		return;
	}
	
	const float ElapsedTime = FMath::Max(0.0f, World->GetTimeSeconds() - LaunchStartWorldTime);
	
	const float Alpha = FMath::Clamp(ElapsedTime / LaunchData.FlightDuration, 0.0f, 1.0f);
	
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

void ANSDroppedPart::FinishDropLaunch()
{
	bIsLaunching = false;

	SetActorLocation(LaunchData.TargetLocation);
	// 착지 후에도 바운싱 애니메이션을 위해 틱은 계속 유지
	ForceNetUpdate();
}

ANSDroppedPart* ANSDroppedPart::SpawnInWorld(
	UWorld* World,
	TSubclassOf<ANSDroppedPart> Class,
	const FNSPartData& Part,
	const FNSDropLaunchData& InLaunchData)
{
	if (!World || !InLaunchData.IsValid())
	{
		return nullptr;
	}
	
	if (!Class)
	{
		Class = ANSDroppedPart::StaticClass();
	}
	
	const FTransform SpawnTransform(FRotator::ZeroRotator, InLaunchData.StartLocation);
	
	ANSDroppedPart* Dropped = World->SpawnActorDeferred<ANSDroppedPart>(
		Class,
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);
	
	if (!Dropped)
	{
		return nullptr;
	}
	
	Dropped->Initialize(Part);
	Dropped->FinishSpawning(SpawnTransform);
	Dropped->StartDropLaunch(InLaunchData);
	
	return Dropped;
}

void ANSDroppedPart::Initialize(const FNSPartData& InPart)
{
	if (!HasAuthority())
	{
		return;
	}
	StoredInstance = InPart;

	if (const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		if (const FNSDroppedPartConfigRow* ConfigRow = DataSubsystem->GetDroppedPartConfigRow())
		{
			DespawnDuration = ConfigRow->DespawnDuration;
			BobAmplitude = ConfigRow->BobAmplitude;
			BobSpeed = ConfigRow->BobSpeed;
			RingVFXID = ConfigRow->RingVFXID;
		}
	}

	SetupVisual();

	GetWorldTimerManager().SetTimer(
		DespawnTimerHandle, this, &ANSDroppedPart::HandleDespawnTimerExpired, DespawnDuration, false);
}

void ANSDroppedPart::HandleDespawnTimerExpired()
{
	Destroy();
}

void ANSDroppedPart::TryPickup(APawn* InstigatorPawn)
{
	if (!HasAuthority())
	{
		return;
	}
	if (bIsLaunching)
	{
		return;
	}
	if (!InstigatorPawn)
	{
		return;
	}
	// 서버 재검증(변조 방어) —> 플레이어가 실제로 감지 스피어에 겹쳐 있는지 확인
	if (!DetectionCollision || !DetectionCollision->IsOverlappingActor(InstigatorPawn))
	{
		return;
	}
	
	APlayerController* PC = Cast<APlayerController>(InstigatorPawn->GetController());
	if (!PC)
	{
		return;
	}
	if (!INSInteractable::Execute_CanInteract(this, PC))
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

	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UNSDroppedPartRegistrySubsystem* Registry = World->GetSubsystem<UNSDroppedPartRegistrySubsystem>())
			{
				Registry->UnregisterDrop(this);
			}
		}
		GetWorldTimerManager().ClearTimer(DespawnTimerHandle);
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
	
	// 바운싱 위해서 Z보정값을 저장
	MeshBaseRelativeZ = -BottomOffsetZ;
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, MeshBaseRelativeZ));

	// 위치 보정
	const FVector MeshCenter(MeshBounds.Origin.X, MeshBounds.Origin.Y, MeshBounds.BoxExtent.Z);
	if (DetectionCollision)
	{
		DetectionCollision->SetRelativeLocation(MeshCenter);
	}
	if (PromptAnchor)
	{
		PromptAnchor->SetRelativeLocation(MeshCenter + FVector(0.f, 0.f, MeshBounds.BoxExtent.Z + 30.f));
	}

	// 착지 후 바운싱 애니메이션을 위해 틱 유지 (발사 중이면 이미 켜져 있음)
	SetActorTickEnabled(true);

	const UWorld* World = GetWorld();
	
	// 다른쪽 패턴 따라서 데디케이트 블락코드
	const bool bCanPlayVFX = World && World->GetNetMode() != NM_DedicatedServer;
	if (!bRingVFXPlayed && !RingVFXID.IsNone() && bCanPlayVFX)
	{
		if (UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(this))
		{
			VFXSubsystem->PlayVFXAttached(RingVFXID, MeshComp, NAME_None, MeshBounds.Origin);
		}
		bRingVFXPlayed = true;
	}
}

void ANSDroppedPart::UpdateBobAnimation(float DeltaSeconds)
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

