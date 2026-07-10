// Copyright 2026 One Team. All rights reserved.


#include "NSBaseAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffectExtension.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionTypes.h"
#include "NeoSanctum/Core/Interface/NSHitReactionSourceInterface.h"
#include "NeoSanctum/Core/Interface/NSPlayerAttackFeedbackSourceInterface.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "Net/UnrealNetwork.h"

void UNSBaseAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UNSBaseAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSBaseAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSBaseAttributeSet, BaseDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSBaseAttributeSet, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSBaseAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}

void UNSBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetBaseDamageAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetDefenseAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UNSBaseAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		float DamageAmount = GetDamage();
		SetDamage(0.0f);
		
		if (DamageAmount <= 0.0f)
		{
			SetDamageHitQuality(0.0f);
			return;
		}
		
		// Health 적용 전 대상별 방어 처리(자식 함수에서 재정의)
		DamageAmount = HandlePreHealthDamage(DamageAmount, Data);
		
		if (DamageAmount <= 0.0f)
		{
			SetDamageHitQuality(0.0f);
			return;
		}
		
		const float PreviousHealth = GetHealth();
		const float NewHealth = FMath::Clamp(PreviousHealth - DamageAmount, 0.0f, GetMaxHealth());
		
		SetHealth(NewHealth);
		
		if (!bOutOfHealth && GetHealth() <= 0.0f)
		{
			bOutOfHealth = true;
			OnOutOfHealth.Broadcast();
		}

		// 실제 Health 감소가 확정된 뒤 공격자 로컬 피드백을 요청
		NotifyAttackFeedbackAfterHealthDamage(Data, PreviousHealth);
		// 실제 Health 감소가 확정된 뒤 피격 로컬 피드백을 요청
		NotifyHitTakenFeedbackAfterHealthDamage(Data, PreviousHealth);
		NotifyDamageNumberFeedbackAfterHealthDamage(Data, PreviousHealth);
		NotifyHitReactionAfterHealthDamage(Data, PreviousHealth);
		SetDamageHitQuality(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
		
		if (!bOutOfHealth && GetHealth() <= 0.0f)
		{
			bOutOfHealth = true;
			OnOutOfHealth.Broadcast();
		}
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(GetMaxHealth(), 0.0f));
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMoveSpeedAttribute())
	{
		SetMoveSpeed(FMath::Max(GetMoveSpeed(), 0.0f));
	}
}

float UNSBaseAttributeSet::HandlePreHealthDamage(float DamageAmount, const FGameplayEffectModCallbackData&)
{
	return DamageAmount;
}

