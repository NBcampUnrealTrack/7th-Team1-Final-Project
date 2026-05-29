// Copyright 2026 One Team. All rights reserved.


#include "ANS_AttachGameplayTags.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

void UANS_AttachGameplayTags::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventRef
)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventRef);
	
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(MeshComp->GetOwner());
	if (!ASI) return;
	
	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return;
	
	// 태그 추가
	ASC->AddLooseGameplayTags(Tags);
}

void UANS_AttachGameplayTags::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference
)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(MeshComp->GetOwner());
	if (!ASI) return;
	
	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return;
	
	// 태그 삭제
	ASC->RemoveLooseGameplayTags(Tags); 
}
