local api = vim.api
local fmt = string.format

local M = {}

if not jit then error('no jit found') end


--- DESC: compiles parser.c/h files into a shared library
local build_parser = function(lang)
  assert(type(lang) == 'string', fmt('"lang" has to be a string, got %q', type(lang)))
-- TODO: mention the github pull-request where we took this 
  -- https://github.com/nvim-lualine/lualine.nvim/compare/c5bd65d...c3fad10
  local sep = package.config:sub(1, 1)
  local tags_path = debug.getinfo(1, "S").short_src:gsub('/[^/]+$', '') -- TODO: better name

  local lib_path = table.concat({tags_path, 'parsers', lang}, sep)
  local _lib_files_raw = api.nvim_exec2(fmt('!ls -p %s | grep -v / ', lib_path), {output = true}).output
  local lib_files = _lib_files_raw
                    :gsub('\n[^%.]+.so', '') -- ignore .so files
                    :gsub('%:[^\n]+\n+', '') -- ignore command 
                    :gsub('\n', ' ') 
                    :gsub('([^%s]+)', lib_path .. '/%1') -- attach lib_path to file-names

  local lib_name = fmt('lib%sparser.so', lang)
  local lib_target = fmt('%s/%s', lib_path, lib_name)
  local cflags = '-fpic -shared -rdynamic -Wall -Wextra -Werror -Wpedantic -std=c99'

  local cmd_build = fmt('!cc -o %s %s  %s', lib_target, lib_files, cflags)
  local result = api.nvim_exec2(cmd_build, {output = true}).output
  if result:find('shell returned %d+') then -- TODO: does this handle all kinds of failure?
    error('\n\n' .. result)
  end

  return lib_target
end



--- DESC: load parser library through ffi 
local load_parser = function()
  local libso_path = build_parser('c')

  local ffi = require('ffi')
  local ok, lib = pcall(ffi.load, libso_path)

  if not ok then
    error('failed to load lib') -- TODO: better diagnostics
  end

  -- TODO: it would be cool to pass the io.read('parser.h') output,
  -- cleaned up if possible since ffi has no preprocessor 
  -- and lacks other features as well
  ffi.cdef[[
    void parse(const char* src, uint32_t src_len);
  ]]

  return ffi, lib
end



local file = [[
  int main() {
  }
]]
local file_len = file:len() + 1 -- TODO: better name


local FFI, LIB = load_parser() 


local _cfile = FFI.new('char[?]', file_len) -- TODO: better name
FFI.copy(_cfile, file)
local _cfile_len = FFI.new('uint32_t[1]', file_len ) -- TODO: better name wtf

LIB.parse(_cfile, _cfile_len[0])


return M
