#ifndef CPARSER_H_
#define CPARSER_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>



typedef enum {
  TK_COMMA,
  TK_IDENT,

  TK_IF,
  TK_ELSE,
  TK_WHILE,
  TK_FOR,
  TK_RETURN,
  TK_BREAK,
  TK_CASE,

  TK_OPEN_PAR,
  TK_CLOSE_PAR,

  TK_OPEN_SBRACKET,
  TK_CLOSE_SBRACKET,

  TK_OPEN_CBRACKET,
  TK_CLOSE_CBRACKET,

  TK_EOF,
  TK_UNKNOWN, 
} TokenType;

// NOTE: about TK_UNKNOWN, we do not handle syntax errors, so  
// any typo in a token will be treated as a valid one.
// If we meet one we just fail the parsing rule.

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


typedef struct {
  Token curr;
  Token next;
  const char* src;
  uint32_t src_offset;
  uint32_t src_len;
} Parser;



/*
 * NOTE: in this program a failure 
 * (i.e. global 'err_msg' was set up + return false)
 * is a *program* failure which shuold 
 * only be caused by realloc; nothing 
 * else should cause it to break.
 * If no declaration is found in a file, 
 * the program is still considered successful.
*/ 

// NOTE: 'tokens' and 'err_msg' are declared in 'lua/tags/init.lua'
bool parser_main(const char* src, uint32_t src_len, 
                 Tokens* tokens, 
                 char* lua_err_msg, uint32_t lua_err_msg_len);
// NOTE: these here are declared for 
// documentation purposes, they don't need and 
// won't be imported through the ffi
static bool parse(Parser* p, Tokens* tokens);
static void advance(Parser* p);
static Token next_token(Parser* p);
static bool parse_func_decl(Parser* p, Token* name);
static bool expect(Parser* p, TokenType expected);
static void print_token(Token tk);


#endif
