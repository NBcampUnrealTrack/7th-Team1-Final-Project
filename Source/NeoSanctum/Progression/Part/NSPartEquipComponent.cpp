// Copyright 2026 One Team. All rights reserved.

#include "NSPartEquipComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Data/CommonUpgrade/NSCommonUpgradeUtilityHelper.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"
#include "NeoSanctum/Progression/Part/NSDroppedPart.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"
#include "NeoSanctum/Tag/NSGameplayTags_Part.h"

UNSPartEquipComponent::UNSPartEquipComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNSPartEquipComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNSPartEquipComponent, EquippedParts);
	DOREPLIFETIME_CONDITION(UNSPartEquipComponent, ShopStock, COND_OwnerOnly);
}

void UNSPartEquipComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 진행 중 비동기 로드 콜백이 파괴 후 호출되지 않도록 취소
	for (TPair<FGameplayTag, TSharedPtr<FStreamableHandle>>& Pair : EffectLoadHandles)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->CancelHandle();
		}
	}
	for (TPair<FGameplayTag, TSharedPtr<FStreamableHandle>>& Pair : AbilityLoadHandles)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->CancelHandle();
		}
	}
	EffectLoadHandles.Empty();
	AbilityLoadHandles.Empty();

	Super::EndPlay(EndPlayReason);
}

void UNSPartEquipComponent::EquipPart(const FNSPartData& NewPart, TOptional<FVector> DropLocationOverride)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, NewPart);
	if (!Def)
	{
		return;
	}

	const FPrimaryAssetId DefId = Def->GetPrimaryAssetId();
	const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(this, DefId);
	if (!Row)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipComp] EquipPart: row 없음 (DefId=%s)"), *DefId.ToString());
		return;
	}
	const FGameplayTag Slot = Row->PartSlot;

	DropPartInSlot(Slot, DropLocationOverride);

	FNSPartData Stored = NewPart;
	Stored.Slot = Slot;

	if (FNSPartData* Existing = FindPart(Slot))
	{
		*Existing = Stored;
	}
	else
	{
		EquippedParts.Add(Stored);
	}

	ApplyPartEffect(Slot);
	GrantAbilities(Slot);

	OnPartChanged.Broadcast(Slot, Stored);
}

void UNSPartEquipComponent::ClearAll()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	TArray<FGameplayTag> ClearedSlots;
	for (const FNSPartData& Part : EquippedParts)
	{
		RemovePartEffects(Part.Slot);
		ClearedSlots.Add(Part.Slot);
	}
	EquippedParts.Empty();

	// 비주얼 등 구독자가 슬롯 비움을 반영하도록 빈 파츠로 알림
	for (const FGameplayTag& Slot : ClearedSlots)
	{
		OnPartChanged.Broadcast(Slot, FNSPartData());
	}
}

void UNSPartEquipComponent::CopyRunStateFrom(const UNSPartEquipComponent* Source)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!Source)
	{
		return;
	}

	// 데이터만 이관, 핸들은 이전 ASC 기준이라 무효이므로 복사하지 않음 (ReapplyAll에서 새로 발급)
	EquippedParts = Source->EquippedParts;
}

void UNSPartEquipComponent::ReapplyAll()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	// 이전 ASC 기준 핸들 폐기 후 슬롯별 GE/GA 재발급
	ActiveGEHandles.Empty();
	GrantedAbilityHandlesBySlot.Empty();

	for (const FNSPartData& Part : EquippedParts)
	{
		ApplyPartEffect(Part.Slot);
		GrantAbilities(Part.Slot);
	}
}

bool UNSPartEquipComponent::HasEquippedPart(FGameplayTag Slot) const
{
	return FindPart(Slot) != nullptr;
}

const FNSPartData* UNSPartEquipComponent::GetEquippedPart(FGameplayTag Slot) const
{
	return FindPart(Slot);
}

