// Copyright 2026 One Team. All rights reserved.


#include "NSCurrencyVisualData.h"

const FNSCurrencyVisualRow* UNSCurrencyVisualData::FindVisual(FGameplayTag CurrencyType, ENSCurrencyGrade Grade) const
{
	return Visuals.FindByPredicate([CurrencyType, Grade](const FNSCurrencyVisualRow& Visual)
	{
		return Visual.CurrencyType == CurrencyType && Visual.Grade == Grade;
	});
}
