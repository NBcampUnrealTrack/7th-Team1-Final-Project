// Copyright 2026 One Team. All rights reserved.


#include "NSGraphicSettingWidget.h"
#include "NeoSanctum/UI/Common/NSButtonBase.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

void UNSGraphicSettingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 항목을 추가하고 선택하기 전에 생성 이벤트를 바인딩한다.
	BindOptionWidgetGenerators();
	InitializeResolutionOptions();
	InitializeWindowModeOptions();
	InitializeFrameRateOptions();
	InitializeQualityOptions();
	InitializeAntiAliasingOptions();
	SynchronizeSettings();
	
	UComboBoxString* ComboBoxes[] =
	{
		ResolutionComboBox,
		WindowModeComboBox,
		FrameRateComboBox,
		OverallQualityComboBox,
		AntiAliasingQualityComboBox
	};

	for (UComboBoxString* ComboBox : ComboBoxes)
	{
		if (ComboBox)
		{
			ComboBox->OnSelectionChanged.AddUniqueDynamic(
				this,
				&ThisClass::HandleComboBoxSelectionChanged);
		}
	}

	CenterAllSelectedOptionTexts();
	
	if (ApplyButton)
	{
		ApplyButton->OnClicked().AddUObject(
			this,
			&ThisClass::OnApplyClicked);
	}

	if (ResetButton)
	{
		ResetButton->OnClicked().AddUObject(
			this,
			&ThisClass::OnResetClicked);
	}
	
	FTextLocalizationManager::Get()
	.OnTextRevisionChangedEvent.RemoveAll(this);

	FTextLocalizationManager::Get()
		.OnTextRevisionChangedEvent.AddUObject(
			this,
			&ThisClass::HandleTextRevisionChanged);
}

void UNSGraphicSettingWidget::NativeDestruct()
{
	UnbindOptionWidgetGenerators();
	
	FTextLocalizationManager::Get()
	.OnTextRevisionChangedEvent.RemoveAll(this);
	
	if (ApplyButton)
	{
		ApplyButton->OnClicked().RemoveAll(this);
	}

	if (ResetButton)
	{
		ResetButton->OnClicked().RemoveAll(this);
	}
	
	Super::NativeDestruct();
}

void UNSGraphicSettingWidget::BindOptionWidgetGenerators()
{
	UComboBoxString* ComboBoxes[] =
	{
		ResolutionComboBox,
		WindowModeComboBox,
		FrameRateComboBox,
		OverallQualityComboBox,
		AntiAliasingQualityComboBox
	};

	for (UComboBoxString* ComboBox : ComboBoxes)
	{
		if (ComboBox)
		{
			ComboBox->OnGenerateWidgetEvent.BindDynamic(
				this,
				&ThisClass::GenerateGraphicOptionWidget);
		}
	}
}

void UNSGraphicSettingWidget::UnbindOptionWidgetGenerators()
{
	UComboBoxString* ComboBoxes[] =
	{
		ResolutionComboBox,
		WindowModeComboBox,
		FrameRateComboBox,
		OverallQualityComboBox,
		AntiAliasingQualityComboBox
	};

	for (UComboBoxString* ComboBox : ComboBoxes)
	{
		if (ComboBox)
		{
			ComboBox->OnGenerateWidgetEvent.Unbind();
		}
	}

	GeneratedOptionWidgets.Reset();
}

void UNSGraphicSettingWidget::InitializeResolutionOptions()
{
	ResolutionComboBox->ClearOptions();
	SupportedResolutions.Reset();
	
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(
		SupportedResolutions);
	
	UGameUserSettings* Settings =
		GetGameUserSettings();
	
	const FIntPoint DesktopResolution =
		Settings
			? Settings->GetDesktopResolution()
			: FIntPoint::ZeroValue;
	
	SupportedResolutions.RemoveAll(
		[DesktopResolution](const FIntPoint& Resolution)
		{
			const float AspectRatio =
				static_cast<float>(Resolution.X) /
					static_cast<float>(Resolution.Y);
			
			const bool bIs16By9 =
				FMath::IsNearlyEqual(AspectRatio,
					16.0f / 9.0f,
					0.01f);
			
			const bool bIsDesktopResolution =
				Resolution == DesktopResolution;
			
			return !bIs16By9 &&
				!bIsDesktopResolution;
		});
	
	if (DesktopResolution.X > 0 &&
		DesktopResolution.Y > 0)
	{
		SupportedResolutions.AddUnique(
			DesktopResolution);
	}
	
	SupportedResolutions.Sort(
		[](const FIntPoint& Left, const FIntPoint& Right)
		{
			if (Left.X == Right.X)
				{
				return Left.Y < Right.Y;
				}
			
			return Left.X < Right.X;
			});
	
	for (const FIntPoint& Resolution : SupportedResolutions)
	{
		ResolutionComboBox->AddOption(
			FString::Printf(
				TEXT("%d x %d"),
				Resolution.X,
				Resolution.Y));
	}
}

