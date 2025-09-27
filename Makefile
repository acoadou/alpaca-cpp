BUILD_DIR := build
CMAKE_FLAGS := -DALPACA_BUILD_TESTS=ON
BUILD_JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
BUILD_PARALLEL := --parallel $(BUILD_JOBS)

.PHONY: all clean build test coverage configure package

all: build

configure:
	cmake -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)

build: configure
	cmake --build $(BUILD_DIR) $(BUILD_PARALLEL)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

package: configure
	cmake --build $(BUILD_DIR) $(BUILD_PARALLEL) --target package

indent:
	clang-format -i $(shell find . -regex '.*\.\(cpp\|hpp\)' -not -path "./$(BUILD_DIR)/*" -not -path "./.vscode/*" -not -path "./.git/*")

coverage:
	cmake -S . -B $(BUILD_DIR) $(CMAKE_FLAGS) -DALPACA_ENABLE_COVERAGE=ON
	cmake --build $(BUILD_DIR) $(BUILD_PARALLEL)
	ctest --test-dir $(BUILD_DIR) --output-on-failure
	@if command -v gcovr >/dev/null 2>&1; then \
		gcovr -r . --exclude-directories $(BUILD_DIR)/_deps --print-summary; \
	else \
		echo "gcovr not installed; skipping coverage report."; \
	fi

clean:
	rm -rf $(BUILD_DIR)
