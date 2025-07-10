#include "./cparser.h"

static void print_token(Parser* p, Token* tk)
{
  switch(tk->type) {
    case TK_IDENT: printf("TK_IDENT: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_COMMA: printf("TK_COMMA: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_IF: printf("TK_IF: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_ELSE: printf("TK_ELSE: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_WHILE: printf("TK_WHILE: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_FOR: printf("TK_FOR: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_RETURN: printf("TK_RETURN: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_BREAK: printf("TK_BREAK: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_CASE: printf("TK_CASE: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_OPEN_PAR: printf("TK_OPEN_PAR: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_CLOSE_PAR: printf("TK_CLOSE_PAR: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_OPEN_SBRACKET: printf("TK_OPEN_SBRACKET: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_CLOSE_SBRACKET: printf("TK_CLOSE_SBRACKET: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_OPEN_CBRACKET: printf("TK_OPEN_CBRACKET: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_CLOSE_CBRACKET: printf("TK_CLOSE_CBRACKET: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_EOF: printf("TK_EOF: %.*s", tk->len, &p->src[tk->offset]); break; 
    case TK_UNKNOWN: printf("TK_UNKNOWN: %.*s", tk->len, &p->src[tk->offset]); break; 
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


static bool parse_func_decl(Parser* p)
{
  if (!expect(p, TK_IDENT)) 
    return false; 
  if (!expect(p, TK_IDENT)) 
    return false;
  
  Token func_name = p->curr;

  if (!expect(p, TK_OPEN_PAR)) 
    return false;

  while (p->curr.type != TK_CLOSE_PAR) { // parameters
    advance(p);
    if (p->curr.type == TK_EOF) 
      return false;
  }

  if (!expect(p, TK_OPEN_CBRACKET)) 
    return false;

  while (p->curr.type != TK_CLOSE_CBRACKET) { // function body
    advance(p);
    if (p->curr.type == TK_EOF) 
      return false;
  }

  printf("FUNCTION = %.*s\n", func_name.len, p->src + func_name.offset);
  return true;
}


void parse(const char* src, uint32_t src_len)
{
  Parser p = (Parser){ 
    .src = src, 
    .src_len = src_len,
  };

  advance(&p);

  while (p.curr.type != TK_EOF) {
    if (!parse_func_decl(&p))  
      advance(&p);
  }
}


