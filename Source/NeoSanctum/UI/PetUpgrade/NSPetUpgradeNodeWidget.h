#pragma once
#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "NeoSanctum/Type/NSPetUpgradeMessageTypes.h"
#include "NSPetUpgradeNodeWidget.generated.h"

struct FStreamableHandle;
class UImage;
class UTextBlock;
class UNSPetUpgradeNodeWidget;

// 강화/선택 요청
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNSPetUpgradeRequestedSignature, FGameplayTag, CompanionTag, FGameplayTag, NodeTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSPetSelectRequestedSignature, FGameplayTag, CompanionTag);
// 호버 시 상세 패널에 전달 (노드 데이터 + 자신)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNSPetNodeHoveredSignature, const FNSPetUpgradeNodeViewData&, NodeData, UNSPetUpgradeNodeWidget*, HoveredNode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSPetNodeUnhoveredSignature, FGameplayTag, NodeTag);

UCLASS()
class NEOSANCTUM_API UNSPetUpgradeNodeWidget : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	bool ApplyNodeData(const FNSPetUpgradeNodeViewData& NodeData);
	FGameplayTag GetBoundNodeTag() const { return BoundNodeTag; }

	FNSPetUpgradeRequestedSignature OnUpgradeRequested;
	FNSPetSelectRequestedSignature  OnSelectRequested;
	FNSPetNodeHoveredSignature      OnNodeHovered;
	FNSPetNodeUnhoveredSignature    OnNodeUnhovered;

	// 상태별 시각(잠금/선택/만렙)은 BP에서
	UFUNCTION(BlueprintImplementableEvent, Category="Pet Upgrade")
	void OnNodeStateUpdated(const FNSPetUpgradeNodeViewData& NodeData);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pet Upgrade")
	FGameplayTag BoundNodeTag;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Pet Upgrade")
	FNSPetUpgradeNodeViewData CurrentNodeData;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> NodeIcon;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> NodeHoveredFrameImage;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> NodePressedFrameImage;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

private:
	void ApplyIcon(const TSoftObjectPtr<UTexture2D>& Icon);
	void HandleClickedInternal();
	void HandleHoveredInternal();
	void HandleUnhoveredInternal();
	void HandlePressedInternal();
	void HandleReleasedInternal();

	TSharedPtr<FStreamableHandle> IconLoadHandle;
};