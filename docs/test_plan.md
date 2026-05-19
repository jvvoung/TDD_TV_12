# TDD_TV_Init — 단위 테스트 계획서

| 항목 | 내용 |
|------|------|
| **저장 경로** | `docs/test_plan.md` (팀 문서 SSOT: `docs/`) |
| **프로젝트명** | TDD_TV_Init (`bestreviewer`, C++17) |
| **작성일** | 2026-05-19 |
| **작성 관점** | 시니어 QA 리드 — TDD(Red→Green→Refactor), Google Mock 단위 테스트 |
| **참조 문서** | [`docs/requirements_analysis.md`](requirements_analysis.md) (SSOT), [`README.md`](../README.md), [`.cursorrules`](../.cursorrules) |
| **구현·테스트 대상** | `TVController`, `remoteKey` |
| **수정 금지** | `include/Tuner.h` |
| **테스트 타깅** | `TunerTest`, `TVControllerTest` (`CMakeLists.txt`) |

**실행 명령 (Windows / 프로젝트 루트):**

```bash
cmake -B build && cmake --build build && ctest --test-dir build
```

개별 타깅: `build\TVControllerTest.exe`, `build\TunerTest.exe` (빌드 환경에 따라 경로 상이)

---

## 1. 테스트 전략 요약

### 1.1 단위 테스트 범위

| 구분 | In scope | Out of scope |
|------|----------|--------------|
| **기능** | `TVController::pushButton` 및 private 헬퍼(숫자·확인·선호·검색·업/다운) | Tuner 실구현, RF/하드웨어 리모컨, EPG/볼륨/전원 |
| **계약** | `remoteKey` enum·`to_string` 전 키 | 키 스캔코드·센서 프로토콜 |
| **검증 수단** | `EXPECT_CALL` on `setCH` / `getCurrentCH` / `seekCH` | `std::cout` 로그 (`TVController.h`) |
| **채널 도메인** | `std::string` `"0"`~`"99"`, Controller 선검증 | Tuner `invalid_argument` throw 본문 (`TunerTest` 영역) |
| **파일** | `test/TVControllerTest.cpp` 확장 | `test/TunerTest.cpp` 본문 수정 |
| **빌드** | `gtest_discover_tests`, `ctest` Green | `build/` 산출물 검증, gcov/lcov (본 계획 §8 부록, **선택 적용**) |

비즈니스 규칙·경계·시나리오 ID는 **`requirements_analysis.md` §1, §3~4, §6** 과 **1:1 추적**한다. 본 계획의 **테스트 ID**(`TC-*`) ↔ **시나리오 ID**(`1-1`, `A3` 등) 매핑은 §3·§6 표를 SSOT로 한다.

### 1.2 Mock vs Fake 사용 기준

| 목적 | 도구 | 사용 시점 | 예시 |
|------|------|-----------|------|
| **Controller 행위 검증** | **Google Mock** (`MockTunerForController`) | `pushButton` 후 Tuner 호출 여부·인자·횟수·순서 | `EXPECT_CALL(mock, setCH("12")).Times(1)` |
| **Tuner 인터페이스 계약 학습** | Mock (`TunerTest.cpp`의 `MockTuner`) | 유효/무효 채널, `seekCH` 루프 관례 | `TunerValidChannelTest`, `testSeekCh10times` — **참고만**, 수정 금지 |
| **Fake (선택)** | `FakeTuner` — 내부 `currentCh` 유지 | Mock 시퀀스가 과도할 때 통합에 가까운 시나리오 | 검색 후 연속 업/다운(다회 `getCurrentCH` 일관성) — **기본은 Mock 우선** |

**원칙**

- Controller 테스트의 **Then**은 거의 항상 **`setCH` / `getCurrentCH` / `seekCH` Mock 기대**로 표현한다.
- `favoriteChannels`, `searchResults`는 private → **직접 assert 불가** 시 `setCH` 인자·`seekCH` Times·연속 `pushButton` 후 Mock 기대로 간접 검증(§9).
- Tuner가 무효 채널에 `throw`하는 계약은 **`TunerInvalidChannelTest`**; Controller는 **선검증으로 `setCH` 미호출**만 검증.

### 1.3 TDD 진행 순서 (P0 / P1 / P2)

