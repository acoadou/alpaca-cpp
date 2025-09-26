BUILD_DIR := build
CMAKE_FLAGS := -DALPACA_BUILD_TESTS=ON

.PHONY: all clean build test coverage configure package

all: build

configure:
	cmake -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)

build: configure
	cmake --build $(BUILD_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

package: configure
	cmake --build $(BUILD_DIR) --target package

coverage:
	cmake -S . -B $(BUILD_DIR) $(CMAKE_FLAGS) -DALPACA_ENABLE_COVERAGE=ON
	cmake --build $(BUILD_DIR)
	ctest --test-dir $(BUILD_DIR) --output-on-failure
	@if command -v gcovr >/dev/null 2>&1; then \
		gcovr -r . --exclude-directories $(BUILD_DIR)/_deps --print-summary; \
	else \
		echo "gcovr not installed; skipping coverage report."; \
	fi

clean:
	rm -rf $(BUILD_DIR)