FNSPartData* UNSPartEquipComponent::FindPart(FGameplayTag Slot)
{
	return EquippedParts.FindByPredicate([Slot](const FNSPartData& P) { return P.Slot==Slot; });
}

const FNSPartData* UNSPartEquipComponent::FindPart(FGameplayTag Slot) const
{
	return EquippedParts.FindByPredicate([Slot](const FNSPartData& P) {return P.Slot == Slot;});
}

// ================================================================
// 드롭 / 효과 제거
// ================================================================

void UNSPartEquipComponent::DropPartInSlot(FGameplayTag Slot, TOptional<FVector> LocationOverride)
{
	FNSPartData* Existing = FindPart(Slot);
	if (!Existing)
	{
		return;
	}

	const FNSPartData Dropped = *Existing;

	RemovePartEffects(Slot);
	EquippedParts.RemoveAll([Slot](const FNSPartData& P) { return P.Slot==Slot; });

	// 파츠 교체할때 위치 변경
	const APlayerState* PS = Cast<APlayerState>(GetOwner());
	const APawn* Pawn = PS ? PS->GetPawn() : nullptr;
	const FVector Location = LocationOverride.IsSet() ? LocationOverride.GetValue()
		: (Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector);
	
	// 실제로 그 위치에 장착한 하츠 드롭
	SpawnDroppedPart(Dropped, Location);
}

void UNSPartEquipComponent::SpawnDroppedPart(const FNSPartData& Part, const FVector& Location)
{
	ANSDroppedPart::SpawnInWorld(GetWorld(), DroppedPartClass, Part, Location);
}

void UNSPartEquipComponent::RemovePartEffects(FGameplayTag Slot)
{
	RemoveGEForSlot(Slot);
	RemoveAbilitiesForSlot(Slot);

	if (TSharedPtr<FStreamableHandle>* LoadHandle = EffectLoadHandles.Find(Slot))
	{
		if (LoadHandle->IsValid())
		{
			(*LoadHandle)->CancelHandle();
		}
		EffectLoadHandles.Remove(Slot);
	}
	if (TSharedPtr<FStreamableHandle>* LoadHandle = AbilityLoadHandles.Find(Slot))
	{
		if (LoadHandle->IsValid())
		{
			(*LoadHandle)->CancelHandle();
		}
		AbilityLoadHandles.Remove(Slot);
	}
}

void UNSPartEquipComponent::RemoveGEForSlot(FGameplayTag Slot)
{
	FActiveGameplayEffectHandle* Handle = ActiveGEHandles.Find(Slot);
	if (!Handle || !Handle->IsValid())
	{
		ActiveGEHandles.Remove(Slot);
		return;
	}

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		ASC->RemoveActiveGameplayEffect(*Handle);
	}
	ActiveGEHandles.Remove(Slot);
}

void UNSPartEquipComponent::RemoveAbilitiesForSlot(FGameplayTag Slot)
{
	TArray<FGameplayAbilitySpecHandle>* Handles = GrantedAbilityHandlesBySlot.Find(Slot);
	if (!Handles || Handles->Num() == 0)
	{
		GrantedAbilityHandlesBySlot.Remove(Slot);
		return;
	}

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		for (const FGameplayAbilitySpecHandle& Handle : *Handles)
		{
			if (Handle.IsValid())
			{
				ASC->ClearAbility(Handle);
			}
		}
	}
	GrantedAbilityHandlesBySlot.Remove(Slot);
}

// ================================================================
// GE 적용 및 로드
// ================================================================

void UNSPartEquipComponent::ApplyPartEffect(FGameplayTag Slot)
{
	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}
	
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
	if (!Def || Def->EffectClass.IsNull())
	{
		return;
	}
	
	if (TSubclassOf<UGameplayEffect> Loaded = Def->EffectClass.Get())
	{
		Internal_ApplyGE(Slot, Loaded);
		return;
	}
	
	const FSoftObjectPath Path = Def->EffectClass.ToSoftObjectPath();
	TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Path, FStreamableDelegate::CreateUObject(this, &UNSPartEquipComponent::OnEffectLoaded, Slot));
	EffectLoadHandles.Add(Slot, Handle);
}

