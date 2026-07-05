// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/System/Minimap/NSMinimapCaptureActor.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "DungeonGeneratorBase.h"
#include "DungeonGraph.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavigationSystem.h"
#include "NeoSanctum/Debug/Logging/NSLogCategories.h"
#include "NeoSanctum/System/Minimap/NSMinimapSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Room.h"

ANSMinimapCaptureActor::ANSMinimapCaptureActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
	SetRootComponent(SceneCaptureComponent);

	//상단 직교 캡처 기본 설정
	SceneCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCaptureComponent->CaptureSource = CaptureSource;
	SceneCaptureComponent->bCaptureEveryFrame = false;
	SceneCaptureComponent->bCaptureOnMovement = false;
	SceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	SceneCaptureComponent->MaxViewDistanceOverride = 0.0f;
	SceneCaptureComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	SceneCaptureComponent->bAutoCalculateOrthoPlanes = true;
	SceneCaptureComponent->bUpdateOrthoPlanes = true;
	SceneCaptureComponent->bUseCameraHeightAsViewTarget = true;
	ConfigureMinimapShowFlags();
}

void ANSMinimapCaptureActor::BeginPlay()
{
	Super::BeginPlay();

	//던전 생성기 바인딩 준비
	if (bAutoFindDungeonGenerator && !DungeonGeneratorActor)
	{
		TryAutoBindDungeonGenerator();
	}
	else if (DungeonGeneratorActor)
	{
		BindToDungeonGenerator(DungeonGeneratorActor);
	}
}

void ANSMinimapCaptureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//예약 작업 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CaptureTimerHandle);
		World->GetTimerManager().ClearTimer(PreparedCaptureTimerHandle);
		World->GetTimerManager().ClearTimer(RestoreVisibilityTimerHandle);
		World->GetTimerManager().ClearTimer(RoomReadyCheckTimerHandle);
	}

	//캡처 중 변경한 상태 복구
	RestoreRoomActorVisibilityAfterCapture();
	UnbindRoomLoadEvents();

	if (ADungeonGeneratorBase* DungeonGenerator = Cast<ADungeonGeneratorBase>(DungeonGeneratorActor))
	{
		DungeonGenerator->OnPostGenerationEvent.RemoveDynamic(this, &ThisClass::HandleDungeonPostGeneration);
	}

	Super::EndPlay(EndPlayReason);
}

void ANSMinimapCaptureActor::BindToDungeonGenerator(AActor* NewDungeonGeneratorActor)
{
	// 기존 Dungeon Generator 이벤트 해제
	if (ADungeonGeneratorBase* PreviousDungeonGenerator = Cast<ADungeonGeneratorBase>(DungeonGeneratorActor))
	{
		PreviousDungeonGenerator->OnPostGenerationEvent.RemoveDynamic(this, &ThisClass::HandleDungeonPostGeneration);
	}

	DungeonGeneratorActor = NewDungeonGeneratorActor;

	ADungeonGeneratorBase* DungeonGenerator = Cast<ADungeonGeneratorBase>(DungeonGeneratorActor);
	if (!DungeonGenerator)
	{
		UE_LOG(LogNS, Warning, TEXT("미니맵 캡처 바인딩 실패: 액터가 DungeonGenerator가 아님. 액터=%s"), *GetNameSafe(NewDungeonGeneratorActor));
		return;
	}

	//던전 생성 완료 이벤트 구독
	DungeonGenerator->OnPostGenerationEvent.RemoveDynamic(this, &ThisClass::HandleDungeonPostGeneration);
	DungeonGenerator->OnPostGenerationEvent.AddDynamic(this, &ThisClass::HandleDungeonPostGeneration);
}

void ANSMinimapCaptureActor::CaptureFromCurrentActorTransform(float ManualOrthoWidth)
{
	EnsureRenderTarget();
	EnsureLayerRenderTargets();
	if (!RenderTarget || !SceneCaptureComponent)
	{
		return;
	}

	const float SafeOrthoWidth = FMath::Max(ManualOrthoWidth, 100.0f);
	SceneCaptureComponent->TextureTarget = RenderTarget;
	SceneCaptureComponent->OrthoWidth = SafeOrthoWidth;
	SceneCaptureComponent->CaptureSource = CaptureSource;
	ConfigureMinimapShowFlags();
	SceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	SceneCaptureComponent->ClearShowOnlyComponents();
	SceneCaptureComponent->HiddenActors.Reset();
	SceneCaptureComponent->CaptureScene();

	//수동 캡처 범위 계산
	const FVector Center = GetActorLocation();
	const float HalfWidth = SafeOrthoWidth * 0.5f;
	const FBox CaptureBounds(
		FVector(Center.X - HalfWidth, Center.Y - HalfWidth, Center.Z - CaptureHeight),
		FVector(Center.X + HalfWidth, Center.Y + HalfWidth, Center.Z));

	if (UNSMinimapSubsystem* MinimapSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UNSMinimapSubsystem>() : nullptr)
	{
		MinimapSubsystem->SetMinimap(RenderTarget, CaptureBounds);
	}

}

