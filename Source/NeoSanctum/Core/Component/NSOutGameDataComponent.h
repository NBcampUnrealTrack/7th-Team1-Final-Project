#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "NSOutGameDataComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNSOutGameDataReady);

UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSOutGameDataComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "NS|OutGameData")
    FOnNSOutGameDataReady OnOutGameDataReady;

    template<typename T>
    T* GetData(const FPrimaryAssetId& Id) const;

    template<typename T>
    TArray<T*> GetAllDataOfType(const FPrimaryAssetType& AssetType) const;

    UFUNCTION(BlueprintPure, Category = "NS|OutGameData", meta=(WorldContext="WorldContextObject"))
    static UNSOutGameDataComponent* Get(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "NS|OutGameData")
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
T* UNSOutGameDataComponent::GetData(const FPrimaryAssetId& Id) const
{
    const TObjectPtr<UObject>* Found = DataCache.Find(Id);
    return Found ? Cast<T>(*Found) : nullptr;
}

template<typename T>
TArray<T*> UNSOutGameDataComponent::GetAllDataOfType(const FPrimaryAssetType& AssetType) const
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
