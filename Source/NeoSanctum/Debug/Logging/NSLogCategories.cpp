// Copyright 2026 One Team. All rights reserved.

#include "NSLogCategories.h"

/**
 * 로그 카테고리 정의 파일입니다.
 *
 * 주의:
 * - DECLARE_LOG_CATEGORY_EXTERN은 .h에 1번만 작성합니다.
 * - DEFINE_LOG_CATEGORY는 .cpp에 1번만 작성합니다.
 * - 같은 카테고리를 여러 .cpp에서 DEFINE하면 링크 에러가 발생합니다.
 */

DEFINE_LOG_CATEGORY(LogNS);
DEFINE_LOG_CATEGORY(LogNSGAS);
DEFINE_LOG_CATEGORY(LogNSNetwork);
