cmake_minimum_required(VERSION 3.14)

# コンパイラの明示的な設定（GCCを強制）
set(CMAKE_C_COMPILER "gcc-14")
set(CMAKE_CXX_COMPILER "g++-14")

project(MiddleAgeEconomySim VERSION 0.1.0 LANGUAGES CXX)

# C++17を指定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 警告レベルを設定
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# インクルードディレクトリ
include_directories(include)

# ソースファイルを収集
file(GLOB_RECURSE SOURCES "src/*.cpp")

# メインの実行ファイルのビルドを条件付きに
if(NOT DEFINED BUILD_MAIN OR BUILD_MAIN)
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

# テスト設定
enable_testing()
add_subdirectory(tests)

# パッケージ設定
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# コードカバレッジの設定（ローカルのみ）
# Phase 1ではカバレッジ収集を行わない
set(PHASE_1 TRUE)
if(PHASE_1)
  message(STATUS "Phase 1: Code coverage is disabled")
  option(CODE_COVERAGE "Enable coverage reporting" OFF)
else()
  option(CODE_COVERAGE "Enable coverage reporting" OFF)
endif()

if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND NOT PHASE_1)
  # カバレッジレポート生成用のターゲット
  find_program(LCOV lcov REQUIRED)
  find_program(GENHTML genhtml REQUIRED)
  
  # 実行ファイルとテストにカバレッジフラグを追加
  target_compile_options(${PROJECT_NAME} PRIVATE --coverage -O0)
  target_link_options(${PROJECT_NAME} PRIVATE --coverage)
  
  add_custom_target(coverage
    # カバレッジデータの収集
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${LCOV} --directory . --capture --output-file coverage.info
    # 不要なファイルの除外
    COMMAND ${LCOV} --remove coverage.info '/usr/*' '*/tests/*' '*/googletest/*' --output-file coverage.info
    # HTMLレポートの生成
    COMMAND ${GENHTML} --output-directory coverage coverage.info
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated: file://${CMAKE_BINARY_DIR}/coverage/index.html"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating code coverage report"
  )
endif()