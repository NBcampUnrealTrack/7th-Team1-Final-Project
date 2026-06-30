// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/Type/NSPetUpgradeMessageTypes.h"
#include "NSPetUpgradeNodeWidget.generated.h"


class UButton;
class UTextBlock;

/**
 * 펫 강화 트리의 노드 하나를 표시하는 위젯입니다.
 * 백엔드나 GMS를 직접 참조하지 않고 전달받은 ViewData만 사용합니다.
 */

// 노드가 부모 UI에 강화 요청을 전달할 때 사용하는 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNSPetUpgradeRequestedSignature, FGameplayTag, CompanionTag, FGameplayTag, NodeTag);
UCLASS()
class NEOSANCTUM_API UNSPetUpgradeNodeWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
	
public:
	/**
	 * 이 위젯에 해당하는 노드 데이터를 적용합니다.
	 * BoundNodeTag가 일치하지 않으면 적용하지 않습니다.
	 */
	
	bool ApplyNodeData(
		const FNSPetUpgradeNodeViewData& NodeData);

	
	// 이 위젯이 담당하는 노드 태그 반환
	FGameplayTag GetBoundNodeTag() const
	{
		return BoundNodeTag;
	}
	
	//노드 강화 버튼을 눌렀을때 발생
	FNSPetUpgradeRequestedSignature OnUpgradeRequested;
	
private:
	// 강화 버튼 클릭 처리
	UFUNCTION()
	void RequestUpgrade();

protected:
	//이 위젯이 담당할 강화 노드 태그입니다. 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pet Upgrade")
	FGameplayTag BoundNodeTag;

	//가장 최근에 적용된 노드 표시 데이터입니다
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Pet Upgrade")
	FNSPetUpgradeNodeViewData CurrentNodeData;
	
	// 강화 요청 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> UpgradeButton;

	// 현재 레벨과 최대 레벨 표시
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LevelText;
	
	// 버튼 클릭 이벤트 연결
	virtual void NativeConstruct() override;

	// 버튼 클릭 이벤트 해제
	virtual void NativeDestruct() override;
};
