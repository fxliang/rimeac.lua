-------------------------------------------------------------------------------
-- bit width
local BIT = math.maxinteger > 2^31 and 16 or 8
-- session_id format
local PFORMAT = "%0" .. BIT .. "X"
function on_message(obj, session_id, msg_type, msg_value)
  local msg = "lua > message: ["..PFORMAT.."] [%s] %s"
  print(msg:format(session_id, msg_type, msg_value))
  local state_label = rimeac.get_state_label(msg_type, msg_value)
  if msg_type == "option" then
    local option_name = msg_value:sub(1, 1) == "!" and msg_value:sub(2) or msg_value
    local state = rimeac.get_option(option_name) and 1 or 0
    if state_label ~= '' and state_label ~= nil then
      print(string.format("lua > update option: %s = %d // %s",
        option_name, state, state_label))
    end
  end
end
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-- get absolute path of current script
function script_path()
  local fullpath = debug.getinfo(1,"S").source:sub(2)
  if package.config:sub(1,1) == '\\' then
    local dirname_, filename_ = fullpath:match('^(.*\\)([^\\]+)$')
    local currentDir = io.popen("cd"):read("*l")
    if not dirname_ then dirname_ = '.' end
    if not filename_ then filename_ = fullpath end
    local command = 'cd ' .. dirname_ .. ' && cd'
    local p = io.popen(command)
    fullpath = p:read("*l") .. '\\' .. filename_
    p:close()
    os.execute('cd ' .. currentDir)
    fullpath = fullpath:gsub('[\n\r]*$','')
    dirname, filename = fullpath:match('^(.*\\)([^\\]+)$')
  else
    fullpath = io.popen("realpath '"..fullpath.."'", 'r'):read('a')
    fullpath = fullpath:gsub('[\n\r]*$','')
    dirname, filename = fullpath:match('^(.*/)([^/]-)$')
  end
  dirname = dirname or ''
  filename = filename or fullpath
  return dirname
end

-- get path divider
local div = package.config:sub(1,1) == '\\' and '\\' or '/'
local lib = package.config:sub(1,1) == '\\' and '?.dll' or '?.so'
local script_cpath = script_path() .. div .. lib
-- add the ?.so or ?.dll to package.cpath ensure requiring
-- you must keep the rime.dll or librime.so in current search path
package.cpath = package.cpath .. ';' .. script_cpath
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
require("librimeac")

print("script begins!")

print("hello world in lua!")
rimeac.init_env()
--- rimeac.setup_rime(app_name, shared_data_dir, user_data_dir, log_dir)
rimeac.setup_rime("rimeac.lua", "./shared", "./usr", "./log")
--- rimeac.init_rime()  no params, deploy
rimeac.init_rime()

--- add a session
--- rimeac.add_session() no params
rimeac.add_session()
--- print sessions list
--- rimeac.print_sessions() no params
rimeac.print_sessions()
---[[
rimeac.add_session()
rimeac.print_sessions()

rimeac.add_session()

--- select schema by schema_id
--- rimeac.select_schema(schema_id)
rimeac.select_schema("luna_pinyin")
rimeac.print_sessions()

--- kill a session by index, which is the map index of sessions, always >= 1
--- rimeac.kill_session(session_index) index >= 1
rimeac.kill_session(1)
rimeac.print_sessions()

--- switch to a session by index
--- rimeac.switch_session(session_index) index >= 1
rimeac.switch_session(2)

--- get session_id by index, if it doesn't exists, return 0
--- rimeac.get_session(session_index) index >= 1
local id = rimeac.get_session(10)
print(string.format("get_session id: 0x%x", id))
rimeac.print_sessions()

rimeac.select_schema("luna_pinyin")
rimeac.print_sessions()

--- set option to current session
--- rimeac.set_option(option_name, bool value)
rimeac.set_option("zh_simp", true)
--- rimeac.set_option("zh_trad", false)
local zh_trad = rimeac.get_option("zh_trad")
print("zh_trad status: " .. tostring(zh_trad))

--- get index of current session
local cindex = rimeac.get_index_of_current_session()
print("current session index: ", cindex)

local sid = rimeac.get_session(3)
local sidx = rimeac.get_index_of_session(sid)
print("specific session index: ", sidx)

--- simulate key sequence to current session
--- rimeac.simulate_keys(key_sequence)
print("call commit_composition")
rimeac.simulate_keys("ceshi")
rimeac.print_session()
rimeac.commit_composition(2)
rimeac.print_session()

-- sample to use rimeac.get_context
rimeac.simulate_keys("ceshi")
local commit, status, context = rimeac.get_context()
print("commit: " .. tostring(commit))
print("status: ")
for k, v in pairs(status) do
  print("status." .. k .. ": " .. tostring(v))
end
print("candidates: ")
for _, v in ipairs(context.menu.candidates) do
  print(v.label .. " " .. v.text .. " " .. v.comment)
end
rimeac.clear_composition()

print("call commit_composition_sid")
rimeac.simulate_keys("ceshi")
rimeac.print_session()
local sid = rimeac.get_session(2)
rimeac.commit_composition_sid(sid)
rimeac.print_session()

print("call clear_composition")
rimeac.simulate_keys("ceshi")
--- print current session, status, context, commit
rimeac.print_session()
rimeac.clear_composition()
rimeac.print_session()
print("simulate_keys again 'ceshi'")
rimeac.simulate_keys("ceshi")
rimeac.print_session()
-- get candidates and comments in lua
local cands,cmds = rimeac.get_candidates(), rimeac.get_comments()
if #cands then
  for i, v in ipairs(cands) do
    print(cands[i], cmds[i])
  end
end
--- assert(cands[1] == '测试')
--- follow line will fail
--- assert(cands[2] == '测试')

--- select candidate on current session, >= 0
rimeac.select_candidate(2)
rimeac.print_session()

--- destroy all sessions
--- rimeac.destroy_sessions() no params
rimeac.destroy_sessions()

--- finalize rime
--- rimeac.finalize_rime() no params
rimeac.finalize_rime()

rimeac.finalize_env()
print("script ends!")
