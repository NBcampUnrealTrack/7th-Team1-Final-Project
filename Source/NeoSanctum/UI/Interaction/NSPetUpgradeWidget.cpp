// Copyright 2026 One Team. All rights reserved.

#include "NSPetUpgradeWidget.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Data/AI/NSCompanionDefinition.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Character/Component/NSCompanionProgressionComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Data/AI/NSCompanionCatalog.h"
#include "Components/Image.h"

void UNSPetUpgradeWidget::OpenForInteractor(APlayerController* Interactor)
{
	if (!IsValid(Interactor))
	{
		return;
	}

	OwningController = Interactor;
	
	const ANSPlayerState* NSPlayerState =
		Interactor->GetPlayerState<ANSPlayerState>();
	
	if (!IsValid(NSPlayerState))
	{
		return;
	}
	
	UNSCompanionDefinition* CompanionDefinition =
		NSPlayerState->GetCurrentCompanionDefinition();
	
	if (!IsValid(CompanionDefinition))
	{
		return;
	}
	
	//현재 플레이어가 선택한 펫의 강화 노드 UI
	SetCompanionDefinition(CompanionDefinition);
	BindProgressChanged();
	
	AddToViewport();

	if (ANSPlayerController* PlayerController =
	Cast<ANSPlayerController>(Interactor))
	{
		PlayerController->EnterPetUpgradeInputMode(this);
	}
}

void UNSPetUpgradeWidget::CloseWidget()
{
	UnbindProgressChanged();

	if (ANSPlayerController* PlayerController =
		Cast<ANSPlayerController>(
			OwningController.Get()))
	{
		PlayerController->ExitPetUpgradeInputMode();
	}

	RemoveFromParent();
}
void UNSPetUpgradeWidget::SetCompanionDefinition(UNSCompanionDefinition* NewDefinition)
{
	CurrentCompanionDefinition = NewDefinition;
	SelectedNodeTag = FGameplayTag();
	SelectedNodeIndex = INDEX_NONE;
	
	RefreshUpgradeInfo();
}

void UNSPetUpgradeWidget::SelectUpgradeNodeByIndex(int32 NodeIndex)
{
	if (!CurrentCompanionDefinition ||
	!CurrentCompanionDefinition->UpgradeNodes.IsValidIndex(NodeIndex))
	{
		return;
	}

	SelectedNodeIndex = NodeIndex;
	SelectedNodeTag =
		CurrentCompanionDefinition->UpgradeNodes[NodeIndex].NodeTag;
	RefreshSelectedNodeInfo();
}

void UNSPetUpgradeWidget::SelectUpgradeNodeByTag(FGameplayTag NodeTag)
{
	if (!NodeTag.IsValid())
	{
		return;
	}
	
	APlayerController* PlayerController =
		OwningController.Get();
	
	if (!PlayerController)
	{
		return;
	}
	
	ANSPlayerState* NSPlayerState =
		PlayerController->GetPlayerState<ANSPlayerState>();
	
	if (!NSPlayerState)
	{
		return;
	}
	
	UNSCompanionProgressionComponent* ProgressionComponent =
		NSPlayerState->GetCompanionProgressionComponent();
	
	if (!ProgressionComponent ||
		!ProgressionComponent->Catalog)
	{
		return;
	}
	
	for (UNSCompanionDefinition* Definition :
		ProgressionComponent->Catalog->Companions)
	{
		if (!Definition)
		{
			continue;
		}
		
		for (int32 Index = 0;
			Index < Definition->UpgradeNodes.Num();
			++Index)
		{
			if (Definition->UpgradeNodes[Index].NodeTag != NodeTag)
			{
				continue;
			}
			
			CurrentCompanionDefinition = Definition;
			SelectedNodeIndex = Index;
			SelectedNodeTag = NodeTag;
			
			RefreshSelectedNodeInfo();
			return;
		}
	}
}

