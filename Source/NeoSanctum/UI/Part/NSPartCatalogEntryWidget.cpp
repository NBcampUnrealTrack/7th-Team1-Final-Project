// Copyright 2026 One Team. All rights reserved.

#include "NSPartCatalogEntryWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/UI/Interaction/NSPartEquipWidget.h"

void UNSPartCatalogEntryWidget::SetupEntry(const FNSPartDefinitionRow& Row, UNSPartEquipWidget* OwnerWidget)
{
	// 재-Setup 시 이전 델리게이트 경로가 남아 클릭을 가로채지 않도록 초기화
	ClickHandler.Unbind();

	StoredRow = Row;
	OwnerRef = OwnerWidget;

	if (IsValid(SelectButton))
	{
		SelectButton->OnClicked.AddUniqueDynamic(this, &UNSPartCatalogEntryWidget::OnSelectButtonClicked);
	}
	
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

void UNSPartCatalogEntryWidget::NativeDestruct()
{
	if (LoadHandle.IsValid())
	{
		LoadHandle->CancelHandle();
		LoadHandle.Reset();
	}

	Super::NativeDestruct();
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

void UNSPartCatalogEntryWidget::OnSelectButtonClicked()
{
	NS_LOG(LogNS, Log, "[Catalog] SelectButton 클릭됨");

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

void UNSPartCatalogEntryWidget::SetIsSelected(bool bSelected)
{
	if (IsValid(SelectedIndicator))
	{
		SelectedIndicator->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
