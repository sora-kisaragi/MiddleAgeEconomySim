# 貢献ガイド

## 参照する文書

- [要件定義書](wiki/Requirement.md)・[基本設計書（HLD）](wiki/High_Level_Design.md)・[詳細設計書（LLD）](wiki/Low_Level_Design.md) で設計意図と予定を確認します。参照する `wiki/` は対象コミットに含まれる文書です。GitHub Wiki 本体への反映状況は [#27](https://github.com/sora-kisaragi/MiddleAgeEconomySim/issues/27) と同期結果で確認してください。
- [コーディング規約](wiki/Coding%E2%80%90Standards.md)・[テスト規約](wiki/Testing%E2%80%90Guidelines.md)・[ブランチ戦略](wiki/Branch%E2%80%90Strategy.md) を参照してください。現在のビルド、テスト、PR の実務手順は本ガイドに従います。Wiki 規約の旧記述と対応案は [棚卸し](docs/development-audit-2026-11-11.md#wiki-規約と設計書の更新案) に記録しています。
- 実装済みの挙動は、対象 SHA のコード・テストと取得できた実行結果で確認してください。設計と実装に差があれば Issue／PR に根拠と判断を残します。[Checklist](Checklist.md) と過去の検証レポートは、当時の計画や確認結果を参照するために使います。
- [AGENTS.md](AGENTS.md) はエージェント向けの共通入口です。[開発用 Skill](.agents/skills/middle-age-economy-development/SKILL.md) は小規模開発の手順を案内します。日次・週末実行の通知や報告期間は各自動化の実行指示に従います。

## 作業の流れ

1. 最新 main の SHA、[未解決 Issue](https://github.com/sora-kisaragi/MiddleAgeEconomySim/issues?q=is%3Aissue+is%3Aopen)、[進行中の PR](https://github.com/sora-kisaragi/MiddleAgeEconomySim/pulls)（ドラフトを含む）、過去の関連 PR と検証ログを確認します。
2. 依頼された Issue を進めます。作業を選ぶ場合は、既存 Issue から独立して検証できるものを優先します。候補がなければ既存の提案をコード・テストと照合し、必要に応じて達成条件を絞った Issue を作ります。変更範囲や未マージの依存が重なる場合は先行作業と調整します。
3. 最新 main を起点に、用途に応じて `feature/`、`bugfix/`、`refactor/`、`chore/` 等のブランチを作成します。例は文書整備の場合です。

   ```bash
   git fetch origin main
   git switch -c chore/update-development-guide origin/main
   git rev-parse HEAD
   ```

4. 実装変更では再現テストを用意し、失敗、最小限の修正、既存テストの再実行の順に確認します。テストの実例は [エージェント間送金のテスト](tests/agent_tests/agent_transfer_test.cpp) と [日次統合テスト](tests/system_tests/daily_cash_cycle_test.cpp) を参照してください。
5. レビュー用 PR を `main` 宛てに作成し、関連 Issue と検証結果を記録します。日次開発の終了点はドラフト PR または検証結果の記録です。マージ等の一時的な許可は、指定された作業・期間にのみ適用します。

## ビルドと検証

[CI](.github/workflows/ci.yml) の標準環境は Ubuntu です。現行の [CMakeLists.txt](CMakeLists.txt) は `gcc-14`／`g++-14` の名前を指定し、C++17 を使います。[検証ログ](docs/reports/2026-11-11/verification.log) で確認済みの組合せは GCC 14.2.0、CMake 3.31.6、GoogleTest 1.14.0 です。手順はこの組合せを基準にします。別 OS、Clang、MSVC、CMake 宣言上の最低版での動作は個別の実行結果で確認してください。

チェックアウト済みのリポジトリのルートで実行します。異なるコミットや構成を比較する場合は、作業ツリーとビルドディレクトリを分けてください。

```bash
git rev-parse HEAD
g++-14 --version
cmake --version
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

GoogleTest 1.14.0 は [tests/CMakeLists.txt](tests/CMakeLists.txt) の `FetchContent` で取得します。取得に失敗した場合は、別途入手した同じ v1.14.0 のソースルートを絶対パスで渡します。CMake の取得用キャッシュとは独立したビルドディレクトリを使います。

```bash
cmake -S . -B build-local-gtest -DCMAKE_BUILD_TYPE=Debug \
  -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/absolute/path/to/googletest-v1.14.0
cmake --build build-local-gtest --parallel 2
ctest --test-dir build-local-gtest --output-on-failure
```

GoogleTest を用意できず、`g++-14` が使える場合は、既存の CLI ソースを直接ビルドして動作を確認できます。

```bash
mkdir -p build-cli-only
g++-14 -std=c++17 -Iinclude src/main.cpp src/agent.cpp src/simulation.cpp \
  src/simulation_options.cpp -o build-cli-only/MiddleAgeEconomySim
```

以下は標準ビルドの CLI 確認例です。ローカル GoogleTest を使った場合は実行ファイルのパスを `build-local-gtest/`、直接ビルドでは `build-cli-only/` に読み替えます。

```bash
./build/MiddleAgeEconomySim --help
./build/MiddleAgeEconomySim --scenario balanced --days 3
./build/MiddleAgeEconomySim --scenario shortage --days 3 --csv build/shortage.csv
./build/MiddleAgeEconomySim --scenario oversupply --days 3
./build/MiddleAgeEconomySim --days 0
```

3日間の指数の期待値は順に `100, 100, 100`、`110, 120, 130`、`90, 80, 70` です。正常系は終了コード0、CSVはヘッダと完了した3日分、`--days 0` は非0を確認します。CLI のみ確認した場合は CTest 未実行と記録します。フェーズ1の CMake 設定ではコードカバレッジ収集が無効です。テストの合否・件数、レビュー結果、カバレッジ計測は別々に記録します。

## コードレビューの観点

- **資金と在庫** — 給与・購入・徴税・補助金・融資では支払側と受領側を確認し、日次の総資金を比較します。在庫は生産・出品・購入・消費を追い、売り手付き取引で資金と商品が対応することを確認します。失敗時の双方の残高・在庫・履歴を、対象 API の契約に沿って検証します。
- **入力と数値境界** — 不正商品名、0以下の注文数量、負の価格や在庫、満足度の範囲、資金不足、加算・乗算・型変換の上限を確認します。失敗が明示的に検出できることと、事前検証の失敗で状態が変わらないことを確認します。
- **日次価格と観測** — 同じ日の約定価格を固定し、商品別に供給・有効な需要を集計します。在庫不足の有効な購入希望、持越在庫、重複出品、整数丸め、翌日価格を確認します。固定バスケットの基準と前日比、CSVに記録された完了日を照合します。
- **失敗の伝播** — 通常の不成立と日次処理中断を区別し、日次入口の戻り値、CLIの終了コード・標準エラー、中断日の観測行の有無を確認します。状態変更後に起きた例外の扱いは既存 API の契約を確認します。

## PR と作業記録

- PR には、作業日（JST）、関連 Issue、達成条件、変更内容、対象 SHA、検証環境・コマンド・終了コード・テスト件数と結果、ログの場所、未検証項目、残作業を記録します。実行できなかった場合は理由と実行できた確認を記録してください。
- 日次のコード変更では新規サブエージェントに対象 SHA を渡し、独立レビューを行います。対象の一致、テストまたは CLI の再実行、指摘と対応を記録します。GUI を変更した場合はスクリーンショットを含めます。重大な不具合や検証失敗は解消までレビュー待ちとします。
- レビュー待ちとマージ済みは GitHub の状態で確認します。検証だけ完了した場合は対象 SHA と実行結果を残し、取得できなかった実行履歴は未確認として扱います。

## コミュニケーション

### 質問や議論
- 技術的な質問や議論は[Issues](https://github.com/sora-kisaragi/MiddleAgeEconomySim/issues)で行ってください
- より詳細な議論が必要な場合は[Discussions](https://github.com/sora-kisaragi/MiddleAgeEconomySim/discussions)を利用してください

### コードレビューのエチケット
- 建設的なフィードバックを心がける
- 具体的な改善案を提示する
- レビュー依頼から3営業日以内に返信する
