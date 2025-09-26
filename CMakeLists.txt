cmake_minimum_required(VERSION 3.20)
project(alpaca-cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/FIcstdint>)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-include>
                      $<$<COMPILE_LANGUAGE:CXX>:cstdint>)
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

file(GLOB_RECURSE ALPACA_CPP_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

set(ALPACA_CPP_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
file(MAKE_DIRECTORY "${ALPACA_CPP_GENERATED_INCLUDE_DIR}/alpaca")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.in
  ${ALPACA_CPP_GENERATED_INCLUDE_DIR}/alpaca/version.hpp
  @ONLY
)

add_library(alpaca-cpp ${ALPACA_CPP_SOURCES})
add_library(alpaca::alpaca-cpp ALIAS alpaca-cpp)

target_include_directories(alpaca-cpp
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${ALPACA_CPP_GENERATED_INCLUDE_DIR}>
      $<INSTALL_INTERFACE:include>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

include(FetchContent)

find_package(nlohmann_json 3.11 QUIET)
if (NOT nlohmann_json_FOUND)
  message(STATUS "nlohmann_json not found - fetching v3.11.2 via FetchContent")
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
  )
  FetchContent_GetProperties(nlohmann_json)
  if (NOT nlohmann_json_POPULATED)
    FetchContent_Populate(nlohmann_json)
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${nlohmann_json_SOURCE_DIR}/include"
    )
  endif()
endif()

option(ALPACA_FETCH_IXWEBSOCKET "Allow FetchContent to download ixwebsocket when not provided by the system" ON)

set(ALPACA_VENDORED_IXWEBSOCKET OFF)
find_package(ixwebsocket CONFIG QUIET)
if (ixwebsocket_FOUND)
  message(STATUS "Found ixwebsocket via package configuration")
else()
  if (ALPACA_FETCH_IXWEBSOCKET)
    message(STATUS "ixwebsocket not found - fetching v11.3.3 via FetchContent")
    set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "Disable ixwebsocket's own install rules when vendored" FORCE)
    FetchContent_Declare(
      ixwebsocket
      GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
      GIT_TAG v11.3.3
    )
    FetchContent_MakeAvailable(ixwebsocket)
    set(ALPACA_VENDORED_IXWEBSOCKET ON)
  else()
    message(FATAL_ERROR "ixwebsocket dependency not found and ALPACA_FETCH_IXWEBSOCKET is OFF")
  endif()
endif()

