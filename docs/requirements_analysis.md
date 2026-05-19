# TDD_TV_Init — 요구사항 정밀 해석 (C++ / TDD)

| 항목 | 내용 |
|------|------|
| **프로젝트명** | TDD_TV_Init (`bestreviewer`, C++17) |
| **분석 일자** | 2026-05-19 |
| **분석 관점** | 시니어 C++ QA 엔지니어 — TDD 구현·검증 가능 명세 |
| **참조 파일** | `README.md`, `include/Tuner.h`, `include/TVController.h`, `include/remoteKey.h`, `test/TunerTest.cpp`, `test/TVControllerTest.cpp`, `CMakeLists.txt` |

---

## 0. 범위 요약

| 구분 | 대상 |
|------|------|
| **구현·테스트** | `TVController`, `remoteKey` 확장 |
| **외부 제공·수정 금지** | `Tuner` 인터페이스 (`setCH` / `getCurrentCH` / `seekCH`) |
| **채널 도메인** | `std::string` 표현, 정수 범위 **0~99** |
| **Tuner 가정** | 입력에 대해 정확 동작; Controller 테스트는 Mock/Fake |
| **검증 제외** | `TVController.h`의 `std::cout` 로그 |
| **Out of scope** | Tuner 실구현, 하드웨어 리모컨 프로토콜, 센서 키 매핑, UI/로그 검증, `TunerTest.cpp` 자체 수정 |

**현재 스켈레톤**: `remoteKey`는 `KEY_1`, `KEY_OK`만 존재. `setTunerCh()` 내 `tuner->setCH` 호출은 주석 처리됨.

---

## 1. 기능 영역별 비즈니스 규칙 표

> 아래 표의 **권장 구현 가정**은 §2 모호성 해소안과 동일하다. TDD 시 이 가정으로 테스트를 먼저 고정한다.

### 1.1 숫자 입력·확인 (채널 직접 입력)

| 트리거(키) | 전제조건 | 동작 | Tuner 호출 | 상태 변경 |
|------------|----------|------|------------|-------------|
| `KEY_0`~`KEY_9` | — | 자릿수 버퍼(`processingCH`)에 숫자 반영(§2.1 규칙) | 2자리 확정 시 `setCH(정규화 채널)`; 1자리만 있을 때는 대기 | 버퍼 갱신; 확정 시 버퍼 클리어 |
| `KEY_OK` | 버퍼 길이 **1** | 1자리 채널로 확정 | `setCH(버퍼)` | 버퍼 클리어 |
| `KEY_OK` | 버퍼 **비어 있음** | 무시(채널 변경 없음) | 없음 | 없음 |
| `KEY_OK` | 버퍼 길이 **2** | 이미 2자리 입력 시 자동 확정됨 → 중복 확정 없음 | 없음(이미 호출됨) | 없음 |
| 연속 숫자 (예: `1`,`2`) | 1자리 후 2번째 숫자 | 즉시 2자리 채널 확정 | `setCH("12")` | 버퍼 클리어 후 다음 숫자는 새 입력 |
| 연속 숫자 (예: `1`,`2`,`3`,`4`) | 2자리 확정 후 추가 숫자 | `12` 적용 후 `3`,`4` → `34` 적용 | `setCH("12")` → `setCH("34")` | 각 2자리마다 버퍼 리셋 |
| `4`,`5`,`6` 후 숫자+`KEY_OK` | `45` 확정 후 버퍼 `"6"` | `setCH("6")` | `setCH("6")` | 버퍼 클리어 |
| `4`,`5`,`6` 후 **그 외** 키 | 버퍼에 미확정 1자리 `"6"` | `6` 무효화, 채널 유지 | 없음 | 버퍼 클리어 |
| `0`,`7` | 선행 `0` 처리 | `07` → **`7`** 로 정규화 후 확정 | `setCH("7")` | 버퍼 클리어 |
| 잘못된 조합 (예: `9`,`9`,`9`) | 3번째 숫자 | 앞 2자리 `99` 확정 후 `9`는 새 1자리 | `setCH("99")` 후 버퍼 `"9"` | §2.1 |
| 유효하지 않은 채널(§3) | 버퍼 확정 시 | Controller가 **호출 전** 검증; 무효면 `setCH` 미호출 | 없음 | 버퍼 정책: 클리어(§2.8) |

### 1.2 선호 채널 추가/삭제 (토글)