| 우선순위 | 범위 | README / requirements | TDD 목표 |
|----------|------|------------------------|----------|
| **P0** | 숫자+확인, 2자리 즉시 확정, `0`+`7`, `4`/`5`/`6`+OK·무효화 | 시나리오 **1** → `1-1`~`1-5`, `1-4a/b` | `remoteKey` 0~9·OK, `setTunerCh` 활성화, 버퍼·유효성 |
| **P0** | 업/다운 (검색 없음), 0↔99 래핑 | 시나리오 **5** → `5-1`~`5-4` | ±1, `getCurrentCH`→`setCH` |
| **P1** | 선호 토글, 다음 선호(로테이션) | 시나리오 **2**, **3** → `2-1`~`2-2`, `3-1`~`3-2` | `favoriteChannels` 간접 검증 |
| **P1** | 채널 검색·결과 저장 | 시나리오 **4** → `4-1` | `seekCH` until `""`, 정렬·중복 제거 |
| **P1** | 업/다운 (검색 있음) | 시나리오 **6** → `6-1`~`6-4` | `searchResults.empty()` 분기 |
| **P2** | 경계·모호 해소 | §6.2 **A1**~**A8**, E1~E15 | 빈 OK, `99`+세 번째 숫자, 재검색, 무효 `getCurrentCH`, 단일 검색 결과 래핑 등 |

**사이클**: 각 테스트 1건 추가(Red) → `TVController`/`remoteKey` 최소 구현(Green) → 중복·상수·헬퍼 추출(Refactor) → `ctest` 전체 Green.

---

## 2. 테스트 구조·Fixture 설계

### 2.1 기본 Fixture (`TEST_F`)

```cpp
class TVControllerTest : public ::testing::Test {
protected:
    MockTunerForController mockTuner;
    TVController controller{&mockTuner};

    void givenCurrentChannel(const std::string& ch) {
        ON_CALL(mockTuner, getCurrentCH()).WillByDefault(::testing::Return(ch));
    }

    void expectSetCH(const std::string& ch, int times = 1) {
        EXPECT_CALL(mockTuner, setCH(ch)).Times(times);
    }
};
```

- `testFramework` 스켈레톤 제거 후 위 Fixture로 통일.
- Given 채널: `ON_CALL` / `WillRepeatedly(Return("6"))` — 시나리오별 `givenCurrentChannel`.

### 2.2 Parameterized 테스트 후보 (`TEST_P`)

| Suite 이름 | 파라미터 | 대응 | 참고 패턴 |
|------------|----------|------|-----------|
| `TVControllerWrapChannelTest` | `(current, keyDir, expected)` | `5-3`, `5-4`, E2~E3 | `("99","up","0")`, `("0","down","99")` |
| `TVControllerDigitConfirmTest` | `(keys..., expectedSetCh)` | `1-1`, `1-2`, `1-5` | README 숫자 조합 |
| `TVControllerInvalidChannelTest` | `(digitSequence, mustNotCallSetCh)` | §3.2, 2.15 | `TunerTest` `InvalidChannels` — Controller는 **setCH Times(0)** |
| `TVControllerSearchNavigateTest` | `(searchList, current, up/down, expected)` | `6-1`~`6-4` | 목록 외 채널 `15` 포함 |

`INSTANTIATE_TEST_SUITE_P` 네이밍: `WrapChannels`, `DigitPatterns`, `InvalidBuffer`, `SearchNav` (접두사는 팀 일관성 유지).

### 2.3 파일·타깅·CMake

| 항목 | 권장 | 비고 |
|------|------|------|
| **파일** | `test/TVControllerTest.cpp` **단일 확장** | 시나리오 ~30건 미만 → 분리 불필요 |
| **분리 조건** | 500행 초과 또는 digit/search 팀 분담 시 | `TVControllerDigitTest.cpp` + 동일 `MockTunerForController` 헤더화(선택) |
| **CMake** | 기존 `add_executable(TVControllerTest ...)` 유지 | 신규 파일 추가 시에만 `test/...cpp` 나열 확장 |
| **발견** | `gtest_discover_tests(TVControllerTest)` 유지 | 필터: `ctest -R TVController` |

공통 Mock은 `TVControllerTest.cpp` 상단에 두고, `TunerTest.cpp`의 `MOCK_METHOD` 3종 시그니처를 **동일**하게 유지.

---

## 3. 단위 테스트 범위·우선순위 표

> **테스트 ID** ↔ **requirements §6 시나리오 ID** 1:1. 우선순위: P0(필수) / P1(README 완성) / P2(경계·모호).

