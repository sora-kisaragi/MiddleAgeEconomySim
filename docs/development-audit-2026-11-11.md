# 開発指示・規約と実装の棚卸し

確認日 2026-11-11（JST）。対象は [main `0e8440e06ad0094c6e788011cf138627ca6b4232`](https://github.com/sora-kisaragi/MiddleAgeEconomySim/tree/0e8440e06ad0094c6e788011cf138627ca6b4232) です。確認時の未解決 Issue は [#27](https://github.com/sora-kisaragi/MiddleAgeEconomySim/issues/27) と [#38](https://github.com/sora-kisaragi/MiddleAgeEconomySim/issues/38) で、公開済みの進行中 PR や #27 と特定できる作業ブランチは見つかりませんでした。

## 指示の所在と扱い

| 対象 | 確認した所在・版 | 判定 | 理由と対応 |
| --- | --- | --- | --- |
| プロジェクト用 Skill とエージェント指示 | 対象 SHA の全ツリー（省略なし）。現セッションの利用可能な Skill 一覧と開発端末のユーザー用 Skill ディレクトリ | 新設・外部は要判断 | リポジトリに `SKILL.md`、`AGENTS.md`、Skill 用ディレクトリはありませんでした。参照できた外部範囲でも本プロジェクト専用の登録は確認できませんでした。他の端末・共有範囲は未確認です。[AGENTS.md](../AGENTS.md) と [リポジトリ用 Skill](../.agents/skills/middle-age-economy-development/SKILL.md) を新設する案とします。 |
| [CONTRIBUTING](../CONTRIBUTING.md) と [README](../README.md) | 対象 SHA | 修正 | GCC／Clang／MSVC の並記、存在しないテスト例、CMake の変数指定で有効にできないカバレッジ手順を整理します。検証済みツール、依存取得の代替、実在テスト、レビュー観点は貢献ガイドへ集約します。 |
| Wiki 規約・設計書 | 対象 SHA の `wiki/`。公開 Wiki の同期状況は #27 を参照 | 維持・修正・要判断 | 設計意図、TDD、Feature Branch Workflow を維持します。現行実装との不一致と Wiki 側の編集案は下記に記録します。 |
| 日次開発 | 自動化「経済シミュレーターの日次開発」の現行設定。更新 2026-11-11 09:37 JST | 維持・一部修正案 | 最新 main／Issue／PR／過去結果の確認、1回1件、ドラフト PR、コード変更の独立レビュー、必要時のみ通知は維持します。設計参照順を「対象コミットの `wiki/` を参照し、公開 Wiki の同期状態は別に確認」に揃える案です。過去の時間限定マージ許可は継承しません。 |
| 週末報告 | 自動化「経済シミュレーターの週末作業報告」の現行設定。更新 2026-11-09 10:26 JST | 維持 | JSTの前週土曜から直前金曜までを対象に、PR・Issue・コミット・取得できた日次履歴を照合します。状態を分け、履歴なしの日は未確認とします。11月9日の GCC 13 による直接ビルドは同日の検証記録として扱います。 |
| [Checklist](../Checklist.md)、[旧 Issue 作成案](GITHUB_ISSUES_READY.md)、[当時の課題表](PROJECT_ISSUES.md)、[旧 Phase 1 案](github_issue_templates/issue_05_task_Phase_1_implementation_tasks.md) | 対象 SHA。現行コードと [PR #5](https://github.com/sora-kisaragi/MiddleAgeEconomySim/pull/5) 以降を照合 | 現行手順としては廃止 | 歴史資料として残します。相互作用・市場取引・価格・日次処理・統合テスト・CLI シナリオには現行実装があります。チェック欄から開発完了やカバレッジを判定しません。 |
| [旧課題検出手順](ISSUE_DETECTION_SYSTEM.md) と [追跡スクリプト](../scripts/continuous_issue_tracker.py) | 対象 SHA のファイルツリー・Actions 定義 | 現行手順としては廃止 | `/tmp/analyze_issues.py` はリポジトリに含まれず、例示の定期 Actions もありません。スクリプトの検出結果には対象 SHA のコード・テストによる確認が必要です。 |

本 PR の運用案では、依頼・実行指示で作業範囲と権限を確認し、`AGENTS.md` から貢献ガイドと Skill を読みます。日次の通知や週末の集計期間は自動化の実行指示、設計意図は対象 SHA の HLD/LLD、実装状況はコード・テスト・実行結果を根拠にします。Codex のプロジェクト用ディレクトリで Skill を検出できない環境では、同じ `SKILL.md` を直接参照できます。今回確認した自動化設定は読み取り時点の情報です。

## Wiki 規約と設計書の更新案

[同期ワークフロー](../.github/workflows/sync-main-to-wiki.yml) は main の `wiki/**` 変更で起動し、Wiki 側からの同期も [別ワークフロー](../.github/workflows/sync-wiki-to-main.yml) で動きます。#27 の同期比較に使う文書と復旧用の設定を保全するため、この PR の差分を `wiki/` と両ワークフローの外に置きます。以下は #27 側で同期方向と比較基準を確認した後に、Wiki 文書の編集へ移せる案です。

| 対象 | 判定 | 現状と具体的な更新案 |
| --- | --- | --- |
| [Coding Standards](../wiki/Coding%E2%80%90Standards.md) §1・§5・§9〜11 | 修正 | 命名の基本方針は維持し、既存メンバーと公開 API は周辺実装に合わせます。通常の取引不成立は既存の `bool`、日次中断は `simulateDay()` の戻り値と CLI 終了コードの契約を例示します。ClangFormat・Clang-Tidy・Cppcheck は現行 CI の必須工程から外し、任意のローカル検証として説明します。構造体とクラスは既に混在しており、整理は責務と API の互換性を根拠に判断します。 |
| [Testing Guidelines](../wiki/Testing%E2%80%90Guidelines.md) §4・§6〜12 | 修正 | 現行のスイート名と条件・期待結果が分かるテスト名を例示します。存在しない `Agent("Farmer", 100)`、`getAssets()`、`trade()` の例を [送金テスト](../tests/agent_tests/agent_transfer_test.cpp) への参照に置換します。例示パスは `tests/agent_tests/`、`tests/market_tests/`、`tests/system_tests/` とします。CI の契機は main への push と main 宛て PR です。フェーズ1のカバレッジ収集は無効と記述し、将来の目標値は計測の導入時に判断します。 |
| [Branch Strategy](../wiki/Branch%E2%80%90Strategy.md) | 維持・修正 | main 起点の Feature Branch Workflow と既存のブランチ接頭辞を維持します。日次作業の終了点はレビュー用ドラフトと記述し、マージは権限とレビュー・CI の確認を経る手順にします。Git Flow は将来の案として扱います。 |
| [Wiki Home](../wiki/Home.md) | 修正 | `Coding-Standards` 等の ASCII ハイフン参照を、実ファイル名に合う U+2010 のリンクに直します。ビルド手順は貢献ガイドを参照し、健康・犯罪傾向を実装済みの一覧から将来の設計項目に移します。 |
| [HLD](../wiki/High_Level_Design.md) §3〜5・§6.4、[LLD](../wiki/Low_Level_Design.md) §5〜13 | 修正・要判断 | 概念図と現行 API を明示し、日次順序、現行の課税対象、物流・イベント、意思決定と観測の実装段階を表記します。HLD §5.3 の価格式の基準は市場の当日開始価格です。混在する `struct`／`class` の一括移行は将来の設計判断として扱います。 |

## 実装状況と残作業

実装状況は対象 SHA の実行入口・定義・テストで判断しました。[検証レポート](reports/2026-11-11/README.md) と [ログ](reports/2026-11-11/verification.log) の179件は `24c991f` の実行結果です。棚卸し対象 `0e8440e` でも [main CI の成功](https://github.com/sora-kisaragi/MiddleAgeEconomySim/actions/runs/42936881645) を確認しました。

| 領域 | 現行の実装と検証根拠 | 残作業 |
| --- | --- | --- |
| NPC 行動・生産（HLD §3、LLD §7・11） | 日次入口は企業を配列順に循環割当てし、固定量を生産します。給与は企業から個人へ送金し、小麦の購入・消費と満足度を処理します。[日次実装](../src/simulation.cpp)、[資金保存](../tests/system_tests/daily_cash_cycle_test.cpp)、[食事](../tests/system_tests/food_consumption_test.cpp) | 雇用割当てと固定生産は暫定です。職業・雇用先の選択、労働と生産の連動、嗜好・個人差、健康や犯罪傾向などは未実装です。 |
| 売買・価格（HLD §5.3、LLD §6.1・9） | 日次の売り手付き取引、商品照合、日中固定価格、需給を集計した翌日価格が動きます。直接取引 API は送金を実施します。[市場](../include/market/market.h)、[日次価格テスト](../tests/market_tests/daily_market_test.cpp)、[直接取引](../src/agent.cpp) | 直接取引での物品移動、LLD の価格交渉、複数市場の選択は未実装です。匿名購入と売り手付き取引の混用には LLD §6.1 の所有権制約があります。 |
| 金融（HLD §5、LLD §5.1・7） | 日次の残高条件で融資を検討し、`provideLoan()` は貸し手・借り手の残高と貸付記録を更新します。[貸し手](../include/agent/loan_provider.h)、[公的資金・融資テスト](../tests/agent_tests/public_finance_test.cpp) | 借り手検索はスタブです。元利返済・延滞の状態遷移は日次入口に未接続です。100日観測3条件で融資成立は0件で、融資の確認根拠は別のテストです。 |
| 税制・政策（HLD §5、LLD §10） | 日次の個人残高課税と企業への条件付き補助金は実装済みです。企業向け残高課税と企業価格を調整する API があります。[政府](../include/agent/government.h)、[日次実装](../src/simulation.cpp) | 企業課税と価格統制は日次未接続です。累進所得税・利益課税・取引税、市場への価格規制反映、輸入規制・利率操作は未実装です。 |
| 物流・地域・イベント（HLD §5.2・6、LLD §8・9） | `TradeRoute` 型とデモの経路表示があります。[経路型](../include/system/trade_route.h)、[日次実装](../src/simulation.cpp) | 拠点間の在庫移動、到着日の反映、複数市場と人流、飢饉・戦争等のイベント処理は未実装です。 |
| 観測・履歴（HLD §5.4、LLD §12） | 固定バスケット指数・前日比・総資金を CLI／CSV で観測できます。統合テストと CTest の CLI シナリオがあります。[CLI](../src/main.cpp)、[価格指数](../include/market/price_index.h)、[統合テスト](../tests/system_tests/integration_test.cpp)、[CSV テスト](../tests/cli_csv.cmake) | `Transaction` 型はありますが Agent の履歴管理はスタブです。集計済み総生産量、格差、失業率、流通速度、取引量の時系列は未実装です。 |
| 日次失敗と検証範囲（LLD「日次処理の成否」） | 事前入力の拒否、失敗日の非0終了と観測行の抑止をテストします。[日次事前検証](../tests/system_tests/day_validation_test.cpp)、[CLI 異常系](../tests/cli_failure.cmake) | 実行途中の例外で一日全体を巻き戻す契約はありません。全日ロールバックの要否、大規模性能、リーク検査、カバレッジの完了は未確認です。 |

### 独立した Issue の候補

現時点では設計を要する項目を実装確約とせず、以下の単位で既存 Issue・設計議論と照合してから登録します。

| 候補 | 達成条件 |
| --- | --- |
| 既存の循環割当てから雇用先の選択を分離 | デフォルト時は現行の割当てと給与結果を再現し、別の選択規則で決定的なテストができる。 |
| 貸付時の借り手の解決 | 有効な借り手と不明なIDを区別するテストを追加する。日次元利返済は返済順・期限の仕様決定を経て別 Issue にする。 |
| 日次の課税対象と課税ベースを決める | 残高課税と所得・利益課税の設計差を解消し、変更対象を一つに絞る。税率や価格統制の導入は別 Issue とする。 |
| 交易ルート1本の在庫移動 | 出発時に送出在庫を減らし、到着日に受取在庫を増やし、輸送中を含む数量の整合性を検証する。 |
| イベント1種の決定的な検証 | 固定日指定で単一の商品または企業の供給量に影響を適用し、イベント前後の数量を検証する。 |
| 日次総生産量の観測 | 固定シナリオで期待総量と一致することを検証する。CSV へ追加する場合は既存列の互換性を決める。 |
