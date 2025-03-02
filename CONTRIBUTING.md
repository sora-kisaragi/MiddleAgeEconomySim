# 貢献ガイド (CONTRIBUTING.md)

このプロジェクトへの貢献を歓迎します！  
バグの修正、新機能の追加、ドキュメントの改善など、さまざまな形での参加が可能です。

## 📚 目次
- [重要な開発ルール](#重要な開発ルール)
- [貢献の流れ](#貢献の流れ)
- [開発環境のセットアップ](#開発環境のセットアップ)
- [コードの品質基準](#コードの品質基準)
- [テスト駆動開発](#テスト駆動開発)
- [Pull Requestのプロセス](#pull-requestのプロセス)
- [コミュニケーション](#コミュニケーション)

## 🔗 重要な開発ルール
プロジェクトの一貫性を保つため、以下のガイドラインを参照してください：
- [コーディング規約](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/Coding-Standards)
- [ブランチ戦略](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/Branch-Strategy)
- [テスト規約](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/Testing-Guidelines)
- [基本設計書](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/High_Level_Design)
- [詳細設計書](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/Low_Level_Design)

## 🛠 貢献の流れ

### 1. **Issue を確認する**
新機能の追加やバグ修正を始める前に、まず[Issues](https://github.com/sora-kisaragi/MiddleAgeEconomySim/issues)を確認し、同様の提案やバグレポートがないかを確認してください。

### 2. **新しい Issue を作成する**
既存のIssueがない場合は、以下の情報を含む新しいIssueを作成してください：
- バグの場合: 再現手順、期待される動作、実際の動作
- 機能追加の場合: 機能の詳細な説明、目的、想定される実装方法

### 3. **ブランチを作成する**
Issueが承認されたら、適切な命名規則に従ってブランチを作成します：
```bash
git checkout main
git pull origin main
git checkout -b feature/機能名   # 新機能の場合
git checkout -b bugfix/バグ名    # バグ修正の場合
```

### 4. **コードを実装する**
- [コーディング規約](https://github.com/sora-kisaragi/MiddleAgeEconomySim/wiki/Coding-Standards)に従ってコードを記述してください
- 実装前にテストを書く（テスト駆動開発）ことを推奨します
- 小さな単位で頻繁にコミットしてください

### 5. **テストを実行する**
すべての変更に対して、既存および新規のテストが通過することを確認します：
```bash
cd build
cmake ..
make test
```

### 6. **プルリクエストを作成する**
- 変更内容を簡潔に説明するタイトル
- 詳細な説明（対応するIssue番号への参照を含める）
- テスト結果のスクリーンショットや出力結果（可能であれば）

## 💻 開発環境のセットアップ

### 必要なツール
- C++17対応コンパイラ（GCC 7以降、Clang 5以降、MSVC 2017以降）
- CMake 3.14以降
- Git

### セットアップ手順
1. リポジトリをクローン：
```bash
git clone https://github.com/sora-kisaragi/MiddleAgeEconomySim.git
cd MiddleAgeEconomySim
```

2. ビルド環境を構築：
```bash
mkdir build
cd build
cmake ..
```

3. コンパイルとテスト：
```bash
make
make test
```

## 🧪 テスト駆動開発

このプロジェクトでは、テスト駆動開発（TDD）の原則に従っています：

1. **RED**: まず失敗するテストを書く
2. **GREEN**: テストがパスするように最小限のコードを実装する
3. **REFACTOR**: コードの品質を改善（テストが引き続きパスすることを確認）

テストの作成例：
```cpp
// filepath: tests/person_test.cpp
#include "gtest/gtest.h"
#include "person.h"

TEST(PersonTest, InitialMoneyIsZero) {
    Person p;
    EXPECT_EQ(0, p.money);
}
```

## 📝 コードの品質基準

### コードレビューの基準
- すべての新しいコードにはテストが必要です
- メモリリークがないこと
- 不要なコピーや非効率なアルゴリズムがないこと
- 命名規則がコーディング規約に沿っていること
- 適切なエラー処理が実装されていること

### パフォーマンスの考慮
- 大量のエージェント処理における効率性
- 不要なメモリ割り当てを避ける
- 計算負荷の大きい操作の最適化

## 🤝 コミュニケーション

### 質問や議論
- 技術的な質問や議論は[Issues](https://github.com/sora-kisaragi/MiddleAgeEconomySim/issues)で行ってください
- より詳細な議論が必要な場合は[Discussions](https://github.com/sora-kisaragi/MiddleAgeEconomySim/discussions)を利用してください

### コードレビューのエチケット
- 建設的なフィードバックを心がける
- 具体的な改善案を提示する
- レビュー依頼から3営業日以内に返信する

---

ご協力いただきありがとうございます！あなたの貢献がこのプロジェクトをより良いものにします。