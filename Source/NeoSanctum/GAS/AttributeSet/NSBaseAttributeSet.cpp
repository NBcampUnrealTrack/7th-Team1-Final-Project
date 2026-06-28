// Copyright 2026 One Team. All rights reserved.


#include "NSBaseAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffectExtension.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionTypes.h"
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
			return;
		}
		
		// Health 적용 전 대상별 방어 처리(자식 함수에서 재정의)
		DamageAmount = HandlePreHealthDamage(DamageAmount, Data);
		
		if (DamageAmount <= 0.0f)
		{
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
		NotifyHitReactionAfterHealthDamage(Data, PreviousHealth);
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
	FeedbackContext.HitQuality = ENSHitFeedbackQuality::Normal;
	FeedbackContext.TargetActor = TargetActor;
	FeedbackContext.HitLocation = TargetActor->GetActorLocation();
	FeedbackContext.bTargetDead = GetHealth() <= 0.0f;

	if (const FHitResult* HitResult = Data.EffectSpec.GetEffectContext().GetHitResult())
	{
		FeedbackContext.HitLocation = HitResult->ImpactPoint;
	}

	PlayerController->Client_PlayAttackHitFeedback(FeedbackContext);
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
	ReactionContext.TargetActor = TargetActor;
	ReactionContext.InstigatorActor = Data.EffectSpec.GetEffectContext().GetInstigator();
	ReactionContext.DamageAmount = AppliedHealthDamage;
	ReactionContext.HitQuality = ENSHitFeedbackQuality::Normal;
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