void UNSGraphicSettingWidget::InitializeWindowModeOptions()
{
	WindowModeComboBox->ClearOptions();

	WindowModeComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"Fullscreen",
			"전체 화면").ToString());

	WindowModeComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"BorderlessFullscreen",
			"테두리 없는 전체 화면").ToString());

	WindowModeComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"Windowed",
			"창 모드").ToString());
}

void UNSGraphicSettingWidget::InitializeFrameRateOptions()
{
	FrameRateComboBox->ClearOptions();
	
	FrameRateLimits =
	{
		30.0f,
		60.0f,
		120.0f,
		144.0f,
		0.0f
	};
	
	FrameRateComboBox->AddOption(TEXT("30"));
	FrameRateComboBox->AddOption(TEXT("60"));
	FrameRateComboBox->AddOption(TEXT("120"));
	FrameRateComboBox->AddOption(TEXT("144"));
	FrameRateComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"UnlimitedFrameRate",
			"제한 없음").ToString());
}

void UNSGraphicSettingWidget::InitializeQualityOptions()
{
	OverallQualityComboBox->ClearOptions();

	OverallQualityComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"QualityLow",
			"낮음").ToString());

	OverallQualityComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"QualityMedium",
			"중간").ToString());

	OverallQualityComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"QualityHigh",
			"높음").ToString());

	OverallQualityComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"QualityEpic",
			"최상").ToString());
}

void UNSGraphicSettingWidget::InitializeAntiAliasingOptions()
{
if (!AntiAliasingQualityComboBox)
{
	return;
}

AntiAliasingQualityComboBox->ClearOptions();

AntiAliasingQualityComboBox->AddOption(
	NSLOCTEXT(
		"GraphicSettings",
		"AntiAliasingLow",
		"낮음").ToString());

AntiAliasingQualityComboBox->AddOption(
	NSLOCTEXT(
		"GraphicSettings",
		"AntiAliasingMedium",
		"중간").ToString());

AntiAliasingQualityComboBox->AddOption(
	NSLOCTEXT(
		"GraphicSettings",
		"AntiAliasingHigh",
		"높음").ToString());

AntiAliasingQualityComboBox->AddOption(
	NSLOCTEXT(
		"GraphicSettings",
		"AntiAliasingEpic",
		"최상").ToString());
}
void UNSGraphicSettingWidget::SynchronizeSettings()
{
	UGameUserSettings* Settings =
		GetGameUserSettings();
	
	if (!Settings)
	{
		return;
	}
	
	Settings->LoadSettings(false);
	
	FIntPoint SelectedResolution =
		Settings->GetScreenResolution();
	
	int32 ResolutionIndex =
		SupportedResolutions.IndexOfByKey(
			SelectedResolution);
	
	if (ResolutionIndex == INDEX_NONE)
	{
		SelectedResolution =
			Settings->GetLastConfirmedScreenResolution();

		ResolutionIndex =
			SupportedResolutions.IndexOfByKey(
				SelectedResolution);
	}

	if (ResolutionIndex == INDEX_NONE)
	{
		ResolutionIndex =
			SupportedResolutions.IndexOfByKey(
				Settings->GetDesktopResolution());
	}

	if (ResolutionIndex != INDEX_NONE)
	{
		ResolutionComboBox->SetSelectedIndex(
			ResolutionIndex);
	}
	
	switch (Settings->GetFullscreenMode())
	{
	case EWindowMode::Fullscreen:
		WindowModeComboBox->SetSelectedIndex(0);
		break;

	case EWindowMode::WindowedFullscreen:
		WindowModeComboBox->SetSelectedIndex(1);
		break;

	case EWindowMode::Windowed:
	default:
		WindowModeComboBox->SetSelectedIndex(2);
		break;
	}
	
	const float CurrentFrameRate =
		Settings->GetFrameRateLimit();
	
	int32 FrameRateIndex =
		FrameRateLimits.IndexOfByPredicate(
			[CurrentFrameRate](float FrameRate)
			{
				return FMath::IsNearlyEqual(
					FrameRate,
					CurrentFrameRate);
			});
	
	if (FrameRateIndex == INDEX_NONE)
	{
		FrameRateIndex =
			FrameRateLimits.Add(CurrentFrameRate);
		
		FrameRateComboBox->AddOption(
			FString::FromInt(
				FMath::RoundToInt(
					CurrentFrameRate)));
	}
	
	FrameRateComboBox->SetSelectedIndex(FrameRateIndex);
	
	const int32 BaseQualityLevel =
		GetBaseQualityLevel(Settings);

	OverallQualityComboBox->SetSelectedIndex(
		BaseQualityLevel);
	
	if (AntiAliasingQualityComboBox)
	{
		const int32 AntiAliasingQuality =
			FMath::Clamp(
				Settings->GetAntiAliasingQuality(),
				0,
				3);

		AntiAliasingQualityComboBox->SetSelectedIndex(
			AntiAliasingQuality);
	}
	
	if (VSyncCheckBox)
	{
		VSyncCheckBox->SetIsChecked(
			Settings->IsVSyncEnabled());
	}
}

