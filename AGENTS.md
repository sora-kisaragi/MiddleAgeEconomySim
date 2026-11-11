# MiddleAgeEconomySim の作業指示

- [CONTRIBUTING.md](CONTRIBUTING.md) の作業手順と検証基準に従う。実装済みの挙動は対象コミットのコード・テスト・実行結果、設計意図は同じコミットの [要件定義書](wiki/Requirement.md)・[HLD](wiki/High_Level_Design.md)・[LLD](wiki/Low_Level_Design.md) で確認する。差異があれば根拠を記録し、Issue の範囲内で扱う。
- 日次開発、小規模な Issue の実装・検証、レビュー用 PR の準備では [middle-age-economy-development](.agents/skills/middle-age-economy-development/SKILL.md) を使う。Skill の自動検出がない環境では、ファイルを直接読む。
- C++17 と既存の CMake／GoogleTest を使う。対象 Issue と進行中 PR を確認し、最新 main を起点に作業する。作業範囲と公開・マージなどの権限は、今回の依頼または有効な実行指示で確認する。
