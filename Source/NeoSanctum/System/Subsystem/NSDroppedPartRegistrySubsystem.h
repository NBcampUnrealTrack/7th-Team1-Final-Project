// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NSDroppedPartRegistrySubsystem.generated.h"

class ANSDroppedPart;

/**
 * 서버 전용 드랍 파츠 위치 북키핑 레지스트리
 * 파츠 액터 자체는 지금처럼 모든 클라이언트에 복제되는 공용 액터로 유지됨
 * 이 서브시스템은 스폰 위치를 정할 때 겹침을 피하기 위한 서버 내부 참고용 목록일 뿐, 리플리케이션/가시성과는 무관
 */
UCLASS()
class NEOSANCTUM_API UNSDroppedPartRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 서버 권한에서만 호출 —> ANSDroppedPart::BeginPlay에서 등록
	void RegisterDrop(ANSDroppedPart* Drop);
	// 서버 권한에서만 호출 —> ANSDroppedPart::EndPlay에서 해제
	void UnregisterDrop(ANSDroppedPart* Drop);

	// AvoidRadius 반경 안에 이미 등록된 드랍 파츠가 있는지 확인
	bool IsLocationOccupied(const FVector& Location, float AvoidRadius = 150.f) const;

private:
	TArray<TWeakObjectPtr<ANSDroppedPart>> ActiveDrops;
};