void UNSGraphicSettingWidget::HandleTextRevisionChanged()
{
	InitializeWindowModeOptions();
	InitializeFrameRateOptions();
	InitializeQualityOptions();
	InitializeAntiAliasingOptions();
	SynchronizeSettings();
	CenterAllSelectedOptionTexts();
}

void UNSGraphicSettingWidget::OnApplyClicked()
{
	UGameUserSettings* Settings =
		GetGameUserSettings();
	
	if (!Settings)
	{
		return;
	}
	
	const int32 ResolutionIndex =
		ResolutionComboBox->GetSelectedIndex();
	
	if (SupportedResolutions.IsValidIndex(
		ResolutionIndex))
	{
		Settings->SetScreenResolution(
			SupportedResolutions[ResolutionIndex]);
	}
	
	switch (WindowModeComboBox->GetSelectedIndex())
	{
	case 0:
		Settings->SetFullscreenMode(
			EWindowMode::Fullscreen);
		break;

	case 1:
		Settings->SetFullscreenMode(
			EWindowMode::WindowedFullscreen);
		break;

	case 2:
	default:
		Settings->SetFullscreenMode(
			EWindowMode::Windowed);
		break;
	}

	const int32 FrameRateIndex =
		FrameRateComboBox->GetSelectedIndex();

	if (FrameRateLimits.IsValidIndex(
		FrameRateIndex))
	{
		Settings->SetFrameRateLimit(
			FrameRateLimits[FrameRateIndex]);
	}

	const int32 BaseQualityLevel =
		OverallQualityComboBox->GetSelectedIndex();

	if (BaseQualityLevel >= 0 &&
		BaseQualityLevel <= 3)
	{
		SetBaseQualityLevel(
			Settings,
			BaseQualityLevel);
	}
	
	const int32 AntiAliasingQuality =
		AntiAliasingQualityComboBox
			? AntiAliasingQualityComboBox->GetSelectedIndex()
			: INDEX_NONE;

	if (AntiAliasingQuality >= 0 &&
		AntiAliasingQuality <= 3)
	{
		Settings->SetAntiAliasingQuality(
			AntiAliasingQuality);
	}
	
	Settings->SetVSyncEnabled(
		VSyncCheckBox &&
		VSyncCheckBox->IsChecked());

	Settings->ApplySettings(false);
	Settings->ConfirmVideoMode();
	Settings->SaveSettings();
	SynchronizeSettings();
}

void UNSGraphicSettingWidget::OnResetClicked()
{
	UGameUserSettings* Settings =
		GetGameUserSettings();
	
	if (!Settings)
	{
		return;
	}
	
	Settings->SetScreenResolution(
		Settings->GetDesktopResolution());
	Settings->SetFullscreenMode(
		EWindowMode::WindowedFullscreen);
	Settings->SetFrameRateLimit(0.0f);
	SetBaseQualityLevel(Settings, 3);
	Settings->SetAntiAliasingQuality(3);
	Settings->SetVSyncEnabled(false);

	Settings->ApplySettings(false);
	Settings->ConfirmVideoMode();
	Settings->SaveSettings();
	
	SynchronizeSettings();
}
	UWidget* UNSGraphicSettingWidget::GenerateGraphicOptionWidget(
		FString Item)
	{
		UTextBlock* OptionText =
			NewObject<UTextBlock>(this);

		if (!OptionText)
		{
			return nullptr;
		}

		OptionText->SetText(
			FText::FromString(Item));

		OptionText->SetColorAndOpacity(
			FSlateColor(
				FLinearColor(
					0.9f,
					0.95f,
					1.0f,
					1.0f)));

		FSlateFontInfo FontInfo =
			OptionText->GetFont();

		FontInfo.Size = 20;
		OptionText->SetFont(FontInfo);

		UBorder* OptionContainer =
			NewObject<UBorder>(this);

		if (!OptionContainer)
		{
			GeneratedOptionWidgets.Add(OptionText);
			return OptionText;
		}

		// 텍스트가 목록 행 이미지의 테두리에 가려지지 않도록 여백을 준다.
		OptionContainer->SetPadding(
			FMargin(
				18.0f,
				4.0f,
				12.0f,
				4.0f));

		// 목록 행 배경은 ComboBox의 ItemStyle이 담당한다.
		OptionContainer->SetBrushColor(
			FLinearColor::Transparent);

		OptionContainer->AddChild(OptionText);

		// 반환하는 컨테이너를 UPROPERTY 배열에 보관해 GC로부터 보호한다.
		GeneratedOptionWidgets.Add(OptionContainer);

		return OptionContainer;
	}