void UNSPartEquipComponent::Internal_ApplyGE(FGameplayTag Slot, TSubclassOf<UGameplayEffect> GEClass)
{
	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	// 리롤같이 스텟변경이 있는경우 대비용으로 기존 GE먼저 제거
	RemoveGEForSlot(Slot);

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(NSGameplayTags::Part_Value, Part->CurrentValue);
	ActiveGEHandles.Add(Slot, ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data));
}

void UNSPartEquipComponent::OnEffectLoaded(FGameplayTag Slot)
{
	EffectLoadHandles.Remove(Slot);
	
	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}
	
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
	if (!Def)
	{
		return;
	}
	
	TSubclassOf<UGameplayEffect> Loaded = Def->EffectClass.Get();
	if (!Loaded)
	{
		return;
	}
	Internal_ApplyGE(Slot, Loaded);
}

// ================================================================
// GA 부여 및 로드
// ================================================================

void UNSPartEquipComponent::GrantAbilities(FGameplayTag Slot)
{
	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}
	
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
	if (!Def || Def->GrantedAbilities.Num() == 0)
	{
		return;
	}
	
	bool bAllLoaded = true;
	for (const TSoftClassPtr<UGameplayAbility>& AbilityClass : Def->GrantedAbilities)
	{
		if (!AbilityClass.Get())
		{
			bAllLoaded = false;
			break;
		}
	}
	if (bAllLoaded)
	{
		OnAbilitiesLoaded(Slot);
		return;
	}
	
	TArray<FSoftObjectPath> Paths;
	for (const TSoftClassPtr<UGameplayAbility>& AbilityClass : Def->GrantedAbilities)
	{
		if (!AbilityClass.IsNull())
		{
			Paths.Add(AbilityClass.ToSoftObjectPath());
		}
	}
	
	if (Paths.Num() == 0)
	{
		OnAbilitiesLoaded(Slot);
		return;
	}
	
	TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Paths, FStreamableDelegate::CreateUObject(this, &UNSPartEquipComponent::OnAbilitiesLoaded,
			Slot));
	AbilityLoadHandles.Add(Slot, Handle);
}

void UNSPartEquipComponent::OnAbilitiesLoaded(FGameplayTag Slot)
{
	AbilityLoadHandles.Remove(Slot);
	
	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}
	
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
	if (!Def)
	{
		return;
	}
	
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}
	
	RemoveAbilitiesForSlot(Slot);
	
	TArray<FGameplayAbilitySpecHandle>& Handles = GrantedAbilityHandlesBySlot.FindOrAdd(Slot);
	for (const TSoftClassPtr<UGameplayAbility>& AbilityClass : Def->GrantedAbilities)
	{
		TSubclassOf<UGameplayAbility> Loaded = AbilityClass.Get();
		if (!Loaded)
		{
			continue;
		}
		FGameplayAbilitySpec Spec(Loaded, 1, INDEX_NONE, this);
		Handles.Add(ASC->GiveAbility(Spec));
	}
}

// ================================================================
// 리롤 / 등급업
// ================================================================

void UNSPartEquipComponent::RerollStat(FGameplayTag Slot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}

	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
	const FPrimaryAssetId DefId = Def ? Def->GetPrimaryAssetId() : FPrimaryAssetId();
	const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(this, DefId);
	if (!Def || !Row || !Row->bCanReroll)
	{
		return;
	}

	UNSCurrencyComponent* Currency = GetCurrencyComponent();
	const int64 Cost = GetRerollCost(Slot);
	if (!Currency || Cost < 0 || !Currency->TrySpendTemp(Cost))
	{
		Client_NotifyUpgradeResult(Slot, ENSPartUpgradeResult::NotEnoughCurrency, Currency ? Currency->GetTemp() : 0);
		return;
	}

	Part->CurrentValue = RollValueForRarity(Part->CurrentRarity);
	Part->RollCount++;

	ApplyPartEffect(Slot);
	OnPartChanged.Broadcast(Slot, *Part);
	Client_NotifyUpgradeResult(Slot, ENSPartUpgradeResult::RerollDone, Currency->GetTemp());
}