void ANSMinimapCaptureActor::CaptureDungeonMinimap()
{
	FBox DungeonBounds(ForceInit);
	if (!BuildDungeonWorldBounds(DungeonBounds))
	{
		UE_LOG(LogNS, Warning, TEXT("미니맵 캡처 건너뜀: 던전 경계가 유효하지 않음. 캡처=%s 생성기=%s"),
			*GetNameSafe(this),
			*GetNameSafe(DungeonGeneratorActor));
		return;
	}

	EnsureRenderTarget();
	if (!RenderTarget || !SceneCaptureComponent)
	{
		return;
	}

	DungeonBounds = DungeonBounds.ExpandBy(FVector(BoundsPadding, BoundsPadding, 0.0f));

	//던전 전체를 포함하는 정사각형 캡처 범위 계산
	FVector BoundsCenter = DungeonBounds.GetCenter();
	FVector BoundsSize = DungeonBounds.GetSize();
	float OrthoWidth = FMath::Max(BoundsSize.X, BoundsSize.Y);
	if (bDebugCaptureAroundPlayer)
	{
		if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			const FVector PlayerLocation = PlayerPawn->GetActorLocation();
			BoundsCenter.X = PlayerLocation.X;
			BoundsCenter.Y = PlayerLocation.Y;
			OrthoWidth = DebugPlayerOrthoWidth;
			BoundsSize.X = DebugPlayerOrthoWidth;
			BoundsSize.Y = DebugPlayerOrthoWidth;
		}
	}

	//캡처 카메라 위치와 범위 설정
	SetActorLocation(FVector(BoundsCenter.X, BoundsCenter.Y, BoundsCenter.Z + CaptureHeight));
	SceneCaptureComponent->SetWorldRotation(FRotator(-90.0f, 0.0f, 0.0f));
	SceneCaptureComponent->OrthoWidth = OrthoWidth;
	SceneCaptureComponent->TextureTarget = RenderTarget;
	SceneCaptureComponent->CaptureSource = CaptureSource;
	ConfigureMinimapShowFlags();

	if (bForceRoomsVisibleForCapture)
	{
		SetDungeonRoomsForceVisible(true);
	}

	if (!bUseLayeredCapture)
	{
		PrepareRoomActorVisibilityForCapture();
	}

	//지연 캡처에 사용할 상태 보관
	const FVector2D OrthoHalfSize(OrthoWidth * 0.5f, OrthoWidth * 0.5f);
	PendingCaptureBounds = FBox(
		FVector(BoundsCenter.X - OrthoHalfSize.X, BoundsCenter.Y - OrthoHalfSize.Y, DungeonBounds.Min.Z),
		FVector(BoundsCenter.X + OrthoHalfSize.X, BoundsCenter.Y + OrthoHalfSize.Y, DungeonBounds.Max.Z));
	PendingDungeonBounds = DungeonBounds;
	PendingOrthoWidth = OrthoWidth;
	bHasPreparedCapture = true;

	//가시성 변경이 렌더 스레드에 반영된 뒤 캡처 실행
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreparedCaptureTimerHandle);
		if (CaptureVisibilitySettleDelay <= 0.0f)
		{
			ExecutePreparedCapture();
		}
		else
		{
			World->GetTimerManager().SetTimer(
				PreparedCaptureTimerHandle,
				this,
				&ThisClass::ExecutePreparedCapture,
				CaptureVisibilitySettleDelay,
				false);
		}
	}
}

void ANSMinimapCaptureActor::SetDungeonRoomsForceVisible(bool bForceVisible) const
{
	//스트리밍 룸 표시 상태 강제 적용
	const ADungeonGeneratorBase* DungeonGenerator = Cast<ADungeonGeneratorBase>(DungeonGeneratorActor);
	const UDungeonGraph* DungeonGraph = DungeonGenerator ? DungeonGenerator->GetRooms() : nullptr;
	if (!DungeonGraph)
	{
		return;
	}

	for (URoom* Room : DungeonGraph->GetAllRooms())
	{
		if (Room)
		{
			Room->ForceVisibility(bForceVisible);
		}
	}
}

void ANSMinimapCaptureActor::ExecutePreparedCapture()
{
	if (!bHasPreparedCapture || !SceneCaptureComponent || !RenderTarget)
	{
		RestoreRoomActorVisibilityAfterCapture();
		return;
	}

	if (bCaptureEveryFrameDuringVisibilityWindow)
	{
		//가시성 안정화 구간 동안 지속 캡처
		SceneCaptureComponent->bCaptureEveryFrame = true;
		SceneCaptureComponent->bCaptureOnMovement = true;
	}

	//설정에 따른 캡처 방식 선택
	if (bUseLayeredCapture && !CaptureLayers.IsEmpty())
	{
		if (bUseNavMeshTextureCapture)
		{
			CapturePreparedNavMeshLayers();
		}
		else
		{
			CapturePreparedLayers();
		}
	}
	else
	{
		SceneCaptureComponent->CaptureScene();

		if (UNSMinimapSubsystem* MinimapSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UNSMinimapSubsystem>() : nullptr)
		{
			MinimapSubsystem->SetMinimap(RenderTarget, PendingCaptureBounds);
		}

		//디버그 파일 출력
		DumpRenderTargetToFile();
	}

	//캡처 완료 후 가시성 복구 예약
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RestoreVisibilityTimerHandle);
		if (CaptureVisibilityRestoreDelay <= 0.0f)
		{
			FinishPreparedCapture();
		}
		else
		{
			World->GetTimerManager().SetTimer(
				RestoreVisibilityTimerHandle,
				this,
				&ThisClass::FinishPreparedCapture,
				CaptureVisibilityRestoreDelay,
				false);
		}
	}
}

void ANSMinimapCaptureActor::FinishPreparedCapture()
{
	if (SceneCaptureComponent)
	{
		//마지막 캡처 후 지속 캡처 비활성화
		SceneCaptureComponent->CaptureScene();
		SceneCaptureComponent->bCaptureEveryFrame = false;
		SceneCaptureComponent->bCaptureOnMovement = false;
	}

	if (bForceRoomsVisibleForCapture)
	{
		SetDungeonRoomsForceVisible(false);
	}
	RestoreRoomActorVisibilityAfterCapture();

	bHasPreparedCapture = false;
	PendingCaptureBounds = FBox(ForceInit);
	PendingDungeonBounds = FBox(ForceInit);
	PendingOrthoWidth = 0.0f;
}

void ANSMinimapCaptureActor::PrepareRoomActorVisibilityForCapture()
{
	RestoreRoomActorVisibilityAfterCapture();

	//씬 캡처 표시 목록 초기화
	if (SceneCaptureComponent)
	{
		SceneCaptureComponent->ClearShowOnlyComponents();
		SceneCaptureComponent->HiddenActors.Reset();
		SceneCaptureComponent->PrimitiveRenderMode = bUseShowOnlyActorsWithTag
			? ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList
			: ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	}

	const ADungeonGeneratorBase* DungeonGenerator = Cast<ADungeonGeneratorBase>(DungeonGeneratorActor);
	const UDungeonGraph* DungeonGraph = DungeonGenerator ? DungeonGenerator->GetRooms() : nullptr;
	if (!DungeonGraph)
	{
		return;
	}

	for (URoom* Room : DungeonGraph->GetAllRooms())
	{
		if (!Room || !Room->Instance)
		{
			continue;
		}

		ULevel* LoadedLevel = Room->Instance->GetLoadedLevel();
		if (!LoadedLevel)
		{
			continue;
		}

		for (AActor* Actor : LoadedLevel->Actors)
		{
			if (!IsValid(Actor) || Actor == this)
			{
				continue;
			}

			const bool bHiddenByTag = HiddenActorTags.ContainsByPredicate([Actor](const FName& HiddenTag)
			{
				return !HiddenTag.IsNone() && Actor->ActorHasTag(HiddenTag);
			});
			const bool bShowOnlyActor = !ShowOnlyActorTag.IsNone() && Actor->ActorHasTag(ShowOnlyActorTag);

			if (!ActorHiddenStateBeforeCapture.Contains(Actor))
			{
				//원래 숨김 상태 보관
				ActorHiddenStateBeforeCapture.Add(Actor, Actor->IsHidden());
			}

			if (bHiddenByTag)
			{
				Actor->SetActorHiddenInGame(true);
				continue;
			}

			if (bUseShowOnlyActorsWithTag)
			{
				if (bShowOnlyActor && SceneCaptureComponent)
				{
					Actor->SetActorHiddenInGame(false);
					SceneCaptureComponent->ShowOnlyActorComponents(Actor, true);
				}
				continue;
			}

			Actor->SetActorHiddenInGame(false);
		}
	}
}

