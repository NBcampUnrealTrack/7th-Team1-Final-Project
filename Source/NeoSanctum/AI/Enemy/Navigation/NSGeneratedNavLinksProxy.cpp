// Copyright 2026 One Team. All rights reserved.


#include "NSGeneratedNavLinksProxy.h"

#include "Navigation/PathFollowingComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"

bool UNSGeneratedNavLinksProxy::OnLinkMoveStarted(UObject* PathComp, const FVector& DestPoint)
{
	UPathFollowingComponent* PathFollowingComponent = Cast<UPathFollowingComponent>(PathComp);
	if (!PathFollowingComponent)
	{
		return false;
	}

	AActor* Agent = PathFollowingComponent->GetOwner();

	if (AController* Controller = Cast<AController>(Agent))
	{
		Agent = Controller->GetPawn();
	}

	ANSEnemyCharacterBase* Character = Cast<ANSEnemyCharacterBase>(Agent);
	if (!Character)
	{
		return false;
	}

	TWeakObjectPtr<UPathFollowingComponent> WeakPathFollowingComponent = PathFollowingComponent;

	FNSNavLinkTraversalFinishedDelegate OnTraversalFinished =
		FNSNavLinkTraversalFinishedDelegate::CreateWeakLambda(
			this,
			[this, WeakPathFollowingComponent]()
			{
				UPathFollowingComponent* ValidPathFollowingComponent =
					WeakPathFollowingComponent.Get();

				if (!ValidPathFollowingComponent)
				{
					return;
				}

				ValidPathFollowingComponent->FinishUsingCustomLink(this);
			});

	return Character->StartNavLinkTraversal(
		DestPoint,
		OnTraversalFinished);
}
