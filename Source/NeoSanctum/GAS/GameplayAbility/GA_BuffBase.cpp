// Copyright 2026 One Team. All rights reserved.


#include "GA_BuffBase.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/Weapon/Summon/NSTurret.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_BuffBase::UGA_BuffBase()
{
	DurationStatTag = NSGameplayTags::CombatStat_Duration;
	CooldownStatTag = NSGameplayTags::CombatStat_Cooldown;
	CooldownSetByCallerTag = NSGameplayTags::Effect_Cooldown_BuffBase;
	RadiusStatTag = NSGameplayTags::CombatStat_BuffRadius;
}

void UGA_BuffBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ActorInfo->IsNetAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	float Duration = 0.0f;
	if (!TryGetBuffDuration(Duration) || Duration <= 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TArray<AActor*> Targets;
	CollectBuffTargets(Targets);
	ApplyBuffToTargets(Targets, Duration);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_BuffBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_BuffBase::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!CooldownGameplayEffectClass || !CooldownSetByCallerTag.IsValid())
	{
		return;
	}

	float CooldownDuration = 0.0f;
	if (!TryGetBuffCooldown(CooldownDuration))
	{
		return;
	}

	FGameplayEffectSpecHandle CooldownSpecHandle =
		MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, GetAbilityLevel());

	if (!CooldownSpecHandle.IsValid() || !CooldownSpecHandle.Data.IsValid())
	{
		return;
	}

	// CombatStat에서 읽은 Cooldown 값을 GE SetByCaller로 전달
	CooldownSpecHandle.Data->SetSetByCallerMagnitude(CooldownSetByCallerTag, CooldownDuration);

	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpecHandle);
}

void UGA_BuffBase::ApplyBuffToTargets(const TArray<AActor*>& Targets, float Duration)
{
	for (AActor* TargetActor : Targets)
	{
		TryApplyBuffToTarget(TargetActor, Duration);
	}
}

bool UGA_BuffBase::TryApplyBuffToTarget(AActor* TargetActor, float Duration)
{
	if (!TargetActor || HasBuffStateTag(TargetActor))
	{
		return false;
	}

	const FNSBuffApplyEntry* ApplyEntry = FindBuffApplyEntryForTarget(TargetActor);
	if (!ApplyEntry)
	{
		return false;
	}

	bool bApplied = false;

	switch (ApplyEntry->ApplyType)
	{
	case ENSBuffApplyType::CombatStatModifier:
		{
			FGameplayTag AbilityTag;
			if (!TryGetCombatStatAbilityTag(AbilityTag))
			{
				return false;
			}
			bApplied = ApplyCombatStatModifierBuff(TargetActor, *ApplyEntry, AbilityTag, Duration);
			break;
		}
	case ENSBuffApplyType::GameplayEffect:
		bApplied = ApplyGameplayEffectBuff(TargetActor, *ApplyEntry, Duration);
		break;
	default:
		break;
	}

	if (bApplied)
	{
		AddTemporaryBuffPresentation(TargetActor, Duration);
	}

	return bApplied;
}

const FNSBuffApplyEntry* UGA_BuffBase::FindBuffApplyEntryForTarget(const AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return nullptr;
	}

	for (const FNSBuffApplyEntry& ApplyEntry : BuffApplyEntries)
	{
		if (ApplyEntry.TargetActorClass && TargetActor->IsA(ApplyEntry.TargetActorClass))
		{
			return &ApplyEntry;
		}
	}

	return nullptr;
}

bool UGA_BuffBase::ApplyCombatStatModifierBuff(
	AActor* TargetActor,
	const FNSBuffApplyEntry& ApplyEntry,
	const FGameplayTag& AbilityTag,
	float Duration)
{
	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(TargetActor);
	UNSAbilitySystemComponent* TargetASC =
		AbilitySystemInterface ? Cast<UNSAbilitySystemComponent>(AbilitySystemInterface->GetAbilitySystemComponent()) : nullptr;

	if (!TargetASC)
	{
		return false;
	}

	bool bApplied = false;
	for (const FNSBuffCombatStatModifier& CombatStatModifier : ApplyEntry.CombatStatModifiers)
	{
		if (!CombatStatModifier.TargetAbilityTag.IsValid() ||
			!CombatStatModifier.TargetStatTag.IsValid() ||
			!CombatStatModifier.SourceValueStatTag.IsValid())
		{
			continue;
		}

		float Value = 0.0f;
		if (!TryGetFinalAbilityStat(AbilityTag, CombatStatModifier.SourceValueStatTag, Value))
		{
			continue;
		}

		const FGuid ModifierHandle = TargetASC->AddTemporaryCombatStatModifier(
			CombatStatModifier.TargetAbilityTag,
			CombatStatModifier.TargetStatTag,
			CombatStatModifier.Operation,
			Value,
			Duration
		);

		bApplied |= ModifierHandle.IsValid();
	}

	return bApplied;
}

