// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/UI/Common/NSButtonBase.h"
#include "Engine/DataTable.h"
#include "NSCharacterSelectSkillSlotWidget.generated.h"

class UCommonTextBlock;
class UImage;
class UTexture2D;
class UWidget;
struct FNSInputDisplayData;

/**
 * 캐릭터 선택 화면에서 어떤 스킬 슬롯인지 구분.
 *
 * 현재 입력 배치는 기본 공격, Q, E, 우클릭 순서.
 * 실제 입력 표시는 FNSInputDisplayData에서 가져옴.
 */
UENUM(BlueprintType)
enum class ENSCharacterSelectSkillSlot : uint8
{
	BaseAttack UMETA(DisplayName = "기본 공격"),
	Skill1 UMETA(DisplayName = "스킬 1"),
	Skill2 UMETA(DisplayName = "스킬 2"),
	Skill3 UMETA(DisplayName = "스킬 3")
};

DECLARE_MULTICAST_DELEGATE_OneParam(FNSCharacterSelectSkillSlotEvent, ENSCharacterSelectSkillSlot);

/**
 * 캐릭터 선택 화면의 스킬 슬롯 하나를 표시하고 상호작용 이벤트를 전달하는 버튼 위젯.
 *
 * 스킬 아이콘과 입력 아이콘, 현재 상세 패널에 표시중인 상태를 표현하고
 * 호버 시작/종료 및 클릭 이벤트를 부모 캐릭터 선택 위젯에 알려줌.
 *
 * 어떤 이벤트로 상세 정보를 변경할지는 부모 위젯이 결정하며,
 * 캐릭터별 스킬 조회와 설명 및 스탯 표시는 담당하지 않음.
 */
UCLASS()
class NEOSANCTUM_API UNSCharacterSelectSkillSlotWidget : public UNSButtonBase
{
	GENERATED_BODY()

public:
	// 선택한 캐릭터의 스킬 입력 표시 데이터를 슬롯에 적용.
	void SetupSlot(
		ENSCharacterSelectSkillSlot InSlotType,
		const FDataTableRowHandle& InSkillUIDataRow,
		const FNSInputDisplayData& InInputDisplayData
	);

	// 현재 상세 패널에 표시 중인 슬롯인지 표현.
	void SetSlotPreviewed(bool bPreviewed);

	// 이전 캐릭터의 스킬 데이터가 남지 않도록 슬롯을 비움.
	void ResetSlot();

	ENSCharacterSelectSkillSlot GetSlotType() const { return SlotType; }
	const FDataTableRowHandle& GetSkillUIDataRow() const { return SkillUIDataRow; }

	UTexture2D* GetInputIconTexture() const { return CurrentInputIconTexture.Get(); }

	FNSCharacterSelectSkillSlotEvent OnSlotHovered;
	FNSCharacterSelectSkillSlotEvent OnSlotUnhovered;
	FNSCharacterSelectSkillSlotEvent OnSlotClicked;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> SkillIconImage;

	// 데이터에서 입력 아이콘 대신 텍스트 표시를 선택했을 때 사용.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Input")
	TObjectPtr<UCommonTextBlock> InputText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Input")
	TObjectPtr<UImage> InputIconImage;

	// 호버가 끝난 뒤에도 현재 상세 정보 대상임을 표시.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UWidget> PreviewedIndicator;

private:
	void HandleHovered();
	void HandleUnhovered();
	void HandleClicked();
	void ClearSlot();

	ENSCharacterSelectSkillSlot SlotType = ENSCharacterSelectSkillSlot::BaseAttack;

	UPROPERTY(Transient)
	FDataTableRowHandle SkillUIDataRow;

	// 상세 패널에서도 같은 입력 아이콘을 사용하므로 현재 슬롯의 텍스처를 보관.
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CurrentInputIconTexture;
};