### §1.1 숫자 입력·확인

| 테스트 ID | P | 시나리오 | 검증 포인트 | Mock 기대 요약 |
|-----------|---|----------|-------------|----------------|
| TC-DIG-01 | P0 | 1-1 | 1자리+OK → 채널 1 | `setCH("1")` ×1 |
| TC-DIG-02 | P0 | 1-2 | 1,2 → 12, OK 없음 | `setCH("12")` ×1 |
| TC-DIG-03 | P0 | 1-3 | 1,2,3,4 → 12 then 34 | `setCH("12")`, `setCH("34")` |
| TC-DIG-04a | P0 | 1-4a | 4,5,6,OK → 45 then 6 | `setCH("45")`, `setCH("6")` |
| TC-DIG-04b | P0 | 1-4b | 4,5,6 후 UP → 6 무효 | `setCH("45")` only; 이후 `setCH` Times(0) |
| TC-DIG-05 | P0 | 1-5 | 0,7 → 7 (선행 0 정규화) | `setCH("7")` ×1 |
| TC-DIG-06 | P2 | A7 | 1만 입력, OK 없음 | `setCH` Times(0) |
| TC-DIG-07 | P2 | A1 | 빈 버퍼+OK | `setCH` Times(0) |
| TC-DIG-08 | P2 | A2 | 9,9 → 99 후 버퍼 "9" | `setCH("99")`; 추가 입력 전까지 `setCH` 없음 |
| TC-DIG-09 | P2 | — (§3.2) | 무효 버퍼 확정 시 미호출 | `setCH` Times(0) (`TEST_P` Invalid) |
| TC-ERR-01 | P2 | E14 | `getCurrentCH` 무효 문자열 시 UP/DOWN/선호 no-op | `getCurrentCH` → `"abc"`; `setCH` Times(0) |

### §1.2 선호 채널 토글

| 테스트 ID | P | 시나리오 | 검증 포인트 | Mock 기대 요약 |
|-----------|---|----------|-------------|----------------|
| TC-FAV-01 | P1 | 2-1 | 비선호 → 추가, 채널 유지 | `getCurrentCH` → `"6"`; `setCH` Times(0) |
| TC-FAV-02 | P1 | 2-2 | 선호 → 제거 | `getCurrentCH`; `setCH` Times(0); 2회 토글 시 동일 |
| TC-FAV-03 | P2 | A6 | 버퍼 "6" 중 토글 → 버퍼 클리어 | `getCurrentCH`; `setCH` Times(0) |

### §1.3 다음 선호 채널

| 테스트 ID | P | 시나리오 | 검증 포인트 | Mock 기대 요약 |
|-----------|---|----------|-------------|----------------|
| TC-NFAV-01 | P1 | 3-1 | 현재 6, 선호 {1,4,12,56} → 12 | `getCurrentCH`, `setCH("12")` |
| TC-NFAV-02 | P1 | 3-2 | 현재 56 → 로테이션 1 | `setCH("1")` |
| TC-NFAV-03 | P2 | A3 | 선호 없음 | `setCH` Times(0) |
| TC-NFAV-04 | P2 | E5 | 선호 1개 {7}, 현재 7 (§9 결정안) | §9.1: `setCH("7")` ×1 **고정** |

### §1.4 채널 검색

| 테스트 ID | P | 시나리오 | 검증 포인트 | Mock 기대 요약 |
|-----------|---|----------|-------------|----------------|
| TC-SRCH-01 | P1 | 4-1 | seek 4,6,14,"" → 3개 저장 후 검색 모드 | `seekCH` Times(4) |
| TC-SRCH-02 | P2 | A4 | seek 즉시 "" → 이후 ±1 | `seekCH` ×1; 업 시 `setCH("51")` |
| TC-SRCH-03 | P2 | A5 | 재검색 시 결과 교체 | 2차 `seekCH` 시퀀스; 업은 `{10}` 기준만 |

### §1.5 업/다운 — 검색 없음

| 테스트 ID | P | 시나리오 | 검증 포인트 | Mock 기대 요약 |
|-----------|---|----------|-------------|----------------|
| TC-UPDN-01 | P0 | 5-1 | 6 + UP → 7 | `getCurrentCH`, `setCH("7")` |
| TC-UPDN-02 | P0 | 5-2 | 6 + DOWN → 5 | `setCH("5")` |
| TC-UPDN-03 | P0 | 5-3 | 99 + UP → 0 | `setCH("0")` (`TEST_P` 후보) |
| TC-UPDN-04 | P0 | 5-4 | 0 + DOWN → 99 | `setCH("99")` (`TEST_P` 후보) |

