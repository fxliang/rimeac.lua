#ifndef _WIN32
#include <lua5.4/lua.hpp>
#else
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#endif

#include <LuaBridge/LuaBridge.h>
#include <rime_api.h>
#include <rime_levers_api.h>
#include <filesystem>
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
lua_State* L;
RimeApi* rime;
SessionsMap sessions_map;
RimeSessionId current_session;

void print_status(RimeStatus* status) {
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
void print_composition(RimeComposition* composition) {
  const char* preedit = composition->preedit;
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
void print_menu(RimeMenu* menu) {
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
void print_context(RimeContext* context) {
  if (context->composition.length > 0 || context->menu.num_candidates > 0) {
    print_composition(&context->composition);
  } else {
    printf("(not composing)\n");
  }
  print_menu(&context->menu);
}
void print_session() {
  RimeApi* rime = rime_get_api();
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
void on_message(void* context_object,
                RimeSessionId session_id,
                const char* message_type,
                const char* message_value) {
  printf("message: [%p] [%s] %s\n", (void*)session_id, message_type, message_value);
  RimeApi* rime = rime_get_api();
  if (RIME_API_AVAILABLE(rime, get_state_label) &&
      !strcmp(message_type, "option")) {
    Bool state = message_value[0] != '!';
    const char* option_name = message_value + !state;
    const char* state_label =
        rime->get_state_label(session_id, option_name, state);
    if (state_label) {
      printf("updated option: %s = %d // %s\n", option_name, state,
             state_label);
    }
  }
}

inline void init_env() {
  L = luaL_newstate();
  luaL_openlibs(L);
  codepage = SetConsoleOutputCodePage();
}
inline void finalize_lua(){
  lua_close(L);
}
inline void finalize_env(){
  finalize_lua();
  if (!rime)
    rime = rime_get_api();
  rime->finalize();
  SetConsoleOutputCodePage(codepage);
}
void setup_rime(const char* app_name, const char* shared, const char* usr,
    const char* log) {
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
}
void init_rime() {
  fprintf(stderr, "initializing...\n");
  if (!rime)
    rime = rime_get_api();
  rime->initialize(NULL);
  if (rime->start_maintenance(True))
    rime->join_maintenance_thread();
  fprintf(stderr, "ready.\n");
}
void finalize_rime() {
  if (!rime)
    rime = rime_get_api();
  rime->finalize();
}
bool simulate_keys(const char* keys) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return false;
  }
  //printf("simulate keys on session: %p, %s\n", (void*)current_session, keys);
  return rime->simulate_key_sequence(current_session, keys);
}
bool select_schema(const char* schema_id) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return false;
  }
  return rime->select_schema(current_session, schema_id);
}
void set_option(const char* option_name, bool value) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return;
  }
  rime->set_option(current_session, option_name, value);
}
bool get_option(const char* option_name) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return false;
  }
  return rime->get_option(current_session, option_name);
}
bool select_candidate(int index) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return false;
  }
  return (index > 0 &&
      rime->select_candidate_on_current_page(current_session, (index-1)));
}
void print_sessions() {
  printf("current sessions list:\n");
  for (const auto& p : sessions_map) {
    char schema_id[256] = {0};
    rime->get_current_schema(p.second, schema_id, sizeof(schema_id));
    char mk = (p.second == current_session) ? '>' : ' ';
    printf("%c %d. session_id: %p, schema_id: %s\n", mk, p.first, (void*)p.second,
        schema_id);
  }
}
bool add_session() {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return false;
  }
  RimeSessionId id = rime->create_session();
  if (!id) {
    fprintf(stderr, "Error creating new rime session.\n");
    return false;
  }
  if (sessions_map.size())
    sessions_map[sessions_map.rbegin()->first + 1] = id;
  else
    sessions_map[1] = id;
  current_session = id;
  return true;
}
void destroy_sessions() {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return;
  }
  for(const auto& s : sessions_map) {
    printf("destroy session: %p\n", (void*)s.second);
    rime->destroy_session(s.second);
  }
  sessions_map.clear();
}
void kill_session(int id) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return;
  }
  auto it = sessions_map.find(id);
  if (it == sessions_map.end()) {
    fprintf(stderr, "no session by this index!\n");
    return;
  }
  if (it != sessions_map.begin())
    it--;
  else
    it++;
  printf("destroy session: %p\n", (void*)sessions_map[id]);
  rime->destroy_session(sessions_map[id]);
  sessions_map.erase(id);
  current_session = it->second;
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
int get_session(lua_State* L) {
  lua_Integer index = luaL_checkinteger(L, 1);
  auto id = get_session_c((int)index);
  lua_pushinteger(L, (lua_Integer)id);
  return 1;
}
bool switch_session(int index) {
  RimeSessionId id = get_session_c(index);
  if (!id)
    return false;
  else
    current_session = id;
  return true;
}
int get_index_of_current_session() {
  for(auto& p : sessions_map) {
    if(p.second == current_session)
      return p.first;
  }
  return 0;
}
// with parameter session_id in lua
int get_index_of_session(lua_State* L) {
  lua_Integer id = luaL_checkinteger(L, 1);
  lua_Integer idx = 0;
  for(auto& p : sessions_map) {
    if(p.second == id) {
      idx = p.first;
      break;
    }   
  }
  lua_pushinteger(L, idx);
  return 1;
}

