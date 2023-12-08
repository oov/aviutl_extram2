-- Extram2 @GIT_TAG@ ( @GIT_REVISION@ ) by oov

local P = {}

local bridge = require("bridge")
local exe_path = obj.getinfo("script_path") .. "Extram2.exe"

local function call(params, mode, p, w, h)
  return bridge.call(exe_path, params, mode, p, w, h)
end

local function parse_size(s)
  local p = s:find(" ", 1, true)
  if p == nil then
    return nil, nil
  end
  return tonumber(s:sub(1, p-1)), tonumber(s:sub(p+1))
end

--- 保存したテキストや画像を削除する
-- @param string key 保存名
-- @return string 成功したかどうか
function P:del(key)
  return call("del\0" .. key) == "1"
end

--- 保存されたテキストを読み込む
-- @param string key 保存名
-- @return string 成功したかどうか
function P:get_str(key)
  local r = call("get_str\0" .. key)
  if r:sub(1,1) == "1" then
    return r:sub(2)
  end
  return nil
end

--- テキストを保存する
-- key や value には任意の内容が使えるが "\0" や "\0" は使えないので注意
-- どうしても使いたい場合は事前にエスケープ処理などを挟むこと
-- @param string key 保存名
-- @param string value 保存内容
-- @return boolean 成功したかどうか
function P:set_str(key, value)
  return call("set_str\0" .. key .. "\0" .. value) == "1"
end

--- 保存されている画像の幅と高さを返す
-- @param string key 保存名
-- @return number, number 保存した画像の幅と高さ
-- @return nil, nil 失敗したとき
function P:get_size(key)
  local r = call("get_size\0" .. key)
  if r:sub(1,1) == "1" then
    return parse_size(r:sub(2))
  end
  return nil, nil
end

--- key に保存された画像データを読み込む
-- 保存されているサイズが事前にわかっている場合は get_direct の方が速い
-- @param string key 保存名
-- @return boolean 成功したかどうか
function P:get(key)
  local w, h = self:get_size(key)
  if w == nil or h == nil then
    return false
  end
  obj.setoption("drawtarget", "tempbuffer", w, h)
  obj.load("tempbuffer")
  local pixels, width, height = obj.getpixeldata()
  if not self:get_direct(key, pixels, width, height) then
    return false
  end
  obj.putpixeldata(pixels)
  return true
end

--- key に保存された画像データを読み込む
-- 画像の幅や高さについてはケアされないので自力で合わせる必要がある
-- @param string key 保存名
-- @param userdata pixels ピクセルデータ
-- @param number width ピクセルデータの幅
-- @param number height ピクセルデータの高さ
-- @return boolean 成功したかどうか
function P:get_direct(key, pixels, width, height)
  return call("get\0" .. key, "wp", pixels, width, height) == "1"
end

--- key に画像データを保存する
-- @param string key 保存名
-- @return number, number 保存した画像の幅と高さ
-- @return nil, nil 失敗したとき
function P:set(key)
  return self:set_direct(key, obj.getpixeldata())
end

--- key に画像データを保存する
-- 第二引数以降には obj.getpixeldata() からの戻り値を渡す
-- @param string key 保存名
-- @param userdata pixels ピクセルデータ
-- @param number width ピクセルデータの幅
-- @param number height ピクセルデータの高さ
-- @return number, number 保存した画像の幅と高さ
-- @return nil, nil 失敗したとき
function P:set_direct(key, pixels, width, height)
  local r = call("set\0" .. key, "rp", pixels, width, height)
  if r:sub(1,1) == "1" then
    return parse_size(r:sub(2))
  end
  return nil, nil
end

return P
