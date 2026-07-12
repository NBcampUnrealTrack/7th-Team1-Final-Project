#include "NSGameplayCueNotify_WarningDecal.h"

#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NeoSanctum/AI/Enemy/HelperActor/NSBossArenaBounds.h"


ANSGameplayCueNotify_WarningDecal::ANSGameplayCueNotify_WarningDecal()
{
	bAutoDestroyOnRemove = true;
	bAllowMultipleOnActiveEvents = false;
	bAllowMultipleWhileActiveEvents = false;
}

bool ANSGameplayCueNotify_WarningDecal::OnActive_Implementation(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters)
{
	Super::OnActive_Implementation(MyTarget, Parameters);

	if (!DecalMaterial)
	{
		return true;
	}

	FRotator FinalRotation = DecalRotation;   // 기본: 바닥 투영(pitch -90 등)
	
	if (!Parameters.Normal.IsNearlyZero())
	{
		FinalRotation.Yaw = Parameters.Normal.Rotation().Yaw;
	}

	// 기본값(256)은 폴백. 아레나가 실려오면 존 실측 크기로 덮어씀
	FVector FinalDecalSize = DecalSize;
	if (const ANSBossArenaBounds* Arena = Cast<ANSBossArenaBounds>(Parameters.SourceObject.Get()))
	{
		const int32 ZoneCount = FMath::RoundToInt(Parameters.NormalizedMagnitude);
		if (ZoneCount > 0)
		{
			// ZoneExtent: X=진행축 half, Y=룸폭 half, Z=높이 half (footprint엔 Z 불필요)
			const FVector ZoneExtent = Arena->GetZoneBoxExtent(ZoneCount, 0.f);

			// 데칼 로컬축: X=투영 깊이, Y/Z=바닥 footprint 반경.
			// DecalSize.X(투영 깊이)는 기존값 유지, Y/Z에 존 수평 half-extent 매핑
			FinalDecalSize = FVector(DecalSize.X, ZoneExtent.Y, ZoneExtent.X);
		}
	}

	SpawnedDecal = UGameplayStatics::SpawnDecalAtLocation(
		this,
		DecalMaterial,
		FinalDecalSize,          // ← DecalSize 대신 계산값
		Parameters.Location,
		FinalRotation,
		0.f);

	return true;
}

bool ANSGameplayCueNotify_WarningDecal::OnRemove_Implementation(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters)
{
	if (IsValid(SpawnedDecal))
	{
		SpawnedDecal->DestroyComponent();
		SpawnedDecal = nullptr;
	}

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}