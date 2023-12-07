Extram2 @GIT_TAG@ ( @GIT_REVISION@ )
====================================

Extram2 は拡張編集で Lua からデータを外部プログラムに一時保存し、再利用できるようにするためのプログラムです。  
使用には [bridge.dll](https://github.com/oov/aviutl_bridge) の導入が必要です。

以前の Extram.dll は[拡張編集RAMプレビュー](https://github.com/oov/aviutl_rampreview)の付属品でしたが、Extram2 では独立して動作するようになります。

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

付属するファイルを全て `exedit.auf` と同じ場所にコピーすればインストール完了です。
`bridge.dll` も必要になるので忘れずにインストールしておいてください。

アンインストールは導入したファイルを削除するだけで完了です。

使い方
------

### キャッシュテキスト2

[キャッシュテキスト](https://github.com/oov/aviutl_cachetext) を Extram2 に移植したものです。  
機能も全く同じです。

拡張編集の右クリックメニューから `メディアオブジェクトの追加`→`キャッシュテキスト2` を選ぶと、オブジェクトが追加されます。

`ここにテキストを書く` の部分を書き換えると表示されるテキストを変更できます。  
その他のパラメーターも通常のテキストオブジェクトと同じように扱うことができます。

1行目にある `<?m,s=    0    ,[==[` の `0` の部分を書き換えると、キャッシュの活用方法を切り替えることができます。  
`0` 以外に `-1` と `1` が設定可能で、意味はそれぞれ以下の通りです。

- `-1` - キャッシュ無効
- `0` - オブジェクトの編集中のみキャッシュ無効
- `1` - キャッシュ有効

通常の用途であれば `0` のままで十分にキャッシュの恩恵が得られます。  
`1` にするとオブジェクトの編集中にもキャッシュが活用されより快適に作業ができるようになる可能性がありますが、キャッシュが有効の間はサイズの変更などがすぐには反映されなくなります。  

キャッシュデータはテキスト本文が変わると自動的に再生成されます。

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
ただし、元データの変化を検知するために画像の内容をチェックするため、キャッシュテキストより効率はかなり落ちます。

使い方は、キャッシュしたいオブジェクトに `キャッシュエフェクト2上` と `キャッシュエフェクト2下` の２つのアニメーション効果を追加し、キャッシュさせたいエフェクトをその間に追加します。

`キャッシュエフェクト2上` にはモードオプションがあり、意味はキャッシュテキストと同じです。

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
