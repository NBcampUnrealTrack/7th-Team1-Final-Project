// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSGateAccessComponent.generated.h"

class UNSPlayerProgressComponent;

/**
* 폰별 입장 게이트 접근 처리 컴포넌트 (거점 플레이어 캐릭터에 부착).
* "이 폰이 해금한 게이트는 통과 허용(폰별) + (로컬이면) 그 게이트 외형 갱신".
*
* - 통과: 게이트는 기본 잠금(Block)이고, 해금한 폰만 자기 캡슐의 IgnoreActorWhenMoving으로 무시한다.
*   서버(권위)·소유 클라(예측)에서 같은 진행도를 보고 동일하게 적용하므로 일관된다.
* - 외형: 각 클라가 본인 진행도 기준으로만 로컬에서 색을 바꾼다.
* - 진행도 동기화/변경은 PlayerState의 UNSPlayerProgressComponent::OnProgressChanged로 잡는다.
*/
UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSGateAccessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSGateAccessComponent();

	// 캐릭터의 PossessedBy(서버) / OnRep_PlayerState(클라)에서 호출. 중복 호출 안전.
	void InitializeGateAccess();

private:
	// 진행도 기준으로 월드의 모든 게이트에 대해 통과 허용(폰별) + 로컬 외형 갱신
	void RefreshAllGates();

	UNSPlayerProgressComponent* GetProgressComponent() const;

	// OnProgressChanged 중복 바인딩 방지용
	FDelegateHandle ProgressChangedHandle;
	TWeakObjectPtr<UNSPlayerProgressComponent> BoundProgress;
};
