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

local function execute_special_command(line)
  if line:match("^select schema ") then
    print("select schema: ", line:sub(15))
    if not rimeac.select_schema(line:sub(15)) then
      print("cannot select schema: " .. line:sub(15))
    end
    return true
  elseif line:match("^set option ") then
    local value = line:sub(12)[1]~='!'
    print("set option ", line:sub(12), value and "on" or "off")
    rimeac.set_option(line:sub(12), value)
    return true
  elseif line:match("^select candidate ") then
    print("select candidate ", line:sub(18))
    local index = tonumber(line:sub(18))
    if not rimeac.select_candidate(index) then
      print("cannot select candidate " .. tostring(index))
    end
    rimeac.print_session()
    return true
  elseif line:match("^print candidate list") then
    local cands = rimeac.get_candidates()
    if not cands or #cands == 0 then
      print("no candidates")
    end
    rimeac.print_session()
    return true
  elseif line:match("^delete on current page ") then
    local index = tonumber(line:sub(24))
    print("delete on current page", index)
    if not rimeac.delete_candidate_on_current_page(index) then
      print("failed to delete index " .. line:sub(8) .. " on current page")
    end
    rimeac.print_session()
    return true
  elseif line:match("^delete ") then
    local index = tonumber(line:sub(8))
    print("delete ", index)
    if not rimeac.delete_candidate(index)  then
      print("failed to delete index " .. line:sub(8))
    end
    rimeac.print_session()
    return true
  elseif line == "print schema list" then
    local list = rimeac.get_schema_id_list()
    local names = rimeac.get_schema_name_list()
    local schema = rimeac.get_current_schema()
    for i = 1, #list do
      print(tostring(i) .. " " .. list[i] .. " " .. names[i])
    end
    print("current schema: " .. schema)
    return true
  elseif line == "synchronize" then
    return rimeac.synchronize()
  end
  return false
end

-------------------------------------------------------------------
local continue = true
-------------------------------------------------------------------
local function main_loop()
  rimeac.init_rime()
  rimeac.add_session()
  while continue do
    local input = io.read()
    if input == "exit" then
      continue = false
      break
    elseif input == "reload" then
      rimeac.destroy_sessions()
      rimeac.finalize_rime()
      main_loop()
    end
    if not execute_special_command(input) then
      if rimeac.simulate_keys(input) then
        rimeac.print_session()
      else
        print("Error processing key sequence: " .. input)
      end
    end
    if input == "synchronize" then 
      rimeac.destroy_sessions()
      rimeac.finalize_rime()
      main_loop()
    end
  end
  rimeac.destroy_sessions()
  rimeac.finalize_rime()
end

-- setup rime
rimeac.setup_rime("rimeac.lua", "./shared", "./usr", "./log")
-- main loop
main_loop()

