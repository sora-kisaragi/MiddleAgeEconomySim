# コーディング規約

## 1. **命名規則**
   - **変数名**: 
     - ローカル変数は`camelCase`（例：`userName`、`totalAmount`）
     - メンバー変数やプライベート変数は`m_`プレフィックスを付ける（例：`m_userName`）
   - **関数名**: 
     - メソッドや関数名も`camelCase`（例：`calculateTotal()`、`getUserName()`）
   - **クラス名**:
     - クラス名は`PascalCase`（例：`UserManager`、`OrderProcessor`）
   - **定数名**: 
     - 定数は全て大文字、単語間にアンダースコア（`UPPER_SNAKE_CASE`）（例：`MAX_BUFFER_SIZE`）
   - **列挙型**:
     - 列挙型の名前は`PascalCase`（例：`ColorType`）、列挙子は`UPPER_SNAKE_CASE`（例：`RED`, `BLUE`）

## 2. **インデントとフォーマット**
   - **インデント**: 
     - スペース4つ（タブではなくスペースを使用）を推奨。
     - 一貫して同じ数のスペースを使用してインデントをつける。
   - **行の長さ**: 
     - 一行あたり最大80文字を推奨（長い行は途中で改行）。
   - **波括弧**:
     - 関数や制御構造（`if`, `for`など）の開始と終了の波括弧は必ず書く（1行であっても省略しない）。
     - `{` は関数定義の後に同じ行で書く（例：`if (x > 0) {`）。

## 3. **コメント**
   - **関数コメント**:
     - 関数ごとに何をする関数か、引数と戻り値、エラーハンドリングの説明を簡潔に記載。
     - 例: 
       ```cpp
       // Calculates the total price including tax.
       // @param price: The base price
       // @param taxRate: The tax rate
       // @return The total price
       double calculateTotal(double price, double taxRate);
       ```
   - **TODOコメント**:
     - 必要な実装や修正が必要な箇所には`TODO`コメントを残す。
   - **関数内コメント**:
     - 複雑な処理や意図が不明瞭な部分にコメントをつけるが、コード自体が自己説明的であるべき。

## 4. **スペースと改行**
   - **演算子の前後にスペース**:
     - 演算子（`+`, `-`, `=`, `==`, `&&`など）の前後にスペースを入れる。
     - 例: `a + b`、`if (x == 5)` と書く。
   - **カンマとセミコロンの後にはスペース**:
     - `,` や `;` の後にはスペースを入れる。
     - 例: `int x = 5, y = 10;`
   - **空行の使用**:
     - 関数間、クラスのメンバー間、重要なコードブロック間に空行を挿入して可読性を向上させる。

## 5. **エラーハンドリング**
   - C++では例外を使ったエラーハンドリングを推奨する場合が多い（例：`try-catch`）。
   - 可能であれば、エラーの発生を適切に報告し、関数の戻り値としてエラーコードを返すことを避ける（例：`int foo()` のようにエラーコードを返す代わりに、`void foo()` として例外を投げる）。
   - **例外を投げるタイミング**や**キャッチのタイミング**に関するルールを決める（例えば、エラーは最初に発生した場所で投げ、キャッチは一元管理）。

## 6. **コードの分割と関数設計**
   - **1つの関数は1つのことを行う**: 1つの関数が複数の責任を持たないようにする。関数は短く、1つの明確なタスクを担当。
   - **関数の引数**: 引数が多くなりすぎないように設計する。もし多くなる場合は、構造体やクラスを使用してまとめる。
   - **早期リターン**: 必要に応じて、条件分岐が深くなる前に早期リターンを行い、コードの可読性を高める。

## 7. **ガイドライン**
   - **const修飾子**:
     - 可能な限り引数や変数に`const`を付けて、変更されないことを明示。
     - 例: `const int foo = 10;`、`void bar(const std::string& str);`
   - **ポインタと参照の使い分け**:
     - 参照（`&`）が必要な場合は参照を使用し、ポインタは本当に必要なときだけ使用。
   - **スマートポインタ**: 
     - ポインタを直接使うよりも、可能な限り`std::unique_ptr`や`std::shared_ptr`を使用してリソース管理を自動化する。

## 8. **その他**
   - **C++標準ライブラリ**の使用を推奨（特に`std::vector`や`std::string`など）。
   - **静的解析ツール**（例: `Clang-Tidy`、`Cppcheck`）を使うことを検討し、コード品質を保つ。

## 9. **自動スタイルチェック**
   - プロジェクトでは以下の自動チェックツールを使用します：
     - **ClangFormat**: コードフォーマットの自動適用
     - **Clang-Tidy**: 静的解析によるコード品質チェック
     - **Cppcheck**: メモリリークや未定義動作の検出
   
   - VScodeの設定例:
     ```json
     {
       "editor.formatOnSave": true,
       "C_Cpp.clang_format_style": "{ BasedOnStyle: Google, IndentWidth: 4, ... }"
     }
     ```
   
   - CI/CDパイプラインでも上記ツールを使用し、コミット前にローカルで確認することを推奨します。

## 10. **構造体とクラスの使い分け**
   - **構造体（struct）**: 主にデータ集約のために使用し、メンバは基本的にpublic
     - 例: `struct Point { int x; int y; };`
   
   - **クラス（class）**: データとそれを操作する機能をカプセル化するために使用し、メンバは基本的にprivate
     - 例: 
       ```cpp
       class Person {
       private:
           std::string m_name;
           int m_age;
       public:
           void setName(const std::string& name);
           std::string getName() const;
       };
       ```
   
   - フェーズ1では構造体を中心に使用し、フェーズ2以降でクラスベースの設計に移行する予定です。

## 11. **テスト駆動開発におけるコーディング規約**
   - **テスト可能なコード設計**:
     - 依存性注入を活用し、コンポーネント間の結合度を低く保つ
     - グローバル変数や静的変数の使用を最小限に抑える
   
   - **テストコードの命名規則**:
     - テストクラスは `Test` サフィックスを付ける（例: `PersonTest`）
     - テストメソッドは `test` プレフィックスを付ける（例: `testNameSetting`）
     - または、TestCase_Condition_ExpectedBehavior の形式でも可
       （例: `Person_EmptyName_ThrowsException`）
   
   - **テストフィクスチャ**:
     - テストデータの準備は `SetUp()` メソッドで行い、クリーンアップは `TearDown()` メソッドで行う
     - 複数のテストで共通して使用するフィクスチャはクラスのメンバ変数として定義

## 12. **コードの例と反例**

### 良い例：
```cpp
// 関数は1つのことだけを行う
bool isValidUser(const std::string& username) {
    if (username.empty()) {
        return false;
    }
    
    if (username.length() < 3) {
        return false;
    }
    
    return true;
}
```

### 悪い例
```cpp
// 複数の責任を持つ関数
bool processUser(std::string username) {  // constが欠けている
    // ユーザー名の検証
    if (username == "") {  // empty()を使うべき
        return false;
    }
    
    // データベースへの接続 (別関数にすべき)
    Database db;
    db.connect();
    
    // ユーザーの保存 (別関数にすべき)
    return db.saveUser(username);
}
```