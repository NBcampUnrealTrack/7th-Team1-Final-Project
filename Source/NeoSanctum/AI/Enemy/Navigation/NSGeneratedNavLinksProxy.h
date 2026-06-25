// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/GeneratedNavLinksProxy.h"
#include "NSGeneratedNavLinksProxy.generated.h"

/**
 * 자동 생성된 Nav Link에 도달한 Character를 목적지 방향으로 이동시킨다.
 */
UCLASS()
class NEOSANCTUM_API UNSGeneratedNavLinksProxy : public UBaseGeneratedNavLinksProxy
{
	GENERATED_BODY()
	
public:
    virtual bool OnLinkMoveStarted(UObject* PathComp, const FVector& DestPoint) override;
};
