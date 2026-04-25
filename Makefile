.PHONY: all build release debug clean run test lint format docker help

BUILD_DIR   := build
BINARY      := $(BUILD_DIR)/AlgoCatalyst
DATA_FILE   := data/tick_data.csv
SCENARIO    := catalyst
TICKS       := 10000

all: release

## Build targets

release:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(BUILD_DIR) -j$$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)

debug:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(BUILD_DIR) -j$$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)

clean:
	rm -rf $(BUILD_DIR)

## Data generation

data:
	python3 scripts/generate_data.py --scenario $(SCENARIO) --ticks $(TICKS)

data-all:
	python3 scripts/generate_data.py --scenario catalyst --ticks $(TICKS)
	python3 scripts/generate_data.py --scenario meanrev  --ticks $(TICKS)
	python3 scripts/generate_data.py --scenario trending --ticks $(TICKS)
	python3 scripts/generate_data.py --scenario volatile --ticks $(TICKS)

## Run and visualize

run: release data
	$(BINARY) --data $(DATA_FILE) --symbol AAPL --output trades.csv

plot:
	python3 scripts/plot_performance.py --trades trades.csv

viz: run plot

## Optimization and analysis

optimize:
	python3 scripts/optimize_params.py --scenario $(SCENARIO) --ticks $(TICKS)

walk-forward:
	python3 scripts/walk_forward.py --scenario $(SCENARIO) --ticks $(TICKS)

compare:
	python3 scripts/compare_strategies.py --ticks $(TICKS)

## Code quality

format:
	clang-format -i src/*.cpp include/*.h

lint:
	@command -v cppcheck >/dev/null 2>&1 && \
		cppcheck --enable=all --suppress=missingInclude src/ include/ || \
		echo "cppcheck not found — skipping"

## Docker

docker-build:
	docker build -t algo-catalyst .

docker-run:
	docker run --rm -v $$(pwd)/data:/app/data -v $$(pwd)/trades.csv:/app/trades.csv \
		algo-catalyst --data data/tick_data.csv --symbol AAPL

## Python deps

venv:
	python3 -m venv venv
	venv/bin/pip install -r requirements.txt

help:
	@grep -E '^## ' Makefile | sed 's/## /\n  /'
	@echo ""
	@echo "  Usage: make [target]"
	@echo ""
	@echo "  Targets:"
	@grep -E '^[a-zA-Z_-]+:' Makefile | grep -v '^\.PHONY' | awk -F: '{printf "    %-20s\n", $$1}'
