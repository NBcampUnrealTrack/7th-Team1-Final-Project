#include "NSHubDataComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/GameStateBase.h"

// Project Settings > Asset Manager 에 등록된 이름과 반드시 일치
const FPrimaryAssetType UNSHubDataComponent::HubAssetType  = TEXT("NSHubData");
const FPrimaryAssetType UNSHubDataComponent::PartAssetType = TEXT("NSPartData");

UNSHubDataComponent* UNSHubDataComponent::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject) return nullptr;
    if (AGameStateBase* GS = WorldContextObject->GetWorld()->GetGameState())
        return GS->FindComponentByClass<UNSHubDataComponent>();
    return nullptr;
}

void UNSHubDataComponent::BeginPlay()
{
    Super::BeginPlay();
    StartLoading();
}

void UNSHubDataComponent::EndPlay(EEndPlayReason::Type EndPlayReason)
{
    LoadHandle.Reset();
    DataCache.Empty();
    bIsReady = false;
    Super::EndPlay(EndPlayReason);
}

void UNSHubDataComponent::StartLoading()
{
    UAssetManager& AM = UAssetManager::Get();

    TArray<FSoftObjectPath> AllPaths;
    for (const FPrimaryAssetType& Type : { HubAssetType, PartAssetType })
    {
        TArray<FAssetData> Assets;
        AM.GetPrimaryAssetDataList(Type, Assets);
        for (const FAssetData& AD : Assets)
            AllPaths.Add(AD.GetSoftObjectPath());
    }

    if (AllPaths.IsEmpty())
    {
        bIsReady = true;
        OnHubDataReady.Broadcast();
        return;
    }

    LoadHandle = AM.GetStreamableManager().RequestAsyncLoad(
        AllPaths,
        FStreamableDelegate::CreateUObject(this, &ThisClass::OnAssetsLoaded));
}

void UNSHubDataComponent::OnAssetsLoaded()
{
    UAssetManager& AM = UAssetManager::Get();

    for (const FPrimaryAssetType& Type : { HubAssetType, PartAssetType })
    {
        TArray<FPrimaryAssetId> Ids;
        AM.GetPrimaryAssetIdList(Type, Ids);
        for (const FPrimaryAssetId& Id : Ids)
            if (UObject* Obj = AM.GetPrimaryAssetObject(Id))
                DataCache.Add(Id, Obj);
    }

    bIsReady = true;
    OnHubDataReady.Broadcast();
}
