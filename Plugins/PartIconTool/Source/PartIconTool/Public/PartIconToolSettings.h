// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PartIconToolSettings.generated.h"

/**
 * 파츠 아이콘 캡처 설정. Project Settings > Plugins > Part Icon Tool 에서 조절.
 * 값 변경 시 빌드 없이 다음 Generate부터 즉시 반영됨.
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Part Icon Tool"))
class UPartIconToolSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	// 캡처 해상도 (정사각형, px)
	UPROPERTY(EditAnywhere, config, Category = "Capture", meta = (ClampMin = "32", ClampMax = "2048"))
	int32 IconResolution = 256;

	// 카메라 오빗 피치 (상하 각도, 음수=위에서 내려봄)
	UPROPERTY(EditAnywhere, config, Category = "Capture")
	float OrbitPitch = -11.25f;

	// 카메라 오빗 야 (좌우 회전 각도)
	UPROPERTY(EditAnywhere, config, Category = "Capture")
	float OrbitYaw = -157.5f;

	// 줌 오프셋 (양수 = 카메라 더 멀리 → 메시 작게)
	UPROPERTY(EditAnywhere, config, Category = "Capture")
	float OrbitZoom = 0.f;

	// 메시 여백 배율 (클수록 메시가 화면에서 작게 보임)
	UPROPERTY(EditAnywhere, config, Category = "Capture", meta = (ClampMin = "1.0"))
	float DistanceMultiplier = 1.15f;

	// 정면 라이트 밝기 (클수록 밝음, 과하면 흰색으로 탐)
	UPROPERTY(EditAnywhere, config, Category = "Capture", meta = (ClampMin = "0.0"))
	float LightBrightness = 1.0f;
};