void UNSPartEquipComponent::UpgradeRarity(FGameplayTag Slot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return;
	}
	if (Part->CurrentRarity == ENSPartRarity::Legendary)
	{
		return;
	}

	const FNSPartUpgradeRow* UpgradeRow = NSPartUtils::ResolvePartUpgradeRow(this, Part->CurrentRarity);
	if (!UpgradeRow)
	{
		return;
	}

	UNSCurrencyComponent* Currency = GetCurrencyComponent();
	if (!Currency || !Currency->TrySpendTemp(UpgradeRow->UpgradeCost))
	{
		Client_NotifyUpgradeResult(Slot, ENSPartUpgradeResult::NotEnoughCurrency, Currency ? Currency->GetTemp() : 0);
		return;
	}

	if (FMath::FRand() > UpgradeRow->UpgradeSuccessChance)
	{
		Client_NotifyUpgradeResult(Slot, ENSPartUpgradeResult::UpgradeFail, Currency->GetTemp());
		return;
	}

	Part->CurrentRarity = static_cast<ENSPartRarity>(static_cast<uint8>(Part->CurrentRarity) + 1);
	Part->RollCount = 0;

	Part->CurrentValue = RollValueForRarity(Part->CurrentRarity);

	ApplyPartEffect(Slot);
	OnPartChanged.Broadcast(Slot, *Part);
	Client_NotifyUpgradeResult(Slot, ENSPartUpgradeResult::UpgradeSuccess, Currency->GetTemp());
}

float UNSPartEquipComponent::RollValueForRarity(ENSPartRarity Rarity) const
{
	const FNSPartUpgradeRow* Row = NSPartUtils::ResolvePartUpgradeRow(this, Rarity);
	if (!Row)
	{
		return 0.f;
	}

	return FMath::RandRange(Row->ValueRange.Min, Row->ValueRange.Max);
}

int64 UNSPartEquipComponent::GetRerollCost(FGameplayTag Slot) const
{
	const FNSPartData* Part = FindPart(Slot);
	if (!Part)
	{
		return -1;
	}

	const FNSPartUpgradeRow* Row = NSPartUtils::ResolvePartUpgradeRow(this, Part->CurrentRarity);
	if (!Row)
	{
		return -1;
	}

	const int64 BaseCost = Row->RerollBaseCost + Row->RerollCostIncrement * Part->RollCount;
	// 파츠 구매 가격 할인(GetShopPrice)과는 별도 NodeId — 레벨을 따로 올릴 수 있어야 함
	return ApplyPartDiscount(NSCommonUpgradeUtility::NodeId_PartRerollDiscount, BaseCost);
}

int64 UNSPartEquipComponent::ApplyPartDiscount(FName UtilityNodeId, int64 BaseCost) const
{
	if (BaseCost <= 0)
	{
		return BaseCost;
	}

	const AActor* Owner = GetOwner();
	const UNSPlayerProgressComponent* ProgressComp =
		Owner ? Owner->FindComponentByClass<UNSPlayerProgressComponent>() : nullptr;
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);

	const double Percent = NSCommonUpgradeUtility::GetPercent(DataSubsystem, ProgressComp, UtilityNodeId);
	if (Percent == 0.0)
	{
		return BaseCost;
	}

	const int64 Discounted = FMath::FloorToInt64(static_cast<double>(BaseCost) * (1.0 + Percent * 0.01));
	// TrySpendTemp(0)은 항상 실패하므로, 100% 할인이어도 최소 1은 받아야 리롤이 성립함
	return FMath::Max<int64>(Discounted, 1);
}