void UNSBaseAttributeSet::NotifyAttackFeedbackAfterHealthDamage(
	const FGameplayEffectModCallbackData& Data,
	const float PreviousHealth) const
{
	// Health 감소량이 있을 때만 피드백 Context를 생성
	const float AppliedHealthDamage = FMath::Max(PreviousHealth - GetHealth(), 0.0f);
	if (AppliedHealthDamage <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (!ShouldTriggerPlayerAttackFeedback(Data))
	{
		return;
	}

	AActor* TargetActor = Data.Target.GetAvatarActor();
	AActor* InstigatorActor = Data.EffectSpec.GetEffectContext().GetInstigator();
	APawn* InstigatorPawn = Cast<APawn>(InstigatorActor);
	if (!TargetActor || !InstigatorPawn)
	{
		return;
	}

	ANSPlayerController* PlayerController = Cast<ANSPlayerController>(InstigatorPawn->GetController());
	if (!PlayerController)
	{
		return;
	}

	FNSHitFeedbackContext FeedbackContext;
	FeedbackContext.HitQuality = ResolveDamageHitQuality();
	FeedbackContext.TargetActor = TargetActor;
	FeedbackContext.HitLocation = TargetActor->GetActorLocation();
	FeedbackContext.bTargetDead = GetHealth() <= 0.0f;

	if (const FHitResult* HitResult = Data.EffectSpec.GetEffectContext().GetHitResult())
	{
		FeedbackContext.HitLocation = HitResult->ImpactPoint;
	}

	PlayerController->Client_PlayAttackHitFeedback(FeedbackContext);
}

void UNSBaseAttributeSet::NotifyHitTakenFeedbackAfterHealthDamage(
	const FGameplayEffectModCallbackData&,
	const float) const
{
	// 사실상 아직은 Player 전용로직이므로 virtual 함수를 PlayerAttributeSet에서 구현
}

void UNSBaseAttributeSet::NotifyDamageNumberFeedbackAfterHealthDamage(
	const FGameplayEffectModCallbackData& Data,
	float PreviousHealth) const
{
	// 숫자는 실제 Health에 1 이상 들어간 피해만 보여줌.
	const float AppliedHealthDamage = FMath::Max(PreviousHealth - GetHealth(), 0.0f);
	if (AppliedHealthDamage < 1.0f)
	{
		return;
	}

	AActor* TargetActor = Data.Target.GetAvatarActor();
	if (!TargetActor || !TargetActor->HasAuthority())
	{
		return;
	}

	ANSPlayerController* ViewerController = ResolveDamageNumberViewerController(Data, TargetActor);
	if (!ViewerController)
	{
		return;
	}

	// UI가 값을 다시 계산하지 않도록 서버에서 표시 숫자까지 확정.
	FNSDamageNumberFeedbackContext FeedbackContext;
	FeedbackContext.DamageAmount = static_cast<float>(FMath::RoundToInt(AppliedHealthDamage));
	FeedbackContext.bIsCritical = ResolveDamageHitQuality() == ENSHitFeedbackQuality::Critical;
	FeedbackContext.WorldLocation = ResolveDamageNumberWorldLocation(Data, TargetActor);
	FeedbackContext.DamageLayer = ENSHitReactionDamageLayer::Health;

	// 대상이 이미 가진 피격 분류를 UI에도 그대로 전달.
	if (const UNSHitReactionComponent* HitReactionComponent =
		TargetActor->FindComponentByClass<UNSHitReactionComponent>())
	{
		FeedbackContext.TargetType = HitReactionComponent->GetTargetType();
	}

	ViewerController->Client_PlayDamageNumberFeedback(FeedbackContext);
}

ANSPlayerController* UNSBaseAttributeSet::ResolveDamageNumberViewerController(
	const FGameplayEffectModCallbackData& Data,
	const AActor* TargetActor) const
{
	// 플레이어가 맞은 피해는 데미지 숫자를 띄우지 않음.
	const APawn* TargetPawn = Cast<APawn>(TargetActor);
	if (TargetPawn && Cast<ANSPlayerController>(TargetPawn->GetController()))
	{
		return nullptr;
	}

	// 플레이어 공격으로 몬스터나 오브젝트가 피해를 받았을 때만 공격자에게 보여줌.
	AActor* InstigatorActor = Data.EffectSpec.GetEffectContext().GetInstigator();
	const APawn* InstigatorPawn = Cast<APawn>(InstigatorActor);
	return InstigatorPawn ? Cast<ANSPlayerController>(InstigatorPawn->GetController()) : nullptr;
}

FVector UNSBaseAttributeSet::ResolveDamageNumberWorldLocation(
	const FGameplayEffectModCallbackData& Data,
	const AActor* TargetActor) const
{
	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();

	// 직접 맞춘 공격은 실제 충돌 지점에 숫자를 띄움.
	if (const FHitResult* HitResult = EffectContext.GetHitResult())
	{
		return HitResult->ImpactPoint;
	}

	// 범위 피해는 폭발 원점이 공통이니, 각 대상 위치를 따로 사용.
	return TargetActor ?
		TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f) : FVector::ZeroVector;
}

bool UNSBaseAttributeSet::ShouldTriggerPlayerAttackFeedback(
	const FGameplayEffectModCallbackData& Data) const
{
	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();

	const UObject* SourceCandidates[] =
	{
		EffectContext.GetEffectCauser(),
		EffectContext.GetSourceObject(),
		EffectContext.GetInstigator()
	};

	for (const UObject* SourceCandidate : SourceCandidates)
	{
		const INSPlayerAttackFeedbackSourceInterface* FeedbackSource =
			Cast<INSPlayerAttackFeedbackSourceInterface>(SourceCandidate);
		if (!FeedbackSource)
		{
			continue;
		}
		
		return FeedbackSource->ShouldTriggerPlayerAttackFeedback();
	}
	
	return true;
}

