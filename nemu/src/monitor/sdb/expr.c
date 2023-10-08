/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <memory/paddr.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdbool.h>

enum {
  TK_NOTYPE = 256, TK_EQ,TK_NEQ,
  TK_POS,TK_NEG,
  TK_DEREF,
  TK_GT,TK_LT,TK_GE,TK_LE,
  TK_AND,TK_OR,
  TK_NUM,// 10 & 16
  TK_REG,
  //TK_VAR,
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"\\-", '-'},         //minus
  {"\\*", '*'},         //multiply
  {"\\/", '/'},           //divide
  {"<",TK_LT},
  {">",TK_GT},
  {"<=",TK_LE},
  {">=",TK_GE},
  {"==", TK_EQ},        // equal
  {"!=",TK_NEQ},
  {"&&",TK_AND},
  {"\\|\\|",TK_OR},
  {"\\(", '('},
  {"\\)", ')'},
  {"(0x)?[0-9]+", TK_NUM},
  {"\\$\\w+", TK_REG},
  //{"[A-Za-z_]\\w*", TK_VAR},
};
static int bound_types[] = {')',TK_NUM,TK_REG};
static int nop_types[] = {'(',')',TK_NUM,TK_REG};
static int unary_types[] = {TK_NEG,TK_POS,TK_DEREF};
static bool matchtypes(int type,int types[],int len){
  for(int i=0;i< len;i++){
    if(type == types[i]) return true;
  }
  return false;
}

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[100];
} Token;

static Token tokens[1000] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        if(rules[i].token_type == TK_NOTYPE) break;
        tokens[nr_token].type = rules[i].token_type;
        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NUM:
          strncpy(tokens[nr_token].str,substr_start,substr_len);
          tokens[nr_token].str[substr_len] = '\0';//avoid overflow
          break;
          case TK_REG:
          //case TK_VAR:
          strncpy(tokens[nr_token].str,substr_start+1,substr_len-1);
          tokens[nr_token].str[substr_len-1] = '\0';//avoid overflow
          break;
          case '*':
          case '+':
          case '-':
          if(nr_token==0 || !matchtypes(tokens[nr_token-1].type,bound_types,3)){
              switch(rules[i].token_type){
                case '-':tokens[nr_token].type = TK_NEG;break;
                case '+':tokens[nr_token].type = TK_POS;break;
                case '*':tokens[nr_token].type = TK_DEREF;break;
              }
            }
          break;
        }
        nr_token++;


        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p,int q){
  if(tokens[p].type=='(' && tokens[q].type==')'){
    int cnt=0;
    for(int i=p;i<=q;i++){
      if(tokens[i].type=='('){
        cnt++;
      }
      if(tokens[i].type==')'){
        cnt--;
      }
      if(cnt == 0) return i==q;
    }
  }
  return false;
}

int find(int p,int q){
  int par=0;
  int res=-1;
  int priority=0;
  for(int i=p;i<=q;i++){
    /*if(tokens[i].type == TK_NUM){
      continue;
    }*/
    if(tokens[i].type == '('){
      par++;
    }
    else if(tokens[i].type == ')'){
      if(par == 0){
        return -1;
      }
      par--;
    }
    else if(matchtypes(tokens[i].type,nop_types,4)){
      continue;
    }
    else if(par > 0){
      continue;
    }
    else{
      int tmp = 0;
      switch(tokens[i].type){
        case TK_NEG:
        case TK_DEREF:
        case TK_POS:
        tmp = 1;break;
        case '*':
        case '/':
        tmp = 2;break;
        case '+':
        case '-':
        tmp = 3;break;
        case TK_LT:
        case TK_GT:
        case TK_GE:
        case TK_LE:
        tmp = 4;break;
        case TK_EQ:
        case TK_NEQ:
        tmp = 5;break;
        case TK_AND:
        tmp = 6;break;
        case TK_OR:
        tmp = 7;break;
        default: return -1;
      }
      if(tmp>priority || (tmp==priority && !matchtypes(tokens[i].type,unary_types,3))){
        priority = tmp;
        res = i;
      }
    }
  }
  if(par !=0) {return -1;}
  else{
  return res;
  }
}
word_t eval_operand(int i,bool *confirm){
  switch(tokens[i].type){
    case TK_NUM:
    if(strncmp("0x",tokens[i].str,2)==0) return strtol(tokens[i].str,NULL,16);
    else return strtol(tokens[i].str,NULL,10);
    case TK_REG:
    return isa_reg_str2val(tokens[i].str,confirm);
    default: *confirm = false;
    return 0;
  }
}
//unary operator
word_t calc1(int op,word_t val, bool *confirm){
  switch(op){
    case TK_NEG: return -val;
    case TK_POS: return val;
    case TK_DEREF: return paddr_read(val,4);
    default: *confirm = false;
    return 0;
  }
  return 0;
}

word_t calc2(word_t val1,int op,word_t val2,bool *confirm){
  switch(op){
    case '+': return val1+val2;
    case '-': return val1-val2;
    case '*': return val1*val2;
    case '/':if(val2==0){
      *confirm = false;
      return 0;
    }
    return (sword_t)val1 / (sword_t)val2;
    case TK_AND: return val1 && val2;
    case TK_OR: return val1 || val2;
    case TK_EQ: return val1 == val2;
    case TK_NEQ: return val1 != val2;
    case TK_GT: return val1 > val2;
    case TK_LT: return val1 < val2;
    case TK_GE: return val1 >= val2;
    case TK_LE: return val1 <= val2;
    default: *confirm = false; return 0;
  }
}

word_t eval(int p,int q,bool *confirm){
  *confirm = true;
  if(p > q){
    *confirm = false;
    return 0;
    //bad expression
  }
  else if(p == q){
    /*if(tokens[p].type!= TK_NUM){
      *confirm = false;
      Log("Error,don't input like '('");
      return 0;
    
    }
    word_t result = strtol(tokens[p].str,NULL,10);
    return result;
    //single token*/ 
    return eval_operand(p,confirm);
  }
  else if(check_parentheses(p,q)==true){
    return eval(p+1,q-1,confirm);
  }
  else{
    int dom=find(p,q);
    if(dom < 0){
      *confirm =false;
      return 0;
    }
    bool ok1,ok2;
    word_t val1 = eval(p,dom-1,&ok1);
    word_t val2 = eval(dom+1,q,&ok2);
    
    if(!ok2){
      *confirm = false;
      return 0;
    }
    if(ok1){
      word_t ret=calc2(val1,tokens[dom].type,val2,confirm);
      return ret;
    }
    else{
      word_t ret=calc1(tokens[dom].type,val2,confirm);
      return ret;
    }

    /*switch(tokens[dom].type){
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': if(val2 == 0){
        *confirm = false;
        return 0;
      }
      return (sword_t)val1 / (sword_t)val2;
      default: assert(0);
    }*/

    //we should do more things here
  }
}






word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  return eval(0,nr_token-1,success);
  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}