set(ALPACA_CPP_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/alpaca-cpp" CACHE STRING "Install path for alpaca-cpp CMake package files")

if (ALPACA_VENDORED_IXWEBSOCKET)
  get_target_property(_alpaca_ixwebsocket_real_target ixwebsocket::ixwebsocket ALIASED_TARGET)
  if (_alpaca_ixwebsocket_real_target)
    set(ALPACA_IXWEBSOCKET_TARGET ${_alpaca_ixwebsocket_real_target})
  else()
    set(ALPACA_IXWEBSOCKET_TARGET ixwebsocket::ixwebsocket)
  endif()

  if (TARGET ${ALPACA_IXWEBSOCKET_TARGET})
    get_target_property(_alpaca_ix_headers ${ALPACA_IXWEBSOCKET_TARGET} PUBLIC_HEADER)
    if (_alpaca_ix_headers)
      set(_alpaca_ix_abs_headers "")
      foreach(_alpaca_ix_header IN LISTS _alpaca_ix_headers)
        list(APPEND _alpaca_ix_abs_headers "${ixwebsocket_SOURCE_DIR}/${_alpaca_ix_header}")
      endforeach()
      set_target_properties(${ALPACA_IXWEBSOCKET_TARGET} PROPERTIES PUBLIC_HEADER "${_alpaca_ix_abs_headers}")
    endif()
    target_include_directories(${ALPACA_IXWEBSOCKET_TARGET} INTERFACE
      $<BUILD_INTERFACE:${ixwebsocket_SOURCE_DIR}/ixwebsocket>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    install(TARGETS ${ALPACA_IXWEBSOCKET_TARGET}
      EXPORT alpaca-ixwebsocketTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ixwebsocket)

    install(EXPORT alpaca-ixwebsocketTargets
      NAMESPACE ixwebsocket::
      DESTINATION ${ALPACA_CPP_INSTALL_CMAKEDIR})
  endif()
endif()


target_link_libraries(alpaca-cpp
  PUBLIC
    CURL::libcurl
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    ixwebsocket::ixwebsocket)

target_compile_features(alpaca-cpp PUBLIC cxx_std_20)

install(TARGETS alpaca-cpp
  EXPORT alpaca-cppTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${ALPACA_CPP_GENERATED_INCLUDE_DIR}/alpaca/version.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/alpaca)

install(EXPORT alpaca-cppTargets
        NAMESPACE alpaca::
        DESTINATION ${ALPACA_CPP_INSTALL_CMAKEDIR})

if (ALPACA_VENDORED_IXWEBSOCKET)
  set(ALPACA_VENDORED_IXWEBSOCKET_VALUE TRUE)
else()
  set(ALPACA_VENDORED_IXWEBSOCKET_VALUE FALSE)
endif()

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/alpaca-cppConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/alpaca-cppConfig.cmake
  INSTALL_DESTINATION ${ALPACA_CPP_INSTALL_CMAKEDIR}
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/alpaca-cppConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/alpaca-cppConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/alpaca-cppConfigVersion.cmake
  DESTINATION ${ALPACA_CPP_INSTALL_CMAKEDIR})

option(ALPACA_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if (ALPACA_ENABLE_COVERAGE)
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(ALPACA_COVERAGE_COMPILE_OPTIONS -O0 -g --coverage)
    set(ALPACA_COVERAGE_LINK_OPTIONS --coverage)
  else()
    message(FATAL_ERROR "Code coverage is only supported with GCC or Clang compilers")
  endif()
endif()

if (ALPACA_ENABLE_COVERAGE)
  target_compile_options(alpaca-cpp PRIVATE ${ALPACA_COVERAGE_COMPILE_OPTIONS})
  target_link_options(alpaca-cpp PRIVATE ${ALPACA_COVERAGE_LINK_OPTIONS})
endif()

option(ALPACA_BUILD_TESTS "Build alpaca-cpp tests" ON)
if (ALPACA_BUILD_TESTS)
  enable_testing()

  # Prevent Googletest from installing its headers and archives alongside the SDK.
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  file(GLOB_RECURSE ALPACA_CPP_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)

  add_executable(alpaca-cpp-tests ${ALPACA_CPP_TEST_SOURCES})

  if (ALPACA_ENABLE_COVERAGE)
    target_compile_options(alpaca-cpp-tests PRIVATE ${ALPACA_COVERAGE_COMPILE_OPTIONS})
    target_link_options(alpaca-cpp-tests PRIVATE ${ALPACA_COVERAGE_LINK_OPTIONS})
  endif()
  target_link_libraries(alpaca-cpp-tests
    PRIVATE
      alpaca-cpp
      GTest::gtest_main
      GTest::gmock)

  include(GoogleTest)
  gtest_discover_tests(alpaca-cpp-tests)

  set(FETCHCONTENT_ALIAS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/fetchcontent-alias-smoke)
  add_test(
    NAME fetchcontent-alias-configure
    COMMAND ${CMAKE_COMMAND}
            -S ${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/fetchcontent_alias
            -B ${FETCHCONTENT_ALIAS_BINARY_DIR}
            -D ALPACA_CPP_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR})
  add_test(
    NAME fetchcontent-alias-build
    COMMAND ${CMAKE_COMMAND} --build ${FETCHCONTENT_ALIAS_BINARY_DIR})
  set_tests_properties(fetchcontent-alias-build PROPERTIES DEPENDS fetchcontent-alias-configure)
endif()

set(CPACK_PACKAGE_NAME "alpaca-cpp")
set(CPACK_PACKAGE_VENDOR "alpaca-cpp")
set(CPACK_PACKAGE_CONTACT "maintainers@alpaca-cpp")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern C++ client for the Alpaca Trade API")
set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "alpaca-cpp Maintainers")
include(CPack)