| 트리거(키) | 전제조건 | 동작 | Tuner 호출 | 상태 변경 |
|------------|----------|------|------------|-------------|
| `KEY_FAVORITE_TOGGLE` (가칭) | 현재 채널이 선호 목록에 **없음** | 선호 목록에 추가 | `getCurrentCH()` | `favoriteChannels`에 현재 채널 추가(정렬 유지) |
| 동일 | 현재 채널이 선호 목록에 **있음** | 선호 목록에서 삭제 | `getCurrentCH()` | 목록에서 제거 |
| 동일 | 숫자 입력 버퍼 비어 있지 않음 | §2.4: 버퍼 무효화 후 토글 | `getCurrentCH()` | 버퍼 클리어 + 선호 토글 |

### 1.3 다음 선호 채널 (순환 포함)

| 트리거(키) | 전제조건 | 동작 | Tuner 호출 | 상태 변경 |
|------------|----------|------|------------|-------------|
| `KEY_NEXT_FAVORITE` (가칭) | 선호 목록 **비어 있음** | 채널 변경 없음 | 없음 | 없음 |
| 동일 | 선호 목록 **있음** | 현재 채널보다 **큰** 값 중 **최소** 채널로 이동; 없으면 목록 **최소값**(로테이션) | `getCurrentCH()` → `setCH(대상)` | 숫자 버퍼 클리어(§2.4) |
| 예시 | 선호 `{1,4,12,56}`, 현재 `6` | `12`로 이동 | `setCH("12")` | — |
| 예시 | 선호 `{1,4,12,56}`, 현재 `56` | 로테이션 → `1` | `setCH("1")` | — |

### 1.4 채널 검색 (전체 스캔·결과 저장)

| 트리거(키) | 전제조건 | 동작 | Tuner 호출 | 상태 변경 |
|------------|----------|------|------------|-------------|
| `KEY_CHANNEL_SEARCH` (가칭) | — | `seekCH()`를 **빈 문자열 반환 전까지** 반복 호출, 반환값을 검색 결과 목록에 저장 | `seekCH()` × N (0 ≤ N ≤ 100 가정) | `searchResults` **전체 교체**; 오름차순·중복 제거(§3.3) |
| 동일 | `seekCH()`가 즉시 `""` | 검색 결과 **0개** | `seekCH()` 1회 | `searchResults` = `{}` |
| 동일 | 숫자 버퍼 존재 | §2.4: 버퍼 무효화 후 검색 | 위와 동일 | 버퍼 클리어 |

### 1.5 채널 업/다운 — 검색 결과 **없음** (±1, 0↔99 래핑)

| 트리거(키) | 전제조건 | 동작 | Tuner 호출 | 상태 변경 |
|------------|----------|------|------------|-------------|
| `KEY_CH_UP` | `searchResults.empty()` | 현재+1, **99→0** | `getCurrentCH()` → `setCH` | 버퍼 클리어 |
| `KEY_CH_DOWN` | `searchResults.empty()` | 현재−1, **0→99** | `getCurrentCH()` → `setCH` | 버퍼 클리어 |
| 예시 | 현재 `6` | 업→`7`, 다운→`5` | `setCH("7")` / `setCH("5")` | — |
| 예시 | 현재 `99` / `0` | 업→`0` / 다운→`99` | `setCH("0")` / `setCH("99")` | — |

### 1.6 채널 업/다운 — 검색 결과 **있음** (저장 목록 내 이전/다음, 래핑)

| 트리거(키) | 전제조건 | 동작 | Tuner 호출 | 상태 변경 |
|------------|----------|------|------------|-------------|
| `KEY_CH_UP` | `searchResults` 비어 있지 않음 | 현재 채널 **초과**인 결과 중 **최소**; 없으면 결과 **최소값**(래핑) | `getCurrentCH()` → `setCH` | 버퍼 클리어 |
| `KEY_CH_DOWN` | 동일 | 현재 채널 **미만**인 결과 중 **최대**; 없으면 결과 **최대값**(래핑) | `getCurrentCH()` → `setCH` | 버퍼 클리어 |
| 예시 | 결과 `{4,6,14}`, 현재 `6` | 업→`14`, 다운→`4` | `setCH("14")` / `setCH("4")` | — |
| 예시 | 결과 `{4,6,14}`, 현재 `15`(목록 외) | 업→`4`, 다운→`14` | `setCH("4")` / `setCH("14")` | — |

