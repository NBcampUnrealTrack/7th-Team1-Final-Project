// Copyright 2026 One Team. All rights reserved.

#include "PartIconTool.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "ContentBrowserMenuContexts.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/PackageName.h"
#include "PartIconToolSettings.h"
#include "PreviewScene.h"
#include "ShaderCompiler.h"
#include "ToolMenus.h"
#include "UObject/SavePackage.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "NeoSanctum/Data/Part/NSPartDefinition.h"

#define LOCTEXT_NAMESPACE "PartIconTool"

namespace
{
	// 파츠 Definition의 PartMesh 썸네일을 캡처해 텍스처 애셋으로 저장하고 Icon에 지정.
	// 성공 시 생성/갱신된 텍스처 반환, 실패 시 nullptr.
	UTexture2D* GeneratePartIcon(UNSPartDefinition* Def)
	{
		if (!IsValid(Def))
		{
			return nullptr;
		}

		USkeletalMesh* Mesh = Def->PartMesh.LoadSynchronous();
		if (!IsValid(Mesh))
		{
			UE_LOG(LogTemp, Warning, TEXT("[PartIconTool] %s: PartMesh가 비어 있어 건너뜀"), *Def->GetName());
			return nullptr;
		}

		// 메시 렌더 리소스가 준비되도록 스트리밍 완료 대기
		Mesh->WaitForPendingInitOrStreaming();
		FlushRenderingCommands();

		const UPartIconToolSettings* Settings = GetDefault<UPartIconToolSettings>();
		const int32 Width = Settings->IconResolution;
		const int32 Height = Settings->IconResolution;

		// 메시만 투명 배경에 렌더하기 위한 격리 프리뷰 씬
		FPreviewScene PreviewScene(FPreviewScene::ConstructionValues()
			.SetCreatePhysicsScene(false)
			.SetTransactional(false));

		USkeletalMeshComponent* PreviewMeshComp = NewObject<USkeletalMeshComponent>(GetTransientPackage());
		PreviewMeshComp->SetSkeletalMesh(Mesh);
		PreviewScene.AddComponent(PreviewMeshComp, FTransform::Identity);

		// 투명 클리어 컬러를 가진 렌더 타겟 (BGRA8)
		UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
		RenderTarget->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
		RenderTarget->InitCustomFormat(Width, Height, PF_B8G8R8A8, false);
		RenderTarget->UpdateResourceImmediate(true);

		// 카메라 오빗각 (Project Settings > Plugins > Part Icon Tool 에서 조절)
		const FBoxSphereBounds Bounds = PreviewMeshComp->Bounds;
		const float FOVDeg = 30.f;
		const float TargetDist = Bounds.SphereRadius / FMath::Tan(FMath::DegreesToRadians(FOVDeg * 0.5f));
		const float CamDist = TargetDist * Settings->DistanceMultiplier + Settings->OrbitZoom;

		const FRotator CamRotation(Settings->OrbitPitch, Settings->OrbitYaw, 0.f);
		const FVector CamLocation = Bounds.Origin - CamRotation.Vector() * CamDist;

		// 정면 라이팅 (밝기 과하면 흰색으로 타버림)
		PreviewScene.SetLightDirection(CamRotation);
		PreviewScene.SetLightBrightness(Settings->LightBrightness);

		USceneCaptureComponent2D* Capture = NewObject<USceneCaptureComponent2D>(GetTransientPackage());
		Capture->TextureTarget = RenderTarget;
		Capture->CaptureSource = SCS_FinalColorLDR;     // 렌더 안 된 영역은 클리어(투명) 유지
		Capture->bCaptureEveryFrame = false;
		Capture->bCaptureOnMovement = false;
		Capture->FOVAngle = FOVDeg;
		Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		Capture->ShowOnlyComponents.Add(PreviewMeshComp);
		Capture->ShowFlags.SetFog(false);
		Capture->ShowFlags.SetAtmosphere(false);
		Capture->ShowFlags.SetTemporalAA(false);        // 단일 프레임 캡처 노이즈 방지
		Capture->ShowFlags.SetBloom(false);             // 배경 점 노이즈(블룸 번짐) 방지

		// 자동 노출 고정 (씬 캡처가 어둡게/밝게 깎는 것 방지)
		Capture->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
		Capture->PostProcessSettings.AutoExposureMinBrightness = 1.f;
		Capture->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
		Capture->PostProcessSettings.AutoExposureMaxBrightness = 1.f;

		PreviewScene.AddComponent(Capture, FTransform(CamRotation, CamLocation));

		// 머티리얼 셰이더 컴파일 완료 대기 (안 하면 기본(흰색) 머티리얼로 캡처됨)
		if (GShaderCompilingManager)
		{
			GShaderCompilingManager->FinishAllCompilation();
		}
		FlushRenderingCommands();

		// 1패스: 색상 (FinalColorLDR — 색은 정확하나 알파는 항상 불투명)
		Capture->CaptureScene();
		FlushRenderingCommands();

		FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags ReadFlags(RCM_UNorm, CubeFace_MAX);
		ReadFlags.SetLinearToGamma(false);
		if (!RTResource || !RTResource->ReadPixels(Pixels, ReadFlags) || Pixels.Num() != Width * Height)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PartIconTool] %s: 씬 캡처 픽셀 읽기 실패"), *Def->GetName());
			return nullptr;
		}

		// 2패스: 커버리지 마스크 (SceneColorHDR 알파 = 1 - 불투명도)
		UTextureRenderTarget2D* MaskRT = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
		MaskRT->ClearColor = FLinearColor(0.f, 0.f, 0.f, 1.f);   // 배경 = 불투명도 0 → 알파 1
		MaskRT->InitCustomFormat(Width, Height, PF_FloatRGBA, true);
		MaskRT->UpdateResourceImmediate(true);

		Capture->TextureTarget = MaskRT;
		Capture->CaptureSource = SCS_SceneColorHDR;
		Capture->CaptureScene();
		FlushRenderingCommands();

		FTextureRenderTargetResource* MaskResource = MaskRT->GameThread_GetRenderTargetResource();
		TArray<FLinearColor> MaskPixels;
		if (!MaskResource || !MaskResource->ReadLinearColorPixels(MaskPixels) || MaskPixels.Num() != Width * Height)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PartIconTool] %s: 마스크 픽셀 읽기 실패"), *Def->GetName());
			return nullptr;
		}

		// 마스크 커버리지(= 1 - 알파)로 색상 픽셀의 알파를 덮어써 누끼 생성
		for (int32 i = 0; i < Pixels.Num(); ++i)
		{
			const float Coverage = FMath::Clamp(1.f - MaskPixels[i].A, 0.f, 1.f);
			Pixels[i].A = static_cast<uint8>(FMath::RoundToInt(Coverage * 255.f));
		}

		// Definition과 같은 폴더에 T_<DefName>_Icon 으로 저장
		const FString DefPackagePath = FPackageName::GetLongPackagePath(Def->GetOutermost()->GetName());
		const FString TexName = FString::Printf(TEXT("T_%s_Icon"), *Def->GetName());
		const FString TexPackageName = DefPackagePath / TexName;

		UPackage* TexPackage = CreatePackage(*TexPackageName);
		if (!TexPackage)
		{
			return nullptr;
		}
		TexPackage->FullyLoad();

		// 같은 이름의 기존 텍스처가 있으면 재사용(덮어쓰기)
		UTexture2D* NewTexture = FindObject<UTexture2D>(TexPackage, *TexName);
		if (!NewTexture)
		{
			NewTexture = NewObject<UTexture2D>(TexPackage, *TexName, RF_Public | RF_Standalone);
		}

		// FColor 배열은 BGRA8 레이아웃이라 Source에 그대로 주입
		NewTexture->Source.Init(Width, Height, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
		NewTexture->SRGB = true;
		NewTexture->CompressionSettings = TC_EditorIcon;
		NewTexture->LODGroup = TEXTUREGROUP_UI;
		NewTexture->NeverStream = true;
		NewTexture->UpdateResource();
		NewTexture->PostEditChange();

		FAssetRegistryModule::AssetCreated(NewTexture);
		TexPackage->MarkPackageDirty();

		// Icon 필드 지정
		Def->Icon = NewTexture;
		Def->MarkPackageDirty();

		// 두 패키지 디스크 저장
		auto SaveOne = [](UPackage* Package, UObject* Asset)
		{
			if (!Package || !Asset)
			{
				return;
			}
			const FString FileName = FPackageName::LongPackageNameToFilename(
				Package->GetName(), FPackageName::GetAssetPackageExtension());
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
			SaveArgs.SaveFlags = SAVE_NoError;
			UPackage::SavePackage(Package, Asset, *FileName, SaveArgs);
		};
		SaveOne(TexPackage, NewTexture);
		SaveOne(Def->GetOutermost(), Def);

		UE_LOG(LogTemp, Log, TEXT("[PartIconTool] %s -> %s 생성 및 Icon 지정 완료"),
			*Def->GetName(), *TexPackageName);
		return NewTexture;
	}

	void OnGenerateClicked(const FToolMenuContext& MenuContext)
	{
		const UContentBrowserAssetContextMenuContext* Context =
			MenuContext.FindContext<UContentBrowserAssetContextMenuContext>();
		if (!Context)
		{
			return;
		}

		int32 SuccessCount = 0;
		int32 SkipCount = 0;
		for (const FAssetData& AssetData : Context->SelectedAssets)
		{
			UNSPartDefinition* Def = Cast<UNSPartDefinition>(AssetData.GetAsset());
			if (!Def)
			{
				continue;
			}
			if (GeneratePartIcon(Def))
			{
				++SuccessCount;
			}
			else
			{
				++SkipCount;
			}
		}

		const FText Message = FText::Format(
			LOCTEXT("ResultMsg", "파츠 아이콘 생성 완료: 성공 {0}개, 건너뜀 {1}개"),
			FText::AsNumber(SuccessCount), FText::AsNumber(SkipCount));
		FNotificationInfo Info(Message);
		Info.ExpireDuration = 5.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void FPartIconToolModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPartIconToolModule::RegisterMenus));
}

void FPartIconToolModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FPartIconToolModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
	if (!Menu)
	{
		return;
	}

	FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
	Section.AddDynamicEntry("GeneratePartIcon",
		FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
		{
			const UContentBrowserAssetContextMenuContext* Context =
				InSection.Context.FindContext<UContentBrowserAssetContextMenuContext>();
			if (!Context)
			{
				return;
			}

			// 선택 항목 중 파츠 Definition이 하나라도 있을 때만 메뉴 노출
			const UClass* PartClass = UNSPartDefinition::StaticClass();
			bool bHasPart = false;
			for (const FAssetData& AssetData : Context->SelectedAssets)
			{
				if (AssetData.IsInstanceOf(PartClass))
				{
					bHasPart = true;
					break;
				}
			}
			if (!bHasPart)
			{
				return;
			}

			FToolUIAction Action;
			Action.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&OnGenerateClicked);

			InSection.AddMenuEntry(
				"GeneratePartIcon",
				LOCTEXT("GeneratePartIconLabel", "Generate Icon from Mesh"),
				LOCTEXT("GeneratePartIconTip", "파츠 스켈레탈 메시 썸네일을 캡처해 텍스처로 저장하고 Icon에 지정합니다."),
				FSlateIcon(),
				Action);
		}));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPartIconToolModule, PartIconTool)