bool UGA_BuffBase::ApplyGameplayEffectBuff(AActor* TargetActor, const FNSBuffApplyEntry& ApplyEntry, float Duration)
{
	if (!TargetActor || !ApplyEntry.EffectClass)
	{
		return false;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return false;
	}

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetASC =
		AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;

	if (!TargetASC)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(ApplyEntry.EffectClass, GetAbilityLevel(), EffectContext);

	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return false;
	}

	// GE 유지시간도 Buff Duration으로 통일
	SpecHandle.Data->SetDuration(Duration, true);

	FGameplayTag AbilityTag;
	if (TryGetCombatStatAbilityTag(AbilityTag))
	{
		ApplySetByCallerMappingsToSpec(SpecHandle, AbilityTag);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	return true;
}

void UGA_BuffBase::ApplySetByCallerMappingsToSpec(
	FGameplayEffectSpecHandle& SpecHandle,
	const FGameplayTag& AbilityTag) const
{
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	for (const FNSSetByCallerFromCombatStat& Mapping : SetByCallerMappings)
	{
		if (!Mapping.CombatStatTag.IsValid() || !Mapping.SetByCallerTag.IsValid())
		{
			continue;
		}

		float Magnitude = 0.0f;
		if (!TryGetFinalAbilityStat(AbilityTag, Mapping.CombatStatTag, Magnitude))
		{
			continue;
		}

		SpecHandle.Data->SetSetByCallerMagnitude(Mapping.SetByCallerTag, Magnitude);
	}
}

bool UGA_BuffBase::HasBuffStateTag(const AActor* TargetActor) const
{
	if (!BuffStateTag.IsValid())
	{
		return false;
	}

	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(TargetActor);
	const UAbilitySystemComponent* TargetASC =
		AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;

	return TargetASC && TargetASC->HasMatchingGameplayTag(BuffStateTag);
}

void UGA_BuffBase::AddTemporaryBuffPresentation(AActor* TargetActor, float Duration) const
{
	if ((!BuffStateTag.IsValid() && !BuffGameplayCueTag.IsValid()) || Duration <= 0.0f)
	{
		return;
	}

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetASC =
		AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;

	if (!TargetASC)
	{
		return;
	}

	if (BuffStateTag.IsValid())
	{
		TargetASC->AddLooseGameplayTag(BuffStateTag);
	}

	if (BuffGameplayCueTag.IsValid())
	{
		TargetASC->AddGameplayCue(BuffGameplayCueTag);
	}

	if (UWorld* World = TargetASC->GetWorld())
	{
		FTimerHandle TimerHandle;
		TWeakObjectPtr<UAbilitySystemComponent> WeakASC = TargetASC;
		const FGameplayTag StateTag = BuffStateTag;
		const FGameplayTag CueTag = BuffGameplayCueTag;
		World->GetTimerManager().SetTimer(
			TimerHandle,
			FTimerDelegate::CreateWeakLambda(TargetASC, [WeakASC, StateTag, CueTag]()
			{
				if (UAbilitySystemComponent* ASC = WeakASC.Get())
				{
					if (StateTag.IsValid())
					{
						ASC->RemoveLooseGameplayTag(StateTag);
					}

					if (CueTag.IsValid())
					{
						ASC->RemoveGameplayCue(CueTag);
					}
				}
			}),
			Duration,
			false
		);
	}
}

bool UGA_BuffBase::TryGetBuffDuration(float& OutDuration) const
{
	FGameplayTag AbilityTag;
	if (!DurationStatTag.IsValid() || !TryGetCombatStatAbilityTag(AbilityTag))
	{
		return false;
	}

	return TryGetFinalAbilityStat(AbilityTag, DurationStatTag, OutDuration);
}

bool UGA_BuffBase::TryGetBuffCooldown(float& OutCooldown) const
{
	// Cooldown도 CombatStat 데이터에서 조회
	FGameplayTag AbilityTag;
	if (!CooldownStatTag.IsValid() || !TryGetCombatStatAbilityTag(AbilityTag))
	{
		return false;
	}

	float CooldownDuration = 0.0f;
	if (!TryGetFinalAbilityStat(AbilityTag, CooldownStatTag, CooldownDuration))
	{
		return false;
	}

	constexpr float MinCooldownDuration = 0.1f;
	OutCooldown = FMath::Max(CooldownDuration, MinCooldownDuration);
	return true;
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