void ANSMinimapCaptureActor::RestoreRoomActorVisibilityAfterCapture()
{
	//캡처 전 숨김 상태 복구
	for (const TPair<TWeakObjectPtr<AActor>, bool>& Pair : ActorHiddenStateBeforeCapture)
	{
		if (AActor* Actor = Pair.Key.Get())
		{
			Actor->SetActorHiddenInGame(Pair.Value);
		}
	}

	ActorHiddenStateBeforeCapture.Reset();

	if (SceneCaptureComponent)
	{
		SceneCaptureComponent->ClearShowOnlyComponents();
		SceneCaptureComponent->HiddenActors.Reset();
		SceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	}
}

void ANSMinimapCaptureActor::HandleDungeonPostGeneration()
{
	BeginWaitingForDungeonRooms();
}

void ANSMinimapCaptureActor::TryAutoBindDungeonGenerator()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ADungeonGeneratorBase> It(World); It; ++It)
	{
		BindToDungeonGenerator(*It);
		return;
	}

	UE_LOG(LogNS, Warning, TEXT("미니맵 캡처 실패: 현재 월드에서 DungeonGenerator를 찾지 못함. 캡처=%s"), *GetNameSafe(this));
}

void ANSMinimapCaptureActor::BeginWaitingForDungeonRooms()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(CaptureTimerHandle);
	World->GetTimerManager().ClearTimer(RoomReadyCheckTimerHandle);
	UnbindRoomLoadEvents();

	RoomReadyWaitStartTime = World->GetTimeSeconds();
	StableReadyCheckCount = 0;
	BindRoomLoadEvents();

	CheckDungeonRoomsReady();
	if (!World->GetTimerManager().IsTimerActive(CaptureTimerHandle))
	{
		World->GetTimerManager().SetTimer(
			RoomReadyCheckTimerHandle,
			this,
			&ThisClass::CheckDungeonRoomsReady,
			RoomReadyCheckInterval,
			true);
	}
}

void ANSMinimapCaptureActor::CheckDungeonRoomsReady()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	BindRoomLoadEvents();

	int32 ReadyRoomCount = 0;
	int32 TotalRoomCount = 0;
	const bool bRoomsReady = AreDungeonRoomsReady(ReadyRoomCount, TotalRoomCount);
	const int32 RenderablePrimitiveCount = CountRenderableRoomPrimitiveComponents();
	const bool bRenderableReady = RenderablePrimitiveCount > 0;
	FBox DungeonBounds(ForceInit);
	const bool bHasDungeonBounds = BuildDungeonWorldBounds(DungeonBounds);
	FVector PawnLocation = FVector::ZeroVector;
	const bool bLocalPlayerReady = !bWaitForLocalPlayerPawnInDungeon || (bHasDungeonBounds && IsLocalPlayerReadyForCapture(DungeonBounds, PawnLocation));
	const bool bStableReady = bRoomsReady && bRenderableReady && bLocalPlayerReady;
	const double ElapsedTime = World->GetTimeSeconds() - RoomReadyWaitStartTime;

	if (bStableReady)
	{
		++StableReadyCheckCount;
	}
	else
	{
		StableReadyCheckCount = 0;
	}

	const bool bEnoughStableChecks = StableReadyCheckCount >= FMath::Max(RequiredStableReadyCheckCount, 1);
	if (!bEnoughStableChecks && (MaxRoomReadyWaitTime <= 0.0f || ElapsedTime < MaxRoomReadyWaitTime))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(RoomReadyCheckTimerHandle);
	UnbindRoomLoadEvents();

	if (!bEnoughStableChecks)
	{
		UE_LOG(LogNS, Warning, TEXT("미니맵 캡처 진행: 준비 대기 시간 초과. 캡처=%s 준비된룸=%d 전체룸=%d 렌더가능컴포넌트=%d 로컬플레이어준비=%s 폰위치=%s 안정확인=%d/%d 대기시간=%.2f"),
			*GetNameSafe(this),
			ReadyRoomCount,
			TotalRoomCount,
			RenderablePrimitiveCount,
			bLocalPlayerReady ? TEXT("true") : TEXT("false"),
			*PawnLocation.ToString(),
			StableReadyCheckCount,
			RequiredStableReadyCheckCount,
			ElapsedTime);
	}
	if (CaptureDelay <= 0.0f)
	{
		CaptureDungeonMinimap();
	}
	else
	{
		World->GetTimerManager().SetTimer(
			CaptureTimerHandle,
			this,
			&ThisClass::ExecuteScheduledCapture,
			CaptureDelay,
			false);
	}
}

void ANSMinimapCaptureActor::ExecuteScheduledCapture()
{
	CaptureDungeonMinimap();
}

void ANSMinimapCaptureActor::HandleRoomLevelLoaded()
{
	CheckDungeonRoomsReady();
}

