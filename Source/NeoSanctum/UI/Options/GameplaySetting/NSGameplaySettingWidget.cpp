// Copyright 2026 One Team. All rights reserved.


#include "NSGameplaySettingWidget.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "NeoSanctum/UI/Options/NSUISettingsSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Components/HorizontalBox.h"
#include "Components/ButtonSlot.h"
#include "Components/ComboBoxString.h"
#include "Styling/SlateTypes.h"

void UNSGameplaySettingWidget::ApplyCrosshairColor(FLinearColor NewColor)
{
	UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem();
	
	if (!Settings)
	{
		return;
	}
	
	Settings->SetCrosshairColor(NewColor);
	UpdateColorPreview(Settings->GetCrosshairColor());
	
	PendingCrosshairColor = Settings->GetCrosshairColor();
	SynchronizeSliders(PendingCrosshairColor);
	UpdateApplyButtonState();
}

void UNSGameplaySettingWidget::ResetCrosshairColor()
{
	UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem();
	
	if (!Settings)
	{
		return;
	}
	
	Settings->ResetCrosshairColor();
	UpdateColorPreview(Settings->GetCrosshairColor());
	
	PendingCrosshairColor = Settings->GetCrosshairColor();
	SynchronizeSliders(PendingCrosshairColor);
	UpdateApplyButtonState();
}

UNSUISettingsSubsystem* UNSGameplaySettingWidget::GetUISettingSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	
	return GameInstance
	? GameInstance->GetSubsystem<UNSUISettingsSubsystem>()
	: nullptr;
}

void UNSGameplaySettingWidget::UpdateColorPreview(const FLinearColor& NewColor)
{
	if (CrosshairColorPreview)
	{
		CrosshairColorPreview->SetColorAndOpacity(NewColor);
	}
}

void UNSGameplaySettingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (const UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem())
	{
		PendingCrosshairColor =
			Settings->GetCrosshairColor();

		UpdateColorPreview(PendingCrosshairColor);
		SynchronizeSliders(PendingCrosshairColor);
	}

	if (RedSlider)
	{
		RedSlider->OnValueChanged.AddDynamic(
			this,
			&ThisClass::OnRedValueChanged);
	}

	if (GreenSlider)
	{
		GreenSlider->OnValueChanged.AddDynamic(
			this,
			&ThisClass::OnGreenValueChanged);
	}

	if (BlueSlider)
	{
		BlueSlider->OnValueChanged.AddDynamic(
			this,
			&ThisClass::OnBlueValueChanged);
	}

	if (ApplyCustomColorButton)
	{
		ApplyCustomColorButton->OnClicked.AddDynamic(
			this,
			&ThisClass::OnApplyCustomColorClicked);
	}
	
	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->SetMinValue(0.10f);
		MouseSensitivitySlider->SetMaxValue(10.00f);
		MouseSensitivitySlider->SetStepSize(0.01f);
	}

	if (const UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem())
	{
		PendingMouseSensitivity =
			Settings->GetMouseSensitivity();

		if (MouseSensitivitySlider)
		{
			MouseSensitivitySlider->SetValue(
				PendingMouseSensitivity);
		}
	}

	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->OnValueChanged.AddDynamic(
			this,
			&ThisClass::OnMouseSensitivityChanged);

		MouseSensitivitySlider->OnMouseCaptureEnd.AddDynamic(
			this,
			&ThisClass::OnMouseSensitivityCaptureEnd);

		MouseSensitivitySlider->OnControllerCaptureEnd.AddDynamic(
			this,
			&ThisClass::OnMouseSensitivityCaptureEnd);
	}

	if (MouseSensitivityValueText)
	{
		MouseSensitivityValueText->OnTextCommitted.AddUniqueDynamic(
			this,
			&ThisClass::OnMouseSensitivityTextCommitted);
	}
	
	if (LanguageComboBox)
	{
		LanguageComboBox->OnGenerateWidgetEvent.BindDynamic(
			this,
			&ThisClass::GenerateLanguageOptionWidget);

		MakeComboBoxPopupTransparent(
			LanguageComboBox);
	}

	UpdateMouseSensitivityText();
	UpdateApplyButtonState();
	InitializeLanguageOptions();

	CenterSelectedOptionText(
		LanguageComboBox);
	
	// 초기 표시용
	CenterSelectedOptionText(LanguageComboBox);

	// 실제 레이아웃 너비가 결정된 다음 다시 중앙 정렬
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(
				this,
				[this]()
				{
					CenterSelectedOptionText(LanguageComboBox);
				}));
	}
	ApplyCrosshairSettingsLayout();
}

