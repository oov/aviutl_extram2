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

--- �ۑ������e�L�X�g��摜���폜����
-- @param string key �ۑ���
-- @return string �����������ǂ���
function P:del(key)
  return call("del\0" .. key) == "1"
end

--- �ۑ����ꂽ�e�L�X�g��ǂݍ���
-- @param string key �ۑ���
-- @return string �����������ǂ���
function P:get_str(key)
  local r = call("get_str\0" .. key)
  if r:sub(1,1) == "1" then
    return r:sub(2)
  end
  return nil
end

--- �e�L�X�g��ۑ�����
-- key �� value �ɂ͔C�ӂ̓��e���g���邪 "\0" �� "\0" �͎g���Ȃ��̂Œ���
-- �ǂ����Ă��g�������ꍇ�͎��O�ɃG�X�P�[�v�����Ȃǂ����ނ���
-- @param string key �ۑ���
-- @param string value �ۑ����e
-- @return boolean �����������ǂ���
function P:set_str(key, value)
  return call("set_str\0" .. key .. "\0" .. value) == "1"
end

--- �ۑ�����Ă���摜�̕��ƍ�����Ԃ�
-- @param string key �ۑ���
-- @return number, number �ۑ������摜�̕��ƍ���
-- @return nil, nil ���s�����Ƃ�
function P:get_size(key)
  local r = call("get_size\0" .. key)
  if r:sub(1,1) == "1" then
    return parse_size(r:sub(2))
  end
  return nil, nil
end

--- key �ɕۑ����ꂽ�摜�f�[�^��ǂݍ���
-- �ۑ�����Ă���T�C�Y�����O�ɂ킩���Ă���ꍇ�� get_direct �̕�������
-- @param string key �ۑ���
-- @return boolean �����������ǂ���
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

--- key �ɕۑ����ꂽ�摜�f�[�^��ǂݍ���
-- �摜�̕��⍂���ɂ��Ă̓P�A����Ȃ��̂Ŏ��͂ō��킹��K�v������
-- @param string key �ۑ���
-- @param userdata pixels �s�N�Z���f�[�^
-- @param number width �s�N�Z���f�[�^�̕�
-- @param number height �s�N�Z���f�[�^�̍���
-- @return boolean �����������ǂ���
function P:get_direct(key, pixels, width, height)
  return call("get\0" .. key, "wp", pixels, width, height) == "1"
end

--- key �ɉ摜�f�[�^��ۑ�����
-- @param string key �ۑ���
-- @return number, number �ۑ������摜�̕��ƍ���
-- @return nil, nil ���s�����Ƃ�
function P:set(key)
  return self:set_direct(key, obj.getpixeldata())
end

--- key �ɉ摜�f�[�^��ۑ�����
-- �������ȍ~�ɂ� obj.getpixeldata() ����̖߂�l��n��
-- @param string key �ۑ���
-- @param userdata pixels �s�N�Z���f�[�^
-- @param number width �s�N�Z���f�[�^�̕�
-- @param number height �s�N�Z���f�[�^�̍���
-- @return number, number �ۑ������摜�̕��ƍ���
-- @return nil, nil ���s�����Ƃ�
function P:set_direct(key, pixels, width, height)
  local r = call("set\0" .. key, "rp", pixels, width, height)
  if r:sub(1,1) == "1" then
    return parse_size(r:sub(2))
  end
  return nil, nil
end

return P