void ANSMinimapCaptureActor::BindRoomLoadEvents()
{
	const ADungeonGeneratorBase* DungeonGenerator = Cast<ADungeonGeneratorBase>(DungeonGeneratorActor);
	const UDungeonGraph* DungeonGraph = DungeonGenerator ? DungeonGenerator->GetRooms() : nullptr;
	if (!DungeonGraph)
	{
		return;
	}

	for (URoom* Room : DungeonGraph->GetAllRooms())
	{
		if (!Room || !Room->Instance || BoundRoomInstances.Contains(Room->Instance))
		{
			continue;
		}

		Room->Instance->OnLevelLoaded.RemoveDynamic(this, &ThisClass::HandleRoomLevelLoaded);
		Room->Instance->OnLevelLoaded.AddDynamic(this, &ThisClass::HandleRoomLevelLoaded);
		BoundRoomInstances.Add(Room->Instance);
	}
}

void ANSMinimapCaptureActor::UnbindRoomLoadEvents()
{
	for (ULevelStreamingDynamic* RoomInstance : BoundRoomInstances)
	{
		if (RoomInstance)
		{
			RoomInstance->OnLevelLoaded.RemoveDynamic(this, &ThisClass::HandleRoomLevelLoaded);
		}
	}

	BoundRoomInstances.Reset();
}

bool ANSMinimapCaptureActor::AreDungeonRoomsReady(int32& OutReadyRoomCount, int32& OutTotalRoomCount) const
{
	OutReadyRoomCount = 0;
	OutTotalRoomCount = 0;

	const ADungeonGeneratorBase* DungeonGenerator = Cast<ADungeonGeneratorBase>(DungeonGeneratorActor);
	const UDungeonGraph* DungeonGraph = DungeonGenerator ? DungeonGenerator->GetRooms() : nullptr;
	if (!DungeonGraph)
	{
		return false;
	}

	const TArray<URoom*>& Rooms = DungeonGraph->GetAllRooms();
	OutTotalRoomCount = Rooms.Num();
	if (Rooms.IsEmpty())
	{
		return false;
	}

	for (const URoom* Room : Rooms)
	{
		if (!Room || !Room->Instance || !Room->IsInstanceInitialized())
		{
			continue;
		}

		++OutReadyRoomCount;
	}

	return OutReadyRoomCount == OutTotalRoomCount;
}

int32 ANSMinimapCaptureActor::CountRenderableRoomPrimitiveComponents() const
{
	const ADungeonGeneratorBase* DungeonGenerator = Cast<ADungeonGeneratorBase>(DungeonGeneratorActor);
	const UDungeonGraph* DungeonGraph = DungeonGenerator ? DungeonGenerator->GetRooms() : nullptr;
	if (!DungeonGraph)
	{
		return 0;
	}

	int32 RenderablePrimitiveCount = 0;
	for (const URoom* Room : DungeonGraph->GetAllRooms())
	{
		if (!Room || !Room->Instance)
		{
			continue;
		}

		const ULevel* LoadedLevel = Room->Instance->GetLoadedLevel();
		if (!LoadedLevel)
		{
			continue;
		}

		for (AActor* Actor : LoadedLevel->Actors)
		{
			if (!IsValid(Actor))
			{
				continue;
			}

			TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
			Actor->GetComponents(PrimitiveComponents);
			for (const UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
			{
				if (!PrimitiveComponent || !PrimitiveComponent->IsRegistered() || !PrimitiveComponent->IsRenderStateCreated())
				{
					continue;
				}

				++RenderablePrimitiveCount;
			}
		}
	}

	return RenderablePrimitiveCount;
}

bool ANSMinimapCaptureActor::IsLocalPlayerReadyForCapture(const FBox& DungeonBounds, FVector& OutPawnLocation) const
{
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn || PlayerPawn->IsPendingKillPending())
	{
		return false;
	}

	OutPawnLocation = PlayerPawn->GetActorLocation();
	const FBox ExpandedBounds = DungeonBounds.ExpandBy(FVector(BoundsPadding, BoundsPadding, CaptureHeight));
	return ExpandedBounds.IsInsideOrOn(OutPawnLocation);
}

bool ANSMinimapCaptureActor::BuildDungeonWorldBounds(FBox& OutWorldBounds) const
{
	const ADungeonGeneratorBase* DungeonGenerator = Cast<ADungeonGeneratorBase>(DungeonGeneratorActor);
	if (!DungeonGenerator)
	{
		return false;
	}

	const UDungeonGraph* DungeonGraph = DungeonGenerator->GetRooms();
	if (!DungeonGraph)
	{
		return false;
	}

	const TArray<URoom*>& Rooms = DungeonGraph->GetAllRooms();
	for (const URoom* Room : Rooms)
	{
		if (!Room)
		{
			continue;
		}

		bool bUsedActorBounds = false;
		if (Room->Instance)
		{
			const ULevel* LoadedLevel = Room->Instance->GetLoadedLevel();
			if (LoadedLevel)
			{
				for (AActor* Actor : LoadedLevel->Actors)
				{
					if (!IsValid(Actor) || Actor == this)
					{
						continue;
					}

					const bool bHiddenByTag = HiddenActorTags.ContainsByPredicate([Actor](const FName& HiddenTag)
					{
						return !HiddenTag.IsNone() && Actor->ActorHasTag(HiddenTag);
					});
					if (bHiddenByTag)
					{
						continue;
					}

					TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
					Actor->GetComponents(PrimitiveComponents);
					bool bHasRenderablePrimitive = false;
					for (const UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
					{
						if (PrimitiveComponent && PrimitiveComponent->IsRegistered() && PrimitiveComponent->IsRenderStateCreated())
						{
							bHasRenderablePrimitive = true;
							break;
						}
					}

					if (!bHasRenderablePrimitive)
					{
						continue;
					}

					const FBox ActorBounds = Actor->GetComponentsBoundingBox(true, true);
					if (!ActorBounds.IsValid || ActorBounds.GetSize().IsNearlyZero())
					{
						continue;
					}

					OutWorldBounds += ActorBounds;
					bUsedActorBounds = true;
				}
			}
		}

		if (bUsedActorBounds)
		{
			continue;
		}

		const FVector RoomBoundsCenter = Room->GetBoundsCenter();
		const FVector RoomBoundsExtent = Room->GetBoundsExtent();
		if (RoomBoundsExtent.IsNearlyZero())
		{
			continue;
		}

		OutWorldBounds += FBox::BuildAABB(RoomBoundsCenter, RoomBoundsExtent);
	}

	return OutWorldBounds.IsValid != 0;
}

void ANSMinimapCaptureActor::EnsureRenderTarget()
{
	const int32 ClampedRenderTargetSize = FMath::Clamp(RenderTargetSize, 128, 4096);
	if (RenderTarget && RenderTarget->SizeX == ClampedRenderTargetSize && RenderTarget->SizeY == ClampedRenderTargetSize)
	{
		return;
	}

	//단일 캡처 렌더 타겟 생성
	RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("MinimapRenderTarget"));
	RenderTarget->ClearColor = ClearColor;
	RenderTarget->InitCustomFormat(ClampedRenderTargetSize, ClampedRenderTargetSize, PF_B8G8R8A8, false);
	RenderTarget->UpdateResourceImmediate(true);
}

