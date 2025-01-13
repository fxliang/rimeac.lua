extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#include <filesystem>
#include <iostream>
#include <map>
#include <rime_api.h>
#include <rime_levers_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifdef _WIN32
#define LIBRIMEAC_API __declspec(dllexport)
#include <windows.h>
inline unsigned int SetConsoleOutputCodePage(unsigned int codepage = CP_UTF8) {
  unsigned int cp = GetConsoleOutputCP();
  SetConsoleOutputCP(codepage);
  return cp;
}
#else
#define LIBRIMEAC_API
inline unsigned int SetConsoleOutputCodePage(unsigned int codepage = 65001) {
  return 0;
}
#endif /* _WIN32 */

using SessionsMap = std::map<int, RimeSessionId>;
namespace fs = std::filesystem;
unsigned int codepage;
lua_State *l;
RimeApi *rime;
SessionsMap sessions_map;
RimeSessionId current_session;

extern "C" {
void print_status(RimeStatus *status) {
  printf("schema: %s / %s\n", status->schema_id, status->schema_name);
  printf("status: ");
  if (status->is_disabled)
    printf("disabled ");
  if (status->is_composing)
    printf("composing ");
  if (status->is_ascii_mode)
    printf("ascii ");
  if (status->is_full_shape)
    printf("full_shape ");
  if (status->is_simplified)
    printf("simplified ");
  printf("\n");
}
void print_composition(RimeComposition *composition) {
  const char *preedit = composition->preedit;
  if (!preedit)
    return;
  size_t len = strlen(preedit);
  size_t start = composition->sel_start;
  size_t end = composition->sel_end;
  size_t cursor = composition->cursor_pos;
  for (size_t i = 0; i <= len; ++i) {
    if (start < end) {
      if (i == start) {
        putchar('[');
      } else if (i == end) {
        putchar(']');
      }
    }
    if (i == cursor)
      putchar('|');
    if (i < len)
      putchar(preedit[i]);
  }
  printf("\n");
}
void print_menu(RimeMenu *menu) {
  if (menu->num_candidates == 0)
    return;
  printf("page: %d%c (of size %d)\n", menu->page_no + 1,
         menu->is_last_page ? '$' : ' ', menu->page_size);
  for (int i = 0; i < menu->num_candidates; ++i) {
    bool highlighted = i == menu->highlighted_candidate_index;
    printf("%d. %c%s%c%s\n", i + 1, highlighted ? '[' : ' ',
           menu->candidates[i].text, highlighted ? ']' : ' ',
           menu->candidates[i].comment ? menu->candidates[i].comment : "");
  }
}
void print_context(RimeContext *context) {
  if (context->composition.length > 0 || context->menu.num_candidates > 0) {
    print_composition(&context->composition);
  } else {
    printf("(not composing)\n");
  }
  print_menu(&context->menu);
}
int print_session(lua_State *L) {
  RimeApi *rime = rime_get_api();
  RimeSessionId session_id = current_session;
  RIME_STRUCT(RimeCommit, commit);
  RIME_STRUCT(RimeStatus, status);
  RIME_STRUCT(RimeContext, context);

  if (rime->get_commit(session_id, &commit)) {
    printf("commit: %s\n", commit.text);
    rime->free_commit(&commit);
  }

  if (rime->get_status(session_id, &status)) {
    print_status(&status);
    rime->free_status(&status);
  }

  if (rime->get_context(session_id, &context)) {
    print_context(&context);
    rime->free_context(&context);
  }
  return 0;
}
void on_message(void *context_object, RimeSessionId session_id,
                const char *message_type, const char *message_value) {
  // try to load on_message from external lua script
  lua_getglobal(l, "on_message");
  // if on_message is defined in lua script
  if (lua_isfunction(l, -1)) {
    lua_pushlightuserdata(l, context_object);
    lua_pushnumber(l, static_cast<lua_Number>(session_id));
    lua_pushstring(l, message_type);
    lua_pushstring(l, message_value);
    if (lua_pcall(l, 4, 0, 0) != LUA_OK) {
      const char *error_message = lua_tostring(l, -1);
      std::cerr << "Error calling on_message: " << error_message << std::endl;
      lua_pop(l, 1);
    }
  } else {
    // default on_message
    printf("message: [%p] [%s] %s\n", (void *)session_id, message_type,
           message_value);
    RimeApi *rime = rime_get_api();
    if (RIME_API_AVAILABLE(rime, get_state_label) &&
        !strcmp(message_type, "option")) {
      Bool state = message_value[0] != '!';
      const char *option_name = message_value + !state;
      const char *state_label =
          rime->get_state_label(session_id, option_name, state);
      if (state_label) {
        printf("updated option: %s = %d // %s\n", option_name, state,
               state_label);
      }
    }
    lua_pop(l, 1);
  }
}
int get_state_label(lua_State *L) {
  RimeApi *rime = rime_get_api();

  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    lua_pushstring(L, "");
    return 1;
  }
  const char *message_type = lua_tostring(L, 1);
  const char *message_value = lua_tostring(L, 2);
  if (RIME_API_AVAILABLE(rime, get_state_label) &&
      !strcmp(message_type, "option")) {
    Bool state = message_value[0] != '!';
    const char *option_name = message_value + !state;
    const char *state_label =
        rime->get_state_label(current_session, option_name, state);
    if (state_label)
      lua_pushstring(L, state_label);
    else
      lua_pushstring(L, "");
  } else
    lua_pushstring(L, "");
  return 1;
}