void UNSGameplaySettingWidget::NativeDestruct()
{
	if (RedSlider)
	{
		RedSlider->OnValueChanged.RemoveAll(this);
	}

	if (GreenSlider)
	{
		GreenSlider->OnValueChanged.RemoveAll(this);
	}

	if (BlueSlider)
	{
		BlueSlider->OnValueChanged.RemoveAll(this);
	}

	if (ApplyCustomColorButton)
	{
		ApplyCustomColorButton->OnClicked.RemoveAll(this);
	}

	if (LanguageComboBox)
	{
		LanguageComboBox->OnSelectionChanged.RemoveAll(this);
		LanguageComboBox->OnGenerateWidgetEvent.Unbind();
	}

	GeneratedLanguageOptionWidgets.Reset();
	
	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->OnValueChanged.RemoveAll(this);
		MouseSensitivitySlider->OnMouseCaptureEnd.RemoveAll(this);
		MouseSensitivitySlider->OnControllerCaptureEnd.RemoveAll(this);
	}
	
	if (MouseSensitivityValueText)
	{
		MouseSensitivityValueText->OnTextCommitted.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UNSGameplaySettingWidget::OnRedValueChanged(float Value)
{
	if (bSynchronizingSliders)
	{
		return;
	}

	PendingCrosshairColor.R = Value;
	UpdateColorPreview(PendingCrosshairColor);
	UpdateRGBValueTexts();
	UpdateApplyButtonState();
}

void UNSGameplaySettingWidget::OnGreenValueChanged(float Value)
{
	if (bSynchronizingSliders)
	{
		return;
	}

	PendingCrosshairColor.G = Value;
	UpdateColorPreview(PendingCrosshairColor);
	UpdateRGBValueTexts();
	UpdateApplyButtonState();
}

void UNSGameplaySettingWidget::OnBlueValueChanged(float Value)
{
	if (bSynchronizingSliders)
	{
		return;
	}

	PendingCrosshairColor.B = Value;
	UpdateColorPreview(PendingCrosshairColor);
	UpdateRGBValueTexts();
	UpdateApplyButtonState();
}

void UNSGameplaySettingWidget::OnApplyCustomColorClicked()
{
	ApplyCrosshairColor(PendingCrosshairColor);
}

UWidget* UNSGameplaySettingWidget::GenerateLanguageOptionWidget(
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
		GeneratedLanguageOptionWidgets.Add(OptionText);
		return OptionText;
	}

	OptionContainer->SetPadding(
		FMargin(
			18.0f,
			4.0f,
			12.0f,
			4.0f));

	OptionContainer->SetBrushColor(
		FLinearColor::Transparent);

	OptionContainer->AddChild(OptionText);

	GeneratedLanguageOptionWidgets.Add(
		OptionContainer);

	return OptionContainer;
}

void UNSGameplaySettingWidget::OnLanguageSelectionChanged(FString SelectionItem, ESelectInfo::Type SelectionType)
{
	CenterSelectedOptionText(LanguageComboBox);
	
	UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem();
	
	if (!Settings)
	{
		return;
	}
	
	const FString CultureCode =
		SelectionItem == TEXT("English")
			? TEXT("en")
			: TEXT("ko-KR");
	
	Settings->SetLanguageCode(CultureCode);
}