void ANSMinimapCaptureActor::EnsureLayerRenderTargets()
{
	if (!bUseLayeredCapture)
	{
		return;
	}

	const int32 ClampedRenderTargetSize = FMath::Clamp(RenderTargetSize, 128, 4096);
	LayerRenderTargets.SetNum(CaptureLayers.Num());

	for (int32 LayerArrayIndex = 0; LayerArrayIndex < CaptureLayers.Num(); ++LayerArrayIndex)
	{
		UTextureRenderTarget2D* ExistingRenderTarget = LayerRenderTargets[LayerArrayIndex];
		if (ExistingRenderTarget && ExistingRenderTarget->SizeX == ClampedRenderTargetSize && ExistingRenderTarget->SizeY == ClampedRenderTargetSize)
		{
			continue;
		}

		//층별 캡처 렌더 타겟 생성
		const FName RenderTargetName = *FString::Printf(TEXT("MinimapLayer_%d"), CaptureLayers[LayerArrayIndex].LayerIndex);
		UTextureRenderTarget2D* NewRenderTarget = NewObject<UTextureRenderTarget2D>(this, RenderTargetName);
		NewRenderTarget->ClearColor = ClearColor;
		NewRenderTarget->InitCustomFormat(ClampedRenderTargetSize, ClampedRenderTargetSize, PF_B8G8R8A8, false);
		NewRenderTarget->UpdateResourceImmediate(true);
		LayerRenderTargets[LayerArrayIndex] = NewRenderTarget;
	}
}

void ANSMinimapCaptureActor::ConfigureMinimapShowFlags() const
{
	if (!SceneCaptureComponent || !bDisableAtmosphereFogAndPostProcess)
	{
		return;
	}

	//미니맵 캡처에 불필요한 렌더링 요소 비활성화
	static const TCHAR* FlagsToDisable[] =
	{
		TEXT("Atmosphere"),
		TEXT("Fog"),
		TEXT("VolumetricFog"),
		TEXT("VolumetricCloud"),
		TEXT("Cloud"),
		TEXT("SkyLighting"),
		TEXT("PostProcessing"),
		TEXT("Bloom"),
		TEXT("EyeAdaptation"),
		TEXT("Tonemapper"),
		TEXT("MotionBlur"),
		TEXT("LensFlares"),
		TEXT("ScreenSpaceReflections"),
		TEXT("AmbientOcclusion"),
		TEXT("DepthOfField"),
		TEXT("Vignette")
	};

	for (const TCHAR* FlagName : FlagsToDisable)
	{
		const int32 FlagIndex = FEngineShowFlags::FindIndexByName(FlagName);
		if (FlagIndex != INDEX_NONE)
		{
			SceneCaptureComponent->ShowFlags.SetSingleFlag(static_cast<uint32>(FlagIndex), false);
		}
	}
}

void ANSMinimapCaptureActor::PrepareRoomActorVisibilityForLayer(const FNSMinimapCaptureLayerConfig& LayerConfig)
{
	if (SceneCaptureComponent)
	{
		//층별 캡처 표시 목록 초기화
		SceneCaptureComponent->ClearShowOnlyComponents();
		SceneCaptureComponent->HiddenActors.Reset();
		SceneCaptureComponent->PrimitiveRenderMode = bUseShowOnlyActorsWithTag
			? ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList
			: ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	}

	const ADungeonGeneratorBase* DungeonGenerator = Cast<ADungeonGeneratorBase>(DungeonGeneratorActor);
	const UDungeonGraph* DungeonGraph = DungeonGenerator ? DungeonGenerator->GetRooms() : nullptr;
	if (!DungeonGraph)
	{
		return;
	}

	const float LayerMinZ = FMath::Min(LayerConfig.FloorZ, LayerConfig.CeilingZ);
	const float LayerMaxZ = FMath::Max(LayerConfig.FloorZ, LayerConfig.CeilingZ);
	//액터 높이와 층 높이 범위 교차 여부 기준으로 표시
	for (URoom* Room : DungeonGraph->GetAllRooms())
	{
		if (!Room || !Room->Instance)
		{
			continue;
		}

		ULevel* LoadedLevel = Room->Instance->GetLoadedLevel();
		if (!LoadedLevel)
		{
			continue;
		}

		for (AActor* Actor : LoadedLevel->Actors)
		{
			if (!IsValid(Actor) || Actor == this)
			{
				continue;
			}

			if (!ActorHiddenStateBeforeCapture.Contains(Actor))
			{
				//원래 숨김 상태 보관
				ActorHiddenStateBeforeCapture.Add(Actor, Actor->IsHidden());
			}

			const bool bHiddenByTag = HiddenActorTags.ContainsByPredicate([Actor](const FName& HiddenTag)
			{
				return !HiddenTag.IsNone() && Actor->ActorHasTag(HiddenTag);
			});
			if (bHiddenByTag)
			{
				Actor->SetActorHiddenInGame(true);
				continue;
			}

			const FBox ActorBounds = Actor->GetComponentsBoundingBox(true, true);
			const bool bIntersectsLayer = ActorBounds.IsValid && ActorBounds.Max.Z >= LayerMinZ && ActorBounds.Min.Z <= LayerMaxZ;
			const bool bShowOnlyActor = !ShowOnlyActorTag.IsNone() && Actor->ActorHasTag(ShowOnlyActorTag);

			if (bUseShowOnlyActorsWithTag)
			{
				const bool bShouldShow = bIntersectsLayer && bShowOnlyActor;
				Actor->SetActorHiddenInGame(!bShouldShow);
				if (bShouldShow && SceneCaptureComponent)
				{
					SceneCaptureComponent->ShowOnlyActorComponents(Actor, true);
				}
				continue;
			}

			Actor->SetActorHiddenInGame(!bIntersectsLayer);
		}
	}
}

