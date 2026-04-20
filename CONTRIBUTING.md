# Contributing to Algo-Catalyst

Thank you for your interest in contributing! This document outlines the development workflow.

## Development Setup

```bash
# Clone
git clone https://github.com/your-handle/Algo-Catalyst.git
cd Algo-Catalyst

# Build (Debug with warnings)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Python environment
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Code Style

C++ source is formatted with **clang-format** (see `.clang-format`):

```bash
# Format all C++ files in-place
find src include -name '*.cpp' -o -name '*.h' | xargs clang-format -i -style=file
```

Python scripts are linted with **ruff**:

```bash
pip install ruff
ruff check scripts/
ruff format scripts/
```

## Commit Message Format

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <short description>

Types: feat, fix, refactor, perf, test, docs, ci, chore, style
```

Examples:
- `feat(indicators): add Stochastic Oscillator`
- `fix(engine): prevent double-fill on duplicate timestamps`
- `docs: update README with new CLI flags`

## Pull Request Guidelines

1. Branch from `main` using `feature/<name>` or `fix/<name>`
2. Keep PRs focused — one feature or fix per PR
3. Ensure CI passes (build + lint + smoke test)
4. Update `README.md` if adding user-facing features
5. Add a brief description of the change in the PR body

## Adding a New Indicator

1. Declare the method in `include/Indicators.h`
2. Implement in `src/Indicators.cpp` — include warm-up tracking
3. Add `reset()` cleanup
4. (Optional) Use the indicator in a Strategy

## Adding a New Strategy

1. Subclass `Strategy` in `include/Strategy.h`
2. Implement `processMarketUpdate()` in `src/Strategy.cpp`
3. Wire it up in `src/main.cpp` or expose via a CLI flag

## Reporting Bugs

Please include:
- OS and compiler version
- CMake configuration used
- Minimal reproducer (data file + command)
- Expected vs. actual output
