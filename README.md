# 中世近代貨幣経済シミュレーション

このプロジェクトは、中世近代の貨幣経済シミュレーションを実現するシステムです。  
ユーザーは仮想のエージェント（農民、貴族、兵士など）を通じて経済活動を体験し  
システム内での交易や財産管理、エージェントの行動がどのように相互作用するかを観察します。

## 目次
- [中世近代貨幣経済シミュレーション](#中世近代貨幣経済シミュレーション)
  - [目次](#目次)
  - [プロジェクト概要](#プロジェクト概要)
  - [開発環境](#開発環境)
  - [インストール手順](#インストール手順)
    - [必要なツール](#必要なツール)
  - [フェーズ1の目標](#フェーズ1の目標)
  - [テスト駆動開発（TDD）](#テスト駆動開発tdd)
  - [プロジェクト構成](#プロジェクト構成)
  - [コンパイル手順](#コンパイル手順)
  - [使用ライブラリ](#使用ライブラリ)
  - [実行方法](#実行方法)
  - [性能と拡張性](#性能と拡張性)
  - [ドキュメント](#ドキュメント)
  - [ドキュメントについて](#ドキュメントについて)
  - [ライセンス](#ライセンス)

---

## プロジェクト概要

このシミュレーションの目的は、中世近代における貨幣流通とその影響を観察することです。  
ゲームや物語の世界で使用できる経済システムとして応用できるように設計されています。

シミュレーション内でのエージェント（NPC）は、  
労働・消費・貯蓄を行い、市場で商品を売買したり,政府から税金を徴収されたりします。  
システム内では戦争や飢饉などの外的要因が市場に影響を与え、経済がどのように回るかを確認できます。

---

## 開発環境

このプロジェクトは以下の開発環境を使用しています。

- **言語**: C++17
- **ビルドツール**: CMake
- **テストツール**: Google Test
- **CI/CD**: GIthub Actions
- **エディタ**:  VSCode推奨 (GitHub Copilot)
- **プラットフォーム**: Linux / macOS / Windows (クロスプラットフォーム)
- **開発手法**: テスト駆動開発（TDD）

---

## インストール手順

### 必要なツール

- **CMake**: プロジェクトのビルドに使用
  - [CMake公式サイト](https://cmake.org/download/)
  
- **コンパイラ**: C++コンパイラ（GCCやClangを推奨）
  - Linux: `apt install build-essential`
  - macOS: Xcode Command Line Tools
  - Windows: Visual Studioのインストール

---

## フェーズ1の目標

- エージェントの基本的な行動（農民、貴族など）
- エージェント間での取引システム
- エージェントの状態（健康、犯罪傾向など）の管理
- 貨幣経済シミュレーションの基本的なフロー構築

> 注: フェーズ1では開発速度を優先し、構造体（struct）ベースの実装を行います。
> フェーズ2以降で段階的にクラスベースの実装へ移行する予定です。
>
> **開発手法**: このプロジェクトはテスト駆動開発（TDD）を採用し、機能実装の前に
> テストを作成することで品質と設計の整合性を確保します。

---

## テスト駆動開発（TDD）

このプロジェクトでは、テスト駆動開発（Test-Driven Development）の原則に従って開発を進めます：

1. 最初にテストを書く（機能実装前に失敗するテストを作成）
2. テストが失敗することを確認する（Red）
3. テストが成功するための最小限の実装を行う（Green）
4. コードをリファクタリングする（Refactor）
5. この流れを繰り返す

TDDを採用することで、以下のメリットを得られます：
- 設計の明確化
- バグの早期発見
- リファクタリングの安全性向上
- ドキュメントとしてのテスト

詳細は[テストの実施方法](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/Testing-Guidelines.md)をご参照ください。

---

## プロジェクト構成


    /MiddleAgeEconomySim
    │
    ├── CMakeLists.txt           # CMakeの設定ファイル
    ├── src/                    # ソースコード
    │   ├── main.cpp             # メインエントリーポイント
    │   ├── economy_simulator.cpp # 経済シミュレーター本体
    │   └── ...                 # その他のソースコード
    │
    ├── include/                 # ヘッダーファイル
    │   ├── person.h             # 人物クラス
    │   ├── business.h           # 企業クラス
    │   └── ...                 # その他のヘッダーファイル
    │
    ├── wiki/                    # Wiki用ページ（要件定義書・設計書など）
    │   ├── Requirement.md       # 要件定義書
    │   ├── High_Level_Design.md # 基本設計書
    │   ├── Low_Level_Design.md  # 詳細設計書
    │   └── ...                 # その他のWikiドキュメント
    │
    └── README.md                # プロジェクトの説明


**`/wiki/`** フォルダには要件定義書や設計書をGitHub Wikiとして格納します。  
これにより、システムの進行に伴って適宜内容を更新・補足できます。

Github Copilotにシステム要件を認識させるためにこちらのプロジェクト内に保管します。

---

## コンパイル手順

1. **ソースコードをクローン**

   ```bash
   git clone https://github.com/sora-kisaragi/MiddleAgeEconomySim.git
   cd MiddleAgeEconomySim
   ```

2. **ビルドディレクトリを作成**

   ```bash
   mkdir build
   cd build
   ```

3. **CMakeでビルド設定**

   ```bash
   cmake ..
   ```

4. **プロジェクトをビルド**

   ```bash
   make
   ```

---

## 使用ライブラリ

このプロジェクトでは、特定の外部ライブラリを使用していませんが、必要に応じて後で追加する可能性があります。

---

## 実行方法

コンパイル後、以下のコマンドでシミュレーションを実行できます。

```bash
./MiddleAgeEconomySim
```

---

## 性能と拡張性

このシミュレーションは以下の特性を考慮して設計されています：

- **スケーラビリティ**: 小規模（村レベル）から大規模（国家レベル）へ段階的に拡張可能
- **パフォーマンス**: 大量のエージェント処理に対応するため、影響の少ないNPCをグループ化
- **安定性**: 価格の急激な変動を防ぐための緩衝メカニズム実装
- **拡張性**: 新しいエージェントタイプやイベントを容易に追加できる設計

---

## ドキュメント

- [要件定義書](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/Requirement.md)
- [基本設計書](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/High_Level_Design.md)
- [詳細設計書](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/Low_Level_Design.md)
- [ブランチ戦略](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/Branch-Strategy.md)
- [コーディング規約](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/Coding-Standards.md)
- [テストの実施方法](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/Testing-Guidelines.md)

---

## ドキュメントについて

開発ドキュメントはwikiフォルダ内とGitHub Wikiの両方に同期して管理されています。
どちらで編集しても自動的に同期されるため、ローカル環境でもGitHub上でも
ドキュメントを編集できます。

主なドキュメント:
- [基本設計書(HLD)](wiki/High_Level_Design.md)
- [詳細設計書(LLD)](wiki/Low_Level_Design.md)

---

## ライセンス

このプロジェクトは、[Apache License](./LICENSE)の下で公開されています。

---