inline void finalize_lua() { lua_close(l); }
int init_env(lua_State *L) {
  codepage = SetConsoleOutputCodePage();
  return 0;
}
int finalize_env(lua_State *L) {
#ifndef MODULE
  finalize_lua();
#endif
  if (!rime)
    rime = rime_get_api();
  rime->finalize();
  SetConsoleOutputCodePage(codepage);
  return 0;
}
int setup_rime(lua_State *L) {
  const char *app_name = lua_tostring(L, 1);
  const char *shared = lua_tostring(L, 2);
  const char *usr = lua_tostring(L, 3);
  const char *log = lua_tostring(L, 4);
  if (!rime)
    rime = rime_get_api();
  RIME_STRUCT(RimeTraits, traits);
  traits.app_name = app_name;
  traits.shared_data_dir = shared;
  traits.user_data_dir = usr;
  traits.log_dir = log;
  rime->setup(&traits);
  rime->set_notification_handler(&on_message, NULL);
  if (!fs::exists(log)) {
    fs::create_directories(log);
  }
  return 0;
}
int init_rime(lua_State *L) {
  fprintf(stderr, "initializing...\n");
  if (!rime)
    rime = rime_get_api();
  bool full_check = false;
  int n = lua_gettop(L);
  if (n == 1)
    full_check = lua_toboolean(L, 1);
  rime->initialize(NULL);
  if (rime->start_maintenance(full_check))
    rime->join_maintenance_thread();
  fprintf(stderr, "ready.\n");
  return 0;
}

