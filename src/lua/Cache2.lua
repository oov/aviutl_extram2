-- Cache2 @GIT_TAG@ ( @GIT_REVISION@ ) by oov

local P = {}

local Extram2 = require('Extram2')

------------------------------------------------------------
-- キャッシュエフェクト
------------------------------------------------------------

local g_effects = {}
local g_effect = nil

local function delete_effect(key)
  Extram2.del(key)
  g_effects[key] = nil
end

local function get_hash_key()
  local userdata, width, height = obj.getpixeldata()
  return width .. "-" .. height .. "-" .. Extram2.calc_hash(userdata, width, height)
end

--- キャッシュエフェクトの始点
-- モードに使用可能なのは以下の値
-- -1 キャッシュ完全無効
--  0 編集中のみキャッシュ無効
--  1 キャッシュ完全有効
-- @param string name キャッシュ名
-- @param number mode モード
function P.effect_before(mode)
  P.gc()

  local key = "CacheEffect:" .. obj.layer.."-"..obj.index.."-"..obj.num .. "-" .. get_hash_key()
  local m = g_effects[key]
  if m == nil then
    -- キャッシュ済みのデータがなかった
    g_effect = {
      key = key,
    }
    return
  end
  if (mode == -1) or (mode == 0 and m.frame == obj.frame and obj.getoption("gui")) then
    -- キャッシュを消去すべき状況だった
    -- 1. モードが -1 だった
    -- 2. モードが 0 でカーソルの位置が変わらずに再描画されていた（編集作業中と思われる）
    delete_effect(key)
    g_effect = {
      key = key,
    }
    return
  end
  local width, height = Extram2.get_size(key)
  if width == nil then
    -- キャッシュデータがあるはずだったが、Extram2.exe 側にデータがなかった
    delete_effect(key)
    g_effect = {
      key = key,
    }
    return
  end
  -- キャッシュ済みデータを読み込むのに必要な情報をまとめておく
  g_effect = {
    key = key,
    width = width,
    height = height,
    m = m,
  }
  -- 画像を 1x1 にすることで間に挟まるエフェクトの負荷を下げる
  obj.setoption("drawtarget", "tempbuffer", 1, 1)
  obj.load("tempbuffer")
end

--- キャッシュエフェクトの後処理
function P.effect_after()
  if g_effect == nil then
    -- 状態がおかしい
    return
  end
  if g_effect.width == nil then
    -- キャッシュ済みデータがなかったので通常通り描画された
    -- 今の状態をキャッシュとして控えておき次回に備える
    local width = Extram2.set(g_effect.key)
    if width == nil then
      g_effect = nil
      error("画像の保存に失敗しました")
    end
    g_effects[g_effect.key] = {
      frame = obj.frame,
      t = os.clock(),
      d = 0,
      ox = obj.ox,
      oy = obj.oy,
      oz = obj.oz,
      rx = obj.rx,
      ry = obj.ry,
      rz = obj.rz,
      cx = obj.cx,
      cy = obj.cy,
      cz = obj.cz,
      zoom = obj.zoom,
      alpha = obj.alpha,
      aspect = obj.aspect,
    }
    g_effect = nil
    return
  end
  -- キャッシュ済みデータがあるはずなので読み込む
  obj.setoption("drawtarget", "tempbuffer", g_effect.width, g_effect.height)
  obj.load("tempbuffer")
  local p, w, h = obj.getpixeldata()
  if not Extram2.get_direct(g_effect.key, p, w, h) then
    delete_effect(g_effect.key)
    g_effect = nil
    error("画像の読み込みに失敗しました")
  end
  obj.putpixeldata(p)
  local m = g_effect.m
  m.t = os.clock()
  m.d = 0
  m.frame = obj.frame
  obj.ox = m.ox
  obj.oy = m.oy
  obj.oz = m.oz
  obj.rx = m.rx
  obj.ry = m.ry
  obj.rz = m.rz
  obj.cx = m.cx
  obj.cy = m.cy
  obj.cz = m.cz
  obj.zoom = m.zoom
  obj.alpha = m.alpha
  obj.aspect = m.aspect
  g_effect = nil
end

function P.effect(mode, fn)
  P.effect_before(mode)
  if g_effect.width == nil then
    fn()
  end
  P.effect_after()
end

------------------------------------------------------------
-- キャッシュテキスト
------------------------------------------------------------

local g_texts = {}

local function delete_text(key)
  if g_texts[key] ~= nil then
    for i = 0, g_texts[key].num do
      Extram2.del(key .. "-" .. i)
    end
  end
  g_texts[key] = nil
end

local g_creating = false
local g_beforekey = nil
local g_key = nil
local g_msg = nil

