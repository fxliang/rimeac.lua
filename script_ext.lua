--- add the so/dll/lua files in cwd to package.cpath
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
package.path = package.path .. ';' .. script_path() .. div .. '?.lua'
-------------------------------------------------------------------------------
require("librimeac")
-- do the jobs in script.lua
require("script")