> **분기 규칙**: `searchResults.empty()`이면 §1.5, 아니면 §1.6. 선호 목록은 업/다운 분기에 **관여하지 않음**.

---

## 2. `README.md` 모호·누락 명세 정리

| # | README 인용 | 해석 A | 해석 B | **권장 구현 가정 (TDD용)** | 테스트로 고정할 Acceptance |
|---|-------------|--------|--------|---------------------------|---------------------------|
| 2.1 | `‘1’ ‘확인’` → 1번 / `‘1’` 후 `‘2’` → 12번 | 1자리는 **확인 필수** | 모든 입력이 즉시 반영 | **1자리: `KEY_OK`로 확정. 2자리: 두 번째 숫자 입력 시 즉시 `setCH`.** | `1`+OK→`setCH("1")`; `1`,`2`→`setCH("12")` OK 없음 |
| 2.2 | `‘1’,’2’,’3’,’4’` → 12 후 34 | 2자리마다 자동 확정 | 4자리 누적 후 한 번에 34 | **2자리 자동 확정 후 버퍼 리셋, 이후 숫자는 새 조합** | `setCH("12")` 1회, `setCH("34")` 1회, 총 2회 |
| 2.3 | `‘4’,’5’,’6’` 후 “6번 또는 6_번” | `6`+확인만 유효 | `6`+다른 숫자로 6x 가능 | **`6` 단독 1자리 상태; `KEY_OK` 또는 두 번째 숫자로만 확정. `6_`는 미확정 1자리 시각적 표현(구현: 버퍼 `"6"`)** | `4`,`5`,`6`, OK→`setCH("6")`; `4`,`5`,`6`, UP→`setCH` 없음, 버퍼 empty |
| 2.4 | `그 외의 버튼을 누르면 6 은 무효화` | 숫자·OK 외 모든 키 | 비숫자만 | **숫자·`KEY_OK` 외 모든 키**(업/다운/선호/검색/다음선호 포함)가 미확정 버퍼 클리어 | `6` 대기 중 `KEY_CH_UP` → `setCH` 없음 |
| 2.5 | `‘0’,’7’` → 7번 | `07`→7 정규화 | `0` 무시 후 `7`만 | **버퍼가 `"0"`일 때 다음 숫자 d → 버퍼를 `"d"`로 대체(append 아님), 2자리면 즉시 확정** | `0`,`7`→`setCH("7")` 1회 |
| 2.6 | 채널 0~99 | 3자리 입력 허용 | 최대 2자리 | **최대 2자리; 세 번째 숫자는 새 1자리 시작** | `9`,`9`,`5`→`setCH("99")` 후 버퍼 `"5"` |
| 2.7 | 채널검색 — “모든 채널을 검색” | 고정 100회 | `seekCH` until empty | **`TunerTest.cpp` 관례: `seekCH()` 반복, `""`이면 종료** | Mock `seekCH` 시퀀스 `{"5","12",""}` → 결과 크기 2 |
| 2.8 | 검색 중 `seekCH` 횟수 | 최대 99회 | 무제한 until empty | **빈 문자열까지; 상한 100회 방어 루프(선택)** | 3회 반환 후 `""` → `EXPECT_CALL(seekCH, Times(4))` |
| 2.9 | 검색 결과 정렬 | 입력 순서 유지 | 오름차순 | **오름차순 정렬 + 중복 제거** | `{14,4,6}` 저장 → 내부 `{4,6,14}` |
| 2.10 | 재검색 | 이전 결과에 append | 교체 | **새 검색 시 `searchResults` 전체 교체** | 1차 `{4,6}` 2차 `{10}` → 업/다운은 `{10}` 기준 |
| 2.11 | 업/다운 vs 검색/선호 | 검색 있으면 항상 §6 | 검색 없을 때만 §5 | **`searchResults.empty()` 단일 분기** | 검색 후 업→목록 탐색; 검색 목록 clear 후 업→±1 |
| 2.12 | 선호 채널 순서 | 삽입 순서 | 숫자 오름차순 | **오름차순 유지(`std::set` 또는 정렬 vector)** | `{56,1,12}` → 다음선호 로직은 `{1,12,56}` |
| 2.13 | 선호 토글 시 채널 변경 | 변경 없음 | — | **목록만 변경, `setCH` 없음** | `EXPECT_CALL(setCH, Times(0))` |
| 2.14 | 다음 선호 — 현재=선호값 | 다음 값? | 자기 제외 다음 | **현재보다 큰 최소; 같으면 그 다음 큰 값(없으면 로테이션)** | 현재 `12`, 선호 `{1,4,12,56}` → `56` |
| 2.15 | 유효하지 않은 채널 | Tuner에 위임 | Controller 선검증 | **Controller가 0~99 검증 후 `setCH`; 무효 시 미호출** | `setCH` never called for invalid buffer |
| 2.16 | 초기 채널 | `0` | 미정 | **테스트마다 Mock `getCurrentCH`로 Given 지정** | `WillRepeatedly(Return("6"))` |
| 2.17 | `processingCH`와 Tuner | 항상 동기 | 버퍼만 로컬 | **`setCH` 성공 시 Tuner가 진실 공급원; 버퍼는 입력 중간 상태만** | `setCH` 후 `getCurrentCH`는 Mock이 반환한 값 |

