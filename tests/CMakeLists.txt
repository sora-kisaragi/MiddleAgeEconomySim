# Google Testの設定
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# テストファイルを収集
file(GLOB_RECURSE TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/agent_tests/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/market_tests/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/system_tests/*.cpp"
)

# テスト実行ファイルをビルド
add_executable(unit_tests ${TEST_SOURCES})

# ソースファイルを除外（main.cppを除く）
file(GLOB_RECURSE PROJECT_SOURCES 
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# GoogleTestとのリンク
target_link_libraries(unit_tests PRIVATE
    GTest::gtest_main
    GTest::gmock_main
)

# インクルードディレクトリを追加
target_include_directories(unit_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# テストの自動検出
include(GoogleTest)
gtest_discover_tests(unit_tests)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(unit_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()