void UNSPetUpgradeWidget::RefreshUpgradeInfo()
{
	if (DefaultSelectedNodeTag.IsValid())
	{
		SelectUpgradeNodeByTag(
			DefaultSelectedNodeTag);

		if (SelectedNodeIndex != INDEX_NONE)
		{
			return;
		}
	}

	if (CurrentCompanionDefinition &&
		!CurrentCompanionDefinition->UpgradeNodes.IsEmpty())
	{
		SelectUpgradeNodeByIndex(0);
	}
}
void UNSPetUpgradeWidget::RefreshSelectedNodeInfo()
{
	if (!CurrentCompanionDefinition ||
		!CurrentCompanionDefinition->UpgradeNodes.IsValidIndex(
			SelectedNodeIndex))
	{
		return;
	}

	const FNSCompanionUpgradeNode& UpgradeNode =
		CurrentCompanionDefinition->UpgradeNodes[SelectedNodeIndex];

	ANSPlayerState* NSPlayerState = nullptr;
	UNSPlayerProgressComponent* ProgressComponent = nullptr;
	UNSCompanionProgressionComponent* CompanionProgression = nullptr;

	if (APlayerController* PlayerController =
		OwningController.Get())
	{
		NSPlayerState =
			PlayerController->GetPlayerState<ANSPlayerState>();
	}

	if (NSPlayerState)
	{
		ProgressComponent =
			NSPlayerState->GetProgressComponent();

		CompanionProgression =
			NSPlayerState->GetCompanionProgressionComponent();
	}

	const int32 CurrentLevel =
		ProgressComponent
			? ProgressComponent->GetCompanionNodeLevel(
				UpgradeNode.NodeTag)
			: 0;
	
	const int64 UpgradeCost =
	UpgradeNode.BaseUpgradeCost +
	(CurrentLevel * UpgradeNode.CostIncreasePerLevel);

	const int64 CurrentCurrency =
		ProgressComponent
			? ProgressComponent->GetCommonCurrency()
			: 0;

	const bool bCanAfford =
		CurrentCurrency >= UpgradeCost;

	bool bUnlocked = true;
	bool bRequirementDataValid = true;
	int32 RequiredUpgradeTotal = 0;
	UNSCompanionDefinition* RequiredDefinition = nullptr;

	// 현재 노드가 속한 Definition의 선행 해금 조건을 검사한다.
	if (CurrentCompanionDefinition->RequiredCompanionTag.IsValid() &&
		CurrentCompanionDefinition->RequiredUpgradeCount > 0)
	{
		if (CompanionProgression &&
			CompanionProgression->Catalog)
		{
			RequiredDefinition =
				CompanionProgression->Catalog->FindByTag(
					CurrentCompanionDefinition
						->RequiredCompanionTag);
		}

		if (!RequiredDefinition || !ProgressComponent)
		{
			bUnlocked = false;
			bRequirementDataValid = false;
		}
		else
		{
			for (const FNSCompanionUpgradeNode& RequiredNode :
				RequiredDefinition->UpgradeNodes)
			{
				RequiredUpgradeTotal +=
					ProgressComponent->GetCompanionNodeLevel(
						RequiredNode.NodeTag);
			}

			bUnlocked =
				RequiredUpgradeTotal >=
				CurrentCompanionDefinition
					->RequiredUpgradeCount;
		}
	}

	if (SelectedNodeNameText)
	{
		const FText NodeName =
			UpgradeNode.DisplayName.IsEmpty()
				? FText::FromName(
					UpgradeNode.NodeTag.GetTagName())
				: UpgradeNode.DisplayName;

		SelectedNodeNameText->SetText(NodeName);
	}

	if (SelectedNodeLevelText)
	{
		SelectedNodeLevelText->SetText(
			FText::Format(
				NSLOCTEXT(
					"PetUpgradeWidget",
					"NodeLevel",
					"레벨 {0} / {1}"),
				FText::AsNumber(CurrentLevel),
				FText::AsNumber(UpgradeNode.MaxLevel)));
	}

	if (SelectedNodeEffectText)
	{
		SelectedNodeEffectText->SetText(
			UpgradeNode.Description.IsEmpty()
				? FText::Format(
					NSLOCTEXT(
						"PetUpgradeWidget",
						"MagnitudePerLevel",
						"레벨당 증가량: {0}"),
					FText::AsNumber(
						UpgradeNode.MagnitudePerLevel))
				: UpgradeNode.Description);
	}

	if (SelectedNodeIcon)
	{
		if (UpgradeNode.Icon.IsNull())
		{
			SelectedNodeIcon->SetVisibility(
				ESlateVisibility::Collapsed);
		}
		else
		{
			SelectedNodeIcon->SetBrushFromSoftTexture(
				UpgradeNode.Icon,
				false);

			SelectedNodeIcon->SetVisibility(
				ESlateVisibility::HitTestInvisible);
		}
	}

	const bool bMaxLevel =
	CurrentLevel >= UpgradeNode.MaxLevel;

	if (SelectedNodeConditionText)
	{
		if (bMaxLevel)
		{
			SelectedNodeConditionText->SetText(
				NSLOCTEXT(
					"PetUpgradeWidget",
					"MaxLevel",
					"최대 레벨"));
		}
		else if (!bRequirementDataValid)
		{
			SelectedNodeConditionText->SetText(
				NSLOCTEXT(
					"PetUpgradeWidget",
					"InvalidRequirement",
					"해금 정보를 확인할 수 없습니다."));
		}
		else if (!bUnlocked)
		{
			const FText RequiredName =
				RequiredDefinition->DisplayName.IsEmpty()
					? FText::FromName(
						RequiredDefinition
							->CompanionTag.GetTagName())
					: RequiredDefinition->DisplayName;

			SelectedNodeConditionText->SetText(
				FText::Format(
					NSLOCTEXT(
						"PetUpgradeWidget",
						"LockedRequirement",
						"{0} 강화 {1}회 필요 (현재 {2}회)"),
					RequiredName,
					FText::AsNumber(
						CurrentCompanionDefinition
							->RequiredUpgradeCount),
					FText::AsNumber(
						RequiredUpgradeTotal)));
		}
		else if (!bCanAfford)
		{
			SelectedNodeConditionText->SetText(
				NSLOCTEXT(
					"PetUpgradeWidget",
					"NotEnoughCurrency",
					"재화가 부족합니다."));
		}
		else
		{
			SelectedNodeConditionText->SetText(
				NSLOCTEXT(
					"PetUpgradeWidget",
					"CanUpgrade",
					"강화 가능"));
		}
	}

	if (UpgradeCostText)
	{
		UpgradeCostText->SetText(
			UpgradeCost <= 0
				? NSLOCTEXT(
					"PetUpgradeWidget",
					"FreeUpgrade",
					"비용: 무료")
				: FText::Format(
					NSLOCTEXT(
						"PetUpgradeWidget",
						"UpgradeCost",
						"비용: {0} / 보유: {1}"),
					FText::AsNumber(UpgradeCost),
					FText::AsNumber(CurrentCurrency)));
	}

	if (UpgradeButton)
	{
		UpgradeButton->SetIsEnabled(
			bUnlocked &&
			bRequirementDataValid &&
			!bMaxLevel &&
			bCanAfford);
	}
}

