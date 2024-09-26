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

テキストに下にある `]==];mode =  0  ;save =  3  ;` の数字の部分を書き換えると、キャッシュの活用方法を切り替えることができます。  
基本的には変更しなくても問題ありません。

- `mode` - 動作モード
  - `-1` - キャッシュ無効
  - `0` - オブジェクトの編集中のみキャッシュ無効
  - `1` - キャッシュ有効
- `save` - データの場所
  - `0` - AviUtl のメモリ上に配置（AviUtlのメモリ空間を圧迫する）
  - `1` - 外部プロセスに保存（AviUtlのメモリ空間を圧迫しないが少し遅い）
  - `2` - `0` の機能 + 拡張編集のキャッシュも使う
  - `3` - `1` の機能 + 拡張編集のキャッシュも使う

通常の用途であれば変更しなくても十分にキャッシュの恩恵が得られます。  
メモリ不足で困っている場合は `save` を `1` にすることでメモリを節約できますが、キャッシュテキストで消費するメモリ量はあまり多くないので、問題が解決する可能性は低いです。

`2` と `3` は拡張編集が持つキャッシュの仕組みも併用することで、更に高速化を図ります。  
ただし拡張編集のキャッシュは空き容量確保のために勝手に消去されることがあり、そうなると期待通りの速度は出ません。  
そのため `2` と `3` は充分に余裕があるときだけ使用するのがオススメです。

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
- キャッシュモードを `1` にした状態で `文字毎に個別オブジェクト` を切り替えたときにテキストが描画されない
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
-- @param number cache_mode キャッシュモード
--             -1 = 常に最新データを使う
--              0 = オブジェクト編集中以外はキャッシュを使う
--              1 = 常にキャッシュを使う
-- @param number save_mode 保存モード
--              0 - キャッシュをプロセス内に保存する
--              1 - キャッシュを外部プロセスに保存する
--              2 - キャッシュをプロセス内に保存する + 拡張編集のキャッシュも使う
--              3 - キャッシュを外部プロセスに保存する + 拡張編集のキャッシュも使う
require('Cache2').effect_before_custom(hash, cache_mode, save_mode)
```

### Extram2 と Intram2 について

`Extram2` と `Intram2` は同じ機能を提供する Lua 用モジュールで、データの保存先のみが異なります。

以下に実装されている機能を示します。  
この説明には `Extram2` を用いていますが、`Intram2` でも同様です。

```lua
  local Extram2 = require('Extram2')

  -- 保存されたデータを削除
  -- 対象のデータが見つからなくても成功とみなされます
  -- 戻り値はありません
  Extram2:del(key)

  -- 保存されていたデータを読み込む
  -- 内部で適切なサイズのバッファーを確保し、自動的に obj.getpixeldata / obj.putpixeldata を呼び出します
  -- 取得に成功した場合は true、失敗した場合は false を返します
  local r = Extram2:get(key)

  -- 保存されていたデータを読み込む
  -- data, w, h には obj.getpixeldata で取得したものを渡してください
  -- obj.putpixeldata も自動的には行われないので、必要に応じて呼び出してください
  -- 保存されていたデータとサイズが異なる場合は失敗として扱われます
  -- 取得に成功した場合は true、失敗した場合は false を返します
  local r = Extram2:get_direct(key, data, w, h)

  -- 保存されていたデータのサイズを取得
  -- 取得に失敗した場合は nil, nil を返します
  local w, h = Extram2:get_size(key)

  -- 画像を保存します
  -- 保存に成功した場合は保存した画像の幅と高さを返します
  -- obj に画像がないとき（obj.w か obj.h が 0 の場合）はエラーになります
  local w, h = Extram2:set(key)

  -- 画像を保存します
  -- data, w, h には obj.getpixeldata で取得したものを渡してください
  -- 保存に成功した場合は true、失敗した場合は false を返します
  local r = Extram2:set_direct(key, data, w, h)

  -- 文字列を読み込む
  -- データが見つからないときや保存されていたのが画像だった場合は nil を返します
  local str = Extram2:get_str(key)

  -- 文字列を保存します
  -- 保存に成功した場合は true、失敗した場合は false を返します
  local r = Extram2:set_str(key, str)

  -- 画像の内容を元にハッシュ値を計算し、文字列で返します
  -- data, w, h には obj.getpixeldata で取得したものを渡してください
  local r = Extram2.calc_hash(data, w, h)
```

更新履歴
--------

更新履歴は [CHANGELOG.md](https://github.com/oov/aviutl_extram2/blob/main/CHANGELOG.md) を参照してください。

## Credits

Extram2 is made possible by the following open source softwares.

### [hashmap.c](https://github.com/tidwall/hashmap.c)

> [!NOTE]
> This program/library used [a modified version of hashmap.c](https://github.com/oov/hashmap.c/tree/simplify).

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

### [Mingw-w64](https://github.com/mingw-w64/mingw-w64)

<details>
<summary>MinGW-w64 runtime licensing</summary>

```
MinGW-w64 runtime licensing
***************************

