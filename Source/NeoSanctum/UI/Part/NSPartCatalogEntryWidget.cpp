// Copyright 2026 One Team. All rights reserved.

#include "NSPartCatalogEntryWidget.h"
#include "Components/Image.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/UI/Interaction/NSPartEquipWidget.h"

UNSPartCatalogEntryWidget::UNSPartCatalogEntryWidget()
{
	bSelectable = true;
	bInteractableWhenSelected = true;
}

void UNSPartCatalogEntryWidget::SetupEntry(const FNSPartDefinitionRow& Row, UNSPartEquipWidget* OwnerWidget)
{
	// 재-Setup 시 이전 델리게이트 경로가 남아 클릭을 가로채지 않도록 초기화
	ClickHandler.Unbind();

	StoredRow = Row;
	OwnerRef = OwnerWidget;

	UNSPartDefinition* Def = Row.Definition.Get();
	if (Def)
	{
		OnDefinitionLoaded();
		return;
	}

	if (Row.Definition.IsNull())
	{
		NS_LOG(LogNS, Warning, "[Catalog] Row.Definition이 Null입니다. Row 데이터에 Definition 소프트레퍼런스가 비어있습니다.");
		return;
	}

	NS_LOG(LogNS, Log, "[Catalog] Definition 비동기 로드 요청: {Definition}", ("Definition", Row.Definition.ToString()));

	TWeakObjectPtr<UNSPartCatalogEntryWidget> WeakThis(this);
	LoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Row.Definition.ToSoftObjectPath(),
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnDefinitionLoaded();
				WeakThis->LoadHandle.Reset();
			}
		});
}

void UNSPartCatalogEntryWidget::SetupEntry(const FNSPartDefinitionRow& Row, FNSOnCatalogEntryClicked InClickHandler)
{
	// 공용 Setup이 ClickHandler를 초기화하므로 호출 후에 바인딩
	SetupEntry(Row, static_cast<UNSPartEquipWidget*>(nullptr));
	ClickHandler = InClickHandler;
}

void UNSPartCatalogEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(HoverHighlight))
	{
		HoverHighlight->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsValid(PressedHighlight))
	{
		PressedHighlight->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsValid(SelectedIndicator))
	{
		SelectedIndicator->SetVisibility(ESlateVisibility::Collapsed);
	}

	OnClicked().AddUObject(this, &ThisClass::HandleClicked);
	OnHovered().AddUObject(this, &ThisClass::HandleHovered);
	OnUnhovered().AddUObject(this, &ThisClass::HandleUnhovered);
	OnPressed().AddUObject(this, &ThisClass::HandlePressed);
	OnReleased().AddUObject(this, &ThisClass::HandleReleased);
	OnIsSelectedChanged().AddUObject(this, &ThisClass::HandleSelectionChanged);
}

void UNSPartCatalogEntryWidget::NativeDestruct()
{
	if (LoadHandle.IsValid())
	{
		LoadHandle->CancelHandle();
		LoadHandle.Reset();
	}

	OnClicked().RemoveAll(this);
	OnHovered().RemoveAll(this);
	OnUnhovered().RemoveAll(this);
	OnPressed().RemoveAll(this);
	OnReleased().RemoveAll(this);
	OnIsSelectedChanged().RemoveAll(this);

	Super::NativeDestruct();
}

void UNSPartCatalogEntryWidget::HandleHovered()
{
	if (IsValid(HoverHighlight))
	{
		HoverHighlight->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UNSPartCatalogEntryWidget::HandleUnhovered()
{
	if (IsValid(HoverHighlight))
	{
		HoverHighlight->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSPartCatalogEntryWidget::HandlePressed()
{
	if (IsValid(PressedHighlight))
	{
		PressedHighlight->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UNSPartCatalogEntryWidget::HandleReleased()
{
	// 선택된 상태(GetSelected())면 눌림 이미지를 계속 유지하고, 아니면 원래대로 해제
	if (IsValid(PressedHighlight) && !GetSelected())
	{
		PressedHighlight->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSPartCatalogEntryWidget::HandleSelectionChanged(bool bInSelected)
{
	if (IsValid(SelectedIndicator))
	{
		SelectedIndicator->SetVisibility(bInSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// 선택된 항목은 눌림 이미지를 고정으로 보여주고, 선택 해제되면 원래 상태로 되돌림
	if (IsValid(PressedHighlight))
	{
		PressedHighlight->SetVisibility(bInSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UNSPartCatalogEntryWidget::OnDefinitionLoaded()
{
	UNSPartDefinition* Def = StoredRow.Definition.Get();
	if (!Def)
	{
		NS_LOG(LogNS, Warning, "[Catalog] OnDefinitionLoaded 호출됐지만 Definition Get() 실패: {Definition}", ("Definition", StoredRow.Definition.ToString()));
		return;
	}

	if (!IsValid(IconImage))
	{
		NS_LOG(LogNS, Warning, "[Catalog] IconImage 바인딩이 유효하지 않습니다. WBP에서 IconImage 이름/타입을 확인하세요.");
		return;
	}

	if (UTexture2D* Tex = Def->Icon.Get())
	{
		NS_LOG(LogNS, Log, "[Catalog] Icon 이미 로드되어 있음, SetBrushFromTexture 호출: {Texture}", ("Texture", Tex->GetName()));
		IconImage->SetBrushFromTexture(Tex);
		// 위젯이 아직 트리에 붙기 전(AddChild 전)에 브러시가 설정되는 경우가 있어
		// 첫 페인트 때 반영이 안 되고 남아있을 수 있음 — 명시적으로 다시 그리도록 강제
		IconImage->InvalidateLayoutAndVolatility();
	}
	else if (!Def->Icon.IsNull())
	{
		NS_LOG(LogNS, Log, "[Catalog] Icon 비동기 로드 요청: {Icon}", ("Icon", Def->Icon.ToString()));
		TWeakObjectPtr<UNSPartCatalogEntryWidget> WeakThis(this);
		TSoftObjectPtr<UTexture2D> SoftIcon = Def->Icon;
		LoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
			SoftIcon.ToSoftObjectPath(),
			[WeakThis, SoftIcon]()
			{
				if (!WeakThis.IsValid())
				{
					return;
				}

				if (UTexture2D* Tex = SoftIcon.Get())
				{
					NS_LOG(LogNS, Log, "[Catalog] Icon 비동기 로드 완료: {Texture}", ("Texture", Tex->GetName()));
					WeakThis->IconImage->SetBrushFromTexture(Tex);
					WeakThis->IconImage->InvalidateLayoutAndVolatility();
				}
				else
				{
					NS_LOG(LogNS, Warning, "[Catalog] Icon 비동기 로드 완료 콜백이지만 Get() 실패: {Icon}", ("Icon", SoftIcon.ToString()));
				}
				WeakThis->LoadHandle.Reset();
			});
	}
	else
	{
		NS_LOG(LogNS, Warning, "[Catalog] Def->Icon 소프트레퍼런스가 비어있습니다 (Definition: {Definition})", ("Definition", Def->GetName()));
	}
}

void UNSPartCatalogEntryWidget::HandleClicked()
{
	NS_LOG(LogNS, Log, "[Catalog] 클릭됨");

	if (ClickHandler.IsBound())
	{
		ClickHandler.Execute(StoredRow, this);
		return;
	}

	UNSPartEquipWidget* Owner = OwnerRef.Get();
	if (!IsValid(Owner))
	{
		NS_LOG(LogNS, Warning, "[Catalog] OwnerRef가 유효하지 않습니다.");
		return;
	}

	Owner->SelectCatalogPart(StoredRow, this);
}
