// Copyright 2026 One Team. All rights reserved.


#include "NSHUDWidget.h"
#include "NSHPShieldWidget.h"

void UNSHUDWidget::UpdateHealthAndShield(
	float CurrentHealth,
	float MaxHealth,
	float CurrentShield,
	float MaxShield
	)
{
	//HP / Shield 위젯이 없으면 갱신X
	// TODO(영웅): 게임 시작시 플레의어의 체력/실드 값 연동
	// TODO(영웅): 현재 체력/ 실드 값 변경시 자동 갱신 연결
	if (!HPShieldWidget)
	{
		return;
	}
	
	//HP/Shield UI 갱신
	HPShieldWidget->SetHealth(CurrentHealth, MaxHealth);
	HPShieldWidget->SetShield(CurrentShield, MaxShield);
}