---

## 3. 문자열·숫자 처리 주의점

### 3.1 채널 표현

| 항목 | 규칙 |
|------|------|
| API 형식 | Tuner는 `std::string` (`"0"`~`"99"`) |
| 내부 버퍼 | `processingCH`도 동일 문자열 누적 (스켈레톤과 일치) |
| 정규화 | 선행 `0` 제거: `"07"`→`"7"`, `"00"`→`"0"` (두 자리 모두 0이면 `"0"`) |
| 비교 | 선호/검색 목록은 **문자열이 아닌 정수값**으로 비교 권장: `std::stoi` 후 `0~99` (파싱 실패 시 방어) |
| 출력 형식 | `setCH` 인자는 **선행 0 없음** (`"7"` not `"07"`) unless Tuner contract says otherwise — `TunerTest`는 `"0"`,`"12"` 형태 |

### 3.2 유효/무효 채널 (`TunerTest.cpp` 경계)

| 구분 | 예시 | Controller 동작 |
|------|------|-----------------|
| 유효 | `"0"`, `"4"`, `"12"`, `"99"` | `setCH` 호출 |
| 무효 | `"-12"`, `"-2"`, `"100"`, `"9999"` | `setCH` **미호출** (Tuner가 throw할 수 있음) |
| 경계 | `"0"`, `"99"` | 정상 |
| 버퍼 확정 | `stoi` 후 `0 <= ch <= 99` | 아니면 무시 + 버퍼 클리어 |

### 3.3 선호·검색 목록

| 항목 | 권장 |
|------|------|
| 자료구조 | 선호: `std::set<int>` 또는 `std::vector<int>` + 정렬; 검색: `std::vector<int>` 정렬 |
| 중복 | `seekCH` 반복 시 동일 채널 반환 가능 → **삽입 시 중복 제거** |
| 정렬 | **오름차순** 고정 (README 예시 `{4,6,14}`, `{1,4,12,56}`) |
| 비교 | 업/다운·다음선호는 **정수 크기** 비교; 문자열 `"14"` vs `"6"` 오류 방지 |

### 3.4 `std::stoi` 사용 시

- `processingCH`가 비어 있으면 호출 금지.
- `invalid_argument` / `out_of_range` catch → 무효 입력으로 처리.
- 업/다운 ±1 계산은 **정수 연산 후 `std::to_string`** 권장 (래핑 명확).

---

## 4. 경계값·예외 조건 목록

