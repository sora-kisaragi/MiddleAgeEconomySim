# Google Testの設定
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  # キャッシュディレクトリを指定してローカルに保存
  SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/googletest-src
  BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/googletest-build
  # ダウンロード済みの場合は再取得しない
  FIND_PACKAGE_ARGS NAMES GTest
)

# GoogleTestが既にシステムにインストールされているか確認
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  # システムにインストールされていない場合のみダウンロード
  message(STATUS "GoogleTest not found in system, downloading...")
  FetchContent_MakeAvailable(googletest)
else()
  message(STATUS "Using system GoogleTest")
endif()

# テストファイルを収集
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

# テスト実行ファイルをビルド
add_executable(unit_tests ${TEST_SOURCES})

# 適切なGoogleTestライブラリをリンク
if(GTest_FOUND)
  target_link_libraries(unit_tests GTest::GTest GTest::Main)
else()
  target_link_libraries(unit_tests gtest gtest_main)
endif()

# テストを登録
include(GoogleTest)
gtest_discover_tests(unit_tests)

# テスト対象のソースを含める（ただしmain.cppは除外）
file(GLOB_RECURSE PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# プロジェクトソースをテストに含める
target_sources(unit_tests PRIVATE ${PROJECT_SOURCES})

# インクルードディレクトリを追加
target_include_directories(unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
