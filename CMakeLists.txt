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