int initialize(lua_State *L) {
  if (!rime)
    rime = rime_get_api();
  rime->initialize(NULL);
  return 0;
}
int finalize_rime(lua_State *L) {
  if (!rime)
    rime = rime_get_api();
  rime->finalize();
  return 0;
}
int simulate_keys(lua_State *L) {
  const char *keys = lua_tostring(L, 1);
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    lua_pushboolean(L, false);
    return 1;
  }
  // printf("simulate keys on session: %p, %s\n", (void*)current_session, keys);
  auto ret = rime->simulate_key_sequence(current_session, keys);
  lua_pushboolean(L, ret);
  return 1;
}
int select_schema(lua_State *L) {
  const char *schema_id = lua_tostring(L, 1);
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    lua_pushboolean(L, false);
    return 1;
  }
  auto ret = rime->select_schema(current_session, schema_id);
  lua_pushboolean(L, ret);
  return 1;
}
int set_option(lua_State *L) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return 0;
  }
  const char *option_name = lua_tostring(L, 1);
  bool value = lua_toboolean(L, 2);
  rime->set_option(current_session, option_name, value);
  return 0;
}
int get_option(lua_State *L) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    lua_pushboolean(L, false);
    return 1;
  }
  const char *option_name = lua_tostring(L, 1);
  auto ret = rime->get_option(current_session, option_name);
  lua_pushboolean(L, ret);
  return 1;
}
int select_candidate(lua_State *L) {
  auto index = lua_tointeger(L, 1);
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    lua_pushboolean(L, false);
    return 1;
  }
  auto ret = (index > 0) && rime->select_candidate_on_current_page(
                                current_session, ((size_t)index - 1));
  lua_pushboolean(L, ret);
  return 1;
}
int delete_candidate_on_current_page(lua_State *L) {
  auto index = lua_tointeger(L, 1);
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    lua_pushboolean(L, false);
    return 1;
  }
  auto ret = (index > 0) && rime->delete_candidate_on_current_page(
                                current_session, ((size_t)index - 1));
  lua_pushboolean(L, ret);
  return 1;
}
int delete_candidate(lua_State *L) {
  auto index = lua_tointeger(L, 1);
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    lua_pushboolean(L, false);
    return 1;
  }
  auto ret = (index > 0) &&
             rime->delete_candidate(current_session, ((size_t)index - 1));
  lua_pushboolean(L, ret);
  return 1;
}
int print_sessions(lua_State *L) {
  printf("current sessions list:\n");
  for (const auto &p : sessions_map) {
    char schema_id[256] = {0};
    rime->get_current_schema(p.second, schema_id, sizeof(schema_id));
    char mk = (p.second == current_session) ? '>' : ' ';
    printf("%c %d. session_id: %p, schema_id: %s\n", mk, p.first,
           (void *)p.second, schema_id);
  }
  return 0;
}
int add_session(lua_State *L) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    lua_pushboolean(L, false);
    return 1;
  }
  RimeSessionId id = rime->create_session();
  if (!id) {
    fprintf(stderr, "Error creating new rime session.\n");
    lua_pushboolean(L, false);
    return 1;
  }
  if (sessions_map.size())
    sessions_map[sessions_map.rbegin()->first + 1] = id;
  else
    sessions_map[1] = id;
  current_session = id;
  lua_pushboolean(L, true);
  return 1;
}
int destroy_sessions(lua_State *L) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return 0;
  }
  for (const auto &s : sessions_map) {
    printf("destroy session: %p\n", (void *)s.second);
    rime->destroy_session(s.second);
  }
  sessions_map.clear();
  return 0;
}
int kill_session(lua_State *L) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return 0;
  }
  auto id = lua_tointeger(L, 1);
  auto it = sessions_map.find((int)id);
  if (it == sessions_map.end()) {
    fprintf(stderr, "no session by this index!\n");
    return 0;
  }
  if (it != sessions_map.begin())
    it--;
  else
    it++;
  printf("destroy session: %p\n", (void *)sessions_map[(int)id]);
  rime->destroy_session(sessions_map[(int)id]);
  sessions_map.erase((int)id);
  current_session = it->second;
  return 0;
}
RimeSessionId get_session_c(int index) {
  if (index == 0)
    return current_session;
  else if (sessions_map.find(index) != sessions_map.end())
    return (RimeSessionId)sessions_map[index];
  else
    return 0;
}

// without parameters in lua
int get_session(lua_State *L) {
  lua_Integer index = lua_tointeger(L, 1);
  auto id = get_session_c((int)index);
  lua_pushinteger(L, (lua_Integer)id);
  return 1;
}
int switch_session(lua_State *L) {
  auto index = lua_tointeger(L, 1);
  RimeSessionId id = get_session_c((int)index);
  bool ret = true;
  if (!id)
    ret = false;
  else
    current_session = id;
  lua_pushboolean(L, ret);
  return 1;
}
int get_index_of_current_session(lua_State *L) {
  for (auto &p : sessions_map) {
    if (p.second == current_session) {
      lua_pushinteger(L, p.first);
      return 1;
    }
  }
  lua_pushinteger(L, 0);
  return 1;
}
// with parameter session_id in lua
int get_index_of_session(lua_State *L) {
  lua_Integer id = lua_tointeger(L, 1);
  lua_Integer idx = 0;
  for (auto &p : sessions_map) {
    if (p.second == id) {
      idx = p.first;
      break;
    }
  }
  lua_pushinteger(L, idx);
  return 1;
}

// with parameter session_id in lua
int commit_composition_sid(lua_State *L) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    lua_pushboolean(L, false);
    return 1;
  }
  lua_Integer sid = lua_tointeger(L, 1);
  auto ret = rime->commit_composition((RimeSessionId)sid);
  lua_pushboolean(L, ret);
  return 1;
}
// with parameter session_id in lua
int clear_composition_sid(lua_State *L) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return 0;
  }
  lua_Integer sid = lua_tointeger(L, 1);
  rime->clear_composition((RimeSessionId)sid);
  return 0;
}

// with parameter session index in lua, or without parameters for
// current_session
int commit_composition(lua_State *L) {
  bool ret = false;
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
  } else {
    int n = lua_gettop(L);
    if (!n)
      ret = rime->commit_composition(current_session);
    else {
      int idx = (int)lua_tointeger(L, 1);
      auto id = get_session_c((int)idx);
      if (id)
        ret = rime->commit_composition((RimeSessionId)id);
      else
        printf("Error: specific index %d doesn't match any session.\n", idx);
    }
  }
  lua_pushboolean(L, ret);
  return 1;
}
// with parameter session index in lua, or without parameters for
// current_session
int clear_composition(lua_State *L) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
  } else {
    int n = lua_gettop(L);
    if (!n)
      rime->clear_composition(current_session);
    else {
      int idx = (int)lua_tointeger(L, 1);
      auto id = get_session_c((int)idx);
      if (id)
        rime->clear_composition((RimeSessionId)id);
      else
        printf("Error: specific index %d doesn't match any session.\n", idx);
    }
  }
  return 0;
}