--- テキストオブジェクトが呼び出すキャッシュテキストのエントリーポイント
-- @param string message メッセージ本文
-- @param number mode 動作モード
--             -1 = 常に最新データを使う
--              0 = オブジェクト編集中以外はキャッシュを使う
--              1 = 常にキャッシュを使う
function P.text_before(message, mode, unescape)
  P.gc()

  if unescape then
    -- 拡張編集の GUI 上で入力されたテキストは Shift_JIS の駄目文字への対策が行われるが、
    -- そもそも文字列をダブルクォートで括っていない場合にはゴミになるので除去しておく
    message = message:gsub("([\128-\160\224-\255]\092)\092","%1")
  end
  g_msg = message
  g_beforekey = nil
  g_key = "CacheText:" .. obj.layer
  local tex = g_texts[g_key]
  if tex ~= nil then
    if (tex.msg ~= g_msg) or
       (mode == -1) or
       (mode == 0 and tex.frame == obj.frame and obj.getoption("gui")) then
      -- 編集中カーソル移動せずに再描画された（サイズを変更した場合など）か、
      -- テキスト内容が変わったか、キャッシュ無効モードならキャッシュを破棄
      delete_text(g_key)
      tex = nil
    end
  end
  if tex == nil then
    mes(g_msg)
    g_creating = true
  else
    mes("<s1,Arial>" .. string.rep(".", tex.num))
    g_creating = false
  end
end

local function store_text(key)
  -- キャッシュ作成
  local c = g_texts[key]
  if c == nil then
    c = {
      t = os.clock(),
      frame = obj.frame,
      d = 0,
      msg = g_msg,
      num = obj.num,
      img = {},
    }
    g_texts[key] = c
  end
  if obj.w == 0 or obj.h == 0 then
    return
  end
  -- 画像データがありそうならキャッシュに書き込む
  local w, h = Extram2.set(key .. "-" .. obj.index)
  c.img[obj.index] = {
    w = w,
    h = h,
    ox = obj.ox,
    oy = obj.oy,
    oz = obj.oz,
    rx = obj.rx,
    ry = obj.ry,
    rz = obj.rz,
    cx = obj.cx,
    cy = obj.cy,
    cz = obj.cz,
    zoom = obj.zoom,
    alpha = obj.alpha,
    aspect = obj.aspect,
  }
end

local function load_text(key)
  local c = g_texts[key]
  if c == nil then
    error("invalid internal state")
  end
  if c.num ~= obj.num then
    -- キャッシュ有効時に「文字毎に個別オブジェクト」のチェックが切り替えられた
    -- 画像の枚数が変わるが今回はテキストが描画されていないので諦めるしかない
    delete_text(key)
    g_beforekey = nil
    g_key = nil
    return
  end
  if obj.index == 0 then
    c.t = os.clock()
    c.frame = obj.frame
    c.d = 0
  end
  local cimg = c.img[obj.index]
  if cimg == nil then
    -- 描画する必要がなさそう
    if obj.getoption("multi_object") then
      -- フォントサイズ 1 の Arial のピリオドが残ってしまうので消す
      obj.setoption("drawtarget", "tempbuffer", 1, 1)
      obj.load("tempbuffer")
    end
    return
  end
  obj.setoption("drawtarget", "tempbuffer", cimg.w, cimg.h)
  obj.load("tempbuffer")
  if not Extram2.get(key .. "-" .. obj.index) then
    -- キャッシュからの読み込みに失敗した場合は諦める（手動で消された場合など）
    -- データに不整合が起きているので一旦すべて仕切り直す
    delete_text(key)
    g_beforekey = nil
    g_key = nil
    return
  end
  obj.putpixeldata(data)
  obj.ox = cimg.ox
  obj.oy = cimg.oy
  obj.oz = cimg.oz
  obj.rx = cimg.rx
  obj.ry = cimg.ry
  obj.rz = cimg.rz
  obj.cx = cimg.cx
  obj.cy = cimg.cy
  obj.cz = cimg.cz
  obj.zoom = cimg.zoom
  obj.alpha = cimg.alpha
  obj.aspect = cimg.aspect
end

--- アニメーション効果「キャッシュテキスト2」から呼び出される関数
function P.text_after()
  if g_key ~= nil then
    if g_creating then
      store_text(g_key)
    else
      load_text(g_key)
    end
    g_beforekey = g_key
    g_key = nil
    return
  end
  if g_beforekey ~= nil and obj.index > 0 then
    if g_creating then
      store_text(g_beforekey)
    else
      load_text(g_beforekey)
    end
    return
  end
end

------------------------------------------------------------
-- ガベージコレクター
------------------------------------------------------------

local function delete_unused(table, now, lifetime, remover)
  for key, c in pairs(table) do
    if c.t + lifetime < now then
      -- 最近使われていないデータを発見
      if c.d == 0 then
        c.d = 1 -- 削除対象としてマーク
      else
        remover(key) -- 実際に削除
      end
    end
  end
end

local lifetime = 3 -- 秒
local gcinterval = 10 -- 秒
local lastgc = 0
function P.gc()
  local t = os.clock()
  if lastgc + gcinterval >= t then
    -- まだあんまり時間が経ってない
    return
  end
  delete_unused(g_texts, t, lifetime, delete_text)
  delete_unused(g_effects, t, lifetime, delete_effect)
  lastgc = os.clock()
end

return P
