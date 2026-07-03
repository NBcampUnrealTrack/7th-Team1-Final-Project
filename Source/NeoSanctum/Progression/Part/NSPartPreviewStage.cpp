// Copyright 2026 One Team. All rights reserved.

#include "NSPartPreviewStage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/AssetManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "TimerManager.h"

ANSPartPreviewStage::ANSPartPreviewStage()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(false);

	// 회전하지 않는 고정 원점 — 메시/카메라/조명이 전부 이 밑에 형제로 붙어서
	// 메시만 회전해도 카메라가 같이 돌아가지 않는다
	StageRoot = CreateDefaultSubobject<USceneComponent>(TEXT("StageRoot"));
	SetRootComponent(StageRoot);

	PreviewMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewMeshComponent"));
	PreviewMeshComponent->SetupAttachment(StageRoot);
	PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// 점유율 계산을 무시하고 항상 풀 밉을 유지
	PreviewMeshComponent->bForceMipStreaming = true;

	CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
	CaptureComponent->SetupAttachment(StageRoot);
	CaptureComponent->ProjectionType = ECameraProjectionMode::Perspective;
	CaptureComponent->FOVAngle = 30.f;
	CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->ShowOnlyActors.Add(this);

	KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(StageRoot);
	KeyLight->SetRelativeLocation(FVector(-150.f, -150.f, 150.f));
	KeyLight->Intensity = 5000.f;
	KeyLight->AttenuationRadius = 500.f;

	RimLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RimLight"));
	RimLight->SetupAttachment(StageRoot);
	RimLight->SetRelativeLocation(FVector(150.f, 150.f, 100.f));
	RimLight->Intensity = 3000.f;
	RimLight->AttenuationRadius = 500.f;

	// 캡처가 월드 스카이박스(우주 배경)를 그대로 찍지 않도록 메시 뒤에 단색 배경판을 깐다.
	// 이 액터 소속이라 ShowOnlyActors에 자동 포함된다
	BackdropComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackdropComponent"));
	BackdropComponent->SetupAttachment(StageRoot);
	BackdropComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaneMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (PlaneMesh.Succeeded())
	{
		BackdropComponent->SetStaticMesh(PlaneMesh.Object);
	}
	if (PlaneMaterial.Succeeded())
	{
		BackdropComponent->SetMaterial(0, PlaneMaterial.Object);
	}
}

void ANSPartPreviewStage::BeginPlay()
{
	Super::BeginPlay();

	RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
		this, RenderTargetSize, RenderTargetSize, ETextureRenderTargetFormat::RTF_RGBA8);
	CaptureComponent->TextureTarget = RenderTarget;

	if (UMaterialInstanceDynamic* BackdropMID = BackdropComponent->CreateAndSetMaterialInstanceDynamic(0))
	{
		BackdropMID->SetVectorParameterValue(TEXT("Color"), BackdropColor);
	}
}

void ANSPartPreviewStage::SetPreviewMesh(USkeletalMesh* Mesh)
{
	PreviewMeshComponent->SetSkeletalMesh(Mesh);
	PreviewMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
	PreviewMeshComponent->SetRelativeLocation(FVector::ZeroVector);
	ZoomFactor = 1.f;

	if (!Mesh)
	{
		return;
	}

	FrameCameraOnMesh();
	CaptureComponent->CaptureScene();

	// 밉 로드는 비동기라 첫 캡처엔 저해상도 텍스처가 찍힐 수 있다.
	// 밉이 도착할 시간을 두고 재캡처해서 정상 텍스처로 갱신한다
	TWeakObjectPtr<ANSPartPreviewStage> WeakThis(this);
	GetWorldTimerManager().SetTimer(
		RecaptureTimerHandle,
		[WeakThis]()
		{
			if (ANSPartPreviewStage* Stage = WeakThis.Get())
			{
				Stage->CaptureComponent->CaptureScene();
			}
		},
		0.3f,
		false);
}

void ANSPartPreviewStage::AddManualYaw(float DeltaYawDegrees)
{
	if (!PreviewMeshComponent->GetSkeletalMeshAsset())
	{
		return;
	}

	PreviewMeshComponent->AddLocalRotation(FRotator(0.f, DeltaYawDegrees, 0.f));
	CaptureComponent->CaptureScene();
}

void ANSPartPreviewStage::AddZoom(float WheelDelta)
{
	if (!PreviewMeshComponent->GetSkeletalMeshAsset())
	{
		return;
	}

	ZoomFactor = FMath::Clamp(ZoomFactor * (1.f - WheelDelta * ZoomStep), MinZoomFactor, MaxZoomFactor);
	UpdateCameraLocation();
	CaptureComponent->CaptureScene();
}