### §1.6 업/다운 — 검색 있음

| 테스트 ID | P | 시나리오 | 검증 포인트 | Mock 기대 요약 |
|-----------|---|----------|-------------|----------------|
| TC-SUPDN-01 | P1 | 6-1 | {4,6,14}, 현재 6, UP → 14 | `setCH("14")` |
| TC-SUPDN-02 | P1 | 6-2 | 동일, DOWN → 4 | `setCH("4")` |
| TC-SUPDN-03 | P1 | 6-3 | 현재 15(목록 외), UP → 4 | `setCH("4")` |
| TC-SUPDN-04 | P1 | 6-4 | 현재 15, DOWN → 14 | `setCH("14")` |
| TC-SUPDN-05 | P2 | A8 | 검색 {6}, 현재 6, UP (§9 결정안) | §9.2: `setCH("6")` ×1 **고정** |

---

## 4. 경계값 케이스 목록

| # | 영역 | 입력 / 상태 | 기대 (Then) | 테스트 ID / 시나리오 |
|---|------|-------------|-------------|----------------------|
| B1 | 채널 | `0`, `1`, `99` 단독 확정 | 유효 `setCH` | `TEST_P` Valid (`TunerTest` Values 참고) |
| B2 | 래핑 | 99 + UP (검색 없음) | `setCH("0")` | 5-3, TC-UPDN-03 |
| B3 | 래핑 | 0 + DOWN (검색 없음) | `setCH("99")` | 5-4, TC-UPDN-04 |
| B4 | 숫자 | 1자리 + OK | `setCH` 1회 | 1-1 |
| B5 | 숫자 | 2자리 연속 (12) | OK 없이 `setCH("12")` | 1-2 |
| B6 | 숫자 | `0` + `7` | `setCH("7")` (not `"07"`) | 1-5 |
| B7 | 숫자 | `9`, `9` 후 세 번째 `9` | `99` 확정, 버퍼 `"9"` | A2 |
| B8 | 숫자 | 빈 버퍼 + OK | no-op | A1 |
| B9 | 선호 | 0개 + NEXT_FAVORITE | 채널 유지 | A3, E4 |
| B10 | 선호 | 1개 / 다수 | 토글·다음 선호 | 2-1, 3-1, 3-2 |
| B11 | 선호 | 현재 56, {1,4,12,56} | 로테이션 → 1 | 3-2, E6 |
| B12 | 검색 | `seekCH` 즉시 `""` | 결과 0개; 이후 ±1 | A4, E7 |
| B13 | 검색 | 단일 `{6}` | 업/다운 래핑 → 6 | A8, E10 |
| B14 | 검색 | {4,6,14}, 현재 15 | UP→4, DOWN→14 | 6-3, 6-4, E9 |
| B15 | 분기 | `searchResults.empty()` true | §1.5 ±1 | 5-x |
| B16 | 분기 | `searchResults` non-empty | §1.6 목록 탐색 | 6-x |

---

## 5. 예외·특이 케이스 목록

| # | 케이스 | When | Then | 테스트 / 비고 |
|---|--------|------|------|----------------|
| X1 | 미확정 버퍼 + 비숫자·비OK | `4`,`5`,`6` 후 UP/DOWN/선호/검색/NEXT_FAV | `45`만 확정; `"6"` 무효; `setCH` 추가 없음 | 1-4b, §2.4 |
| X2 | Controller 선검증 | 버퍼가 100 등 무효(2자리 조합 후 stoi>99) | `setCH` never | TC-DIG-09, E12 |
| X3 | `getCurrentCH` 무효 문자열 | UP/DOWN/선호/다음선호 | no-op, `setCH` Times(0) | E14, 신규 TC-ERR-01 |
| X4 | 선호 없음 + NEXT_FAVORITE | KEY_NEXT_FAVORITE | `setCH` Times(0) | A3 |
| X5 | 선호 토글 | FAVORITE_TOGGLE | `setCH` Times(0), `getCurrentCH` only | 2-1, 2-2 |
| X6 | Tuner `invalid_argument` | Mock `setCH` throw | **TunerTest** (`TunerInvalidChannelTest`) | Controller 테스트와 **역할 분리** |
| X7 | 입력 중 검색/선호/업다운 | 버퍼 비어 있지 않음 | 버퍼 클리어 후 해당 동작 | A6, E15 |
| X8 | 연속 KEY_OK | 빈 버퍼 | no-op | A1 |
| X9 | 1차 검색 후 2차 검색 | 결과 교체 | 2차 목록만 업/다운에 사용 | A5 |