// std::vector<std::string> to Lua table
int vectorStringToLua(lua_State *L, const std::vector<std::string> &vec) {
  lua_newtable(L);
  for (size_t i = 0; i < vec.size(); ++i) {
    lua_pushinteger(L, i + 1);         // Lua table starts from 1
    lua_pushstring(L, vec[i].c_str()); //  string to Lua
    lua_settable(L, -3);               // table[i+1] = vec[i]
  }
  return 1;
}

int get_candidates(lua_State *L) {
  std::vector<std::string> ret;
  RIME_STRUCT(RimeContext, context);
  if (rime->get_context(current_session, &context)) {
    if (context.menu.num_candidates) {
      for (int i = 0; i < context.menu.num_candidates; ++i) {
        ret.push_back(context.menu.candidates[i].text);
      }
    }
    rime->free_context(&context);
  }
  return vectorStringToLua(L, ret);
}
int get_comments(lua_State *L) {
  std::vector<std::string> ret;
  RIME_STRUCT(RimeContext, context);
  if (rime->get_context(current_session, &context)) {
    if (context.menu.num_candidates) {
      for (int i = 0; i < context.menu.num_candidates; ++i) {
        ret.push_back(context.menu.candidates[i].comment
                          ? context.menu.candidates[i].comment
                          : "");
      }
    }
    rime->free_context(&context);
  }
  return vectorStringToLua(L, ret);
}

int synchronize(lua_State *L) {
  auto ret = rime->sync_user_data();
  lua_pushboolean(L, ret);
  return 1;
}

int export_user_dict(lua_State *L) {
  const char *dict_name = lua_tostring(L, 1);
  const char *file_path = lua_tostring(L, 2);
  auto path = fs::absolute(fs::path(file_path));
  auto api = (RimeLeversApi *)rime_get_api()->find_module("levers")->get_api();
  auto ret = api->export_user_dict(dict_name, path.u8string().c_str());
  lua_pushboolean(L, ret);
  return 1;
}

int import_user_dict(lua_State *L) {
  const char *dict_name = lua_tostring(L, 1);
  const char *file_path = lua_tostring(L, 2);
  auto path = fs::absolute(fs::path(file_path));
  auto api = (RimeLeversApi *)rime_get_api()->find_module("levers")->get_api();
  auto ret = api->import_user_dict(dict_name, path.u8string().c_str());
  lua_pushboolean(L, ret);
  return 1;
}

int restore_user_dict(lua_State *L) {
  const char *file_path = lua_tostring(L, 1);
  auto path = fs::absolute(fs::path(file_path));
  auto api = (RimeLeversApi *)rime_get_api()->find_module("levers")->get_api();
  auto ret = api->restore_user_dict(path.u8string().c_str());
  lua_pushboolean(L, ret);
  return 1;
}

int backup_user_dict(lua_State *L) {
#ifdef _WIN32
  char dir[MAX_PATH] = {0};
#else
  char dir[PATH_MAX] = {0};
#endif
  rime_get_api()->get_user_data_sync_dir(dir, sizeof(dir) / sizeof(char));
  try {
    if (!fs::exists(fs::path(dir)))
      fs::create_directories(fs::path(dir));
  } catch (const fs::filesystem_error &e) {
    std::cerr << "exception when creating directory: " << dir << ", "
              << e.what() << std::endl;
  }
  const char *dict_name = lua_tostring(L, 1);
  auto path = std::string(dir) + ".userdb.txt";
  auto api = (RimeLeversApi *)rime_get_api()->find_module("levers")->get_api();
  auto ret = api->backup_user_dict(dict_name);
  lua_pushboolean(L, ret);
  return 1;
}

int get_schema_id_list(lua_State *L) {
  std::vector<std::string> ret;
  RimeSchemaList list;
  if (rime->get_schema_list(&list)) {
    for (size_t i = 0; i < list.size; ++i)
      ret.push_back(list.list[i].schema_id);
    rime->free_schema_list(&list);
  }
  return vectorStringToLua(L, ret);
}

