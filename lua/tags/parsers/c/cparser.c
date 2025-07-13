#include "./cparser.h"

/*
 * NOTE: we keep the macros here instead 
 * of putting them into the header file because 
 * soon I would like to directly read the 
 * '*parser.h' files-content into 'ffi.cdef', 
 * as it is error prone to pass declarations manually.
 *
 * Unfortunately as of now (07/25) 'ffi' doesn't 
 * support any preprocessing and pre-processor 
 * tokens are not allowed:
 * https://luajit.org/ext_ffi_api.html
 * under 'ffi.cdef(def)'
 *
 * TODO: when we will read the files to ffi.cdef we 
 * have to clean up the comments as well 
 *
*/



// NOTE: gl -> global  
char gl_err_msg[256] = {0};


#define return_defer(val) do {  \
  failed = (val);  \
  goto defer; \
} while(0)


#define error(...) do { \
  int len = snprintf(gl_err_msg, sizeof(gl_err_msg), \
                      "\n[%s:%d]: ", __FILE__, __LINE__); \
  snprintf(gl_err_msg + len, sizeof(gl_err_msg), __VA_ARGS__); \
  return_defer(1); \
} while (0);


#define da_append(da, elem, da_init_cap, da_type) do { \
  if ((da)->len + 1 >= (da)->cap) { \
    (da)->cap = (da)->cap == 0 ? (da_init_cap) : (da)->cap * 2; \
    void* tmp = realloc((da)->items, (da)->cap * sizeof(*(da)->items)); \
    if (tmp == NULL) { \
      error("not enough memory for '" #da_type "' dynamic array"); \
    } \
    (da)->items = tmp; \
  } \
  (da)->items[(da)->len++] = (elem); \
} while (0)

#define DA_TOKENS_INIT_CAP  64





static void print_token(Token tk)
{
  switch(tk.type) {
    case TK_IDENT: printf("TK_IDENT: "); break; 
    case TK_COMMA: printf("TK_COMMA: "); break; 
    case TK_IF: printf("TK_IF: "); break; 
    case TK_ELSE: printf("TK_ELSE: "); break; 
    case TK_WHILE: printf("TK_WHILE: "); break; 
    case TK_FOR: printf("TK_FOR: "); break; 
    case TK_RETURN: printf("TK_RETURN: "); break; 
    case TK_BREAK: printf("TK_BREAK: "); break; 
    case TK_CASE: printf("TK_CASE: "); break; 
    case TK_OPEN_PAR: printf("TK_OPEN_PAR: "); break; 
    case TK_CLOSE_PAR: printf("TK_CLOSE_PAR: "); break; 
    case TK_OPEN_SBRACKET: printf("TK_OPEN_SBRACKET: "); break; 
    case TK_CLOSE_SBRACKET: printf("TK_CLOSE_SBRACKET: "); break; 
    case TK_OPEN_CBRACKET: printf("TK_OPEN_CBRACKET: "); break; 
    case TK_CLOSE_CBRACKET: printf("TK_CLOSE_CBRACKET: "); break; 
    case TK_EOF: printf("TK_EOF: "); break; 
    case TK_UNKNOWN: printf("TK_UNKNOWN: "); break; 
  }
  printf("\n");
}