---

## 6. 시나리오 ↔ 테스트 케이스 매트릭스

### 6.1 README 시나리오 (§6.1)

| 시나리오 | 제안 테스트 이름 (`TEST_F` / `TEST_P`) | Given (1줄) | When (1줄) | Then (1줄) | `EXPECT_CALL` 핵심 |
|----------|----------------------------------------|-------------|------------|------------|-------------------|
| **1-1** | `GivenZero_WhenOneThenOk_ThenChannelOne` | 현재 `"0"`, 버퍼 비움 | `KEY_1`, `KEY_OK` | 채널 1 | `setCH("1")` Times(1) |
| **1-2** | `GivenZero_WhenOneTwo_ThenChannelTwelveNoOk` | 현재 `"0"` | `KEY_1`, `KEY_2` | 12, OK 없음 | `setCH("12")` Times(1) |
| **1-3** | `GivenZero_WhenFourDigits_ThenTwelveThenThirtyFour` | 현재 `"0"` | `1,2,3,4` | 12 후 34 | `setCH("12")`, `setCH("34")` |
| **1-4a** | `GivenZero_WhenFourFiveSixOk_ThenFortyFiveThenSix` | 현재 `"0"` | `4,5,6`, OK | 45 후 6 | `setCH("45")`, `setCH("6")` |
| **1-4b** | `GivenPendingSix_WhenChUp_ThenFortyFiveUnchanged` | `4,5,6`까지 입력됨 | `KEY_CH_UP` | 45 유지, 6 무효 | `setCH("45")` only; 이후 Times(0) |
| **1-5** | `GivenZero_WhenZeroSeven_ThenChannelSeven` | 현재 `"0"` | `KEY_0`, `KEY_7` | 채널 7 | `setCH("7")` Times(1) |
| **2-1** | `GivenSixNotFavorite_WhenToggle_ThenAddedNoSetCh` | 현재 `"6"`, 선호 없음 | `KEY_FAVORITE_TOGGLE` | 선호 추가, 채널 유지 | `getCurrentCH`; `setCH` Times(0) |
| **2-2** | `GivenSixFavorite_WhenToggle_ThenRemovedNoSetCh` | 현재 `"6"`, 선호 {6} | `KEY_FAVORITE_TOGGLE` | 선호 제거 | `setCH` Times(0) |
| **3-1** | `GivenSixAndFavorites_WhenNextFav_ThenTwelve` | 현재 `"6"`, 선호 {1,4,12,56} | `KEY_NEXT_FAVORITE` | 12 | `getCurrentCH`; `setCH("12")` |
| **3-2** | `GivenFiftySix_WhenNextFav_ThenRotateToOne` | 현재 `"56"`, 동일 선호 | `KEY_NEXT_FAVORITE` | 1 | `setCH("1")` |
| **4-1** | `GivenTuner_WhenSearch_ThenSeekUntilEmpty` | — | `KEY_CHANNEL_SEARCH` | 결과 3개 | `seekCH` Times(4), Return `"4","6","14",""` |
| **5-1** | `GivenSixNoSearch_WhenUp_ThenSeven` | `"6"`, 검색 없음 | `KEY_CH_UP` | 7 | `getCurrentCH`; `setCH("7")` |
| **5-2** | `GivenSixNoSearch_WhenDown_ThenFive` | 동일 | `KEY_CH_DOWN` | 5 | `setCH("5")` |
| **5-3** | `GivenNinetyNineNoSearch_WhenUp_ThenZero` | `"99"` | `KEY_CH_UP` | 0 | `setCH("0")` |
| **5-4** | `GivenZeroNoSearch_WhenDown_ThenNinetyNine` | `"0"` | `KEY_CH_DOWN` | 99 | `setCH("99")` |
| **6-1** | `GivenSearchList_WhenUpFromSix_ThenFourteen` | 검색 {4,6,14}, 현재 6 | `KEY_CH_UP` | 14 | `setCH("14")` |
| **6-2** | `GivenSearchList_WhenDownFromSix_ThenFour` | 동일 | `KEY_CH_DOWN` | 4 | `setCH("4")` |
| **6-3** | `GivenSearchList_WhenUpFromFifteen_ThenFour` | 현재 15 | `KEY_CH_UP` | 4 | `setCH("4")` |
| **6-4** | `GivenSearchList_WhenDownFromFifteen_ThenFourteen` | 현재 15 | `KEY_CH_DOWN` | 14 | `setCH("14")` |