int64 UNSPartEquipComponent::GetUpgradeCost(FGameplayTag Slot) const
{
	const FNSPartData* Part = FindPart(Slot);
	if (!Part || Part->CurrentRarity == ENSPartRarity::Legendary)
	{
		return -1;
	}

	const FNSPartUpgradeRow* Row = NSPartUtils::ResolvePartUpgradeRow(this, Part->CurrentRarity);
	if (!Row)
	{
		return -1;
	}
	return Row->UpgradeCost;
}

float UNSPartEquipComponent::GetUpgradeChance(FGameplayTag Slot) const
{
	const FNSPartData* Part = FindPart(Slot);
	if (!Part || Part->CurrentRarity == ENSPartRarity::Legendary)
	{
		return -1.f;
	}

	const FNSPartUpgradeRow* Row = NSPartUtils::ResolvePartUpgradeRow(this, Part->CurrentRarity);
	if (!Row)
	{
		return -1.f;
	}
	return Row->UpgradeSuccessChance;
}

void UNSPartEquipComponent::Client_NotifyUpgradeResult_Implementation(FGameplayTag Slot, ENSPartUpgradeResult Result, int64 NewTempBalance)
{
	OnUpgradeResult.Broadcast(Slot, Result, NewTempBalance);
}

// ================================================================
// 인런 상점
// ================================================================

void UNSPartEquipComponent::GenerateShopStock()
{
	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(GetOwner());
	if (!DataSS)
	{
		return;
	}

	ShopStock.Reset();

	for (const TPair<FGameplayTag, FNSPartSlotRow>& SlotPair : DataSS->GetAllSlotRows())
	{
		// 이 부위의 등장 후보 (카탈로그 캐시는 bEnabled 필터 완료 상태)
		TArray<const FNSPartDefinitionRow*> Candidates;
		for (const TPair<FPrimaryAssetId, FNSPartDefinitionRow>& PartPair : DataSS->GetAllPartRows())
		{
			if (PartPair.Value.PartSlot == SlotPair.Key)
			{
				Candidates.Add(&PartPair.Value);
			}
		}
		if (Candidates.Num() == 0)
		{
			continue;
		}

		for (int32 Index = 0; Index < StockCountPerSlot; ++Index)
		{
			// 중복 허용 랜덤 선택
			const FNSPartDefinitionRow* Pick = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];

			FNSPartData Item;
			Item.DefinitionPtr = Pick->Definition;
			Item.Slot = SlotPair.Key;
			Item.CurrentRarity = RollShopRarity();

			Item.CurrentValue = RollValueForRarity(Item.CurrentRarity);

			ShopStock.Add(Item);
		}
	}

	bShopStockGenerated = true;
	
	OnShopStockChanged.Broadcast();
}

ENSPartRarity UNSPartEquipComponent::RollShopRarity() const
{
	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(GetOwner());
	if (!DataSS)
	{
		return ENSPartRarity::Common;
	}

	float TotalWeight = 0.f;
	for (const TPair<ENSPartRarity, FNSPartUpgradeRow>& Pair : DataSS->GetAllPartUpgradeRows())
	{
		TotalWeight += FMath::Max(Pair.Value.ShopWeight, 0.f);
	}
	if (TotalWeight <= 0.f)
	{
		return ENSPartRarity::Common;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (const TPair<ENSPartRarity, FNSPartUpgradeRow>& Pair : DataSS->GetAllPartUpgradeRows())
	{
		Roll -= FMath::Max(Pair.Value.ShopWeight, 0.f);
		if (Roll <= 0.f)
		{
			return Pair.Key;
		}
	}
	return ENSPartRarity::Common;
}

int64 UNSPartEquipComponent::GetShopPrice(ENSPartRarity Rarity) const
{
	const FNSPartUpgradeRow* Row = NSPartUtils::ResolvePartUpgradeRow(this, Rarity);
	if (!Row)
	{
		return -1;
	}
	return Row->ShopPrice;
}

void UNSPartEquipComponent::OnRep_ShopStock()
{
	OnShopStockChanged.Broadcast();
}

// ================================================================
// 조회 헬퍼
// ================================================================

UAbilitySystemComponent* UNSPartEquipComponent::GetOwnerASC() const
{
	APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (!PS)
	{
		return nullptr;
	}
	
	APawn* Pawn = PS->GetPawn();
	if (!Pawn)
	{
		return nullptr;
	}
	
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
	if (!ASI)
	{
		return nullptr;
	}
	return ASI->GetAbilitySystemComponent();
}

UNSCurrencyComponent* UNSPartEquipComponent::GetCurrencyComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	return Owner->FindComponentByClass<UNSCurrencyComponent>();
}

