cmake_minimum_required(VERSION 3.14)
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

# 実行ファイルをビルド
add_executable(${PROJECT_NAME} ${SOURCES})

# テスト設定
enable_testing()
add_subdirectory(tests)

# パッケージ設定
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# コードカバレッジの設定（ローカルのみ）
option(CODE_COVERAGE "Enable coverage reporting" OFF)
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # カバレッジフラグの追加
  add_compile_options(--coverage -O0)
  add_link_options(--coverage)
  
  # カバレッジレポート生成用のターゲット
  find_program(LCOV lcov REQUIRED)
  find_program(GENHTML genhtml REQUIRED)
  
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