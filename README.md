# NeoSanctum

> **4인 협동 TPS 로그라이트 슈터**
>
> **Unreal Engine 5.7** 기반의 멀티플레이 로그라이트 프로젝트

<div align="center">

<img width="1672" height="941" alt="Slide4_Image1 (1)" src="https://github.com/user-attachments/assets/a99ed618-2536-4065-b371-26f16fb9ec78" />

[🎥 Gameplay Video](https://youtu.be/Mh_9p5twGzQ?si=uNTrMjrztk6NnABP)

</div>

---

# 프로젝트 소개

NeoSanctum은 미래의 포스트 아포칼립스 세계를 배경으로 하는 **4인 협동 TPS 로그라이트 게임**입니다.

플레이어는 거점(Hub)에서 캐릭터를 준비하고 스테이지에 진입하여 다양한 적과 전투를 진행합니다.

런(Run) 동안 획득한 증강(Augment)과 파츠를 조합하여 자신만의 빌드를 완성하고,
런 종료 후에는 거점으로 돌아와 영구 성장 시스템을 통해 다음 도전을 준비합니다.

---

# 프로젝트 개요

| 항목 | 내용 |
|------|------|
| 프로젝트 | NeoSanctum |
| 장르 | TPS 로그라이트 |
| 엔진 | Unreal Engine 5.7 |
| 언어 | C++ |
| 플랫폼 | Windows |
| 플레이 인원 | 1 ~ 4명 |
| 네트워크 | Listen Server |
| 개발 형태 | 팀 프로젝트 |

---

# 핵심 콘텐츠

## ① 4인 협동 TPS

- 최대 4인 협동 플레이
- 캐릭터 중복 선택 가능
- 각 캐릭터의 역할을 활용한 협동 전투

---

## ② 로그라이트 성장 시스템

런마다 다양한 증강과 파츠를 획득하여 빌드를 구성합니다.

- 증강(Augment)
- 파츠
- 골드
- NPC 상점

매번 다른 조합을 통해 새로운 플레이 경험을 제공합니다.

---

## ③ 영구 성장 시스템

런 종료 후 획득한 재화를 사용하여

- 공통 스킬트리
- 캐릭터 스킬트리
- NPC 해금

등의 영구 성장 요소를 강화할 수 있습니다.

---

# 게임 루프

NeoSanctum은 세 가지 루프를 중심으로 설계되었습니다.

```

거점 (Macro Loop)

    ↓

스테이지 진행 (Mid Loop)

    ↓

룸 전투 (Micro Loop)

    ↓

증강 선택

    ↓

다음 스테이지

    ↓

런 종료

    ↓

거점 복귀

```

### Micro Loop

- 이동
- 회피
- 사격
- 스킬 사용

즉각적인 전투의 재미를 제공합니다.

---

### Mid Loop

- 증강 선택
- 파츠 구매
- 빌드 구성

매 런마다 새로운 플레이 스타일을 경험합니다.

---

### Macro Loop

- 스킬트리 성장
- NPC 해금
- 다음 런 준비

플레이가 반복될수록 성장하는 구조를 제공합니다.

---

# 플레이어 캐릭터

## Ranger

원거리 화력에 특화된 캐릭터

### Skill

- 기본 공격
- 로켓
- 수류탄
- 공격속도 버프

---

## Engineer

보조 화력과 생존 지원에 특화된 캐릭터

### Skill

- 샷건
- 터렛 소환
- 방어막
- 공격속도 버프

---

# 적 구성

프로젝트에서는 다양한 전투 패턴을 가진 AI를 구현했습니다.

- 근접 몬스터
- 원거리 몬스터
- 엘리트 몬스터

각 몬스터는 서로 다른 Behavior Tree를 기반으로 행동하며,
상황에 따라 엄폐, 추적, 공격을 수행합니다.

---

# 주요 시스템

## ⚔ 전투 시스템

Gameplay Ability System(GAS)을 기반으로 다양한 전투 기능을 구현했습니다.

**주요 기능**

- 기본 공격 및 캐릭터 스킬
- 투사체 시스템
- 버프 / 디버프
- 피격 및 사망 처리
- Gameplay Tag 기반 입력 처리

**사용 기술**

- `Gameplay Ability System`
- `Gameplay Effect`
- `Gameplay Cue`
- `Gameplay Tag`
- `Projectile`
- `Niagara`

---

## 🤖 AI 시스템

Behavior Tree 기반의 적 AI를 구현했습니다.

**주요 기능**

- 순찰 및 추적
- 근접 / 원거리 AI
- 엄폐 및 Peek & Shoot
- Threat 기반 타깃 변경
- 근접 공격 예약 시스템

**사용 기술**

- `Behavior Tree`
- `Blackboard`
- `EQS`
- `AI Perception`
- `Navigation`

---

## 📈 성장 시스템

로그라이트 특성을 살린 다양한 성장 요소를 제공합니다.

**주요 기능**

- 증강(Augment)
- 파츠 장착 및 강화
- 재화 획득
- NPC 상점 및 리롤

**사용 기술**

- `Primary Data Asset`
- `Gameplay Effect`
- `Inventory Component`

---

## 🌐 멀티플레이 시스템

최대 4인이 함께 플레이할 수 있는 협동 환경을 제공합니다.

**주요 기능**

- Listen Server
- 로비 및 파티
- Ready 시스템
- Replication
- Seamless Travel

**사용 기술**

- `Replication`
- `RPC`
- `PlayerState`
- `GameState`

---

## 📊 데이터 시스템

프로젝트 전반을 Data Driven 구조로 설계했습니다.

**주요 기능**

- 플레이어 데이터
- 몬스터 데이터
- 증강 데이터
- 파츠 데이터
- 런 데이터 관리

**사용 기술**

- `Primary Data Asset`
- `Subsystem`
- `Save Game`

---

## 🎨 UI & 콘텐츠 시스템

게임 플레이를 지원하는 다양한 UI와 콘텐츠를 제공합니다.

**주요 기능**

- HUD
- Lobby
- 증강 선택 UI
- 상점 UI
- 결과 화면

**사용 기술**

`CommonUI`
`MVVM`
`UMG`


---

# 기술 스택

| 분야 | 기술 |
|------|------|
| Engine | Unreal Engine 5.7 |
| Language | C++ |
| Combat | Gameplay Ability System |
| AI | Behavior Tree / EQS / AI Perception |
| Network | Listen Server / Replication |
| Animation | Motion Warping / Pose Search |
| Input | Enhanced Input |
| UI | CommonUI / MVVM |
| VFX | Niagara |
| Procedural | PCG |

---

# 프로젝트 구조

```

Source
├── AI
├── Character
├── Combat
├── Core
├── Data
├── GAS
├── Input
├── Progression
├── System
└── UI

```
