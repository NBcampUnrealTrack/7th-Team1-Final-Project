#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "NSBossControlDevice.generated.h"

class USkeletalMeshComponent;
class UAbilitySystemComponent;
class UNSMonsterAttributeSet;
class UNSEnemyStateComponent;
class UNSHitReactionComponent;
class UNSDamageFlashComponent;
class UNSDissolveComponent;
class UMaterialInstanceDynamic;
class ANSBossControlDevice;
class UCapsuleComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FNSControlDeviceDestroyed, ANSBossControlDevice*);

UCLASS()
class NEOSANCTUM_API ANSBossControlDevice : public AActor,
                                            public IAbilitySystemInterface,
                                            public IGenericTeamAgentInterface
{
    GENERATED_BODY()
public:
    ANSBossControlDevice();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ASC; }

    // 보스(Enemy)의 자기 공격을 HasSameTeam으로 걸러내기 위한 Enemy 팀 고정
    virtual FGenericTeamId GetGenericTeamId() const override
    {
        return FGenericTeamId(static_cast<uint8>(ETeamId::Enemy));
    }

    // 서버에서 파괴 시 1회 (보스가 바인딩 → 무적 해제)
    FNSControlDeviceDestroyed OnControlDeviceDestroyed;

    void ApplyGroundPlacement(const FVector& InGroundLocation, const FRotator& InGroundRotation);
    float GetPivotToMeshBottomOffset() const;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnRep_GroundPlaced();
    void ApplyPlacementTransform();

    void HandleDeadStateChanged(bool bDead);  // StateComponent 사망 훅(전 머신)
    void SetupFlashMaterials();               // 피격 플래시용 MID 생성 + 등록

protected:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UCapsuleComponent> CollisionComponent;   // 루트 겸 피격 콜라이더
    
    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, Category = "GAS")
    TObjectPtr<UAbilitySystemComponent> ASC;

    UPROPERTY()
    TObjectPtr<UNSMonsterAttributeSet> AttributeSet;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UNSEnemyStateComponent> StateComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UNSHitReactionComponent> HitReactionComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UNSDamageFlashComponent> DamageFlashComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UNSDissolveComponent> DissolveComponent;

    // 존재하는 동안 자신에게 유지할 지속 GameplayCue (VFX+사운드)
    UPROPERTY(EditDefaultsOnly, Category = "ControlDevice|Cue")
    FGameplayTag SustainCueTag;

    UPROPERTY(EditDefaultsOnly, Category = "ControlDevice|Cue")
    FGameplayTag DestroyCueTag;
    
    UPROPERTY(EditAnywhere, Category = "ControlDevice", meta = (ClampMin = "1.0"))
    float InitialHealth = 100.f;

private:
    bool bDied = false;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> FlashMIDs;  // GC 방지 보관

    UPROPERTY(ReplicatedUsing = OnRep_GroundPlaced)
    bool bGroundPlaced = false;

    UPROPERTY(Replicated)
    FVector_NetQuantize GroundLocation = FVector::ZeroVector;

    UPROPERTY(Replicated)
    FRotator GroundRotation = FRotator::ZeroRotator;
};