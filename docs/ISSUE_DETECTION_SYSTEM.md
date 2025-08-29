# 課題検出・追跡システム (Issue Detection & Tracking System)

## 概要

このシステムは、MiddleAgeEconomySimプロジェクトにおいて、現状の課題と問題点を自動検出し、実施したいタスクのブロッカーを体系的に追跡するためのツールです。

## 検出できる課題の種類

### 🐛 バグ・重要な問題
- **価格変動性の無制限増加**: 経済シミュレーションの安定性を損なう数値的問題
- **メモリリーク**: 無制限に成長するコンテナによるメモリ消費
- **データ整合性問題**: 市場データのリセット時の不整合

### ⚠️ コード品質問題
- **コンパイル警告**: 未使用パラメータ、変数などの警告
- **TODOマーカー**: 未完成の実装を示すコメント
- **ハードコード値**: 設定可能にすべき固定値

### 🔧 設計・統合問題
- **カプセル化違反**: publicメンバーによる内部状態の露出
- **機能統合不足**: 実装済み機能の未利用
- **エージェント相互作用**: 不完全な相互作用システム

## 使用方法

### 基本的な課題検出

```bash
# 基本分析スクリプトの実行
cd /tmp
python3 analyze_issues.py

# 連続的な課題追跡システムの実行
cd /home/runner/work/MiddleAgeEconomySim/MiddleAgeEconomySim
python3 scripts/continuous_issue_tracker.py
```

### 定期的な監視

```bash
# 週次実行のためのcronジョブ例
0 9 * * 1 cd /path/to/repo && python3 scripts/continuous_issue_tracker.py
```

## 生成されるレポート

### 1. 即座の分析結果 (`/tmp/issue_analysis_report.json`)
- 現在検出された全課題のスナップショット
- 重要度別・種類別の分類
- 具体的な修正提案

### 2. GitHub課題テンプレート (`/tmp/github_issues/`)
- 重要課題についての詳細なGitHubイシューテンプレート
- 再現手順と解決提案を含む
- 適切なラベルと優先度設定

### 3. 継続追跡データ (`docs/issue_tracking.json`)
- 時系列での課題発生・解決状況
- 過去10回のスキャン履歴
- 解決済み課題の記録

### 4. プロジェクト状況レポート (`docs/PROJECT_ISSUES.md`)
- 現在の課題状況の日本語サマリー
- 解決のロードマップ
- 進捗追跡情報

## 検出された主要課題（2024-08-29時点）

### 🔴 重要度：高
1. **価格変動性の無制限増加** - 経済シミュレーションの安定性に影響
2. **メモリリーク（履歴ベクター）** - 長時間実行時の安全性に影響

### 🟡 重要度：中
3. **実装済み機能の未統合** - TradeRoute, Government, LoanProviderシステム
4. **フェーズ1タスクの未完成** - エージェント相互作用、統合テストなど

### 🟢 重要度：低
5. **コンパイル警告** - コード品質の改善
6. **設計改善** - カプセル化、ハードコード値の除去

## カスタマイズ

### 新しい課題タイプの追加

```python
# scripts/continuous_issue_tracker.py内で新しいチェック関数を追加
def check_new_issue_type(self):
    issues = []
    # カスタム検出ロジック
    return issues

# run_full_scan()メソッドに追加
custom_issues = self.check_new_issue_type()
all_issues.extend(custom_issues)
```

### 重要度の調整

```python
# 重要度のマッピングをカスタマイズ
severity_mapping = {
    "memory_leak": "critical",
    "build_warning": "warning",
    "integration_gap": "enhancement"
}
```

## CI/CD統合

### GitHub Actionsでの自動実行

```yaml
name: Issue Detection
on:
  schedule:
    - cron: '0 9 * * 1'  # 毎週月曜日9時
  workflow_dispatch:

jobs:
  detect-issues:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Run Issue Detection
        run: python3 scripts/continuous_issue_tracker.py
      - name: Create Issues
        if: env.NEW_CRITICAL_ISSUES > 0
        run: |
          # 重要課題が見つかった場合はGitHubイシューを自動作成
```

## トラブルシューティング

### よくある問題

1. **ビルドエラー**: CMakeやmakeが失敗する場合
   ```bash
   # 依存関係の確認
   sudo apt-get install build-essential cmake
   ```

2. **Python依存関係**: モジュールが見つからない場合
   ```bash
   # 標準ライブラリのみ使用しているため、追加インストール不要
   ```

3. **権限エラー**: スクリプト実行権限がない場合
   ```bash
   chmod +x scripts/continuous_issue_tracker.py
   ```

## 今後の拡張予定

- [ ] **自動修正提案**: 単純な問題の自動修正提案
- [ ] **パフォーマンス監視**: 実行時間・メモリ使用量の追跡
- [ ] **品質メトリクス**: コード複雑度、テストカバレッジの監視
- [ ] **GitHub連携**: 自動的なイシュー作成・更新
- [ ] **ダッシュボード**: Webベースの可視化ツール

## 関連ドキュメント

- [プロジェクト課題一覧](PROJECT_ISSUES.md)
- [チェックリスト](../Checklist.md)
- [要件定義書](../wiki/Requirement.md)
- [貢献ガイド](../CONTRIBUTING.md)