void UNSGameplaySettingWidget::OnMouseSensitivityChanged(float Value)
{
	const float ClampedValue =
		FMath::Clamp(Value, 0.10f, 10.00f);

	PendingMouseSensitivity =
		FMath::RoundToFloat(ClampedValue * 100.0f) / 100.0f;

	UpdateMouseSensitivityText();
}

void UNSGameplaySettingWidget::OnMouseSensitivityCaptureEnd()
{
	UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem();
	
	if (!Settings)
	{
		return;
	}
	Settings->SetMouseSensitivity(PendingMouseSensitivity);
}

void UNSGameplaySettingWidget::OnMouseSensitivityTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	FString InputString =
		Text.ToString().TrimStartAndEnd();

	// 1,25처럼 입력해도 1.25로 처리
	InputString.ReplaceInline(
		TEXT(","),
		TEXT("."));

	float InputValue = 0.0f;

	if (!LexTryParseString(InputValue, *InputString) ||
		!FMath::IsFinite(InputValue))
	{
		// 잘못된 입력이면 현재 정상값으로 복구
		UpdateMouseSensitivityText();
		return;
	}

	InputValue =
		FMath::Clamp(
			InputValue,
			0.10f,
			10.00f);

	PendingMouseSensitivity =
		FMath::RoundToFloat(InputValue * 100.0f) / 100.0f;

	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->SetValue(
			PendingMouseSensitivity);
	}

	UpdateMouseSensitivityText();

	if (UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem())
	{
		Settings->SetMouseSensitivity(
			PendingMouseSensitivity);
	}
}
void UNSGameplaySettingWidget::UpdateMouseSensitivityText()
{
	if (!MouseSensitivityValueText)
	{
		return;
	}

	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.SetUseGrouping(false);
	FormattingOptions.SetMinimumIntegralDigits(1);
	FormattingOptions.SetMaximumIntegralDigits(2);
	FormattingOptions.SetMinimumFractionalDigits(2);
	FormattingOptions.SetMaximumFractionalDigits(2);

	MouseSensitivityValueText->SetText(
		FText::AsNumber(
			PendingMouseSensitivity,
			&FormattingOptions));
}

void UNSGameplaySettingWidget::SynchronizeSliders(
	const FLinearColor& Color)
{
	bSynchronizingSliders = true;

	if (RedSlider)
	{
		RedSlider->SetValue(Color.R);
	}

	if (GreenSlider)
	{
		GreenSlider->SetValue(Color.G);
	}

	if (BlueSlider)
	{
		BlueSlider->SetValue(Color.B);
	}

	bSynchronizingSliders = false;
	UpdateRGBValueTexts();
}

void UNSGameplaySettingWidget::UpdateRGBValueTexts()
{
	if (RedValueText)
	{
		RedValueText->SetText(
			FText::AsNumber(
				FMath::RoundToInt(
					PendingCrosshairColor.R * 255.0f)));
	}

	if (GreenValueText)
	{
		GreenValueText->SetText(
			FText::AsNumber(
				FMath::RoundToInt(
					PendingCrosshairColor.G * 255.0f)));
	}

	if (BlueValueText)
	{
		BlueValueText->SetText(
			FText::AsNumber(
				FMath::RoundToInt(
					PendingCrosshairColor.B * 255.0f)));
	}
}

void UNSGameplaySettingWidget::UpdateApplyButtonState()
{
	const UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem();

	if (!Settings)
	{
		return;
	}

	const bool bHasPendingChanges =
		!PendingCrosshairColor.Equals(
			Settings->GetCrosshairColor());

	if (ApplyCustomColorButton)
	{
		ApplyCustomColorButton->SetIsEnabled(
			bHasPendingChanges);
	}

	if (ApplyCustomColorText)
	{
		ApplyCustomColorText->SetText(
			bHasPendingChanges
				? NSLOCTEXT(
					"GameplaySettings",
					"ApplyCrosshairColor",
					"적용")
				: NSLOCTEXT(
					"GameplaySettings",
					"AppliedCrosshairColor",
					"적용됨"));
	}
}

