// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/UI/Waypoint/NSWaypointMarkerWidget.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Engine/AssetManager.h"
#include "Engine/Texture2D.h"
#include "NeoSanctum/UI/Waypoint/NSWaypointMarkerComponent.h"

void UNSWaypointMarkerWidget::InitializeMarker(UNSWaypointMarkerComponent* InMarker)
{
	MarkerComponent = InMarker;

	if (!InMarker)
	{
		return;
	}

	// 아이콘은 소프트 레퍼런스이므로 비동기 로드 후 적용
	const TSoftObjectPtr<UTexture2D>& Icon = InMarker->GetMarkerIcon();
	if (Icon.IsNull())
	{
		return;
	}

	TWeakObjectPtr<UNSWaypointMarkerWidget> WeakThis(this);
	IconLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Icon.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda([WeakThis, Icon]()
		{
			// 로드 완료 전에 위젯이 소멸했을 수 있으므로 약참조 확인
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (UTexture2D* LoadedIcon = Icon.Get())
			{
				WeakThis->IconImage->SetBrushFromTexture(LoadedIcon);
			}
		}));
}

void UNSWaypointMarkerWidget::UpdateDistance(float DistanceMeters)
{
	// 하한 1M: 0M 표기는 어색하므로 최소 1로 고정
	const int32 Meters = FMath::Max(1, FMath::RoundToInt(DistanceMeters));

	// 정수 미터가 안 바뀌면 SetText 생략 (매 틱 Slate 무효화 방지)
	if (Meters == LastDisplayedMeters)
	{
		return;
	}

	LastDisplayedMeters = Meters;
	DistanceText->SetText(
		FText::Format(NSLOCTEXT("Waypoint", "DistanceFormat", "{0} M"), Meters));
}

void UNSWaypointMarkerWidget::NativeDestruct()
{
	// 진행 중인 아이콘 로드가 있으면 취소 (소멸 후 콜백 방지)
	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}

	Super::NativeDestruct();
}
