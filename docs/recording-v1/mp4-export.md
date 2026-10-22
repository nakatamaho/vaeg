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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# 保存後の投稿用 MP4 変換（M100s11）

## 使い方

1. 録画設定の **「圧縮する」** を選び、通常どおり録画する。
2. 録画停止・MKV の正常保存後、別スレッドで同名の `.mp4` を生成する。
3. 画面右下に **「圧縮中…」** と進捗バー、待機数、**「中止」** を表示する。
   最終書き出し中は **「仕上げ中…」**。録画設定を閉じても表示は続く。
4. 完了すると **「圧縮しました」** と **「保存先を開く」** を約6秒表示する
   （マウスを重ねている間は延長）。失敗は元の録画を保持した旨を表示し、
   **「詳細」** で理由を確認できる。失敗通知は **「閉じる」** まで残る。

録画設定画面でも詳細な状態を確認できる。最後に正常保存した MKV は
**「保存済みの録画からMP4を作成」** でも変換できる。
再起動前などの既存ファイルは、**「圧縮する録画 (.mkv)」** に MKV のフルパスを
引用符なしで入力してから同じボタンを押す。空欄なら、この起動中に最後に保存した録画を使う。

選択は INI に保存され、録画開始時にその録画の設定として確定する。
Native / Displayed のどちらにも使える。自動分割した場合は、正常に閉じた
各 MKV に対して個別の MP4 を作る。リアルタイム H.264 録画ではない。

- **元の FFV1/PCM MKV は変更しない。** FFV1 も可逆圧縮だが、MP4 は投稿向けの
  非可逆 H.264/AAC コピーであり、アーカイブの置き換えではない。
- 映像全体をクロップせず、偶数サイズ・最大 1280×720 に縮小する。
  通常は拡大しない（符号化に必要な最小寸法は 2×2）。
  音声は 48 kHz / stereo / 128 kbit/s AAC。元のメディア時刻を使用する。
- 投稿先・アカウントによる時間、容量、アスペクト比などの制限への適合は別途確認する。
  自動アップロードは行わない。
- 変換は一度に一つ、待機は最大四つ。満杯・変換失敗・中止でも MKV は残る。
- **既存の MP4 は上書きしない。** シンボリックリンクも拒否する。
  再試行する場合は、既存出力を利用者が別名へ退避するなどして保存先を空ける。
- **アプリ終了時は待機・実行中の変換を中止する。** MKV は残る。
  終了後も動き続ける外部エンコーダーやサービスは起動しない。
- POSIX の安全な公開には同じディレクトリ内での hard link が必要。
  非対応のファイルシステムでは明示的に失敗し、上書き可能な処理へ切り替えない。

## バックエンド

| OS | 実装 | 条件 |
|---|---|---|
| macOS | AVFoundation（OS の H.264 / AAC） | 録画 ON。エンコーダー実装を同梱しない |
| Windows | Media Foundation（OS の H.264 / AAC） | 録画 ON。System32 から変換時にロード |
| Linux | 静的 FFmpeg libx264 / native AAC | 明示的なローカル専用ビルドのみ。既定 OFF |

Windows N 等で Media Foundation が存在しなくても、アプリ起動と MKV 録画は
継続できる。MP4 変換は失敗理由を表示する。OS が使用する具体的な GPU や
エンコーダー実装を選別・保証する機能ではない。

通常の静的 FFmpeg は FFV1/PCM のみを許可する。macOS/Windows の MP4 出力に
FFmpeg の H.264/AAC エンコーダーや MP4 muxer を追加する必要はない。
FFV1/PCM の読み取りと resampling のため、録画 ON では libswresample も必要。

## Linux の明示的ローカルビルド

**CI（self-hosted を含む）、公開版、release/packaging/handoff preset では使用禁止。**
ローカル版の実行ファイル・SDK・依存キャッシュを、CI artifact、配布用 staging、
通常のリリースコピー先へ置かない。既知の CI 変数や public preset の marker を
消して回避しない。構成時とビルド時の両方で確認し、構成済みツリーの持ち込みも拒否する。

依存バージョン、公開 package checksum、ライセンス境界は
[ADR-0018](../agents/DECISIONS/ADR-0018-post-recording-mp4.md) を先に確認する。
以下は実際に検証した **Ubuntu 24.04 arm64** プロファイルの手順。
別アーキテクチャや別バージョンの依存物は、リンク前に個別に pin/review する。

