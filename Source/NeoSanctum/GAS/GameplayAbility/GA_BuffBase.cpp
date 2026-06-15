// Copyright 2026 One Team. All rights reserved.


#include "GA_BuffBase.h"

#include "AbilitySystemInterface.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/Weapon/Summon/NSTurret.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_BuffBase::UGA_BuffBase()
{
	DurationStatTag = NSGameplayTags::CombatStat_Duration;
	RadiusStatTag = NSGameplayTags::CombatStat_BuffRadius;
}

void UGA_BuffBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	
}

void UGA_BuffBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_BuffBase::CollectBuffTargets(TArray<AActor*>& OutTargets) const
{
	// TargetType별 후보 수집 진입점
	OutTargets.Reset();
	
	switch (TargetType)
	{
	case ENSBuffTargetType::Self:
		CollectSelfTargets(OutTargets);
		break;
	case ENSBuffTargetType::Radius:
		CollectRadiusTargets(OutTargets);
		break;
	case ENSBuffTargetType::SingleTarget:
		CollectSingleTargetTargets(OutTargets);
		break;
	default:
		break;
	}
}

void UGA_BuffBase::CollectSelfTargets(TArray<AActor*>& OutTargets) const
{
	// 자신도 동일한 필터 경로를 통해 추가
	AddFilteredTarget(GetAvatarActorFromActorInfo(), OutTargets);
}

void UGA_BuffBase::CollectRadiusTargets(TArray<AActor*>& OutTargets) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	
	if (!AvatarActor || !World)
	{
		return;
	}
	
	// Radius 타입도 자신 포함 여부는 TargetFilter로 판단
	CollectSelfTargets(OutTargets);
	
	float Radius = 0.0f;
	if (!TryGetBuffRadius(Radius) || Radius <= 0.0f)
	{
		return;
	}
	
	// Pawn과 WorldDynamic Actor를 후보로 수집
	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BuffRadiusOverlap), false, AvatarActor);
	
	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		AvatarActor->GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(Radius),
		QueryParams
	);
	
	if (!bHasOverlap)
	{
		return;
	}
	
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AddFilteredTarget(OverlapResult.GetActor(), OutTargets);
	}
}

void UGA_BuffBase::CollectSingleTargetTargets(TArray<AActor*>& OutTargets) const
{
	// TODO :
	// 단일 타겟은 Trace를 쏘고 TargetData를 수집하는 등의 별도의 복잡한 로직이 필요함.
	// 다행히 기획상 급하기 필요하지는 않기에 급하기 구현하지 않고 추후 구현으로 미루기로 함.
}

void UGA_BuffBase::AddFilteredTarget(AActor* TargetActor, TArray<AActor*>& OutTargets) const
{
	// 필터 통과 대상만 중복 없이 추가
	if (!PassesTargetFilter(TargetActor))
	{
		return;
	}
	
	OutTargets.AddUnique(TargetActor);
}

bool UGA_BuffBase::PassesTargetFilter(const AActor* TargetActor) const
{
	if (!TargetActor || !HasTargetAbilitySystem(TargetActor))
	{
		return false;
	}
	
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	
	if (TargetActor == AvatarActor)
	{
		// 자신 포함 여부
		return TargetFilter.bIncludeSelf;
	}
	
	if (const ANSTurret* Turret = Cast<ANSTurret>(TargetActor))
	{
		// 자신이 소환한 Turret 포함 여부
		const APawn* AvatarPawn = Cast<APawn>(AvatarActor);
		return TargetFilter.bIncludeOwnedTurrets && AvatarPawn && Turret->GetOwningPawn() == AvatarPawn;
	}
	
	if (TargetActor->IsA<ANSPlayerCharacterBase>())
	{
		// 다른 플레이어 포함 여부
		return TargetFilter.bIncludeOtherPlayers;
	}
	
	return false;
}

bool UGA_BuffBase::HasTargetAbilitySystem(const AActor* TargetActor) const
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(TargetActor);
	return AbilitySystemInterface && AbilitySystemInterface->GetAbilitySystemComponent();
}

bool UGA_BuffBase::TryGetCombatStatAbilityTag(FGameplayTag& OutAbilityTag) const
{
	if (CombatStatAbilityTag.IsValid())
	{
		OutAbilityTag = CombatStatAbilityTag;
		return true;
	}
	
	// 명시한 태그가 없으면 Ability Asset.* 태그를 사용하도록 함
	TArray<FGameplayTag> AssetTags;
	GetAssetTags().GetGameplayTagArray(AssetTags);
	
	for (const FGameplayTag& AssetTag : AssetTags)
	{
		if (AssetTag.IsValid() && AssetTag.ToString().StartsWith(TEXT("Ability.")))
		{
			OutAbilityTag = AssetTag;
			return true;
		}
	}
	
	return false;
}

bool UGA_BuffBase::TryGetBuffRadius(float& OutRadius) const
{
	// Radius도 CombatStat 데이터에서 조회
	FGameplayTag AbilityTag;
	if (!RadiusStatTag.IsValid() || !TryGetCombatStatAbilityTag(AbilityTag))
	{
		return false;
	}
	
	return TryGetFinalAbilityStat(AbilityTag, RadiusStatTag, OutRadius);
}
