#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "NSRunDataComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNSRunDataReady);

UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSRunDataComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "NS|RunData")
    FOnNSRunDataReady OnRunDataReady;

    template<typename T>
    T* GetData(const FPrimaryAssetId& Id) const;

    template<typename T>
    TArray<T*> GetAllDataOfType(const FPrimaryAssetType& AssetType) const;

    UFUNCTION(BlueprintPure, Category = "NS|RunData", meta=(WorldContext="WorldContextObject"))
    static UNSRunDataComponent* Get(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "NS|RunData")
    bool IsReady() const { return bIsReady; }

    static const FPrimaryAssetType MonsterAssetType;
    static const FPrimaryAssetType AugmentAssetType;
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
T* UNSRunDataComponent::GetData(const FPrimaryAssetId& Id) const
{
    const TObjectPtr<UObject>* Found = DataCache.Find(Id);
    return Found ? Cast<T>(*Found) : nullptr;
}

template<typename T>
TArray<T*> UNSRunDataComponent::GetAllDataOfType(const FPrimaryAssetType& AssetType) const
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
