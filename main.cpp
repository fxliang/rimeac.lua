extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#ifndef XMAKE_REPO
#include <LuaBridge/LuaBridge.h>
#else
#include <luabridge3/LuaBridge/LuaBridge.h>
#endif
#include <filesystem>
#include <rime_api.h>
#include <rime_levers_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
inline unsigned int SetConsoleOutputCodePage(unsigned int codepage = CP_UTF8) {
  unsigned int cp = GetConsoleOutputCP();
  SetConsoleOutputCP(codepage);
  return cp;
}
#else
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
void print_session() {
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

inline void init_env() { codepage = SetConsoleOutputCodePage(); }
inline void finalize_lua() { lua_close(l); }
inline void finalize_env() {
#ifndef MODULE
  finalize_lua();
#endif
  if (!rime)
    rime = rime_get_api();
  rime->finalize();
  SetConsoleOutputCodePage(codepage);
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
void finalize_rime() {
  if (!rime)
    rime = rime_get_api();
  rime->finalize();
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
  int index = lua_tointeger(L, 1);
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    lua_pushboolean(L, false);
    return 1;
  }
  auto ret = (index > 0) && rime->select_candidate_on_current_page(
                                current_session, (index - 1));
  lua_pushboolean(L, ret);
  return 1;
}
void print_sessions() {
  printf("current sessions list:\n");
  for (const auto &p : sessions_map) {
    char schema_id[256] = {0};
    rime->get_current_schema(p.second, schema_id, sizeof(schema_id));
    char mk = (p.second == current_session) ? '>' : ' ';
    printf("%c %d. session_id: %p, schema_id: %s\n", mk, p.first,
           (void *)p.second, schema_id);
  }
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
void destroy_sessions() {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return;
  }
  for (const auto &s : sessions_map) {
    printf("destroy session: %p\n", (void *)s.second);
    rime->destroy_session(s.second);
  }
  sessions_map.clear();
}
int kill_session(lua_State *L) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return 0;
  }
  auto id = lua_tointeger(L, 1);
  auto it = sessions_map.find(id);
  if (it == sessions_map.end()) {
    fprintf(stderr, "no session by this index!\n");
    return 0;
  }
  if (it != sessions_map.begin())
    it--;
  else
    it++;
  printf("destroy session: %p\n", (void *)sessions_map[id]);
  rime->destroy_session(sessions_map[id]);
  sessions_map.erase(id);
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
  int index = lua_tointeger(L, 1);
  RimeSessionId id = get_session_c(index);
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
  auto ret = rime->commit_composition(sid);
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
  rime->clear_composition(sid);
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

int get_current_session(lua_State *L) {
  lua_pushinteger(L, (lua_Integer)current_session);
  return 1;
}

// for rimeac.lua
#ifndef MODULE
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
  init_env();
  // --------------------------------------------------------------------------
  l = luaL_newstate();
  luaL_openlibs(l);

#else // for librimeac.so
extern "C" {
#ifdef _WIN32
__declspec(dllexport)
#endif
int luaopen_librimeac(lua_State* L) {
  l = L;
#endif /* MODULE */

  luabridge::getGlobalNamespace(l)
      .beginNamespace("rimeac")
      .addFunction("setup_rime", &setup_rime)
      .addFunction("init_rime", &init_rime)
      .addFunction("finalize_rime", &finalize_rime)
      // handle sessions without params
      .addFunction("destroy_sessions", &destroy_sessions)
      .addFunction("print_sessions", &print_sessions)
      .addFunction("add_session", &add_session)
      // param with session index start from 1
      .addFunction("kill_session", &kill_session)
      .addFunction("get_session", &get_session)
      .addFunction("switch_session", &switch_session)
      // with one parameter: session id
      .addFunction("commit_composition_sid", &commit_composition_sid)
      .addFunction("clear_composition_sid", &clear_composition_sid)
      // on current session if no parameters, or with one parameter: session
      // index
      .addFunction("commit_composition", &commit_composition)
      .addFunction("clear_composition", &clear_composition)
      // on current session
      .addFunction("get_candidates", &get_candidates)
      .addFunction("get_comments", &get_comments)
      .addFunction("print_session", &print_session)
      .addFunction("simulate_keys", &simulate_keys)
      .addFunction("select_schema", &select_schema)
      .addFunction("select_candidate", &select_candidate)
      .addFunction("set_option", &set_option)
      .addFunction("get_option", &get_option)
      .addFunction("get_index_of_current_session",
                   &get_index_of_current_session)
      .addFunction("get_index_of_session", &get_index_of_session)
      .addFunction("get_state_label", &get_state_label)
      .addProperty("current_session", &get_current_session)

// for librimeac.so
#ifdef MODULE
      .addFunction("init_env", &init_env)
      .addFunction("finalize_env", &finalize_env)
#endif
      .endNamespace();
  // for rimeac.lua, load script file
#ifndef MODULE
  // --------------------------------------------------------------------------
  int st = luaL_dofile(l, input ? argv[1] : "script.lua");
  if (st) {
    const char *error_msg = lua_tostring(l, -1);
    printf("Error: %s\n", error_msg);
    lua_pop(l, -1);
  }
  // --------------------------------------------------------------------------
  finalize_env();
#endif /* MODULE */

  return 0;
}
// for librimeac.so, end of extern C
#ifdef MODULE
}
#endif /* extern C */
