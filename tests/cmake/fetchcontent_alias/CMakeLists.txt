cmake_minimum_required(VERSION 3.20)
project(fetchcontent_alias_smoke LANGUAGES CXX)

include(FetchContent)

set(ALPACA_CPP_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" CACHE PATH "Path to alpaca-cpp source tree")

FetchContent_Declare(
  alpaca-cpp
  SOURCE_DIR "${ALPACA_CPP_SOURCE_DIR}"
)
FetchContent_MakeAvailable(alpaca-cpp)

add_executable(my_trader main.cxx)
target_link_libraries(my_trader PRIVATE alpaca::alpaca-cpp)

set_target_properties(my_trader PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
