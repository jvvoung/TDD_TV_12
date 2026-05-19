# TDD_TV_Init — 테스트 결함 목록 (Defect List)

| 항목 | 내용 |
|------|------|
| 프로젝트 | TDD_TV_Init (bestreviewer) |
| 문서 버전 | 1.1 |
| 최종 검증일 | 2026-05-19 |
| 검증 근거 | [`Report/05`](../Report/05.테스트_결함분석.md), [`Report/06`](../Report/06.GoldenMaster_회귀테스트_완료보고서.md), `ctest` 전체 실행 |
| 운영 정책 | [`defect_report.md`](defect_report.md) |
| SSOT | [`requirements_analysis.md`](requirements_analysis.md), [`test_plan.md`](test_plan.md) |
| 검증 기준 | GMock `EXPECT_CALL` (`setCH` / `getCurrentCH` / `seekCH`); `std::cout`는 결함 판단 제외 |

---

## 1. 요약

| 구분 | 건수 |
|------|------|
| **Open** (미해결) | **0** |
| **Closed** (해결·검증 완료) | 0 |
| **Total** | 0 |
| `TunerTest` | 13 / 13 Passed |
| `TVControllerTest` | 39 / 39 Passed |
| `GoldenMasterTest` | 26 / 26 Passed |
| **합계** | **78 / 78 Passed** (실패 0) |

**판정**: 구현 완료(`Report/04`), QA 회귀(`Report/05`), Golden Master(`Report/06`) 시점까지, GMock·Golden 기준 **재현 가능한 테스트 실패·생산 코드 결함 0건**.

---

## 2. 심각도·유형 정의

### Severity

| 값 | 의미 |
|----|------|
| **Critical** | 채널 0~99 위반, 잘못된 `setCH`, 검색/선호 데이터 손상 |
| **Major** | README 시나리오 1~6 핵심 경로 실패 |
| **Minor** | 경계·A시나리오 단건, 리팩토링 회귀 |
| **Info** | 테스트명·로그·문서 불일치, GMock 경고(테스트 통과) |

### ItemType

| 값 | 의미 |
|----|------|
| **Production** | `include/TVController.h`, `include/remoteKey.h` 생산 코드 결함 |
| **Test** | `test/TVControllerTest.cpp` Mock 기대·셋업 결함 |
| **Contract** | `include/Tuner.h` 등 외부 계약 불일치 (본 프로젝트 수정 금지) |
| **Environment** | 빌드·CRT·CMake·도구체인 이슈 |
| **Documentation** | 명세·테스트·구현 불일치 |

---

## 3. 결함 목록

> **현재 등록된 Open 결함 없음.**  
> 실패 로그 발생 시 아래 표에 행을 추가하고, `Report/05` §4 형식으로 RCA·최소 diff를 연계한다.

| ID | Severity | ItemType | Steps | Expected | Actual | Root Cause | Fix Summary | Status |
|----|----------|----------|-------|----------|--------|------------|-------------|--------|
| — | — | — | — | — | — | — | — | — |

---

## 4. 관찰 사항 (결함 아님)

다음은 **테스트 통과** 상태에서 로그에만 나타난 항목이며, Open 결함으로 등록하지 않았다.

| ID | Severity | ItemType | Steps | Expected | Actual | Root Cause | Fix Summary | 비고 |
|----|----------|----------|-------|----------|--------|------------|-------------|------|
| OBS-001 | Info | Test | `TVControllerTest.TC_ERR_InvalidCurrentChUpNoOp` 실행 | `setCH` 미호출, 테스트 Green | GMock **Uninteresting mock function call** (`getCurrentCH` → `"abc"`) | `ON_CALL`만 설정·`EXPECT_CALL(getCurrentCH)` 미선언 | 선택: `EXPECT_CALL(getCurrentCH()).Times(1)` 추가로 경고 제거 | E14 방어 동작 검증 목적; 기능 결함 아님 |

---

## 5. 회귀 시 고위험 시나리오 (참고)

향후 Red 발생 시 우선 조사할 대표 시나리오. **현재는 모두 Green.**

| 시나리오 | 대표 테스트 | 가정 실패 시 Severity | 관련 구현 |
|----------|-------------|----------------------|-----------|
| 1-4b | `TC_1_4b_FourFiveSixUpInvalidatesSix` | Major | `pushButton` KEY_CH_UP + `clearBufferIfAny` |
| 6-3 | `TC_6_3_SearchUpFromFifteenToFour` | Major | `findNextSearchChannel` 목록 외 래핑 |
| A8 | `TC_A8_SingleSearchResultWrap` | Minor | 단일 검색 결과 동일 채널 `setCH` 1회 |
| E14 | `TC_ERR_InvalidCurrentChUpNoOp` | Major | `getCurrentChannelValue()` null → no-op |

---

## 6. 검증·갱신 절차

```powershell
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
ctest --test-dir build -R GoldenMaster
```

실패 시 단건 재현:

```powershell
build\Debug\TVControllerTest.exe --gtest_filter=<실패_테스트_이름>
```

**결함 등록 규칙**

1. `ID`: `DEF-###` (3자리 순번, 예: `DEF-001`)
2. `Steps`: Given/When/Then 또는 키 입력 시퀀스
3. `Expected` / `Actual`: Mock `EXPECT_CALL` 또는 assertion 기준 (로그 아님)
4. `Root Cause`: 파일·함수·라인 (가능 시)
5. `Fix Summary`: 적용 diff 한 줄 요약; 해결 후 Status → **Closed**

---

## 7. 변경 이력

| 일자 | 버전 | 내용 |
|------|------|------|
| 2026-05-19 | 1.0 | 초판 — QA 회귀 52/52 Green, Open 결함 0건, OBS-001 관찰 기록 |
| 2026-05-19 | 1.1 | Golden Master 26건 반영 — 합계 78/78; 운영 정책은 `docs/defect_report.md` 참조 |

---

## 8. 관련 문서

```
docs/defect_report.md (정책·템플릿)
    └─► docs/defect_list.md  ← 본 문서 (레지스트리)
Report/04 → Report/05 → Report/06 → Report/08 (결함 관리 체계)
```

*본 목록은 테스트 실패·결함의 단일 레지스트리이다. Open 결함이 생기면 본 문서와 `Report/05`를 동시에 갱신한다. 분류·보고 형식은 `defect_report.md`를 따른다.*