void UNSGameplaySettingWidget::InitializeLanguageOptions()
{
	if (!LanguageComboBox)
	{
		return;
	}

	// 재초기화 과정에서 기존 선택 이벤트가 호출되지 않도록 먼저 해제
	LanguageComboBox->OnSelectionChanged.RemoveDynamic(
		this,
		&ThisClass::OnLanguageSelectionChanged);

	LanguageComboBox->ClearOptions();
	LanguageComboBox->AddOption(TEXT("한국어"));
	LanguageComboBox->AddOption(TEXT("English"));

	const UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem();

	const bool bIsEnglish =
		Settings &&
		Settings->GetLanguageCode().StartsWith(
			TEXT("en"));

	LanguageComboBox->SetSelectedOption(
		bIsEnglish
			? TEXT("English")
			: TEXT("한국어"));

	LanguageComboBox->OnSelectionChanged.AddUniqueDynamic(
		this,
		&ThisClass::OnLanguageSelectionChanged);
}

void UNSGameplaySettingWidget::CenterSelectedOptionText(UComboBoxString* ComboBox)
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

	FSlateFontInfo FontInfo =
		ComboBox->GetFont();

	// GenerateLanguageOptionWidget()에서 사용하는 글자 크기
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

	// WBP의 SizeBox_387 Width Override가 250
	const float ComboBoxWidth =
		CachedWidth > 0.0f
			? CachedWidth
			: 360.0f;

	const FMargin ButtonPadding =
		ComboBox->GetWidgetStyle()
		.ComboButtonStyle
		.ContentPadding;

	// GenerateLanguageOptionWidget()의 Border 왼쪽 Padding
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

void UNSGameplaySettingWidget::ApplyCrosshairSettingsLayout()
{
	auto CenterButtonText =
		[this](const FName TextWidgetName)
		{
			UTextBlock* ButtonText =
				Cast<UTextBlock>(
					GetWidgetFromName(TextWidgetName));

			if (!ButtonText)
			{
				return;
			}

			ButtonText->SetJustification(
				ETextJustify::Center);

			if (UButtonSlot* ButtonSlot =
				Cast<UButtonSlot>(
					ButtonText->Slot))
			{
				// TextBlock이 버튼 전체 너비를 차지하게 한 뒤
				// 텍스트를 그 영역의 중앙에 배치한다.
				ButtonSlot->SetHorizontalAlignment(
					HAlign_Fill);

				ButtonSlot->SetVerticalAlignment(
					VAlign_Center);

				ButtonSlot->SetPadding(
					FMargin(0.0f));
			}
		};

	CenterButtonText(TEXT("TextBlock_472"));
	CenterButtonText(TEXT("TextBlock_574"));
}

void UNSGameplaySettingWidget::MakeComboBoxPopupTransparent(UComboBoxString* ComboBox)
{
	if (!ComboBox)
	{
		return;
	}

	FComboBoxStyle ComboBoxStyle = ComboBox->GetWidgetStyle();
	FTableRowStyle ItemStyle = ComboBox->GetItemStyle();

	auto MakeBrushTransparent = [](FSlateBrush& Brush)
	{
		Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
		Brush.TintColor = FSlateColor(FLinearColor::Transparent);
	};

	// 콤보박스를 열었을 때 나타나는 외부 회색 배경
	MakeBrushTransparent(
		ComboBoxStyle.ComboButtonStyle.MenuBorderBrush);

	// 목록 항목의 기본 회색 배경
	MakeBrushTransparent(
		ItemStyle.EvenRowBackgroundBrush);

	MakeBrushTransparent(
		ItemStyle.OddRowBackgroundBrush);

	ComboBox->SetWidgetStyle(ComboBoxStyle);
	ComboBox->SetItemStyle(ItemStyle);
}
