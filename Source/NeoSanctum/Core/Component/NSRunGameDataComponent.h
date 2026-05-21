#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "NSRunGameDataComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNSRunGameDataReady);

UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSRunGameDataComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "NS|RunGameData")
    FOnNSRunGameDataReady OnRunGameDataReady;

    template<typename T>
    T* GetData(const FPrimaryAssetId& Id) const;

    template<typename T>
    TArray<T*> GetAllDataOfType(const FPrimaryAssetType& AssetType) const;

    UFUNCTION(BlueprintPure, Category = "NS|RunGameData", meta=(WorldContext="WorldContextObject"))
    static UNSRunGameDataComponent* Get(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "NS|RunGameData")
    bool IsReady() const { return bIsReady; }

    static const FPrimaryAssetType MonsterAssetType;
    static const FPrimaryAssetType AugmentAssetType;
    static const FPrimaryAssetType AugmentPoolAssetType;
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
T* UNSRunGameDataComponent::GetData(const FPrimaryAssetId& Id) const
{
    const TObjectPtr<UObject>* Found = DataCache.Find(Id);
    return Found ? Cast<T>(*Found) : nullptr;
}

template<typename T>
TArray<T*> UNSRunGameDataComponent::GetAllDataOfType(const FPrimaryAssetType& AssetType) const
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