// ================================================================
// 리플리케이션 콜백
// ================================================================

void UNSPartEquipComponent::OnRep_EquippedParts()
{
	UE_LOG(LogTemp, Warning, TEXT("[EquipComp] OnRep_EquippedParts: 배열 크기=%d"), EquippedParts.Num());

	// 배열에서 빠진 슬롯(해제)도 구독자가 반영하도록 빈 파츠로 알림
	if (const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(GetOwner()))
	{
		for (const auto& Pair : DataSS->GetAllSlotRows())
		{
			if (!FindPart(Pair.Key))
			{
				OnPartChanged.Broadcast(Pair.Key, FNSPartData());
			}
		}
	}

	for (const FNSPartData& Part : EquippedParts)
	{
		OnPartChanged.Broadcast(Part.Slot, Part);
	}
}

// ================================================================
// Server RPC
// ================================================================

void UNSPartEquipComponent::Server_RequestEquip_Implementation(FNSPartData NewPart)
{
	EquipPart(NewPart);
}

void UNSPartEquipComponent::Server_RequestReroll_Implementation(FGameplayTag Slot)
{
	RerollStat(Slot);
}

void UNSPartEquipComponent::Server_RequestUpgradeRarity_Implementation(FGameplayTag Slot)
{
	UpgradeRarity(Slot);
}

void UNSPartEquipComponent::Server_RequestPickup_Implementation(ANSDroppedPart* TargetPart)
{
	if (!TargetPart)
	{
		return;
	}

	const APlayerState* PS = Cast<APlayerState>(GetOwner());
	APawn* Pawn = PS ? PS->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	TargetPart->TryPickup(Pawn);
}

void UNSPartEquipComponent::Server_RequestGenerateStock_Implementation()
{
	if (bShopStockGenerated)
	{
		return;
	}
	GenerateShopStock();
}

void UNSPartEquipComponent::Server_RequestPurchase_Implementation(int32 StockIndex)
{
	UNSCurrencyComponent* Currency = GetCurrencyComponent();
	const int64 CurrentBalance = Currency ? Currency->GetTemp() : 0;

	if (!ShopStock.IsValidIndex(StockIndex))
	{
		Client_NotifyUpgradeResult(FGameplayTag(), ENSPartUpgradeResult::SoldOut, CurrentBalance);
		return;
	}

	const FNSPartData Item = ShopStock[StockIndex];

	if (!IsValid(NSPartUtils::ResolvePartDefinition(this, Item)))
	{
		Client_NotifyUpgradeResult(Item.Slot, ENSPartUpgradeResult::SoldOut, CurrentBalance);
		return;
	}

	const int64 Price = GetShopPrice(Item.CurrentRarity);
	if (!Currency || Price < 0 || !Currency->TrySpendTemp(Price))
	{
		Client_NotifyUpgradeResult(Item.Slot, ENSPartUpgradeResult::NotEnoughCurrency, CurrentBalance);
		return;
	}

	ShopStock.RemoveAt(StockIndex);
	EquipPart(Item);

	Client_NotifyUpgradeResult(Item.Slot, ENSPartUpgradeResult::PurchaseDone, Currency->GetTemp());
	OnShopStockChanged.Broadcast();
}