void UNSPetUpgradeWidget::HandleUpgradeClicked()
{
	if (!SelectedNodeTag.IsValid())
	{
		return;
	}
	
	APlayerController* PlayerController =
		OwningController.Get();
	
	if (!PlayerController)
	{
		return;
	}
	
	ANSPlayerState* NSPlayerState =
		PlayerController->GetPlayerState<ANSPlayerState>();
	
	if (!NSPlayerState)
	{
		return;
	}
	
	UNSCompanionProgressionComponent* ProgressionComponent =
		NSPlayerState->GetCompanionProgressionComponent();
	
	if (!ProgressionComponent)
	{
		return;
	}

	
	//실제 레벨과 최대레벨 검증은 서버에서 처리
	ProgressionComponent->Server_TryUpgrade(SelectedNodeTag);
}

void UNSPetUpgradeWidget::HandleCloseClicked()
{
	CloseWidget();
}

void UNSPetUpgradeWidget::BindProgressChanged()
{
	UnbindProgressChanged();

	APlayerController* PlayerController = OwningController.Get();
	if (!PlayerController)
	{
		return;
	}

	ANSPlayerState* NSPlayerState =
		PlayerController->GetPlayerState<ANSPlayerState>();

	if (!NSPlayerState)
	{
		return;
	}

	UNSPlayerProgressComponent* ProgressComponent =
		NSPlayerState->GetProgressComponent();

	if (!ProgressComponent)
	{
		return;
	}

	BoundProgressComponent = ProgressComponent;

	ProgressComponent->OnProgressChanged.AddUObject(
		this,
		&UNSPetUpgradeWidget::HandleProgressChanged);
}

void UNSPetUpgradeWidget::UnbindProgressChanged()
{
	if (UNSPlayerProgressComponent* ProgressComponent =
		BoundProgressComponent.Get())
	{
		ProgressComponent->OnProgressChanged.RemoveAll(this);
	}

	BoundProgressComponent.Reset();
}

void UNSPetUpgradeWidget::HandleProgressChanged()
{
	RefreshSelectedNodeInfo();
}

void UNSPetUpgradeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UpgradeButton)
	{
		UpgradeButton->OnClicked().AddUObject(
			this,
			&UNSPetUpgradeWidget::HandleUpgradeClicked);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked().AddUObject(
			this,
			&UNSPetUpgradeWidget::HandleCloseClicked);
	}
}


void UNSPetUpgradeWidget::NativeDestruct()
{
	UnbindProgressChanged();

	if (UpgradeButton)
	{
		UpgradeButton->OnClicked().RemoveAll(this);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}
