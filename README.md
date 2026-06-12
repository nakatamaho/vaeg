# 88VA Eternal Grafx

http://www.pc88.gr.jp/vaeg/

## リポジトリの諸設定

* ファイル
    * 文字コード: UTF-8
    * 改行
        * 既存ファイル: LF
        * 新規追加分: LF
* コミットメッセージ
    * 文字コード: UTF-8
    * 改行: LF

説明

* ファイルの文字コード
    * 既存ファイルは CP932 から UTF-8 へ変換済み。
    * legacy Windows backend の narrow string literal は実行時 Shift_JIS として扱うため、対応する build では execution charset を Shift_JIS にする。
* ファイルの改行コード
    * 本来 CR+LF だが、CVS利用時代の設定ミスで既存ファイルは LF になってしまっており、そのままとする。変更してしまうとgit blameで履歴が追いにくくなるため。
* コミットメッセージの文字コード
    * UTF-8 とする。

### Git Bash (Windows) 設定例

* 端末ウィンドウの文字コード: UTF-8
    * タイトルバー右クリック > Options > Text > Character set
* shell
    * `LANG=ja_JP.UTF-8`
* git
    ```
    git config core.autocrlf false
    git config core.pager "less"
    ```

説明

* ファイルとコミットメッセージはいずれも UTF-8 とする。
* 改行の自動変換は `.gitattributes` で無効化する。