| # | 조건 | 기대 동작 |
|---|------|-----------|
| E1 | 채널 `0`, `99` | 정상 `setCH`; 업/다운 래핑 기준점 |
| E2 | `99` + CH_UP | `setCH("0")` (검색 결과 없을 때) |
| E3 | `0` + CH_DOWN | `setCH("99")` |
| E4 | 선호 0개 + NEXT_FAVORITE | 채널 유지, `setCH` 없음 |
| E5 | 선호 1개 `{7}`, 현재 `7`, NEXT_FAVORITE | 로테이션 → `7` (자기 자신) 또는 README 미명시 → **권장: 유일 값이면 `setCH("7")` 1회(동일 채널) 또는 무시 — 테스트에서 README 예시 우선: 다수 요소 로테이션만 명시** |
| E6 | 선호 다수, 현재가 최대 초과 | 로테이션 → 최소 선호 |
| E7 | 검색 결과 0개 (`seekCH` 즉시 `""`) | 이후 업/다운은 §1.5 (±1) |
| E8 | 검색 결과 N개, 현재가 목록 **내부** | 업/다운은 이웃 채널 |
| E9 | 검색 결과 N개, 현재가 목록 **외부** | README §6: `15`→업`4`/다운`14` |
| E10 | 검색 결과 1개 `{6}` | 업/다운 모두 `6` (래핑) |
| E11 | 연속 `KEY_OK` (빈 버퍼) | no-op |
| E12 | 버퍼 `100` 불가 (2자리 상한) | `"10"`+`0` → `"10"` 확정 후 `"0"` |
| E13 | Tuner `setCH` throw | Controller는 선검증으로 **회피**; 통합 시 `EXPECT_THROW`는 TunerTest 영역 |
| E14 | `getCurrentCH()`가 무효 문자열 | Controller 방어: 업/다운/선호 시 no-op 또는 `0` 취급 — **권장: no-op + `setCH` 미호출** |
| E15 | 채널 입력 중 선호/검색/업다운 | 미확정 버퍼 무효화 (§2.4) |

---

## 5. `remoteKey` enum · `TVController::pushButton` 설계 힌트

### 5.1 `remoteKey` enum 후보 (README 키 목록)

```cpp
enum class remoteKey {
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_CH_UP,
    KEY_CH_DOWN,
    KEY_OK,
    KEY_CHANNEL_SEARCH,
    KEY_FAVORITE_TOGGLE,   // 선호채널추가
    KEY_NEXT_FAVORITE      // 다음선호채널
};
```

| README 키 | enum 후보 | `to_string()` |
|-----------|-----------|---------------|
| 0~9 | `KEY_0` … `KEY_9` | `"0"`…`"9"` |
| 채널 업 | `KEY_CH_UP` | `"CH_UP"` |
| 채널 다운 | `KEY_CH_DOWN` | `"CH_DOWN"` |
| 확인 | `KEY_OK` | `"OK"` |
| 채널검색 | `KEY_CHANNEL_SEARCH` | `"CH_SEARCH"` |
| 선호채널추가 | `KEY_FAVORITE_TOGGLE` | `"FAV_TOGGLE"` |
| 다음선호채널 | `KEY_NEXT_FAVORITE` | `"NEXT_FAV"` |

### 5.2 `TVController` 내부 상태

| 멤버 | 타입(권장) | 역할 |
|------|------------|------|
| `tuner` | `Tuner*` | DI (기존) |
| `processingCH` | `std::string` | 숫자 입력 버퍼 (기존) |
| `favoriteChannels` | `std::set<int>` | 선호 채널 (토글·다음선호) |
| `searchResults` | `std::vector<int>` | 마지막 채널 검색 결과 (정렬·유일) |

### 5.3 `pushButton` 처리 흐름 (개략)

```
pushButton(key)
  ├─ KEY_0..9     → handleDigit(key)
  ├─ KEY_OK        → handleConfirm()
  ├─ KEY_FAVORITE  → clearBufferIfAny(); toggleFavorite()
  ├─ KEY_NEXT_FAV  → clearBufferIfAny(); nextFavorite()
  ├─ KEY_CH_SEARCH → clearBufferIfAny(); runChannelSearch()
  ├─ KEY_CH_UP     → clearBufferIfAny(); channelUp()
  └─ KEY_CH_DOWN   → clearBufferIfAny(); channelDown()
```

- `setTunerCh(const std::string& ch)`: 유효성 검사 → `tuner->setCH(ch)` (로그 유지 가능, 검증 제외).
- `channelUp/Down`: `searchResults` 분기 → §1.5 / §1.6.
- `runChannelSearch`: `while ((ch = tuner->seekCH()) != "")` 수집 → 정렬·unique.

---

## 6. Google Test 기준 테스트 시나리오 목록

> 형식: **Given** / **When** / **Then** (1줄) + Mock 기대  
> 스타일: `TunerTest.cpp` — `TEST_F`, `TEST_P`, `INSTANTIATE_TEST_SUITE_P`  
> 파일 제안: `test/TVControllerTest.cpp` (단일), 또는 기능별 분리 시 `TVControllerDigitTest.cpp` 등 — **권장: 단일 `TVControllerTest.cpp` + fixture `TVControllerTest`**

### 6.1 README 시나리오 1~6

