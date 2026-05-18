#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "NSHubDataComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNSHubDataReady);

UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSHubDataComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "NS|HubData")
    FOnNSHubDataReady OnHubDataReady;

    template<typename T>
    T* GetData(const FPrimaryAssetId& Id) const;

    template<typename T>
    TArray<T*> GetAllDataOfType(const FPrimaryAssetType& AssetType) const;

    UFUNCTION(BlueprintPure, Category = "NS|HubData", meta=(WorldContext="WorldContextObject"))
    static UNSHubDataComponent* Get(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "NS|HubData")
    bool IsReady() const { return bIsReady; }

    static const FPrimaryAssetType HubAssetType;
    static const FPrimaryAssetType PartAssetType;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
    void StartLoading();
    void OnAssetsLoaded();

    UPROPERTY()
    TMap<FPrimaryAssetId, TObjectPtr<UObject>> DataCache;

    TSharedPtr<FStreamableHandle> LoadHandle;

    bool bIsReady = false;
};

template<typename T>
T* UNSHubDataComponent::GetData(const FPrimaryAssetId& Id) const
{
    const TObjectPtr<UObject>* Found = DataCache.Find(Id);
    return Found ? Cast<T>(*Found) : nullptr;
}

template<typename T>
TArray<T*> UNSHubDataComponent::GetAllDataOfType(const FPrimaryAssetType& AssetType) const
{
    TArray<T*> Result;
    for (const auto& Pair : DataCache)
    {
        if (Pair.Key.PrimaryAssetType == AssetType)
            if (T* Casted = Cast<T>(Pair.Value.Get()))
                Result.Add(Casted);
    }
    return Result;
}