static Token next_token(Parser* p)
{
  while (p->src_offset + 1 <= p->src_len && 
        isspace(p->src[p->src_offset])) {
    p->src_offset++;
  }

  char c = p->src[p->src_offset];

  Token t = (Token){
    .offset = p->src_offset++,
    .len = 1,
  }; 

  switch (c) {
    case '\0': { t.type = TK_EOF; break; } 
    case '(':  { t.type = TK_OPEN_PAR; break; } 
    case ',':  { t.type = TK_COMMA; break; } 
    case ')':  { t.type = TK_CLOSE_PAR; break; } 
    case '{':  { t.type = TK_OPEN_CBRACKET; break; } 
    case '}':  { t.type = TK_CLOSE_CBRACKET; break; } 
    case '[':  { t.type = TK_OPEN_SBRACKET; break; } 
    case ']':  { t.type = TK_CLOSE_SBRACKET; break; } 
    default:   {
      if (!isalpha(c)) { t.type = TK_UNKNOWN; break; }
      
#define match_kword(kw_rest, kw_rest_len, tk_type) do { \
  if (p->src_offset + (kw_rest_len) <= p->src_len \
      && 0 == memcmp(&p->src[p->src_offset], kw_rest, (kw_rest_len))) { \
    t.type = (tk_type); \
    t.len += (kw_rest_len); \
    p->src_offset += (kw_rest_len); \
    goto end; \
  } \
} while (0)

      switch(c) {
        case 'i': { match_kword("f", 1, TK_IF); break; }
        case 'e': { match_kword("lse", 3, TK_ELSE); break; }
        case 'w': { match_kword("hile", 4, TK_WHILE); break; }
        case 'f': { match_kword("or", 2, TK_FOR); break; }
        case 'r': { match_kword("eturn", 5, TK_RETURN); break; } 
        case 'b': { match_kword("reak", 4, TK_BREAK); break; }
        case 'c': { match_kword("ase", 3, TK_CASE); break; }
      }
#undef is_kword
      
      for (size_t i = p->src_offset; i < p->src_len; i++) {
        if (p->src[i] != '_' && !isalnum(p->src[i])) {
          t.type = TK_IDENT;
          p->src_offset = i;
          break;
        }
        t.len++;
      }
    }
  }

end:
  return t;
}


static void advance(Parser* p) 
{
  p->curr = p->next;

  if (p->curr.type != TK_EOF)
    p->next = next_token(p);
}


static bool expect(Parser* p, TokenType expected)
{
  if (p->next.type != expected)
    return false;

  advance(p);
  return true;
}


// NOTE: failure (return false) here is NOT a *program* failure, 
// it just means the parsed statement is NOT a function declaration
static bool parse_func_decl(Parser* p, Token* name)
{
  // type 
  if (!expect(p, TK_IDENT)) 
    return false; 

  // function name
  if (!expect(p, TK_IDENT)) 
    return false;
  *name = p->curr;

  if (!expect(p, TK_OPEN_PAR)) 
    return false;

  // parameters
  while (p->curr.type != TK_CLOSE_PAR) { 
    advance(p);
    if (p->curr.type == TK_EOF) 
      return false;
  }

  if (!expect(p, TK_OPEN_CBRACKET)) 
    return false;

  // function body
  while (p->curr.type != TK_CLOSE_CBRACKET) { 
    advance(p);
    if (p->curr.type == TK_EOF) 
      return false;
  }

  return true;
}


static bool parse(Parser* p, Tokens* tokens)
{
  bool failed = 0;

  advance(p);

  while (p->curr.type != TK_EOF) {
    Token tk = {0};
    if (!parse_func_decl(p, &tk))  
      advance(p); // TODO: advance to the next semicolon
    else
      da_append(tokens, tk, DA_TOKENS_INIT_CAP, Tokens);
  }

defer:
  return failed;
}

/*
 * NOTE: in this program a failure 
 * (i.e. global 'err_msg' was set up + return false)
 * is a *program* failure which shuold 
 * only be caused by realloc; nothing 
 * else should cause it to break.
 * If no declaration is found in a file, 
 * the program is still considered successful.
*/ 

bool parser_main(const char* src, uint32_t src_len, 
                 Tokens* tokens, char* lua_err_msg, uint32_t lua_err_msg_len)
{
  bool failed = 0;
  *tokens = (Tokens){0};

  Parser p = (Parser){ 
    .src = src, 
    .src_len = src_len,
  };


  if (parse(&p, tokens))
    return_defer(1);

defer:
  if (failed)
    strncpy(lua_err_msg, gl_err_msg, lua_err_msg_len);

  return failed;
}