| No | Given-When-Then | Mock/Fake 기대 (`EXPECT_CALL`) |
|----|----------------|--------------------------------|
| **1-1** | **G** 현재 `"0"`, 버퍼 비움 / **W** `KEY_1`, `KEY_OK` / **T** 채널 `1` | `getCurrentCH` (필요 시), `setCH("1")` ×1 |
| **1-2** | **G** 현재 `"0"` / **W** `KEY_1`, `KEY_2` / **T** `12`, OK 없음 | `setCH("12")` ×1 |
| **1-3** | **G** 현재 `"0"` / **W** `1`,`2`,`3`,`4` / **T** `12` 후 `34` | `setCH("12")`, `setCH("34")` |
| **1-4a** | **G** 현재 `"0"` / **W** `4`,`5`,`6`, `KEY_OK` / **T** `45` 후 `6` | `setCH("45")`, `setCH("6")` |
| **1-4b** | **G** `4`,`5`,`6`까지 입력 / **W** `KEY_CH_UP` / **T** `45` 유지, `6` 무효 | `setCH("45")` only; 이후 `setCH` Times(0) |
| **1-5** | **G** 현재 `"0"` / **W** `KEY_0`, `KEY_7` / **T** 채널 `7` | `setCH("7")` ×1 |
| **2-1** | **G** 현재 `"6"`, 선호 없음 / **W** `KEY_FAVORITE_TOGGLE` / **T** 선호에 `6` 추가, 채널 유지 | `getCurrentCH` → `"6"`; `setCH` Times(0) |
| **2-2** | **G** 현재 `"6"`, 선호 `{6}` / **W** `KEY_FAVORITE_TOGGLE` / **T** 선호에서 `6` 제거 | `setCH` Times(0) |
| **3-1** | **G** 현재 `"6"`, 선호 `{1,4,12,56}` / **W** `KEY_NEXT_FAVORITE` / **T** `12` | `getCurrentCH`, `setCH("12")` |
| **3-2** | **G** 현재 `"56"`, 선호 `{1,4,12,56}` / **W** `KEY_NEXT_FAVORITE` / **T** `1` (로테이션) | `setCH("1")` |
| **4-1** | **G** — / **W** `KEY_CHANNEL_SEARCH`, `seekCH`→`"4"`,`"6"`,`"14"`,`""` / **T** 결과 3개 저장 | `seekCH` Times(4); 이후 업/다운은 검색 모드 |
| **5-1** | **G** 현재 `"6"`, 검색 없음 / **W** `KEY_CH_UP` / **T** `7` | `setCH("7")` |
| **5-2** | **G** 현재 `"6"`, 검색 없음 / **W** `KEY_CH_DOWN` / **T** `5` | `setCH("5")` |
| **5-3** | **G** 현재 `"99"`, 검색 없음 / **W** `KEY_CH_UP` / **T** `0` | `setCH("0")` |
| **5-4** | **G** 현재 `"0"`, 검색 없음 / **W** `KEY_CH_DOWN` / **T** `99` | `setCH("99")` |
| **6-1** | **G** 검색 `{4,6,14}`, 현재 `"6"` / **W** `KEY_CH_UP` / **T** `14` | `setCH("14")` |
| **6-2** | **G** 검색 `{4,6,14}`, 현재 `"6"` / **W** `KEY_CH_DOWN` / **T** `4` | `setCH("4")` |
| **6-3** | **G** 검색 `{4,6,14}`, 현재 `"15"` / **W** `KEY_CH_UP` / **T** `4` | `setCH("4")` |
| **6-4** | **G** 검색 `{4,6,14}`, 현재 `"15"` / **W** `KEY_CH_DOWN` / **T** `14` | `setCH("14")` |

### 6.2 경계·모호 해소 추가 시나리오