void ANSPartPreviewStage::WarmupAllPartMeshes(UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(WorldContextObject);
	if (!DataSS)
	{
		return;
	}

	// 파괴하지 않고 레벨 수명 동안 유지 — 핸들/상주 컴포넌트가 살아있어야 밉이 계속 유지된다
	const FTransform SpawnTransform(FVector(0.f, 0.f, -1000.f));
	ANSPartPreviewStage* WarmupStage = World->SpawnActor<ANSPartPreviewStage>(ANSPartPreviewStage::StaticClass(), SpawnTransform);
	if (!WarmupStage)
	{
		return;
	}

	for (const auto& Pair : DataSS->GetAllPartRows())
	{
		const FNSPartDefinitionRow& Row = Pair.Value;
		if (!Row.bEnabled || Row.Definition.IsNull())
		{
			continue;
		}

		TWeakObjectPtr<ANSPartPreviewStage> WeakStage(WarmupStage);
		TSoftObjectPtr<UNSPartDefinition> SoftDef = Row.Definition;
		TSharedPtr<FStreamableHandle> DefHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
			SoftDef.ToSoftObjectPath(),
			[WeakStage, SoftDef]()
			{
				ANSPartPreviewStage* Stage = WeakStage.Get();
				UNSPartDefinition* Def = SoftDef.Get();
				if (!Stage || !Def || Def->PartMesh.IsNull())
				{
					return;
				}

				TSoftObjectPtr<USkeletalMesh> SoftMesh = Def->PartMesh;
				TSharedPtr<FStreamableHandle> MeshHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
					SoftMesh.ToSoftObjectPath(),
					[WeakStage, SoftMesh]()
					{
						if (ANSPartPreviewStage* InnerStage = WeakStage.Get())
						{
							InnerStage->AddWarmupMeshComponent(SoftMesh.Get());
						}
					});
				Stage->WarmupLoadHandles.Add(MeshHandle);
			});
		WarmupStage->WarmupLoadHandles.Add(DefHandle);
	}
}

void ANSPartPreviewStage::AddWarmupMeshComponent(USkeletalMesh* Mesh)
{
	if (!Mesh)
	{
		return;
	}

	USkeletalMeshComponent* WarmComp = NewObject<USkeletalMeshComponent>(this);
	WarmComp->SetupAttachment(StageRoot);
	WarmComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 메시별 전용 상주 컴포넌트 — 살아있는 동안 이 메시의 텍스처가 항상 풀 밉으로 유지된다
	WarmComp->bForceMipStreaming = true;
	WarmComp->SetSkeletalMesh(Mesh);
	WarmComp->RegisterComponent();

	WarmupMeshComponents.Add(WarmComp);
}

namespace
{
	// 메시 원점(피벗)이 아니라 실제 형상 중심을 바라보는 카메라 방향 (중심 → 카메라)
	const FVector PreviewCamDirection = FVector(-1.f, -0.6f, 0.35f).GetSafeNormal();
}

void ANSPartPreviewStage::FrameCameraOnMesh()
{
	const FBoxSphereBounds Bounds = PreviewMeshComponent->CalcBounds(PreviewMeshComponent->GetComponentTransform());
	const float Radius = FMath::Max(Bounds.SphereRadius, 10.f);

	FramedOrigin = Bounds.Origin;
	FramedDistance = Radius * CameraDistanceMultiplier;
	UpdateCameraLocation();

	// 배경판: 메시가 회전해도 닿지 않게 뒤로 물리고, 최대 줌아웃 시에도 화면을 다 덮게 스케일
	const float BackdropOffset = Radius * 3.f;
	const float MaxCamToBackdrop = FramedDistance * MaxZoomFactor + BackdropOffset;
	const float HalfSize = MaxCamToBackdrop * FMath::Tan(FMath::DegreesToRadians(CaptureComponent->FOVAngle * 0.5f)) * 1.5f;

	BackdropComponent->SetWorldLocation(FramedOrigin - PreviewCamDirection * BackdropOffset);
	// 엔진 Plane 메시는 +Z가 법선 — 법선이 카메라를 향하게 회전
	BackdropComponent->SetWorldRotation(FRotationMatrix::MakeFromZ(PreviewCamDirection).Rotator());
	BackdropComponent->SetWorldScale3D(FVector(HalfSize * 2.f / 100.f));
}

void ANSPartPreviewStage::UpdateCameraLocation()
{
	const FVector CamWorldLocation = FramedOrigin + PreviewCamDirection * (FramedDistance * ZoomFactor);

	CaptureComponent->SetWorldLocation(CamWorldLocation);
	CaptureComponent->SetWorldRotation((FramedOrigin - CamWorldLocation).Rotation());
}
