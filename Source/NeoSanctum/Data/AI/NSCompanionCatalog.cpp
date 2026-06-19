// Copyright 2026 One Team. All rights reserved.


#include "NSCompanionCatalog.h"

#include "NSCompanionDefinition.h"

UNSCompanionDefinition* UNSCompanionCatalog::FindByTag(FGameplayTag InCompanionTag) const
{
	if (!InCompanionTag.IsValid()) return nullptr;
	
	for (const TObjectPtr<UNSCompanionDefinition>& CompanionDefinition : Companions)
	{
		if (!IsValid(CompanionDefinition)) continue;
		
		if (CompanionDefinition->CompanionTag == InCompanionTag)
		{
			return CompanionDefinition;
		}
	}
	return nullptr;
}
