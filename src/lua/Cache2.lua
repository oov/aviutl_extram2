-- Cache2 @GIT_TAG@ ( @GIT_REVISION@ ) by oov

local P = {}

local Intram2 = require('Intram2')
local Extram2 = require('Extram2')

local g_target = nil

------------------------------------------------------------
-- �L���b�V���G�t�F�N�g
------------------------------------------------------------

local g_effects = {}
local g_effect = nil

local function delete_effect(key)
  -- 1�����ڂ� cachepos
  local target = key:sub(1,1) == "0" and Intram2 or Extram2
  target:del(key)
  g_effects[key] = nil
end

local function get_hash_key()
  local userdata, width, height = obj.getpixeldata()
  return width .. "-" .. height .. "-" .. g_target.calc_hash(userdata, width, height)
end

--- �L���b�V���G�t�F�N�g�̎n�_
-- @param string hash �摜�̓��e��\�����j�[�N�ȕ�����
-- @param number mode ���[�h
--             -1 = ��ɍŐV�f�[�^���g��
--              0 = �I�u�W�F�N�g�ҏW���ȊO�̓L���b�V�����g��
--              1 = ��ɃL���b�V�����g��
-- @param number save_to_extram2 �L���b�V�����O���v���Z�X�ɕۑ����邩�ǂ���
--              0 - �L���b�V�����v���Z�X���ɕۑ�����
--              1 - �L���b�V�����O���v���Z�X�ɕۑ�����
function P.effect_before_custom(hash, mode, save_to_extram2)
  g_target = save_to_extram2 == 1 and Extram2 or Intram2
  P.gc()

  local cachepos = g_target == Intram2 and "0" or "1";
  local key = cachepos .. "CacheEffect:" .. hash
  local m = g_effects[key]
  if m == nil then
    -- �L���b�V���ς݂̃f�[�^���Ȃ�����
    g_effect = {
      key = key,
    }
    return
  end
  if (mode == -1) or (mode == 0 and m.frame == obj.frame and obj.getoption("gui")) then
    -- �L���b�V�����������ׂ��󋵂�����
    -- 1. ���[�h�� -1 ������
    -- 2. ���[�h�� 0 �ŃJ�[�\���̈ʒu���ς�炸�ɍĕ`�悳��Ă����i�ҏW��ƒ��Ǝv����j
    delete_effect(key)
    g_effect = {
      key = key,
    }
    return
  end
  local width, height = g_target:get_size(key)
  if width == nil then
    -- �L���b�V���f�[�^������͂����Ȃ�����
    delete_effect(key)
    g_effect = {
      key = key,
    }
    return
  end
  -- �L���b�V���ς݃f�[�^��ǂݍ��ނ̂ɕK�v�ȏ����܂Ƃ߂Ă���
  g_effect = {
    key = key,
    width = width,
    height = height,
    m = m,
  }
  -- �摜�� 1x1 �ɂ��邱�ƂŊԂɋ��܂�G�t�F�N�g�̕��ׂ�������
  obj.setoption("drawtarget", "tempbuffer", 1, 1)
  obj.load("tempbuffer")
end

--- �L���b�V���G�t�F�N�g�̎n�_
-- @param number mode ���[�h
--             -1 = ��ɍŐV�f�[�^���g��
--              0 = �I�u�W�F�N�g�ҏW���ȊO�̓L���b�V�����g��
--              1 = ��ɃL���b�V�����g��
-- @param number save_to_extram2 �L���b�V�����O���v���Z�X�ɕۑ����邩�ǂ���
--              0 - �L���b�V�����v���Z�X���ɕۑ�����
--              1 - �L���b�V�����O���v���Z�X�ɕۑ�����
function P.effect_before(mode, save_to_extram2)
  return P.effect_before_custom(get_hash_key(), mode, save_to_extram2)
end

--- �L���b�V���G�t�F�N�g�̌㏈��
function P.effect_after()
  if g_effect == nil then
    -- ��Ԃ���������
    return
  end
  if g_effect.width == nil then
    -- �L���b�V���ς݃f�[�^���Ȃ������̂Œʏ�ʂ�`�悳�ꂽ
    -- ���̏�Ԃ��L���b�V���Ƃ��čT���Ă�������ɔ�����
    local key = g_effect.key
    g_effect = nil
    g_target:set(key)
    g_effects[key] = {
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
    return
  end
  -- �L���b�V���ς݃f�[�^������͂��Ȃ̂œǂݍ���
  if not g_target:get(g_effect.key) then
    delete_effect(g_effect.key)
    g_effect = nil
    error("�摜�̓ǂݍ��݂Ɏ��s���܂���")
  end
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
-- �L���b�V���e�L�X�g
------------------------------------------------------------

local g_texts = {}

local function delete_text(key)
  if g_texts[key] ~= nil then
    for i = 0, g_texts[key].num do
      -- 1�����ڂ� cachepos
      local target = key:sub(1,1) == "0" and Intram2 or Extram2
      target:del(key .. "-" .. i)
    end
  end
  g_texts[key] = nil
end

local g_creating = false
local g_beforekey = nil
local g_key = nil
local g_msg = nil

--- �e�L�X�g�I�u�W�F�N�g���Ăяo���L���b�V���e�L�X�g�̃G���g���[�|�C���g
-- @param string message ���b�Z�[�W�{��
-- @param number mode ���샂�[�h
--             -1 = ��ɍŐV�f�[�^���g��
--              0 = �I�u�W�F�N�g�ҏW���ȊO�̓L���b�V�����g��
--              1 = ��ɃL���b�V�����g��
-- @param number save_to_extram2 �L���b�V�����O���v���Z�X�ɕۑ����邩�ǂ���
--              0 - �L���b�V�����v���Z�X���ɕۑ�����
--              1 - �L���b�V�����O���v���Z�X�ɕۑ�����
-- @param boolean unescape �G�X�P�[�v�������������邩�ǂ���
function P.text_before(message, mode, save_to_extram2, unescape)
  g_target = save_to_extram2 == 1 and Extram2 or Intram2
  P.gc()

  if unescape then
    -- �g���ҏW�� GUI ��œ��͂��ꂽ�e�L�X�g�� Shift_JIS �̑ʖڕ����ւ̑΍􂪍s���邪�A
    -- ����������������_�u���N�H�[�g�Ŋ����Ă��Ȃ��ꍇ�ɂ̓S�~�ɂȂ�̂ŏ������Ă���
    message = message:gsub("([\128-\160\224-\255]\092)\092","%1")
  end
  g_msg = message
  g_beforekey = nil
  local cachepos = g_target == Intram2 and "0" or "1";
  g_key = cachepos .. "CacheText:" .. obj.layer
  local tex = g_texts[g_key]
  if tex ~= nil then
    if (tex.msg ~= g_msg) or
       (mode == -1) or
       (mode == 0 and tex.frame == obj.frame and obj.getoption("gui")) then
      -- �ҏW���J�[�\���ړ������ɍĕ`�悳�ꂽ�i�T�C�Y��ύX�����ꍇ�Ȃǁj���A
      -- �e�L�X�g���e���ς�������A�L���b�V���������[�h�Ȃ�L���b�V����j��
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
  -- �L���b�V���쐬
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
  -- �摜�f�[�^�����肻���Ȃ�L���b�V���ɏ�������
  local w, h = g_target:set(key .. "-" .. obj.index)
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
    -- �L���b�V���L�����Ɂu�������ɌʃI�u�W�F�N�g�v�̃`�F�b�N���؂�ւ���ꂽ
    -- �摜�̖������ς�邪����̓e�L�X�g���`�悳��Ă��Ȃ��̂Œ��߂邵���Ȃ�
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
    -- �`�悷��K�v���Ȃ�����
    if obj.getoption("multi_object") then
      -- �t�H���g�T�C�Y 1 �� Arial �̃s���I�h���c���Ă��܂��̂ŏ���
      obj.setoption("drawtarget", "tempbuffer", 1, 1)
      obj.load("tempbuffer")
    end
    return
  end
  if not g_target:get(key .. "-" .. obj.index) then
    -- �L���b�V������̓ǂݍ��݂Ɏ��s�����ꍇ�͒��߂�i�蓮�ŏ����ꂽ�ꍇ�Ȃǁj
    -- �f�[�^�ɕs�������N���Ă���̂ň�U���ׂĎd�؂蒼��
    delete_text(key)
    g_beforekey = nil
    g_key = nil
    return
  end
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

--- �A�j���[�V�������ʁu�L���b�V���e�L�X�g2�v����Ăяo�����֐�
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
-- �K�x�[�W�R���N�^�[
------------------------------------------------------------

local function delete_unused(table, now, lifetime, remover)
  for key, c in pairs(table) do
    if c.t + lifetime < now then
      -- �ŋߎg���Ă��Ȃ��f�[�^�𔭌�
      if c.d == 0 then
        c.d = 1 -- �폜�ΏۂƂ��ă}�[�N
      else
        remover(key) -- ���ۂɍ폜
      end
    end
  end
end

local lifetime = 3 -- �b
local gcinterval = 10 -- �b
local lastgc = 0
function P.gc()
  local t = os.clock()
  if lastgc + gcinterval >= t then
    -- �܂�����܂莞�Ԃ��o���ĂȂ�
    return
  end
  delete_unused(g_texts, t, lifetime, delete_text)
  delete_unused(g_effects, t, lifetime, delete_effect)
  lastgc = os.clock()
end

return P