### 6.2 경계·모호 해소 (§6.2)

| 시나리오 | 제안 테스트 이름 | Given | When | Then | `EXPECT_CALL` 핵심 |
|----------|------------------|-------|------|------|-------------------|
| **A1** | `GivenEmptyBuffer_WhenOk_ThenNoOp` | 버퍼 비움 | `KEY_OK` | 변경 없음 | `setCH` Times(0) |
| **A2** | `GivenNine_WhenNineAgain_ThenNinetyNineThenBufferNine` | 버퍼 `"9"` | `KEY_9` | 99 확정, 버퍼 9 | `setCH("99")` |
| **A3** | `GivenNoFavorites_WhenNextFav_ThenNoOp` | 선호 없음 | `KEY_NEXT_FAVORITE` | 유지 | `setCH` Times(0) |
| **A4** | `GivenEmptySeek_WhenSearchThenUp_ThenPlusOne` | seek→`""` | 검색 후 UP, 현재 50 | 51 | `seekCH` ×1; `setCH("51")` |
| **A5** | `GivenSecondSearch_WhenUp_ThenTenOnlyList` | 1차 {4,6}, 2차 {10} | 재검색 후 UP | {10} 기준 동작 | 2차 seek 시퀀스; `setCH` per §1.6 |
| **A6** | `GivenBufferSix_WhenFavToggle_ThenClearedAndToggle` | 버퍼 `"6"` | `KEY_FAVORITE_TOGGLE` | 버퍼 클리어+토글 | `getCurrentCH`; `setCH` Times(0) |
| **A7** | `GivenZero_WhenOneOnly_ThenNoSetCh` | — | `KEY_1` only | 대기 | `setCH` Times(0) |
| **A8** | `GivenSingleSearchSix_WhenUp_ThenStillSix` | 검색 {6}, 현재 6 | `KEY_CH_UP` | 6 (래핑) | `setCH("6")` Times(1) — §9.2 |
| **E14** | `GivenInvalidCurrentCh_WhenUp_ThenNoOp` | `getCurrentCH` → `"abc"` | `KEY_CH_UP` | 변경 없음 | `setCH` Times(0) |

### 6.3 테스트 ID ↔ 시나리오 ID (추적용)

| 시나리오 | 테스트 ID |
|----------|-----------|
| 1-1 | TC-DIG-01 |
| 1-2 | TC-DIG-02 |
| 1-3 | TC-DIG-03 |
| 1-4a | TC-DIG-04a |
| 1-4b | TC-DIG-04b |
| 1-5 | TC-DIG-05 |
| 2-1 | TC-FAV-01 |
| 2-2 | TC-FAV-02 |
| 3-1 | TC-NFAV-01 |
| 3-2 | TC-NFAV-02 |
| 4-1 | TC-SRCH-01 |
| 5-1~5-4 | TC-UPDN-01~04 |
| 6-1~6-4 | TC-SUPDN-01~04 |
| A1 | TC-DIG-07 |
| A2 | TC-DIG-08 |
| A3 | TC-NFAV-03 |
| A4 | TC-SRCH-02 |
| A5 | TC-SRCH-03 |
| A6 | TC-FAV-03 |
| A7 | TC-DIG-06 |
| A8 | TC-SUPDN-05 |
| E14 | TC-ERR-01 |

**선호·검색 Given 구성 (private 상태)**: `KEY_FAVORITE_TOGGLE`을 현재 채널에 대해 0~N회 호출해 `{1,4,12,56}` 등을 간접 구성; 검색은 `KEY_CHANNEL_SEARCH` + Mock `seekCH` 시퀀스(`WillOnce` 체인 또는 `InSequence`)로 `4-1`·`6-x` Given 고정.

---

## 7. TDD 실행 계획 (스프린트/단계)

### Phase 0 — 기반 (0.5일)

| 항목 | 내용 |
|------|------|
| **추가 테스트** | Fixture 도입; `testFramework` 제거 |
| **최소 구현** | `remoteKey` 0~9·OK·`to_string` 전체; `pushButton` switch 골격 |
| **리팩토링** | `MockTunerForController`, `givenCurrentChannel` 헬퍼 |
| **완료 기준** | `ctest` Green; 빌드·발견 정상 |

