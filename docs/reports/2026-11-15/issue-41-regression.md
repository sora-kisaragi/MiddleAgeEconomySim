# Issue #41 マージ後の回帰検証

- 作業日：2026-11-15（JST）
- 検証対象：`main@465fdd0aafed8418cd37c0e1b1dba486686f967a`
- 関連：[Issue #41](https://github.com/sora-kisaragi/MiddleAgeEconomySim/issues/41)、[PR #102](https://github.com/sora-kisaragi/MiddleAgeEconomySim/pull/102)、[PR #103](https://github.com/sora-kisaragi/MiddleAgeEconomySim/pull/103)

型付き ActorId と GameTime の日内採番を両方含むマージ後のコミットを検証しました。既存の CTest が全件通り、同一ビルド・同一引数で実行した日次 CLI の観測が一致することを今回の達成条件とします。

GitHub Wiki の HLD と LLD（Wiki HEAD `f4a4dcd7d6f011bb319ebd5ffda552c371fe281f`）が対象コミットの `wiki/` と同一であることを確認しました。固定バスケットによる指数・前日比と、完了日の CSV 出力を CLI 結果と照合しました。

## 実行結果

Linux、GCC 14.2.0、C++17、CMake 3.31.6、GoogleTest 1.14.0。

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/absolute/path/to/googletest-v1.14.0
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

構成・ビルド・CTest はすべて終了コード 0。CTest は 210/210 件成功しました。型と採番の境界、登録順を入れ替えたときの ID の一致、取引記録、ID が未割当の融資で残高と履歴が変わらないことを確認する既存テストを含みます。

`build/MiddleAgeEconomySim --scenario SCENARIO --days 3 --csv PATH` をシナリオごとに異なる出力先へ2回実行しました。全6回が終了コード 0 で、同じシナリオの CSV・標準出力・標準エラーはバイト単位で一致しました。CSV の列名、日番号1〜3、基準100からの各日の変化率も確認しました。

| シナリオ | 日次物価指数 | 日次総資金（コイン） |
| --- | --- | --- |
| balanced | 100 → 100 → 100 | 7000 → 7000 → 7000 |
| shortage | 110 → 120 → 130 | 7000 → 7000 → 7000 |
| oversupply | 90 → 80 → 70 | 7000 → 7000 → 7000 |

`build/MiddleAgeEconomySim --days 0 --csv PATH` は終了コード 1 で、新規 CSV は作られませんでした。

## 残作業

CLI の観測 CSV には ID や時刻の連番が含まれていません。採番器を実際の日次処理に接続し、同じ設定・seed・ビルドで全ゲーム内 ID と時刻順を比較する Issue #41 の受入検証は未実施です。今回の差分は検証記録のみです。