| No | Given-When-Then | Mock/Fake 기대 |
|----|-----------------|----------------|
| **A1** | **G** 빈 버퍼 / **W** `KEY_OK` / **T** no-op | `setCH` Times(0) |
| **A2** | **G** 버퍼 `"9"` / **W** `KEY_9` / **T** `99` 확정 후 버퍼 `"9"` | `setCH("99")`, 이후 버퍼 `"9"` |
| **A3** | **G** 검색 없음, 선호 없음 / **W** `KEY_NEXT_FAVORITE` / **T** 유지 | `setCH` Times(0) |
| **A4** | **G** `seekCH`→`""` 즉시 / **W** 검색 후 `KEY_CH_UP`, 현재 `"50"` / **T** `51` (±1 모드) | `seekCH` ×1; `setCH("51")` |
| **A5** | **G** 1차 검색 `{4,6}`, 2차 `seekCH`→`"10"`,`""` / **W** 재검색 후 업, 현재 `"10"` / **T** 래핑 동작은 단일 `{10}` | `setCH` with search mode on `{10}` only |
| **A6** | **G** 버퍼 `"6"` / **W** `KEY_FAVORITE_TOGGLE` / **T** 버퍼 클리어 + 선호 토글 | `getCurrentCH`; `setCH` Times(0) |
| **A7** | **G** — / **W** `KEY_1` only (OK 없음) / **T** `setCH` 없음 | `setCH` Times(0) |
| **A8** | **G** 검색 `{6}` only, 현재 `"6"` / **W** `KEY_CH_UP` / **T** `6` (단일 래핑) | `setCH("6")` or Times(0) if already on 6 — **테스트에서 하나로 고정** |

### 6.3 테스트 클래스·파일 제안

| 요소 | 제안 |
|------|------|
| Fixture | `class TVControllerTest : public ::testing::Test` + `MockTunerForController mockTuner` + `TVController controller{&mockTuner}` |
| Parameterized | `TVControllerWrapChannelTest` — `("99","up","0")`, `("0","down","99")` 등 |
| Parameterized | `TVControllerValidDigitTest` — README `1+OK`, `1,2`, `0,7` |
| 파일 | `test/TVControllerTest.cpp` (현 스켈레톤 확장) |
| 참고 | `test/TunerTest.cpp`의 `MockTuner`, `INSTANTIATE_TEST_SUITE_P(ValidChannels, …)` 패턴 재사용 |
| Nice to have | 내부 상태 검증용 `friend` 테스트 accessor 또는 package-private — **없으면 `setCH`/`getCurrentCH` 호출 순서만으로 검증** |

---

## 7. 구현 범위 밖·가정

| 항목 | 내용 |
|------|------|
| **Out of scope** | `Tuner` 구현체, 실제 RF/하드웨어 리모컨, 키 스캔코드 매핑, 멀티 디바이스, 전원/볼륨/메뉴 키 |
| **Out of scope** | `std::cout` 및 기타 로그 출력 검증 |
| **Out of scope** | README에 없는 기능: 최근 채널, EPG, 타이머, 입력 타임아웃, 3자리 채널, 해외 채널 번호 |
| **가정** | Tuner `getCurrentCH()`는 Controller가 `setCH` 호출 후 일관된 값을 반환(Mock이 스텁) |
| **가정** | `seekCH()`는 현재 위치에서 증가 방향으로 이동하며, 더 이상 채널 없으면 `""` 반환 (`TunerTest.cpp` 67–68, 85–86행) |
| **가정** | 동시에 하나의 `TVController` 인스턴스만 사용 (스레드 안전 불필요) |
| **가정** | 선호·검색 목록은 프로세스 수명 동안 유지 (영속화 없음) |
| **가정** | 채널 검색은 사용자가 검색 키를 누를 때마다 전체 재스캔 |

---

## 부록 A. README에 없는 기능 (명시적 Out of scope)

- 입력 idle 타임아웃 후 자동 확정
- `6_` UI 표시 및 별도 “부분 입력” 상태 플래그 (버퍼 문자열로 대체 가능)
- 선호 채널 최대 개수 제한
- 검색 결과와 선호 목록 간 우선순위/병합
- `KEY_OK` long-press, 더블 클릭
- 채널 이름/EPG 메타데이터

---

## 부록 B. 현재 코드와의 갭

| 파일 | 현황 | 필요 작업 |
|------|------|-----------|
| `remoteKey.h` | `KEY_1`, `KEY_OK`만 | §5.1 enum 전체 추가 + `to_string` |
| `TVController.h` | `KEY_1`/`KEY_OK`만 처리, `setCH` 주석 | §1 전 기능 + 상태 멤버 |
| `TVControllerTest.cpp` | `testFramework` 스켈레톤 | §6 시나리오 구현 |
| `Tuner.h` | 수정 금지 | — |

---

*본 문서는 TDD Red 단계에서 acceptance 테스트 작성의 단일 기준(Single Source of Truth)으로 사용한다. README와 충돌 시 §2의 권장 가정과 §6 테스트가 구현을 고정한다.*
