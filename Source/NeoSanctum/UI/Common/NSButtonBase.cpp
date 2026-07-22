// Copyright 2026 One Team. All rights reserved.


#include "NSButtonBase.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h" 

void UNSButtonBase::NativeOnClicked()
{
	Super::NativeOnClicked();
	
	if (!ClickSoundID.IsNone())
	{
		if (UNSSoundSubsystem* Sound = UNSSoundSubsystem::Get(this))
		{
			Sound->PlaySound2D(ClickSoundID);
		}
	}
}

void UNSButtonBase::NativeOnHovered()
{
	Super::NativeOnHovered();
	
	if (!HoverSoundID.IsNone())
	{
		if (UNSSoundSubsystem* Sound = UNSSoundSubsystem::Get(this))
		{
			Sound->PlaySound2D(HoverSoundID);
		}
	}
}
