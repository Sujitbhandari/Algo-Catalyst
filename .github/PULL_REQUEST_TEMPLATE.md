## What does this PR do?

A concise description of the change and the problem it solves.

## Type of change

- [ ] Bug fix
- [ ] New feature (indicator, strategy, script, engine improvement)
- [ ] Documentation update
- [ ] Refactor / code quality improvement
- [ ] CI / tooling change

## Checklist

- [ ] Code compiles without warnings (`make release`)
- [ ] Unit tests pass (`ctest --test-dir build`)
- [ ] New indicators have a test in `tests/test_indicators.cpp`
- [ ] New strategies are documented in `docs/strategies.md`
- [ ] New metrics are listed in `docs/performance_metrics.md`
- [ ] `CHANGELOG.md` updated under the `[Unreleased]` section
- [ ] `README.md` updated if user-facing behaviour changed

## How to test

Steps to reproduce or verify the change manually:

```bash
# e.g.
make release
make data
./build/AlgoCatalyst --config configs/aggressive.json
```