// with parameter session_id in lua
int commit_composition_sid(lua_State* L) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return false;
  }
  lua_Integer sid = luaL_checkinteger(L, 1);
  auto ret = rime->commit_composition(sid);
  lua_pushboolean(L, ret);
  return 1;
}
// with parameter session_id in lua
int clear_composition_sid(lua_State* L) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
    return 0;
  }
  lua_Integer sid = luaL_checkinteger(L, 1);
  rime->clear_composition(sid);
  return 0;
}

// with parameter session index in lua, or without parameters for current_session
int commit_composition(lua_State* L) {
  bool ret = false;
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
  } else {
    int n = lua_gettop(L);
    if (!n)
      ret = rime->commit_composition(current_session);
    else {
      int idx = (int)luaL_checkinteger(L, 1);
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
// with parameter session index in lua, or without parameters for current_session
int clear_composition(lua_State* L) {
  if (!rime) {
    fprintf(stderr, "Please init rime first!\n");
  } else {
    int n = lua_gettop(L);
    if (!n)
      rime->clear_composition(current_session);
    else {
      int idx = (int)luaL_checkinteger(L, 1);
      auto id = get_session_c((int)idx);
      if (id)
        rime->clear_composition((RimeSessionId)id);
      else
        printf("Error: specific index %d doesn't match any session.\n", idx);
    }
  }
  return 0;
}

std::vector<std::string> get_candidates() {
  std::vector<std::string> ret;
  RIME_STRUCT(RimeContext, context);
  if (rime->get_context(current_session, &context)) {
    if(context.menu.num_candidates) {
      for (int i = 0; i < context.menu.num_candidates; ++i) {
        ret.push_back(context.menu.candidates[i].text);
      }
    }
    rime->free_context(&context);
  }
  return ret;
}
std::vector<std::string> get_comments() {
  std::vector<std::string> ret;
  RIME_STRUCT(RimeContext, context);
  if (rime->get_context(current_session, &context)) {
    if(context.menu.num_candidates) {
      for (int i = 0; i < context.menu.num_candidates; ++i) {
        ret.push_back(context.menu.candidates[i].comment ?
            context.menu.candidates[i].comment : "");
      }
    }
    rime->free_context(&context);
  }
  return ret;
}

int main(int argc, char* argv[]){
  //printf("hello world in c++!\n");
  init_env();
  // --------------------------------------------------------------------------
  luabridge::getGlobalNamespace(L)
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
    // on current session if no parameters, or with one parameter: session index
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
    .addFunction("get_index_of_current_session", &get_index_of_current_session)
    .addFunction("get_index_of_session", &get_index_of_session)
    .endNamespace();
  // --------------------------------------------------------------------------
  int st = luaL_dofile(L, "script.lua");
  if (st) {
    const char* error_msg = lua_tostring(L, -1);
    printf("Error: %s\n", error_msg);
    lua_pop(L, -1);
  }
  // --------------------------------------------------------------------------
  finalize_env();
  return 0;
}
