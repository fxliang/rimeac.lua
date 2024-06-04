print("script begins!")

print("hello world in lua!")

--- test.setup_rime(app_name, shared_data_dir, user_data_dir, log_dir)
test.setup_rime("rimeac.lua", "./shared", "./usr", "./log")
--- test.init_rime()  no params, deploy
test.init_rime()

--- add a session
--- test.add_session() no params
test.add_session()
--- print sessions list
--- test.print_sessions() no params
test.print_sessions()

test.add_session()
test.print_sessions()

test.add_session()

--- select schema by schema_id
--- test.select_schema(schema_id)
test.select_schema("luna_pinyin")
test.print_sessions()

--- kill a session by index, which is the map index of sessions, always >= 1
--- test.kill_session(session_index) index >= 1
test.kill_session(1)
test.print_sessions()

--- switch to a session by index
--- test.switch_session(session_index) index >= 1
test.switch_session(2)

--- get session_id by index, if it doesn't exists, return 0
--- test.get_session(session_index) index >= 1
local id = test.get_session(10)
print(string.format("get_session id: 0x%x", id))
test.print_sessions()

test.select_schema("luna_pinyin")
test.print_sessions()

--- set option to current session
--- test.set_option(option_name, bool value)
test.set_option("zh_simp", true)
--- test.set_option("zh_trad", false)

--- get index of current session
local cindex = test.get_index_of_current_session()
print("current session index: ", cindex)

local sid = test.get_session(3)
local sidx = test.get_index_of_session(sid)
print("specific session index: ", sidx)

--- simulate key sequence to current session
--- test.simulate_keys(key_sequence)
test.simulate_keys("jx")

--- print current session, status, context, commit
test.print_session()
--- select candidate on current session, >= 0
test.select_candidate(2)
test.print_session()

--- destroy all sessions
--- test.destroy_sessions() no params
test.destroy_sessions()

--- finalize rime
--- test.finalize_rime() no params
test.finalize_rime()

print("script ends!")
