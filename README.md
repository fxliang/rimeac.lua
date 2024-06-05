# rimeac.lua

rime api console with lua

This is a mod of `rime_api_console` from [librime](https://github.com/rime/librime/tools/rime_api_console.cc), make it lua-embedded version. 

You can write your own test process of testing works in `script.lua`, you can make loops and so on(maybe later more)

## exported APIs

```lua
print("script begins!")

print("hello world in lua!")

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

--- get index of current session
local cindex = rimeac.get_index_of_current_session()
print("current session index: ", cindex)

local sid = rimeac.get_session(3)
local sidx = rimeac.get_index_of_session(sid)
print("specific session index: ", sidx)

--- simulate key sequence to current session
--- rimeac.simulate_keys(key_sequence)
rimeac.simulate_keys("jx")

--- print current session, status, context, commit
rimeac.print_session()
--- select candidate on current session, >= 0
rimeac.select_candidate(2)
rimeac.print_session()

--- destroy all sessions
--- rimeac.destroy_sessions() no params
rimeac.destroy_sessions()

--- finalize rime
--- rimeac.finalize_rime() no params
rimeac.finalize_rime()

print("script ends!")

```