### Phase 1 — P0 숫자·확인 (1일)

| 추가 테스트 | TC-DIG-01, 02, 03, 04a, 04b, 05 (시나리오 1-1~1-5) |
| **최소 구현** | `processingCH`, 1자리+OK, 2자리 즉시 확정, `0`+다음 숫자, `setTunerCh`+0~99 검증, `tuner->setCH` |
| **리팩토링** | `handleDigit`, `handleConfirm`, `normalizeChannel`, `MIN/MAX_CHANNEL` |
| **완료 기준** | `ctest` Green; **1-1~1-5, 1-4a/b** 전부 통과 |

### Phase 2 — P0 업/다운 ±1 (0.5일)

| 추가 테스트 | TC-UPDN-01~04 (5-1~5-4); `TEST_P` Wrap optional |
| **최소 구현** | `KEY_CH_UP`/`DOWN`, `searchResults` 빈 경우 ±1·래핑, 버퍼 클리어 |
| **리팩토링** | `channelUp`/`channelDown`, `wrapChannel(int)` |
| **완료 기준** | **5-1~5-4** 통과 |

### Phase 3 — P1 선호·다음 선호 (1일)

| 추가 테스트 | TC-FAV-01~02, TC-NFAV-01~02 (2-1, 2-2, 3-1, 3-2) |
| **최소 구현** | `favoriteChannels` (`std::set<int>`), toggle, next with rotation |
| **리팩토링** | `toggleFavorite`, `nextFavorite` |
| **완료 기준** | **2-1, 2-2, 3-1, 3-2** 통과 |

### Phase 4 — P1 검색 + 검색 모드 업/다운 (1~1.5일)

| 추가 테스트 | TC-SRCH-01, TC-SUPDN-01~04 (4-1, 6-1~6-4) |
| **최소 구현** | `searchResults`, `runChannelSearch`, 분기 §1.5/§1.6 |
| **리팩토링** | `findNextInList`/`findPrevInList` 템플릿화 |
| **완료 기준** | **4-1, 6-1~6-4** 통과 |

### Phase 5 — P2 경계·모호 (1일)

| 추가 테스트 | A1~A8 → TC-DIG-06~09, TC-NFAV-03~04, TC-SRCH-02~03, TC-FAV-03, TC-SUPDN-05, TC-ERR-01 |
| **최소 구현** | E14 방어, A5 재검색, A8 단일 목록 래핑 |
| **리팩토링** | 중복 분기 제거, `clearBufferIfAny` 통합 |
| **완료 기준** | **§6.2 A1~A8** 전부 통과; `TunerTest` 회귀 Green |

### Phase 6 — 회귀·문서 (0.5일)

- `ctest` 전체; requirements §6 체크리스트 대조.
- 커버리지 측정(선택, §8).

---

## 8. 커버리지 목표·측정·개선 전략

### 8.1 목표

| 대상 | 목표 | 비고 |
|------|------|------|
| `include/TVController.h` | **라인 커버리지 90%+** | 권장; 팀 협의로 85% 하향 가능 |
| `include/remoteKey.h` | **라인 100%** (enum·`to_string` switch) | case 누락 방지 |
| **제외** | `Tuner.h`, `std::cout`, `test/`, `build/` | 계약·로그·테스트 코드 |

### 8.2 현황

- `CMakeLists.txt`에 **gcov/lcov 미설정** → 본 스프린트 **필수 아님**; Phase 6 또는 별도 브랜치에서 선택 적용.

### 8.3 미커버 구간 → 추가 테스트

| 미커버 예상 분기 | 추가 `TEST_P` / 테스트 |
|------------------|------------------------|
| `searchResults.empty()` true/false | 이미 5-x vs 6-x; 재검색 A5 |
| 버퍼 0/1/2자리 | A1, A7, 1-4b |
| 선호 0/1/다수 | A3, TC-NFAV-04, 3-2 |
| `getCurrentCH` 파싱 실패 | TC-ERR-01 |
| `KEY_OK` on 2자리 버퍼 | 중복 확정 없음 — A1 변형 |
| 모든 `remoteKey` case | smoke: each key no crash + Times 검증 |

### 8.4 부록 — CMake 커버리지 옵션 (선택)