まず専用のローカル Linux 環境を用意する。コンテナ/VM を使う場合も
[AGENTS.md](../../AGENTS.md) のローカル runtime-storage 規約に従い、公開用環境や
SDK/cache と分離する。**依存物の取得・ビルドより前に**、リポジトリルートから:

```sh
set -eu
cmake -DVAEG_ENABLE_LOCAL_MP4_ENCODERS=ON \
  -DVAEG_ENABLE_RECORDING=ON -DVAEG_STATIC_FFMPEG=ON \
  -DVAEG_LOCAL_MP4_ACKNOWLEDGEMENT=LOCAL_ONLY_NO_DISTRIBUTION \
  -DCMAKE_SYSTEM_NAME="$(uname -s)" -P cmake/mp4_policy.cmake
```

通常の Linux ビルド前提のツール・SDL2 等に加え、ADR の checksum を検証した
`libx264-dev` package をこの専用環境内だけに用意する。FFmpeg tarball も ADR の
公開 URL から取得し、下記で検証する。`FFMPEG_TARBALL` はその絶対パスを指定する。

```sh
set -eu
repo="$PWD"
cmake -DVAEG_ENABLE_LOCAL_MP4_ENCODERS=ON \
  -DVAEG_ENABLE_RECORDING=ON -DVAEG_STATIC_FFMPEG=ON \
  -DVAEG_LOCAL_MP4_ACKNOWLEDGEMENT=LOCAL_ONLY_NO_DISTRIBUTION \
  -DCMAKE_SYSTEM_NAME="$(uname -s)" -P "$repo/cmake/mp4_policy.cmake"
sdk="$repo/build/local-only-mp4-sdk"
test "$(uname -m)" = aarch64
test "$(dpkg-query -W -f='${Version}' libx264-dev)" = '2:0.164.3108+git31e19f9-1'
echo '1a688333a67165dfb53a6562f6dcd2ea45b1d641ee9b9b32c7192e65bfc75b27  /usr/lib/aarch64-linux-gnu/libx264.a' | sha256sum -c -
echo "464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c  $FFMPEG_TARBALL" | sha256sum -c -
test ! -e "$sdk"
mkdir -p "$sdk"
tar -xJf "$FFMPEG_TARBALL" -C "$sdk"
cd "$sdk/ffmpeg-8.1.2"
./configure --prefix="$sdk/stage" --disable-shared --enable-static \
  --disable-programs --disable-doc --disable-debug --disable-asm \
  --disable-autodetect --disable-network --enable-gpl --disable-nonfree \
  --disable-version3 --disable-everything --enable-libx264 \
  --enable-encoder=ffv1,pcm_s16le,libx264,aac --enable-decoder=ffv1,pcm_s16le \
  --enable-muxer=matroska,mp4 --enable-demuxer=matroska --enable-protocol=file \
  --extra-ldflags=-L/usr/lib/aarch64-linux-gnu
make -j4
make install
cp /usr/lib/aarch64-linux-gnu/libx264.a "$sdk/stage/lib/"
cp COPYING.GPLv2 COPYING.LGPLv2.1 "$sdk/"
cp /usr/share/doc/libx264-dev/copyright "$sdk/x264-copyright.txt"
cd "$repo"
PKG_CONFIG_PATH="$sdk/stage/lib/pkgconfig" cmake -S . -B build/local-only-mp4 \
  -G Ninja -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_RECORDING=ON \
  -DVAEG_STATIC_FFMPEG=ON -DVAEG_ENABLE_LOCAL_MP4_ENCODERS=ON \
  -DVAEG_LOCAL_MP4_ACKNOWLEDGEMENT=LOCAL_ONLY_NO_DISTRIBUTION
cmake --build build/local-only-mp4
```

これは個人用ローカル構成の指定であり、ライセンス受諾や特許・再配布の許諾を
代行するものではない。libx264 の GPL と H.264/AAC の特許は別問題。
静的リンク、私的利用、OS API の利用だけを理由に法的な解決を主張しない。

検証範囲・未実施の実機/メディア gate は
[M100s11 task](../agents/tasks/M100s11_recording_post_mp4.md) と
[handoff](handoff.md) を参照する。