void ANSMinimapCaptureActor::CapturePreparedLayers()
{
	//렌더 타겟 기반 층별 캡처 준비
	EnsureLayerRenderTargets();
	if (!SceneCaptureComponent || LayerRenderTargets.Num() != CaptureLayers.Num())
	{
		return;
	}

	RestoreRoomActorVisibilityAfterCapture();

	TArray<FNSMinimapLayer> CapturedLayers;
	CapturedLayers.Reserve(CaptureLayers.Num());

	const FVector CaptureCenter = PendingCaptureBounds.GetCenter();
	//층 설정 순회 캡처
	for (int32 LayerArrayIndex = 0; LayerArrayIndex < CaptureLayers.Num(); ++LayerArrayIndex)
	{
		const FNSMinimapCaptureLayerConfig& LayerConfig = CaptureLayers[LayerArrayIndex];
		UTextureRenderTarget2D* LayerRenderTarget = LayerRenderTargets[LayerArrayIndex];
		if (!LayerRenderTarget)
		{
			continue;
		}

		const float LayerMinZ = FMath::Min(LayerConfig.FloorZ, LayerConfig.CeilingZ);
		const float LayerMaxZ = FMath::Max(LayerConfig.FloorZ, LayerConfig.CeilingZ);

		PrepareRoomActorVisibilityForLayer(LayerConfig);

		RenderTarget = LayerRenderTarget;
		SetActorLocation(FVector(CaptureCenter.X, CaptureCenter.Y, LayerMaxZ + CaptureHeight));
		SceneCaptureComponent->SetWorldRotation(FRotator(-90.0f, 0.0f, 0.0f));
		SceneCaptureComponent->OrthoWidth = PendingOrthoWidth;
		SceneCaptureComponent->TextureTarget = LayerRenderTarget;
		SceneCaptureComponent->CaptureSource = CaptureSource;
		ConfigureMinimapShowFlags();
		SceneCaptureComponent->CaptureScene();

		//캡처 결과 레이어 데이터 생성
		FNSMinimapLayer CapturedLayer;
		CapturedLayer.LayerIndex = LayerConfig.LayerIndex;
		CapturedLayer.Texture = LayerRenderTarget;
		CapturedLayer.WorldBoundsMin = FVector(PendingCaptureBounds.Min.X, PendingCaptureBounds.Min.Y, LayerMinZ);
		CapturedLayer.WorldBoundsMax = FVector(PendingCaptureBounds.Max.X, PendingCaptureBounds.Max.Y, LayerMaxZ);
		CapturedLayer.FloorZ = LayerMinZ;
		CapturedLayer.CeilingZ = LayerMaxZ;
		CapturedLayers.Add(CapturedLayer);

		DumpRenderTargetToFile();
	}

	if (UNSMinimapSubsystem* MinimapSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UNSMinimapSubsystem>() : nullptr)
	{
		//캡처 레이어 데이터 전달
		MinimapSubsystem->SetMinimapLayers(CapturedLayers);
	}
}

void ANSMinimapCaptureActor::CapturePreparedNavMeshLayers()
{
	TArray<FNSMinimapLayer> CapturedLayers;
	CapturedLayers.Reserve(CaptureLayers.Num());
	NavMeshLayerTextures.SetNum(CaptureLayers.Num());

	//NavMesh 텍스처 층별 생성
	for (int32 LayerArrayIndex = 0; LayerArrayIndex < CaptureLayers.Num(); ++LayerArrayIndex)
	{
		const FNSMinimapCaptureLayerConfig& LayerConfig = CaptureLayers[LayerArrayIndex];
		UTexture2D* LayerTexture = BuildNavMeshTextureForLayer(LayerConfig, PendingCaptureBounds);
		if (!LayerTexture)
		{
			continue;
		}

		NavMeshLayerTextures[LayerArrayIndex] = LayerTexture;

		const float LayerMinZ = FMath::Min(LayerConfig.FloorZ, LayerConfig.CeilingZ);
		const float LayerMaxZ = FMath::Max(LayerConfig.FloorZ, LayerConfig.CeilingZ);

		//생성된 텍스처 레이어 데이터 생성
		FNSMinimapLayer CapturedLayer;
		CapturedLayer.LayerIndex = LayerConfig.LayerIndex;
		CapturedLayer.Texture = LayerTexture;
		CapturedLayer.WorldBoundsMin = FVector(PendingCaptureBounds.Min.X, PendingCaptureBounds.Min.Y, LayerMinZ);
		CapturedLayer.WorldBoundsMax = FVector(PendingCaptureBounds.Max.X, PendingCaptureBounds.Max.Y, LayerMaxZ);
		CapturedLayer.FloorZ = LayerMinZ;
		CapturedLayer.CeilingZ = LayerMaxZ;
		CapturedLayers.Add(CapturedLayer);
	}

	if (UNSMinimapSubsystem* MinimapSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UNSMinimapSubsystem>() : nullptr)
	{
		//NavMesh 레이어 데이터 전달
		MinimapSubsystem->SetMinimapLayers(CapturedLayers);
	}

}

