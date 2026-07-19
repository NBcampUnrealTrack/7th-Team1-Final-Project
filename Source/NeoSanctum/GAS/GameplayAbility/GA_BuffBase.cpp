// Copyright 2026 One Team. All rights reserved.


#include "GA_BuffBase.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Combat/Weapon/Summon/NSTurret.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_BuffBase::UGA_BuffBase()
{
	DurationStatTag = NSGameplayTags::CombatStat_Duration;
	RadiusStatTag = NSGameplayTags::CombatStat_BuffRadius;
	RangePulseGameplayCueTag = NSGameplayTags::GameplayCue_Common_Buff_RangePulse;
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

	float Duration = 0.0f;
	if (!TryGetBuffDuration(Duration) || Duration <= 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (TargetType == ENSBuffTargetType::Radius)
	{
		float Radius = 0.0f;
		if (TryGetBuffRadius(Radius) && Radius > 0.0f)
		{
			ExecuteRangePulseGameplayCue(Radius);
		}
	}

	if (!ActorInfo->IsNetAuthority())
	{
		AActor* AvatarActor = ActorInfo->AvatarActor.Get();
		if (TargetFilter.bIncludeSelf && !HasBuffStateTag(AvatarActor))
		{
			// LocalPredicted Self 연출도 State/Cue 수명 관리를 동일하게 사용
			AddTemporaryBuffPresentation(AvatarActor, Duration);
		}

		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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

void UGA_BuffBase::ApplyBuffToTargets(const TArray<AActor*>& Targets, float Duration)
{
	for (AActor* TargetActor : Targets)
	{
		TryApplyBuffToTarget(TargetActor, Duration);
	}
}

bool UGA_BuffBase::TryApplyBuffToTarget(AActor* TargetActor, float Duration)
{
	if (!TargetActor)
	{
		return false;
	}

	const FNSBuffApplyEntry* ApplyEntry = FindBuffApplyEntryForTarget(TargetActor);
	if (!ApplyEntry)
	{
		return false;
	}

	const bool bHasActiveBuff = HasBuffStateTag(TargetActor);
	const bool bCanRefreshActiveBuff =
		bHasActiveBuff &&
		bRefreshDurationOnReapply &&
		ApplyEntry->ApplyType == ENSBuffApplyType::CombatStatModifier;

	if (bHasActiveBuff && !bCanRefreshActiveBuff)
	{
		return false;
	}

	bool bApplied = false;

	switch (ApplyEntry->ApplyType)
	{
	case ENSBuffApplyType::CombatStatModifier:
		{
			if (!SkillAbilityTag.IsValid())
			{
				return false;
			}
			bApplied = ApplyCombatStatModifierBuff(TargetActor, *ApplyEntry, SkillAbilityTag, Duration);
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

	FNSActiveBuffRuntime* ActiveRuntime = nullptr;

	if (bRefreshDurationOnReapply)
	{
		const TWeakObjectPtr<UAbilitySystemComponent> TargetASCKey(TargetASC);
		FNSActiveBuffRuntime& Runtime = ActiveBuffRuntimeByTarget.FindOrAdd(TargetASCKey);

		// 기존 Modifier를 먼저 지워야 두 버프가 겹치지 않음.
		for (const FGuid& ModifierHandle : Runtime.CombatStatModifierHandles)
		{
			TargetASC->RemoveTemporaryCombatStatModifier(ModifierHandle);
		}

		Runtime.CombatStatModifierHandles.Reset();
		ActiveRuntime = &Runtime;
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

		if (ModifierHandle.IsValid())
		{
			bApplied = true;

			if (ActiveRuntime)
			{
				// 다음 재사용 때 제거할 수 있도록 새 핸들을 기억.
				ActiveRuntime->CombatStatModifierHandles.Add(ModifierHandle);
			}
		}
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

	if (SkillAbilityTag.IsValid())
	{
		ApplySetByCallerMappingsToSpec(SpecHandle, SkillAbilityTag);
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

		// @원종: CombatStat 퍼센트 값(30 = 30%)을 GE의 MultiplyAdditive Modifier가 기대하는 비율(0.3)로 변환.
		// GA_BuffBase는 Ranger/Engineer의 버프 스킬만 사용 중이며, SetByCallerMappings는 항상 퍼센트 스탯이라는 전제.
		// 후에 다른 캐릭터도 이 버프 매커니즘을 사용한다면 주의할 필요가 있음.
		SpecHandle.Data->SetSetByCallerMagnitude(Mapping.SetByCallerTag, 1.0f + Magnitude * 0.01f);
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

void UGA_BuffBase::AddTemporaryBuffPresentation(AActor* TargetActor, float Duration)
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

	UWorld* World = TargetASC->GetWorld();
	if (!World)
	{
		return;
	}

	const TWeakObjectPtr<UAbilitySystemComponent> TargetASCKey(TargetASC);
	const bool bTrackRuntime = bRefreshDurationOnReapply;

	FTimerHandle LocalTimerHandle;
	FTimerHandle* TimerHandle = &LocalTimerHandle;
	bool bPresentationAlreadyActive = false;

	if (bTrackRuntime)
	{
		FNSActiveBuffRuntime& Runtime = ActiveBuffRuntimeByTarget.FindOrAdd(TargetASCKey);

		bPresentationAlreadyActive = World->GetTimerManager().IsTimerActive(Runtime.PresentationTimerHandle);

		if (bPresentationAlreadyActive)
		{
			// 첫 타이머가 버프를 먼저 끄지 않도록 취소.
			World->GetTimerManager().ClearTimer(Runtime.PresentationTimerHandle);
		}

		TimerHandle = &Runtime.PresentationTimerHandle;
	}

	if (!bPresentationAlreadyActive)
	{
		// 재적용할 때는 태그와  Cue를 중복으로 쌓지 않음.
		if (BuffStateTag.IsValid())
		{
			TargetASC->AddLooseGameplayTag(BuffStateTag);
		}

		if (BuffGameplayCueTag.IsValid())
		{
			TargetASC->AddGameplayCue(BuffGameplayCueTag);
		}
	}

	const TWeakObjectPtr<UGA_BuffBase> WeakAbility(this);
	const TWeakObjectPtr<UAbilitySystemComponent> WeakASC(TargetASC);
	const FGameplayTag StateTag = BuffStateTag;
	const FGameplayTag CueTag = BuffGameplayCueTag;

	World->GetTimerManager().SetTimer(
		*TimerHandle,
		FTimerDelegate::CreateWeakLambda(
			TargetASC,
			[WeakAbility, WeakASC, TargetASCKey, StateTag, CueTag, bTrackRuntime]()
			{
				UAbilitySystemComponent* ASC = WeakASC.Get();

				if (ASC)
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

				UGA_BuffBase* BuffAbility = WeakAbility.Get();
				if (!bTrackRuntime || !BuffAbility)
				{
					return;
				}

				if (FNSActiveBuffRuntime* Runtime = BuffAbility->ActiveBuffRuntimeByTarget.Find(TargetASCKey))
				{
					if (UNSAbilitySystemComponent* NSASC = Cast<UNSAbilitySystemComponent>(ASC))
					{
						// 수치 버프와 연출이 같은 시점에 끝나도록 정리.
						for (const FGuid& ModifierHandle : Runtime->CombatStatModifierHandles)
						{
							NSASC->RemoveTemporaryCombatStatModifier(ModifierHandle);
						}
					}
				}

				BuffAbility->ActiveBuffRuntimeByTarget.Remove(TargetASCKey);
			}
		),
		Duration,
		false
	);
}

bool UGA_BuffBase::TryGetBuffDuration(float& OutDuration) const
{
	if (!DurationStatTag.IsValid() || !SkillAbilityTag.IsValid())
	{
		return false;
	}

	return TryGetFinalAbilityStat(SkillAbilityTag, DurationStatTag, OutDuration);
}

void UGA_BuffBase::ExecuteRangePulseGameplayCue(const float Radius) const
{
	if (Radius <= 0.0f)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!AvatarActor || !ASC)
	{
		return;
	}
	
	const FGameplayTag CueTag = RangePulseGameplayCueTag;
	if (!CueTag.IsValid())
	{
		return;
	}
	
	FGameplayCueParameters CueParameters;
	CueParameters.Instigator = AvatarActor;
	CueParameters.EffectCauser = AvatarActor;
	CueParameters.RawMagnitude = Radius;
	
	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	FVector CueLocation = AvatarLocation;
	FVector CueNormal = FVector::UpVector;
	
	if (UWorld* World = AvatarActor->GetWorld())
	{
		constexpr float TraceStartOffset = 100.0f;
		constexpr float TraceDownDistance = 500.0f;
		constexpr float SurfaceOffset = 2.0f;

		const FVector TraceStart = AvatarLocation + FVector::UpVector * TraceStartOffset;
		const FVector TraceEnd = AvatarLocation - FVector::UpVector * TraceDownDistance;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BuffRangePulseGroundTrace), false, AvatarActor);
		QueryParams.AddIgnoredActor(AvatarActor);

		FHitResult GroundHit;
		if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams)
			&& GroundHit.bBlockingHit)
		{
			CueNormal = GroundHit.ImpactNormal.GetSafeNormal();
			if (CueNormal.IsNearlyZero())
			{
				CueNormal = FVector::UpVector;
			}

			// 정밀도 문제를 피하기 위해 표면에서 약간 띄움
			CueLocation = GroundHit.ImpactPoint + CueNormal * SurfaceOffset;
		}
	}

	CueParameters.Location = CueLocation;
	// Decal Renderer의 투영 방향을 유지하도록 회전값 초기화
	CueParameters.Normal = FVector::ZeroVector;

	ASC->ExecuteGameplayCue(CueTag, CueParameters);
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
	
	// Pawn과 PlayerConstruct Actor를 후보로 수집
	// 터렛 탐지 Sphere 대신 본체 충돌을 범위 판정에 사용
	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::PlayerConstruct);
	
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

bool UGA_BuffBase::TryGetBuffRadius(float& OutRadius) const
{
	// Radius도 CombatStat 데이터에서 조회
	if (!RadiusStatTag.IsValid() || !SkillAbilityTag.IsValid())
	{
		return false;
	}
	
	return TryGetFinalAbilityStat(SkillAbilityTag, RadiusStatTag, OutRadius);
}
