# 검증 기록

| 기록 | 의미 |
|---|---|
| `2026-10-14-rtx3070-small-current-bag.json/.csv` | 현재 보관 중인 10월 2일 bag의 PyTorch FP32 vs TensorRT FP16 성능 |
| `2026-10-14-archive-smoke.json` | archive 해제·새 빌드·엔진 재생성 후 ROS bbox/fusion 실제 출력 확인 |
| `2026-10-13-rtx3070-small-objects.json/.csv` | 과거 9월 27일 bag의 객체 검출 구간 성능 |
| `2026-10-13-rtx3070-small.json/.csv` | 과거 9월 20일 bag의 미검출 구간 성능 |

확정 결과는 archive에 포함합니다. 로컬 재실행 출력은 `benchmarks/runs/`에 저장해 Git과 archive에서 제외합니다. 자세한 측정 범위·해석은 [성능 문서](../docs/performance.md)에 있습니다.