This program or library was built using MinGW-w64 and statically
linked against the MinGW-w64 runtime. Some parts of the runtime
are under licenses which require that the copyright and license
notices are included when distributing the code in binary form.
These notices are listed below.


========================
Overall copyright notice
========================

Copyright (c) 2009, 2010, 2011, 2012, 2013 by the mingw-w64 project

This license has been certified as open source. It has also been designated
as GPL compatible by the Free Software Foundation (FSF).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

   1. Redistributions in source code must retain the accompanying copyright
      notice, this list of conditions, and the following disclaimer.
   2. Redistributions in binary form must reproduce the accompanying
      copyright notice, this list of conditions, and the following disclaimer
      in the documentation and/or other materials provided with the
      distribution.
   3. Names of the copyright holders must not be used to endorse or promote
      products derived from this software without prior written permission
      from the copyright holders.
   4. The right to distribute this software or to use it for any purpose does
      not give you the right to use Servicemarks (sm) or Trademarks (tm) of
      the copyright holders.  Use of them is covered by separate agreement
      with the copyright holders.
   5. If any files are modified, you must cause the modified files to carry
      prominent notices stating that you changed the files and the date of
      any change.

Disclaimer

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESSED
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

======================================== 
getopt, getopt_long, and getop_long_only
======================================== 

Copyright (c) 2002 Todd C. Miller <Todd.Miller@courtesan.com> 
 
Permission to use, copy, modify, and distribute this software for any 
purpose with or without fee is hereby granted, provided that the above 
copyright notice and this permission notice appear in all copies. 
 	 
THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Sponsored in part by the Defense Advanced Research Projects
Agency (DARPA) and Air Force Research Laboratory, Air Force
Materiel Command, USAF, under agreement number F39502-99-1-0512.

        *       *       *       *       *       *       * 

Copyright (c) 2000 The NetBSD Foundation, Inc.
All rights reserved.

This code is derived from software contributed to The NetBSD Foundation
by Dieter Baron and Thomas Klausner.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.


===============================================================
gdtoa: Converting between IEEE floating point numbers and ASCII
===============================================================

The author of this software is David M. Gay.

Copyright (C) 1997, 1998, 1999, 2000, 2001 by Lucent Technologies
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of Lucent or any of its entities
not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.

        *       *       *       *       *       *       *

The author of this software is David M. Gay.

Copyright (C) 2005 by David M. Gay
All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that the copyright notice and this permission notice and warranty
disclaimer appear in supporting documentation, and that the name of
the author or any of his current or former employers not be used in
advertising or publicity pertaining to distribution of the software
without specific, written prior permission.

THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN
NO EVENT SHALL THE AUTHOR OR ANY OF HIS CURRENT OR FORMER EMPLOYERS BE
LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

        *       *       *       *       *       *       *

The author of this software is David M. Gay.

Copyright (C) 2004 by David M. Gay.
All Rights Reserved
Based on material in the rest of /netlib/fp/gdota.tar.gz,
which is copyright (C) 1998, 2000 by Lucent Technologies.

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of Lucent or any of its entities
not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.


=========================
Parts of the math library
=========================

Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.

Developed at SunSoft, a Sun Microsystems, Inc. business.
Permission to use, copy, modify, and distribute this
software is freely granted, provided that this notice
is preserved.

        *       *       *       *       *       *       *

Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.

Developed at SunPro, a Sun Microsystems, Inc. business.
Permission to use, copy, modify, and distribute this
software is freely granted, provided that this notice
is preserved.

        *       *       *       *       *       *       *

FIXME: Cephes math lib
Copyright (C) 1984-1998 Stephen L. Moshier

It sounds vague, but as to be found at
<http://lists.debian.org/debian-legal/2004/12/msg00295.html>, it gives an
impression that the author could be willing to give an explicit
permission to distribute those files e.g. under a BSD style license. So
probably there is no problem here, although it could be good to get a
permission from the author and then add a license into the Cephes files
in MinGW runtime. At least on follow-up it is marked that debian sees the
version a-like BSD one. As MinGW.org (where those cephes parts are coming
from) distributes them now over 6 years, it should be fine.

===================================
Headers and IDLs imported from Wine
===================================

Some header and IDL files were imported from the Wine project. These files
are prominent maked in source. Their copyright belongs to contributors and
they are distributed under LGPL license.

Disclaimer

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
```
</details>

### [TinyCThread](https://github.com/tinycthread/tinycthread)

> [!NOTE]
> This program/library used [a modified version of TinyCThread](https://github.com/oov/tinycthread).

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
