// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "Logging/LogMacros.h"

/**
 * NeoSanctum 공통 로그 카테고리 선언 파일입니다.
 *
 * 사용 위치:
 * - 로그를 출력하는 .cpp에서 LogNS / LogNSGAS / LogNSNetwork 카테고리가 필요할 때 include합니다.
 * - 일반적으로는 NSLogMacros.h가 이 파일을 포함하므로, 로그 매크로 사용 파일에서는 별도 include가 필요 없습니다.
 *
 * 카테고리 추가 기준:
 * - 처음부터 시스템별 카테고리를 과하게 늘리지 않습니다.
 * - Output Log에서 필터링할 필요가 명확한 시스템만 추가합니다.
 * - 예: GAS 흐름은 LogNSGAS, 접속 / 복제 / 서버-클라 흐름은 LogNSNetwork.
 *
 * Verbosity 사용 규칙:
 * - 팀 컨벤션상 Error / Fatal은 사용하지 않습니다.
 * - 일반 흐름 확인은 Log, 문제가 될 수 있는 상태는 Warning을 사용합니다.
 *
 * 주의:
 * - DECLARE_LOG_CATEGORY_EXTERN은 .h에 1번만 작성합니다.
 * - DEFINE_LOG_CATEGORY는 대응되는 .cpp에 1번만 작성합니다.
 * - 같은 카테고리를 여러 .cpp에서 DEFINE하면 링크 에러가 발생합니다.
 */

// 프로젝트 전반에서 사용할 기본 로그 카테고리입니다.
DECLARE_LOG_CATEGORY_EXTERN(LogNS, Log, All);

// Ability, ASC, Attribute, GameplayEffect, GameplayCue 등 GAS 흐름 확인용입니다.
DECLARE_LOG_CATEGORY_EXTERN(LogNSGAS, Log, All);

// Listen Server, Client, TargetData RPC, 복제 흐름 등 네트워크 확인용입니다.
DECLARE_LOG_CATEGORY_EXTERN(LogNSNetwork, Log, All);
