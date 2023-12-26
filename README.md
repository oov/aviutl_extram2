Extram2 @GIT_TAG@ ( @GIT_REVISION@ )
====================================

Extram2 は拡張編集で Lua からデータを外部プログラムに一時保存し、再利用できるようにするためのプログラムです。  
以前の Extram.dll は[拡張編集RAMプレビュー](https://github.com/oov/aviutl_rampreview)の付属品でしたが、Extram2 では独立して動作します。

Extram2 を活用するスクリプトとして、`キャッシュテキスト2` と `キャッシュエフェクト2` が付属しています。

動作環境
--------

- AviUtl 1.10
- 拡張編集 0.92

拡張編集 0.93rc1 は使用しないでください。

注意事項
-------

Extram2 は無保証で提供されます。  
Extram2 を使用したこと及び使用しなかったことによるいかなる損害について、開発者は何も責任を負いません。

これに同意できない場合、あなたは Extram2 を使用することができません。

ダウンロード
------------

https://github.com/oov/aviutl_extram2/releases

インストール／アンインストール
------------------------------

付属するファイルやフォルダーを全て `exedit.auf` と同じ場所にコピーすればインストール完了です。

アンインストールは導入したファイルを削除するだけで完了です。

使い方
------

### キャッシュテキスト2

[キャッシュテキスト](https://github.com/oov/aviutl_cachetext) を Extram2 に移植したものです。  
機能もほぼ同じです。

拡張編集の右クリックメニューから `メディアオブジェクトの追加`→`キャッシュテキスト2` を選ぶと、オブジェクトが追加されます。

`ここにテキストを書く` の部分を書き換えると表示されるテキストを変更できます。  
その他のパラメーターも通常のテキストオブジェクトと同じように扱うことができます。

テキストに下にある `]==];mode =  0  ;pos =  0  ;` の数字の部分を書き換えると、キャッシュの活用方法を切り替えることができます。  
基本的には変更しなくても問題ありません。

- `mode` - 動作モード
  - `-1` - キャッシュ無効
  - `0` - オブジェクトの編集中のみキャッシュ無効
  - `1` - キャッシュ有効
- `pos` - データの場所場所
  - `0` - AviUtl のメモリ上に配置（AviUtlのメモリ空間を圧迫する）
  - `1` - 外部プロセスに保存（AviUtlのメモリ空間を圧迫しないが少し遅い）

通常の用途であれば変更しなくても十分にキャッシュの恩恵が得られます。  
メモリ不足で困っている場合は `pos` を `1` にすることでメモリを節約できますが、キャッシュテキストで消費するメモリ量はあまり多くないので、問題が解決する可能性は低いです。

#### キャッシュデータについて

キャッシュデータは各レイヤーごとに最後の１つのみを保持しています。  
例えばレイヤー１に `こんにちは` と `さようなら` が順番に配置されているとき、
`さようなら` を表示した時点で `こんにちは` のキャッシュは消えています。

最近使用されていないキャッシュデータは定期的に削除されます。  
ただし、この削除処理はどこかのレイヤーでキャッシュテキストが使われているときに、10秒間隔でしか実行されません。

#### 既知の問題点

- テキスト本文が同じだと別のテキストオブジェクトになってもキャッシュが切り替わらない  
  - 本文に何も差異がない場合スクリプトからは区別が付きません
  - テキスト末尾に `<#>` を加えるなど、本文に変化を加えれば回避できます
- キャッシュモードを `1` にした状態で `文字毎に個別オブジェクト` を切り替えたときや、一括キャッシュ削除を行った場合にテキストが描画されない
  - 状態が切り替わったことを事前に検出する方法がないために起こる問題です
  - カーソル移動などでプレビューが更新されると正常に戻るので実用上の問題はないです

### キャッシュエフェクト2

端的に言うと、テキストではないオブジェクトで使用できるキャッシュテキストです。  
ただし、元データの変化を検知するために画像の全ピクセルをチェックするため、キャッシュテキストより効率はかなり落ちます。

またこれはキャッシュテキストと同じように、入力画像が同じであればエフェクト処理後の画像も同じになるという前提の上で成り立つ仕組みです。  
アニメーションなど動きがあるものに対しては恩恵はありません。

使い方は、キャッシュしたいオブジェクトに `キャッシュエフェクト2上` と `キャッシュエフェクト2下` の２つのアニメーション効果を追加し、キャッシュさせたいエフェクトをその間に追加します。  
`キャッシュエフェクト2上` にはパラメーターがあり、内容はキャッシュテキストと同じです。

#### より高度な使い方

画像が変化しない、あるいは変化をパラメーターなどで検出可能な場合は、`キャッシュエフェクト2上カスタム` が使用できます。

このアニメーション効果には追加パラメーターがあり、`画像の内容を表す文字列` の部分にキャッシュのキーとなる文字列を指定します。  
このテキストが一致しているかどうかを基準にデータが再利用されるようになります。

元画像が一切変化しない場合は適当な文字列を指定するだけで問題ありません。  
変化する場合は、画像の変化に伴って変化する文字列を指定してください。

もし独自のスクリプトと連携させたい場合、以下の方法で直接 Lua から呼び出すこともできます。

```lua
--- キャッシュエフェクトの始点
-- @param string hash 画像の内容を表すユニークな文字列
-- @param number mode モード
--             -1 = 常に最新データを使う
--              0 = オブジェクト編集中以外はキャッシュを使う
--              1 = 常にキャッシュを使う
-- @param number save_to_extram2 キャッシュを外部プロセスに保存するかどうか
--              0 - キャッシュをプロセス内に保存する
--              1 - キャッシュを外部プロセスに保存する
require('Cache2').effect_before_custom(hash, mode, save_to_extram2)
```

更新履歴
--------

更新履歴は [CHANGELOG.md](https://github.com/oov/aviutl_extram2/blob/main/CHANGELOG.md) を参照してください。

## Credits

Extram2 is made possible by the following open source softwares.

### [hashmap.c](https://github.com/tidwall/hashmap.c)

NOTICE: This program used [a modified version of hashmap.c](https://github.com/oov/hashmap.c/tree/simplify).

<details>
<summary>The MIT License</summary>

```
The MIT License (MIT)

Copyright (c) 2020 Joshua J Baker

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
</details>

### [TinyCThread](https://github.com/tinycthread/tinycthread)

NOTICE: This program used [a modified version of TinyCThread](https://github.com/oov/tinycthread).

<details>
<summary>The zlib/libpng License</summary>

```
Copyright (c) 2012 Marcus Geelnard
              2013-2016 Evan Nemerson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
```
</details>
