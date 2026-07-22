// Copyright 2026 One Team. All rights reserved.

#include "NSEnemyVisualParameterTypes.h"

#include "Misc/DataValidation.h"

#if WITH_EDITOR
EDataValidationResult FNSEnemyVisualParameterRow::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = FTableRowBase::IsDataValid(Context);

	// 검증 실패 메시지를 추가하는 함수
	auto AddError = [&Context, &Result](const FString& Message)
	{
		Context.AddError(FText::FromString(Message));
		Result = EDataValidationResult::Invalid;
	};

	if (!bEnabled)
	{
		return Result;
	}

	if (!EnemyId.IsValid())
	{
		AddError(TEXT("EnemyVisualParameterRow EnemyId가 비어 있습니다."));
	}

	return Result;
}
#endif
