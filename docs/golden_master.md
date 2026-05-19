# Golden Master 회귀 테스트

## 역할 분리

- **GMock 단위 테스트** (`TVControllerTest`): `setCH` / `getCurrentCH` / `seekCH` 호출 계약·횟수·인자를 시나리오별로 검증한다.
- **Golden Master** (`GoldenMasterTest`): README/requirements 시나리오 전체를 **행위 트랜스크립트** 스냅샷으로 잠근다.
- 리팩토링 시 GMock이 세부 계약을 지키고, Golden이 **시나리오 단위 회귀**를 잡는다.
- `std::cout` 로그는 Golden 대상이 아니다 (단위 테스트와 동일).
- `Tuner.h` / `TunerTest.cpp`는 수정하지 않는다.

## 계층 (Primary: Layer A)

| 계층 | 내용 | 비고 |
|------|------|------|
| **A (Primary)** | `RecordingTuner` 트랜스크립트 (`setCH`, `getCurrentCH`, `seekCH`) | 로케일·공백 영향 적음 |
| **B (미사용)** | stdout 캡처 | UI 로그 회귀용; 본 프로젝트는 Layer A만 사용 |

## 실행

```bash
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure
ctest --test-dir build -R GoldenMaster
ctest --test-dir build -L golden
```

## Golden 갱신 (의도적 변경만)

```bash
# Windows (PowerShell)
$env:UPDATE_GOLDEN="1"
.\build\Debug\GoldenMasterTest.exe --gtest_filter=GoldenMaster.*

# Linux / macOS
UPDATE_GOLDEN=1 ./build/GoldenMasterTest --gtest_filter=GoldenMaster.*
```

- `test/golden/<scenario_id>.golden` 파일이 actual 내용으로 덮어씌워진다.
- PR에는 golden diff를 리뷰하고, CI에서는 `UPDATE_GOLDEN` 없이 비교한다.

## 실패 시

1. `ctest` 또는 `GoldenMasterTest` 출력의 unified diff를 확인한다.
2. 버그면 구현 수정, 의도된 동작 변경이면 `UPDATE_GOLDEN=1`로 golden만 갱신한다.
3. Windows에서 줄바꿈 문제가 나면 `.gitattributes`의 `eol=lf`를 확인한다.