ENSHitReactionAttackType UNSBaseAttributeSet::ResolveHitReactionAttackType(
	const FGameplayEffectModCallbackData& Data) const
{
	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();

	const UObject* SourceCandidates[] =
	{
		EffectContext.GetEffectCauser(),
		EffectContext.GetSourceObject(),
		EffectContext.GetInstigator()
	};

	for (const UObject* SourceCandidate : SourceCandidates)
	{
		const INSHitReactionSourceInterface* HitReactionSource =
			Cast<INSHitReactionSourceInterface>(SourceCandidate);
		if (!HitReactionSource)
		{
			continue;
		}

		return HitReactionSource->GetHitReactionAttackType();
	}

	return ENSHitReactionAttackType::Any;
}

ENSHitFeedbackQuality UNSBaseAttributeSet::ResolveDamageHitQuality() const
{
	const int32 HitQualityValue = FMath::RoundToInt(GetDamageHitQuality());
	if (HitQualityValue == static_cast<int32>(ENSHitFeedbackQuality::Critical))
	{
		return ENSHitFeedbackQuality::Critical;
	}

	return ENSHitFeedbackQuality::Normal;
}

void UNSBaseAttributeSet::NotifyHitReactionAfterHealthDamage(
	const FGameplayEffectModCallbackData& Data,
	const float PreviousHealth) const
{
	const float AppliedHealthDamage = FMath::Max(PreviousHealth - GetHealth(), 0.0f);
	if (AppliedHealthDamage <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	NotifyHitReaction(
		Data,
		ENSHitReactionDamageLayer::Health,
		AppliedHealthDamage,
		GetHealth() <= 0.0f);
}

void UNSBaseAttributeSet::NotifyHitReaction(
	const FGameplayEffectModCallbackData& Data,
	const ENSHitReactionDamageLayer DamageLayer,
	const float DamageAmount,
	const bool bTargetDead) const
{
	if (DamageAmount <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	AActor* TargetActor = Data.Target.GetAvatarActor();
	if (!TargetActor)
	{
		return;
	}

	UNSHitReactionComponent* HitReactionComponent =
		TargetActor->FindComponentByClass<UNSHitReactionComponent>();
	if (!HitReactionComponent)
	{
		return;
	}

	FNSHitReactionContext ReactionContext;
	ReactionContext.DamageLayer = DamageLayer;
	ReactionContext.AttackType = ResolveHitReactionAttackType(Data);
	ReactionContext.TargetActor = TargetActor;
	ReactionContext.InstigatorActor = Data.EffectSpec.GetEffectContext().GetInstigator();
	ReactionContext.DamageAmount = DamageAmount;
	ReactionContext.HitQuality = ResolveDamageHitQuality();
	ReactionContext.bTargetDead = bTargetDead;
	ReactionContext.HitLocation = TargetActor->GetActorLocation();
	ReactionContext.HitNormal = FVector::UpVector;

	if (const FHitResult* HitResult = Data.EffectSpec.GetEffectContext().GetHitResult())
	{
		ReactionContext.HitLocation = HitResult->ImpactPoint;
		ReactionContext.HitNormal = HitResult->ImpactNormal;
	}

	HitReactionComponent->PlayHitReaction(ReactionContext);
}

void UNSBaseAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSBaseAttributeSet, Health, OldHealth);
}

void UNSBaseAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSBaseAttributeSet, MaxHealth, OldMaxHealth);
}

void UNSBaseAttributeSet::OnRep_BaseDamage(const FGameplayAttributeData& OldBaseDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSBaseAttributeSet, BaseDamage, OldBaseDamage);
}

void UNSBaseAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldDefense)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSBaseAttributeSet, Defense, OldDefense);
}

void UNSBaseAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSBaseAttributeSet, MoveSpeed, OldMoveSpeed);
}