```cmake
# 옵션: -DENABLE_COVERAGE=ON
option(ENABLE_COVERAGE "Build with coverage" OFF)
if(ENABLE_COVERAGE)
  target_compile_options(TVControllerTest PRIVATE --coverage -fprofile-arcs -ftest-coverage)
  target_link_options(TVControllerTest PRIVATE --coverage)
endif()
```

측정 예 (Linux/WSL):

```bash
cmake -B build -DENABLE_COVERAGE=ON && cmake --build build
ctest --test-dir build
lcov --capture --directory build --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/googletest/*' '*/test/*' --output-file coverage.filtered.info
genhtml coverage.filtered.info --output-directory build/coverage_html
```

Windows에서는 llvm-cov / OpenCppCoverage 등 환경별 도구 대체.

---

## 9. 리스크·미결정 사항

### 9.1 테스트에서 고정할 결정안

| requirements 참조 | 모호 내용 | **결정안 (테스트 고정)** |
|-------------------|-----------|---------------------------|
| §2.1, 2.3 | 1자리 확정 시점 | 1자리: **OK 필수**; 2자리: **두 번째 숫자에서 즉시 `setCH`** |
| §2.4 | “그 외 버튼” | 숫자·OK **외 모든 키**가 미확정 버퍼 클리어 |
| §2.5 | `0`,`7` | `"0"` 후 다음 숫자는 **대체(append 아님)** → `"7"` |
| §2.11 | 업/다운 분기 | **`searchResults.empty()` 단일 분기** |
| **E5**, §2.14 | 선호 1개, 현재=유일 선호, NEXT | **`setCH` 동일 채널 1회** (`setCH("7")`) — 로테이션=자기 자신 |
| **A8**, E10 | 검색 `{6}` only, UP | **`setCH("6")` Times(1)** — 이미 6이어도 호출(명시적 래핑) |
| E14 | 무효 `getCurrentCH` | **no-op**, `setCH` Times(0) |
| §2.8 | seek 상한 | 빈 문자열까지; **선택** 100회 방어 루프(구현 시에만, 테스트는 시퀀스로 제어) |

### 9.2 검증 한계·완화

| 한계 | 영향 | 완화 |
|------|------|------|
| `favoriteChannels`, `searchResults` private | 목록 내용 직접 assert 불가 | 연속 `NEXT_FAV`/`UP`으로 `setCH` 순서 검증; 2회 토글 후 동일 채널 |
| 검색 결과 정렬·unique | 내부 순서 미확인 | 6-x 시나리오에서 **업/다운 결과 채널**로 간접 검증 |
| Mock `getCurrentCH` 고정 | `setCH` 후 실제 상태 미반영 | 필요 시 `FakeTuner`로 `setCH` 시 `current` 갱신 (Phase 4+) |
| 로그 | 커버리지·동작 무관 | assert 금지 |

### 9.3 리스크

| 리스크 | 확률 | 대응 |
|--------|------|------|
| 헤더-only 비대화 | 중 | Phase별 private 헬퍼 분리; Green 후 추출 |
| `stoi` 예외 | 저 | try/catch + TC-DIG-09 |
| 시나리오 1·6 동시 회귀 | 중 | Phase 완료마다 full `ctest` |
| gcov 미설정으로 커버리지 미측정 | 중 | §8 부록은 Phase 6 선택 |

---

## 부록 A. `remoteKey` 확장 체크리스트 (구현 시)

- [ ] `KEY_0` … `KEY_9`, `KEY_CH_UP`, `KEY_CH_DOWN`, `KEY_OK`, `KEY_CHANNEL_SEARCH`, `KEY_FAVORITE_TOGGLE`, `KEY_NEXT_FAVORITE`
- [ ] `to_string` 모든 case
- [ ] `TVController::pushButton` 모든 case 위임

## 부록 B. 회귀 체크리스트 (릴리스 전)

- [ ] `ctest` — `TunerTest` + `TVControllerTest` 전부 Green
- [ ] §6.1 시나리오 **1-1~6-4** 매핑 테스트 존재
- [ ] §6.2 **A1~A8** 매핑 테스트 존재
- [ ] `Tuner.h` 미변경
- [ ] `TunerTest.cpp` 미변경

---

*본 계획서는 `docs/requirements_analysis.md`와 함께 TDD Red 단계의 실행 기준으로 사용한다. 충돌 시 requirements §2 권장 가정 및 §6 시나리오가 우선한다.*
