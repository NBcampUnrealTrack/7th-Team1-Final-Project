#include "NSRunDataComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/GameStateBase.h"

// Project Settings > Asset Manager 에 등록된 이름과 반드시 일치
const FPrimaryAssetType UNSRunDataComponent::MonsterAssetType = TEXT("NSMonsterData");
const FPrimaryAssetType UNSRunDataComponent::AugmentAssetType = TEXT("NSAugmentData");
const FPrimaryAssetType UNSRunDataComponent::PartAssetType    = TEXT("NSPartData");

UNSRunDataComponent* UNSRunDataComponent::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject) return nullptr;
    if (AGameStateBase* GS = WorldContextObject->GetWorld()->GetGameState())
        return GS->FindComponentByClass<UNSRunDataComponent>();
    return nullptr;
}

void UNSRunDataComponent::BeginPlay()
{
    Super::BeginPlay();
    StartLoading();
}

void UNSRunDataComponent::EndPlay(EEndPlayReason::Type EndPlayReason)
{
    LoadHandle.Reset();
    DataCache.Empty();
    bIsReady = false;
    Super::EndPlay(EndPlayReason);
}

void UNSRunDataComponent::StartLoading()
{
    UAssetManager& AM = UAssetManager::Get();

    TArray<FSoftObjectPath> AllPaths;
    for (const FPrimaryAssetType& Type : { MonsterAssetType, AugmentAssetType, PartAssetType })
    {
        TArray<FAssetData> Assets;
        AM.GetPrimaryAssetDataList(Type, Assets);
        for (const FAssetData& AD : Assets)
            AllPaths.Add(AD.GetSoftObjectPath());
    }

    if (AllPaths.IsEmpty())
    {
        bIsReady = true;
        OnRunDataReady.Broadcast();
        return;
    }

    LoadHandle = AM.GetStreamableManager().RequestAsyncLoad(
        AllPaths,
        FStreamableDelegate::CreateUObject(this, &ThisClass::OnAssetsLoaded));
}

void UNSRunDataComponent::OnAssetsLoaded()
{
    UAssetManager& AM = UAssetManager::Get();

    for (const FPrimaryAssetType& Type : { MonsterAssetType, AugmentAssetType, PartAssetType })
    {
        TArray<FPrimaryAssetId> Ids;
        AM.GetPrimaryAssetIdList(Type, Ids);
        for (const FPrimaryAssetId& Id : Ids)
            if (UObject* Obj = AM.GetPrimaryAssetObject(Id))
                DataCache.Add(Id, Obj);
    }

    bIsReady = true;
    OnRunDataReady.Broadcast();
}
