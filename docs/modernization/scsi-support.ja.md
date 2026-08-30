<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# PC-88VA SCSI サポートディスク

Language: [English](scsi-support.md) | [日本語](scsi-support.ja.md)

この文書は、PC-88VAでPC-9801-55互換SCSIインターフェースを使い、
固定ディスクまたはMOを接続する手順です。VAEGのSASIサポートディスクは
外付けSCSIターゲットの初期化やフォーマットを自動実行しません。

## 160MB固定SCSIディスク

生成したVA/VA2用SASIサポートディスクから起動し、Aドライブ上の
PC-Engineシステムディスクに対して次を実行します。

```dos
VBUFF -D1 -B12
```

実行後に再起動してください。`-D1` はAドライブ、`-B12` は4096バイトの
論理セクタ用バッファを指定します。

次に、接続したSCSIディスクのIDを指定して起動します。

```dos
SCFORM /SS
```

`SCFORM` の `S` は論理セクタサイズの増加指定です。

| 起動オプション | 論理セクタ |
| --- | ---: |
| `SCFORM` | 1024バイト |
| `SCFORM /S` | 2048バイト |
| `SCFORM /SS` | 4096バイト |
| `SCFORM /SSS` | 8192バイト |

160MB領域では `/SS` を使用します。`/S` では容量入力が同じ行に戻ることが
あります。

メニューでは次を指定します。

```text
SCSI-ID       : 接続した機器のID（例 0）
処理          : 1（領域確保）
開始シリンダ  : 空き領域の先頭（通常 1）
容量          : 表示された空き容量（160MB級では約159）
論理セクタ    : 4096 bytes
```

続いてルートディレクトリエントリ数とクラスタサイズを指定し、FATと
ディレクトリの初期化が完了するまで待ちます。既存データを残す必要が
あるディスクでは「装置初期化」や領域開放を選択しないでください。

処理が終わったら `9` で終了し、再起動します。SCHDが領域を認識した後、
割り当てられたドライブで `DIR` と `CHKDSK` を実行して確認します。

## 128MB MO

MOではSCHDに `-X -D1` を付け、VBUFFは2048バイトにします。
物理・論理フォーマットは破壊的操作なので、STEST55Sの `SFORM` を確認して
から手動で実行します。

```dos
VBUFF -D1 -B11
STEST55S SFORM
```

## 参照

英語版には、VAEGのSCSI実装範囲、ドライバ配置、アーカイブの出所、
および未検証のハードウェア項目を記載しています。生成ディスクでは、
プロファイルに応じて英語・日本語の操作メモを `A:\DOC` に収録できます。
