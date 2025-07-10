#include "./cparser.h"

static void print_token(Token* tk)
{
  switch(tk->type) {
    case TK_IDENT: printf("TK_IDENT"); break;
    case TK_COMMA: printf("TK_COMMA"); break;
    case TK_OPEN_PAR: printf("TK_OPEN_PAR"); break;
    case TK_CLOSE_PAR: printf("TK_CLOSE_PAR"); break;
    case TK_OPEN_SBRACKET: printf("TK_OPEN_SBRACKET"); break;
    case TK_CLOSE_SBRACKET: printf("TK_CLOSE_SBRACKET"); break;
    case TK_OPEN_CBRACKET: printf("TK_OPEN_CBRACKET"); break;
    case TK_CLOSE_CBRACKET: printf("TK_CLOSE_CBRACKET"); break;
    case TK_EOF: printf("TK_EOF"); break;
    case TK_UNKNOWN: printf("TK_UNKNOWN"); break;
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
    .offset = p->src_offset
  }; 

  // TODO: use a switch statement 
  if (c == '\0') {
    t.type = TK_EOF;
  } else if (c == '(') {
    t.type = TK_OPEN_PAR;
    p->src_offset++;
  } else if (c == ',') {
    t.type = TK_COMMA;
    p->src_offset++;
  } else if (c == ')') {
    t.type = TK_CLOSE_PAR;
    p->src_offset++;
  } else if (c == '{') {
    t.type = TK_OPEN_CBRACKET;
    p->src_offset++;
  }
  else if (c == '}') {
    t.type = TK_CLOSE_CBRACKET;
    p->src_offset++;
  }
  else if (c == '[') {
    t.type = TK_OPEN_SBRACKET;
    p->src_offset++;
  } else if (c == ']') {
    t.type = TK_CLOSE_SBRACKET;
    p->src_offset++;
  } else if (isalpha(c)) {
    t.type = TK_IDENT;
    p->src_offset++;
    // NOTE: '<=' cause we need to parse the '\0' as well
    for (size_t i = p->src_offset; i <= p->src_len; i++) { 
      char c = p->src[i];
      if (c != '_' && !isalnum(c)) {
        p->src_offset = i;
        break;
      }
    }
  } else {
    t.type = TK_UNKNOWN;
    p->src_offset++;
  }

  return t;
}


static void advance(Parser* p) 
{
  p->curr = p->next;
  p->next = next_token(p);
}


static void expect(Parser* p, TokenType expected)
{

}

static void parse_func_decl(Parser* p)
{
}


void parse(const char* src, uint32_t src_len)
{
  Parser p = (Parser){ 
    .src = src, 
    .src_len = src_len,
  };

  advance(&p);


  while (p.curr.type != TK_EOF) {
    advance(&p);
    print_token(&p.curr);
  }

}