UTexture2D* ANSMinimapCaptureActor::BuildNavMeshTextureForLayer(const FNSMinimapCaptureLayerConfig& LayerConfig, const FBox& TextureWorldBounds)
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavigationSystem = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	ARecastNavMesh* RecastNavMesh = NavigationSystem ? Cast<ARecastNavMesh>(NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)) : nullptr;
	if (!RecastNavMesh)
	{
		UE_LOG(LogNS, Warning, TEXT("미니맵 NavMesh 텍스처 생성 실패: RecastNavMesh를 찾지 못함. 캡처=%s"), *GetNameSafe(this));
		return nullptr;
	}

	//Recast NavMesh 삼각형 데이터 수집
	FRecastDebugGeometry NavMeshGeometry;
	NavMeshGeometry.bGatherPolyEdges = false;
	NavMeshGeometry.bGatherNavMeshEdges = false;
	RecastNavMesh->GetDebugGeometryForTile(NavMeshGeometry, FNavTileRef());

	const int32 TextureSize = FMath::Clamp(RenderTargetSize, 128, 4096);
	const FColor FillColor = NavMeshFillColor.ToFColor(true);
	const FColor OutlineColor = NavMeshOutlineColor.ToFColor(true);
	const FColor BackgroundColor = NavMeshBackgroundColor.ToFColor(true);
	TArray<uint8> FillMask;
	FillMask.Init(0, TextureSize * TextureSize);

	const FVector BoundsSize = TextureWorldBounds.GetSize();
	if (FMath::IsNearlyZero(BoundsSize.X) || FMath::IsNearlyZero(BoundsSize.Y))
	{
		return nullptr;
	}

	const float LayerMinZ = FMath::Min(LayerConfig.FloorZ, LayerConfig.CeilingZ);
	const float LayerMaxZ = FMath::Max(LayerConfig.FloorZ, LayerConfig.CeilingZ);
	const TArray<FVector>& MeshVerts = NavMeshGeometry.MeshVerts;

	//월드 좌표를 텍스처 픽셀 좌표로 변환
	auto WorldToPixel = [&TextureWorldBounds, &BoundsSize, TextureSize](const FVector& WorldPosition)
	{
		const float U = (WorldPosition.X - TextureWorldBounds.Min.X) / BoundsSize.X;
		const float V = 1.0f - (WorldPosition.Y - TextureWorldBounds.Min.Y) / BoundsSize.Y;
		return FVector2D(U * static_cast<float>(TextureSize - 1), V * static_cast<float>(TextureSize - 1));
	};

	auto Edge = [](const FVector2D& A, const FVector2D& B, const FVector2D& P)
	{
		return (P.X - A.X) * (B.Y - A.Y) - (P.Y - A.Y) * (B.X - A.X);
	};

	int32 DrawnTriangleCount = 0;
	//층 높이에 포함되는 NavMesh 삼각형 래스터라이즈
	for (int32 AreaIndex = 0; AreaIndex < RECAST_MAX_AREAS; ++AreaIndex)
	{
		const TArray<int32>& MeshIndices = NavMeshGeometry.AreaIndices[AreaIndex];
		for (int32 Index = 0; Index + 2 < MeshIndices.Num(); Index += 3)
		{
			const int32 IndexA = MeshIndices[Index + 0];
			const int32 IndexB = MeshIndices[Index + 1];
			const int32 IndexC = MeshIndices[Index + 2];
			if (!MeshVerts.IsValidIndex(IndexA) || !MeshVerts.IsValidIndex(IndexB) || !MeshVerts.IsValidIndex(IndexC))
			{
				continue;
			}

			const FVector& A = MeshVerts[IndexA];
			const FVector& B = MeshVerts[IndexB];
			const FVector& C = MeshVerts[IndexC];
			const float TriangleMinZ = FMath::Min3(A.Z, B.Z, C.Z);
			const float TriangleMaxZ = FMath::Max3(A.Z, B.Z, C.Z);
			if (TriangleMaxZ < LayerMinZ || TriangleMinZ > LayerMaxZ)
			{
				continue;
			}

			const FVector2D PA = WorldToPixel(A);
			const FVector2D PB = WorldToPixel(B);
			const FVector2D PC = WorldToPixel(C);
			const int32 MinX = FMath::Clamp(FMath::FloorToInt(FMath::Min3(PA.X, PB.X, PC.X)), 0, TextureSize - 1);
			const int32 MaxX = FMath::Clamp(FMath::CeilToInt(FMath::Max3(PA.X, PB.X, PC.X)), 0, TextureSize - 1);
			const int32 MinY = FMath::Clamp(FMath::FloorToInt(FMath::Min3(PA.Y, PB.Y, PC.Y)), 0, TextureSize - 1);
			const int32 MaxY = FMath::Clamp(FMath::CeilToInt(FMath::Max3(PA.Y, PB.Y, PC.Y)), 0, TextureSize - 1);
			const float Area = Edge(PA, PB, PC);
			if (FMath::IsNearlyZero(Area))
			{
				continue;
			}

			for (int32 Y = MinY; Y <= MaxY; ++Y)
			{
				for (int32 X = MinX; X <= MaxX; ++X)
				{
					const FVector2D P(static_cast<float>(X) + 0.5f, static_cast<float>(Y) + 0.5f);
					const float W0 = Edge(PB, PC, P);
					const float W1 = Edge(PC, PA, P);
					const float W2 = Edge(PA, PB, P);
					const bool bInside = Area > 0.0f
						? (W0 >= 0.0f && W1 >= 0.0f && W2 >= 0.0f)
						: (W0 <= 0.0f && W1 <= 0.0f && W2 <= 0.0f);
					if (bInside)
					{
						FillMask[Y * TextureSize + X] = 1;
					}
				}
			}

			++DrawnTriangleCount;
		}
	}

	//마스크 확장 처리
	auto DilateMask = [TextureSize](const TArray<uint8>& SourceMask, int32 Radius)
	{
		if (Radius <= 0)
		{
			return SourceMask;
		}

		TArray<uint8> DilatedMask = SourceMask;
		for (int32 Y = 0; Y < TextureSize; ++Y)
		{
			for (int32 X = 0; X < TextureSize; ++X)
			{
				if (SourceMask[Y * TextureSize + X] != 0)
				{
					continue;
				}

				bool bNearFill = false;
				for (int32 OffsetY = -Radius; OffsetY <= Radius && !bNearFill; ++OffsetY)
				{
					const int32 SampleY = Y + OffsetY;
					if (SampleY < 0 || SampleY >= TextureSize)
					{
						continue;
					}

					for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
					{
						const int32 SampleX = X + OffsetX;
						if (SampleX < 0 || SampleX >= TextureSize)
						{
							continue;
						}

						if (SourceMask[SampleY * TextureSize + SampleX] != 0)
						{
							bNearFill = true;
							break;
						}
					}
				}

				if (bNearFill)
				{
					DilatedMask[Y * TextureSize + X] = 1;
				}
			}
		}

		return DilatedMask;
	};

	const int32 FillExpansionPixels = FMath::Clamp(NavMeshFillExpansionPixels, 0, 16);
	const int32 OutlineThicknessPixels = FMath::Clamp(NavMeshOutlineThicknessPixels, 0, 32);
	//채움 영역과 외곽선 마스크 생성
	const TArray<uint8> ExpandedFillMask = DilateMask(FillMask, FillExpansionPixels);
	const TArray<uint8> OutlineMask = DilateMask(ExpandedFillMask, OutlineThicknessPixels);

	TArray<FColor> Pixels;
	Pixels.Init(BackgroundColor, TextureSize * TextureSize);
	int32 FillPixelCount = 0;
	int32 OutlinePixelCount = 0;
	//최종 픽셀 색상 구성
	for (int32 PixelIndex = 0; PixelIndex < Pixels.Num(); ++PixelIndex)
	{
		if (ExpandedFillMask[PixelIndex] != 0)
		{
			Pixels[PixelIndex] = FillColor;
			++FillPixelCount;
		}
		else if (OutlineMask[PixelIndex] != 0)
		{
			Pixels[PixelIndex] = OutlineColor;
			++OutlinePixelCount;
		}
	}

	if (FillPixelCount <= 0 && OutlinePixelCount <= 0)
	{
		UE_LOG(LogNS, Warning, TEXT("미니맵 NavMesh 층 텍스처 건너뜀: 채워진 픽셀이 없음. 캡처=%s 층=%d 삼각형=%d 하단Z=%.1f 상단Z=%.1f"),
			*GetNameSafe(this),
			LayerConfig.LayerIndex,
			DrawnTriangleCount,
			LayerMinZ,
			LayerMaxZ);
		return nullptr;
	}

	//UI 표시용 런타임 텍스처 생성
	UTexture2D* Texture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8);
	if (!Texture)
	{
		return nullptr;
	}

	Texture->SRGB = true;
	Texture->NeverStream = true;
	Texture->CompressionSettings = TC_EditorIcon;
	Texture->MipGenSettings = TMGS_NoMipmaps;
	Texture->LODGroup = TEXTUREGROUP_UI;
	Texture->Filter = TF_Bilinear;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;
	void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();

	DumpTextureToFile(Texture, FString::Printf(TEXT("Layer_%d"), LayerConfig.LayerIndex));

	return Texture;
}

