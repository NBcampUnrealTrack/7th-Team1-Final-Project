#include "NSBossControlDevice.h"

#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"
#include "NeoSanctum/System/Component/NSDamageFlashComponent.h"
#include "NeoSanctum/System/Component/NSDissolveComponent.h"
#include "NeoSanctum/System/Minimap/NSMinimapIconComponent.h"
#include "Components/CapsuleComponent.h"

ANSBossControlDevice::ANSBossControlDevice()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(false);

    CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComponent"));
    SetRootComponent(CollisionComponent);
    CollisionComponent->SetCollisionProfileName(NSCollisionProfiles::EnemyCharacter); // ObjectType=Enemy, PlayerWeaponTrace Block
    CollisionComponent->InitCapsuleSize(60.f, 90.f);   // BP 뷰포트에서 실제 메시에 맞게 조정
    CollisionComponent->SetCanEverAffectNavigation(false);

    Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(CollisionComponent);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // 피격은 캡슐이 담당
    Mesh->SetCanEverAffectNavigation(false);

    ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
    ASC->SetIsReplicated(true);
    ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

    AttributeSet = CreateDefaultSubobject<UNSMonsterAttributeSet>(TEXT("AttributeSet"));

    StateComponent = CreateDefaultSubobject<UNSEnemyStateComponent>(TEXT("StateComponent"));

    HitReactionComponent = CreateDefaultSubobject<UNSHitReactionComponent>(TEXT("HitReactionComponent"));
    HitReactionComponent->SetTargetType(ENSHitFeedbackTargetType::DestructibleObject);

    DamageFlashComponent = CreateDefaultSubobject<UNSDamageFlashComponent>(TEXT("DamageFlashComponent"));
    DissolveComponent = CreateDefaultSubobject<UNSDissolveComponent>(TEXT("DissolveComponent"));
    
    MinimapIconComponent = CreateDefaultSubobject<UNSMinimapIconComponent>(TEXT("MinimapIconComponent"));
    MinimapIconComponent->SetHideWhenOwnerHealthZero(true);
}

void ANSBossControlDevice::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ANSBossControlDevice, bGroundPlaced);
    DOREPLIFETIME(ANSBossControlDevice, GroundLocation);
    DOREPLIFETIME(ANSBossControlDevice, GroundRotation);
}

FVector ANSBossControlDevice::GetAimLocation() const
{
    if (Mesh)
    {
        return Mesh->Bounds.Origin;
    }
    return GetActorLocation();
}

void ANSBossControlDevice::BeginPlay()
{
    Super::BeginPlay();

    ASC->InitAbilityActorInfo(this, this);

    // 전 머신 바인딩: 클라에서도 사망 상태 변화를 받아 디졸브 재생
    if (StateComponent)
    {
        StateComponent->InitState(AttributeSet);
        StateComponent->OnDeadStateChanged.AddUObject(this, &ThisClass::HandleDeadStateChanged);
    }

    SetupFlashMaterials();

    if (HasAuthority())
    {
         AttributeSet->InitMaxHealth(InitialHealth);
         AttributeSet->InitHealth(InitialHealth);
        
        // 확인용 로그 (한 번 찍어보고 지우면 됨)
        UE_LOG(LogTemp, Warning, TEXT("[ControlDevice] Registered=%d InitialHealth=%.1f ASC_Health=%.1f"),
            ASC->GetSpawnedAttributes().Contains(AttributeSet),
            InitialHealth,
            ASC->GetNumericAttribute(UNSBaseAttributeSet::GetHealthAttribute()));
        
        if (SustainCueTag.IsValid())
        {
            ASC->AddGameplayCue_MinimalReplication(SustainCueTag);
        }
    }
}

void ANSBossControlDevice::SetupFlashMaterials()
{
    if (!Mesh || !DamageFlashComponent)
    {
        return;
    }

    DamageFlashComponent->ClearMaterialFlashTargets();
    FlashMIDs.Reset();

    const int32 NumMaterials = Mesh->GetNumMaterials();
    for (int32 Index = 0; Index < NumMaterials; ++Index)
    {
        if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(Index))
        {
            FlashMIDs.Add(MID);
        }
    }

    TArray<UMaterialInstanceDynamic*> FlashTargets;
    FlashTargets.Reserve(FlashMIDs.Num());
    for (const TObjectPtr<UMaterialInstanceDynamic>& MID : FlashMIDs)
    {
        FlashTargets.Add(MID);
    }
    DamageFlashComponent->SetMaterialFlashTargets(FlashTargets);
}

void ANSBossControlDevice::HandleDeadStateChanged(bool bDead)
{
    if (!bDead || bDied)
    {
        return;
    }
    bDied = true;

    if (CollisionComponent)
    {
        CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 피격 대상 제외
    }

    if (HasAuthority())
    {
        OnControlDeviceDestroyed.Broadcast(this);  // 보스 무적 해제 트리거

        if (SustainCueTag.IsValid())
        {
            ASC->RemoveGameplayCue_MinimalReplication(SustainCueTag);
        }
        
        // 파괴 순간 1회 사운드 (instant)
        if (DestroyCueTag.IsValid())
        {
            FGameplayCueParameters CueParams;
            CueParams.Location = GetActorLocation();
            ASC->ExecuteGameplayCue(DestroyCueTag, CueParams);
        }
    }

    // 서버: 디졸브 후 파괴 / 클라: 비주얼만
    if (DissolveComponent)
    {
        DissolveComponent->StartDissolve(HasAuthority());
    }
}

void ANSBossControlDevice::ApplyGroundPlacement(const FVector& InGroundLocation, const FRotator& InGroundRotation)
{
    if (bDied) return;

    GroundLocation = InGroundLocation;
    GroundRotation = InGroundRotation;
    bGroundPlaced = true;

    ApplyPlacementTransform();  // 호스트 즉시 반영, 원격은 OnRep_GroundPlaced
}

void ANSBossControlDevice::OnRep_GroundPlaced()
{
    if (!bGroundPlaced) return;
    ApplyPlacementTransform();
}

void ANSBossControlDevice::ApplyPlacementTransform()
{
    if (GetAttachParentActor())
    {
        DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    }
    SetActorLocationAndRotation(FVector(GroundLocation), GroundRotation);
}

float ANSBossControlDevice::GetPivotToMeshBottomOffset() const
{
    if (!Mesh) return 0.f;
    const float BoundsBottomZ = Mesh->Bounds.Origin.Z - Mesh->Bounds.BoxExtent.Z;
    return GetActorLocation().Z - BoundsBottomZ;
}