UGameUserSettings* UNSGraphicSettingWidget::GetGameUserSettings() const
{
	return UGameUserSettings::GetGameUserSettings();
}

void UNSGraphicSettingWidget::SetBaseQualityLevel(UGameUserSettings* Settings, int32 QualityLevel) const
{
	if (!Settings)
	{
		return;
	}

	const int32 ClampedQuality =
		FMath::Clamp(QualityLevel, 0, 3);

	// 안티앨리어싱은 별도 ComboBox에서 설정하므로 제외한다.
	Settings->SetViewDistanceQuality(ClampedQuality);
	Settings->SetShadowQuality(ClampedQuality);
	Settings->SetGlobalIlluminationQuality(ClampedQuality);
	Settings->SetReflectionQuality(ClampedQuality);
	Settings->SetPostProcessingQuality(ClampedQuality);
	Settings->SetTextureQuality(ClampedQuality);
	Settings->SetVisualEffectQuality(ClampedQuality);
	Settings->SetFoliageQuality(ClampedQuality);
	Settings->SetShadingQuality(ClampedQuality);
}

int32 UNSGraphicSettingWidget::GetBaseQualityLevel(const UGameUserSettings* Settings) const
{
	{
		if (!Settings)
		{
			return 3;
		}

		// 기본 품질 항목은 모두 동일하게 적용되므로
		// TextureQuality를 대표값으로 사용한다.
		return FMath::Clamp(
			Settings->GetTextureQuality(),
			0,
			3);
	}
}

void UNSGraphicSettingWidget::HandleComboBoxSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectInfo)
{
	CenterAllSelectedOptionTexts();
}

void UNSGraphicSettingWidget::CenterSelectedOptionText(UComboBoxString* ComboBox)
{
	if (!ComboBox || !FSlateApplication::IsInitialized())
	{
		return;
	}

	const FString SelectedOption =
		ComboBox->GetSelectedOption();

	if (SelectedOption.IsEmpty())
	{
		return;
	}

	const TSharedRef<FSlateFontMeasure> FontMeasure =
		FSlateApplication::Get()
		.GetRenderer()
		->GetFontMeasureService();

	FSlateFontInfo FontInfo = ComboBox->GetFont();

	// GenerateGraphicOptionWidget()에서 사용하는 크기와 일치시킨다.
	FontInfo.Size = 20;

	const FVector2D TextSize =
		FontMeasure->Measure(
			FStringView(SelectedOption),
			FontInfo,
			1.0f);

	const float CachedWidth =
		ComboBox->GetCachedGeometry()
		.GetLocalSize()
		.X;

	// WBP의 SizeBox Width Override가 360이므로,
	// 첫 레이아웃 전에는 360을 사용한다.
	const float ComboBoxWidth =
		CachedWidth > 0.0f
			? CachedWidth
			: 360.0f;

	const FMargin ButtonPadding =
		ComboBox->GetWidgetStyle()
		.ComboButtonStyle
		.ContentPadding;

	// GenerateGraphicOptionWidget()의 Border 왼쪽 Padding 18을 반영한다.
	constexpr float GeneratedTextLeftPadding = 18.0f;
	constexpr float MinimumContentPadding = 4.0f;

	const float CenteredLeftPadding =
		((ComboBoxWidth - TextSize.X) * 0.5f)
		- ButtonPadding.Left
		- GeneratedTextLeftPadding;

	FMargin ContentPadding =
		ComboBox->GetContentPadding();

	ContentPadding.Left =
		FMath::Max(
			MinimumContentPadding,
			CenteredLeftPadding);

	ComboBox->SetContentPadding(ContentPadding);
}

void UNSGraphicSettingWidget::CenterAllSelectedOptionTexts()
{
	{
		UComboBoxString* ComboBoxes[] =
		{
			ResolutionComboBox,
			WindowModeComboBox,
			FrameRateComboBox,
			OverallQualityComboBox,
			AntiAliasingQualityComboBox
		};

		for (UComboBoxString* ComboBox : ComboBoxes)
		{
			CenterSelectedOptionText(ComboBox);
		}
	}
}
