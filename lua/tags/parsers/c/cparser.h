#ifndef CPARSER_H_
#define CPARSER_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>



typedef enum {
  TK_IDENT,
  TK_COMMA,

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
  TK_UNKNOWN 
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
  Token curr;
  Token next;
  const char* src;
  uint32_t src_offset;
  uint32_t src_len;
} Parser;


void parse(const char* src, uint32_t src_len);
// NOTE: these here are declared for 
// documentation purposes, they don't need and 
// won't be imported through the ffi
static void advance(Parser* p);
static Token next_token(Parser* p);
static void print_token(Parser* p, Token* tk);


#endif
