local api = vim.api
local fmt = string.format

local M = {}

if not jit then error('no jit found') end


local build_parser = function(lang)
  assert(type(lang) == 'string', fmt('"lang" has to be a string, got %q', type(lang)))
-- TODO: mention the github pull-request where we took this 
  -- https://github.com/nvim-lualine/lualine.nvim/compare/c5bd65d...c3fad10
  local sep = package.config:sub(1, 1)
  local tags_folder_path = debug.getinfo(1, "S").source

  if tags_folder_path:sub(1, 1) ~= '@' then
    error('tags.nvim: failed to retrieve project folder path')
  end

  tags_folder_path = tags_folder_path:sub(2, -1):gsub('/[^/]+$', '')

  local lib_path = table.concat({tags_folder_path, 'parsers', lang}, sep)
  local _lib_files_raw = api.nvim_exec2(fmt('!ls -p %s | grep -v / ', lib_path), {output = true}).output
  local lib_files = _lib_files_raw
                    :gsub('\n[^%.]+.so', '') -- ignore .so files
                    :gsub('%:[^\n]+\n+', '') -- ignore command 
                    :gsub('\n', ' ') 
                    :gsub('([^%s]+)', lib_path .. '/%1') -- attach lib_path to file-names

  local lib_name = fmt('lib%sparser.so', lang)
  local lib_target = fmt('%s/%s', lib_path, lib_name)
  local cflags = '-fpic -shared -rdynamic ' .. 
                 '-Wall -Wextra -Werror -Wpedantic' .. 
                 ' -Wno-unused -Wno-comment ' ..
                 '-std=c99 '
                 -- .. '-fsanitize=address'

  local cmd_build = fmt('!cc -o %s %s  %s', lib_target, lib_files, cflags)
  local result = api.nvim_exec2(cmd_build, {output = true}).output
  if result:find('shell returned %d+') then -- TODO: does this handle all kinds of failure?
    error('\n\n' .. 'tags.nvim:\n' .. result)
  end

  return lib_target
end



local load_parser = function()
  local libso_path = build_parser('c')

  local ffi = require('ffi')
  local ok, lib = pcall(ffi.load, libso_path)

  if not ok then
    error('failed to load lib') -- TODO: better diagnostics
  end

  -- TODO: it would be cool to pass the 
  -- io.read('parser.h') output, cleaned 
  -- up if possible since ffi has no preprocessor.
  -- Also if we change something, for example 
  -- the order of enum-items which we may depend on,
  -- it could get hard to debug.
  ffi.cdef[[
    typedef enum {
      TK_COMMA, TK_IDENT,
      TK_IF, TK_ELSE, TK_WHILE,
      TK_FOR, TK_RETURN, TK_BREAK, TK_CASE,
      TK_OPEN_PAR, TK_CLOSE_PAR,
      TK_OPEN_SBRACKET, TK_CLOSE_SBRACKET,
      TK_OPEN_CBRACKET, TK_CLOSE_CBRACKET,
      TK_EOF, TK_UNKNOWN 
    } TokenType;

    typedef struct {
      TokenType type;
      uint32_t offset; 
      uint32_t len;
    } Token;

    typedef struct {
      Token* items;
      size_t len;
      size_t cap;
    } Tokens;

    void free(void *ptr);
    bool parser_main(const char* src, uint32_t src_len, 
                     Tokens* tokens, char* lua_err_msg, uint32_t lua_err_msg_len);
  ]]

  return ffi, lib
end


-- TODO: load it in a autocmd
local FFI, LIB = load_parser() 


local create_tags = function()
  local buftext = table.concat(api.nvim_buf_get_lines(0, 0, -1, false), '\n')
  local buflen = buftext:len()

  local _cbuflen = FFI.new('uint32_t[1]', buflen + 1)
  local _cbuftext = FFI.new('char[?]', buflen + 1)
  FFI.copy(_cbuftext, buftext)

  local tokens = FFI.new('Tokens', {})
  local err_msg_len = 256
  local err_msg = FFI.new('char[?]', err_msg_len, {})
  local failed = LIB.parser_main(_cbuftext, _cbuflen[0], tokens, err_msg, err_msg_len)

  if failed then
    error(FFI.string(err_msg))
  end

  local tags = {}

  for i=0, tonumber(tokens.len) - 1 do 
    local t = tokens.items[i]
    local func_name = buftext:sub(t.offset + 1, t.offset + t.len)
    tags[func_name] = t.offset + 1
  end

  LIB.free(tokens.items)
  return tags
end



M.jump = function()
  local tags = create_tags()
  local word_at_curs = vim.fn.expand('<cword>')

  if tags[word_at_curs] == nil then
    vim.notify(fmt("no tags found for %q", word_at_curs), 
      vim.log.levels.ERROR)
    return 
  end

  vim.cmd(fmt('%dgo', tags[word_at_curs]))
end



return M
