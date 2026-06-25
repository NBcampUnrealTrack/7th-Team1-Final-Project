// Copyright 2026 One Team. All rights reserved.


#include "NSGeneratedNavLinksProxy.h"

#include "Navigation/PathFollowingComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"

bool UNSGeneratedNavLinksProxy::OnLinkMoveStarted(UObject* PathComp, const FVector& DestPoint)
{
	// 생성 링크는 Agent 대신 Agent의 PathFollowingComponent를 전달한다.
	UPathFollowingComponent* PathFollowingComponent = Cast<UPathFollowingComponent>(PathComp);

	if (!PathComp)
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
		// 캐릭터가 아닌경우 점프 X
		return false;
	}

	// 위치(목적지)만 넘기고 점프 계산/실행/착지보정은 캐릭터가 담당.
	Character->StartNavLinkJump(DestPoint);
	
	// 점프 도중 PathFollowing이 이동 속도를 덮어쓰지 않는다.
	return true;
}