int get_schema_name_list(lua_State *L) {
  std::vector<std::string> ret;
  RimeSchemaList list;
  if (rime->get_schema_list(&list)) {
    for (size_t i = 0; i < list.size; ++i)
      ret.push_back(list.list[i].name);
    rime->free_schema_list(&list);
  }
  return vectorStringToLua(L, ret);
}

int get_current_schema(lua_State *L) {
  char current[100] = {0};
  rime->get_current_schema(current_session, current, sizeof(current));
  lua_pushstring(L, current);
  return 1;
}

int get_current_session(lua_State *L) {
  lua_pushinteger(L, (lua_Integer)current_session);
  return 1;
}
}

void register_c_functions(lua_State *L) {
  lua_newtable(L);
  lua_setglobal(L, "rimeac");
  lua_getglobal(L, "rimeac");
#define REG_FUNC(L, func, name)                                                \
  lua_pushcfunction(L, &func);                                                 \
  lua_setfield(L, -2, name)
  // 注册每个 C 函数到 Lua 表中
  REG_FUNC(L, setup_rime, "setup_rime");
  REG_FUNC(L, init_rime, "init_rime");
  REG_FUNC(L, synchronize, "synchronize");
  REG_FUNC(L, finalize_rime, "finalize_rime");
  REG_FUNC(L, initialize, "initialize");
  REG_FUNC(L, finalize_rime, "finalize");
  REG_FUNC(L, export_user_dict, "export_user_dict");
  REG_FUNC(L, import_user_dict, "import_user_dict");
  REG_FUNC(L, backup_user_dict, "backup_user_dict");
  REG_FUNC(L, restore_user_dict, "restore_user_dict");
  REG_FUNC(L, destroy_sessions, "destroy_sessions");
  REG_FUNC(L, print_sessions, "print_sessions");
  REG_FUNC(L, add_session, "add_session");
  REG_FUNC(L, kill_session, "kill_session");
  REG_FUNC(L, get_session, "get_session");
  REG_FUNC(L, switch_session, "switch_session");
  REG_FUNC(L, commit_composition_sid, "commit_composition_sid");
  REG_FUNC(L, clear_composition_sid, "clear_composition_sid");
  REG_FUNC(L, commit_composition, "commit_composition");
  REG_FUNC(L, clear_composition, "clear_composition");
  REG_FUNC(L, get_schema_id_list, "get_schema_id_list");
  REG_FUNC(L, get_schema_name_list, "get_schema_name_list");
  REG_FUNC(L, get_current_schema, "get_current_schema");
  REG_FUNC(L, get_candidates, "get_candidates");
  REG_FUNC(L, get_comments, "get_comments");
  REG_FUNC(L, print_session, "print_session");
  REG_FUNC(L, simulate_keys, "simulate_keys");
  REG_FUNC(L, select_schema, "select_schema");
  REG_FUNC(L, select_candidate, "select_candidate");
  REG_FUNC(L, delete_candidate, "delete_candidate");
  REG_FUNC(L, delete_candidate_on_current_page,
           "delete_candidate_on_current_page");
  REG_FUNC(L, set_option, "set_option");
  REG_FUNC(L, get_option, "get_option");
  REG_FUNC(L, get_index_of_current_session, "get_index_of_current_session");
  REG_FUNC(L, get_index_of_session, "get_index_of_session");
  REG_FUNC(L, get_state_label, "get_state_label");
  lua_pushstring(L, "current_session");
  lua_pushinteger(L, (lua_Integer)current_session);
  lua_settable(L, -3);
#ifdef MODULE
  REG_FUNC(L, init_env, "init_env");
  REG_FUNC(L, finalize_env, "finalize_env");
#endif
#undef REG_FUNC
  lua_pop(L, 1);
}

extern "C" {
#ifdef MODULE
LIBRIMEAC_API int luaopen_librimeac(lua_State *L) {
  l = L;
  register_c_functions(l);
#else
int main(int argc, char *argv[]) {
  bool input = false;
  if (argc == 2) {
    if (fs::exists(fs::path(argv[1])))
      input = true;
    else {
      std::cout << "file " << argv[1] << " does not exists!\n";
      return 1;
    }
  }
  l = luaL_newstate();
  init_env(l);
  luaL_openlibs(l);

  register_c_functions(l);
  if (luaL_dofile(l, input ? argv[1] : "script.lua")) {
    const char *error_msg = lua_tostring(l, -1);
    printf("Error: %s\n", error_msg);
    lua_pop(l, -1);
  }
  finalize_env(l);
#endif /* MODULE */
  return 0;
}
}
