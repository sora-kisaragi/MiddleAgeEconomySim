# 中世近代貨幣経済シミュレーション

このプロジェクトは、中世近代の貨幣経済をシミュレートするゲームシステムの開発です。  
インフレ・デフレ、供給・需要のバランス、政治や自然災害などの要素が経済にどのように影響するかをシミュレートします。

## 目次
- [中世近代貨幣経済シミュレーション](#中世近代貨幣経済シミュレーション)
  - [目次](#目次)
  - [プロジェクト概要](#プロジェクト概要)
  - [開発環境](#開発環境)
  - [インストール手順](#インストール手順)
    - [必要なツール](#必要なツール)
  - [プロジェクト構成](#プロジェクト構成)
  - [コンパイル手順](#コンパイル手順)
  - [使用ライブラリ](#使用ライブラリ)
  - [実行方法](#実行方法)
  - [ドキュメント](#ドキュメント)
    - [補足](#補足)

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
- **エディタ**:  VSCode推奨 (GitHub Copilot)
- **プラットフォーム**: Linux / macOS / Windows (クロスプラットフォーム)

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
    │   ├── requirement.md       # 要件定義書
    │   ├── design.md            # 設計書
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
   git clone https://github.com/yourusername/MiddleAgeEconomySim.git
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

## ドキュメント

- **要件定義書**: [要件定義書](https://github.com/yourusername/MiddleAgeEconomySim/wiki/requirement)
- **設計書**: [設計書](https://github.com/yourusername/MiddleAgeEconomySim/wiki/design)

Wikiにアクセスして、プロジェクトの詳細な要件や設計情報を確認できます。

---

### 補足

- **拡張予定**: このシミュレーションはフェーズごとに進行します。最初はシンプルな経済シミュレーションから始まり、将来的に戦争や自然災害、異なる規模の経済圏への拡張を予定しています。
- **貢献方法**: このプロジェクトはオープンソースです。バグ修正や機能追加の提案などは歓迎します。

---