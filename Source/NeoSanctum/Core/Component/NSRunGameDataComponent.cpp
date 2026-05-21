#include "NSRunGameDataComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/GameStateBase.h"

// Project Settings > Asset Manager 에 등록된 이름과 반드시 일치
const FPrimaryAssetType UNSRunGameDataComponent::MonsterAssetType     = TEXT("NSMonsterData");
const FPrimaryAssetType UNSRunGameDataComponent::AugmentAssetType     = TEXT("NSAugmentData");
const FPrimaryAssetType UNSRunGameDataComponent::AugmentPoolAssetType = TEXT("NSAugmentPool");
const FPrimaryAssetType UNSRunGameDataComponent::PartAssetType        = TEXT("NSPartData");

UNSRunGameDataComponent* UNSRunGameDataComponent::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject) return nullptr;
    if (AGameStateBase* GS = WorldContextObject->GetWorld()->GetGameState())
        return GS->FindComponentByClass<UNSRunGameDataComponent>();
    return nullptr;
}

void UNSRunGameDataComponent::BeginPlay()
{
    Super::BeginPlay();
    StartLoading();
}

void UNSRunGameDataComponent::EndPlay(EEndPlayReason::Type EndPlayReason)
{
    LoadHandle.Reset();
    DataCache.Empty();
    bIsReady = false;
    Super::EndPlay(EndPlayReason);
}

void UNSRunGameDataComponent::StartLoading()
{
    UAssetManager& AM = UAssetManager::Get();

    TArray<FSoftObjectPath> AllPaths;
    for (const FPrimaryAssetType& Type : { MonsterAssetType, AugmentAssetType, AugmentPoolAssetType, PartAssetType })
    {
        TArray<FAssetData> Assets;
        AM.GetPrimaryAssetDataList(Type, Assets);
        for (const FAssetData& AD : Assets)
            AllPaths.Add(AD.GetSoftObjectPath());
    }

    if (AllPaths.IsEmpty())
    {
        bIsReady = true;
        OnRunGameDataReady.Broadcast();
        return;
    }

    LoadHandle = AM.GetStreamableManager().RequestAsyncLoad(
        AllPaths,
        FStreamableDelegate::CreateUObject(this, &ThisClass::OnAssetsLoaded));
}

void UNSRunGameDataComponent::OnAssetsLoaded()
{
    UAssetManager& AM = UAssetManager::Get();

    for (const FPrimaryAssetType& Type : { MonsterAssetType, AugmentAssetType, AugmentPoolAssetType, PartAssetType })
    {
        TArray<FPrimaryAssetId> Ids;
        AM.GetPrimaryAssetIdList(Type, Ids);
        for (const FPrimaryAssetId& Id : Ids)
            if (UObject* Obj = AM.GetPrimaryAssetObject(Id))
                DataCache.Add(Id, Obj);
    }

    bIsReady = true;
    OnRunGameDataReady.Broadcast();
}