void ANSMinimapCaptureActor::DumpRenderTargetToFile()
{
	if (!bDumpRenderTargetToFile || !RenderTarget)
	{
		return;
	}

	//덤프 파일 경로 구성
	const FString SafeDirectory = DebugDumpDirectory.IsEmpty() ? TEXT("MinimapCaptures") : DebugDumpDirectory;
	const FString DumpPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), SafeDirectory));
	const FString DumpName = FString::Printf(TEXT("Minimap_%s_%s_%s_%lld.png"),
		*GetNameSafe(this),
		*GetNameSafe(RenderTarget),
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")),
		FDateTime::Now().GetTicks());
	const FString DumpFilePath = FPaths::Combine(DumpPath, DumpName);

	IFileManager::Get().MakeDirectory(*DumpPath, true);

	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource)
	{
		UE_LOG(LogNS, Warning, TEXT("미니맵 렌더 타겟 덤프 실패: 렌더 타겟 리소스가 없음. 렌더타겟=%s"),
			*GetNameSafe(RenderTarget));
		return;
	}

	//렌더 타겟 픽셀 읽기
	TArray<FColor> SurfaceData;
	FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
	ReadFlags.SetLinearToGamma(true);
	if (!RenderTargetResource->ReadPixels(SurfaceData, ReadFlags) || SurfaceData.IsEmpty())
	{
		UE_LOG(LogNS, Warning, TEXT("미니맵 렌더 타겟 덤프 실패: 픽셀 읽기 실패. 렌더타겟=%s 크기=%dx%d"),
			*GetNameSafe(RenderTarget),
			RenderTarget->SizeX,
			RenderTarget->SizeY);
		return;
	}

	for (FColor& Pixel : SurfaceData)
	{
		Pixel.A = 255;
	}

	//PNG 데이터 압축
	TArray64<uint8> PngData;
	FImageUtils::PNGCompressImageArray(
		RenderTarget->SizeX,
		RenderTarget->SizeY,
		TArrayView64<const FColor>(SurfaceData.GetData(), SurfaceData.Num()),
		PngData);

	if (!FFileHelper::SaveArrayToFile(PngData, *DumpFilePath))
	{
		UE_LOG(LogNS, Warning, TEXT("미니맵 렌더 타겟 덤프 실패: PNG 저장 실패. 파일=%s"), *DumpFilePath);
		return;
	}

	UE_LOG(LogNS, Warning, TEXT("미니맵 렌더 타겟 PNG 덤프 완료. 경로=%s 파일=%s 렌더타겟=%s 크기=%dx%d 바이트=%lld"),
		*DumpPath,
		*DumpName,
		*GetNameSafe(RenderTarget),
		RenderTarget->SizeX,
		RenderTarget->SizeY,
		PngData.Num());
}

void ANSMinimapCaptureActor::DumpTextureToFile(UTexture2D* Texture, const FString& Label) const
{
	if (!bDumpRenderTargetToFile || !Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.IsEmpty())
	{
		return;
	}

	const FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	const int32 Width = Mip.SizeX;
	const int32 Height = Mip.SizeY;
	if (Width <= 0 || Height <= 0)
	{
		return;
	}

	//덤프 파일 경로 구성
	const FString SafeDirectory = DebugDumpDirectory.IsEmpty() ? TEXT("MinimapCaptures") : DebugDumpDirectory;
	const FString DumpPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), SafeDirectory));
	const FString DumpName = FString::Printf(TEXT("Minimap_%s_%s_%s_%lld.png"),
		*GetNameSafe(this),
		*Label,
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")),
		FDateTime::Now().GetTicks());
	const FString DumpFilePath = FPaths::Combine(DumpPath, DumpName);

	IFileManager::Get().MakeDirectory(*DumpPath, true);

	//텍스처 픽셀 읽기
	const void* SourceData = Mip.BulkData.LockReadOnly();
	if (!SourceData)
	{
		Mip.BulkData.Unlock();
		return;
	}

	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(Width * Height);
	FMemory::Memcpy(Pixels.GetData(), SourceData, Pixels.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();

	//PNG 데이터 압축
	TArray64<uint8> PngData;
	FImageUtils::PNGCompressImageArray(
		Width,
		Height,
		TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()),
		PngData);

	if (FFileHelper::SaveArrayToFile(PngData, *DumpFilePath))
	{
		UE_LOG(LogNS, Warning, TEXT("미니맵 텍스처 PNG 덤프 완료. 경로=%s 파일=%s 텍스처=%s 크기=%dx%d 바이트=%lld"),
			*DumpPath,
			*DumpName,
			*GetNameSafe(Texture),
			Width,
			Height,
			PngData.Num());
	}
}
