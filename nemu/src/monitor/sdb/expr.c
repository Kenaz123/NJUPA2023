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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_NUM,// 10 & 16
  TK_REG,
  TK_VAR,
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
  {"==", TK_EQ},        // equal
  {"\\(", '('},
  {"\\)", ')'},
  {"[0-9]+", TK_NUM},
  {"\\$\\w+", TK_REG},
  {"[A-Za-z_]\\w*", TK_VAR},
};

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

static Token tokens[32] __attribute__((used)) = {};
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
          case TK_REG:
          case TK_VAR:
          strncpy(tokens[nr_token].str,substr_start,substr_len);
          tokens[nr_token].str[substr_len] = '\0';//avoid overflow
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
    if(tokens[i].type == TK_NUM){
      continue;
    }
    if(tokens[i].type == '('){
      par++;
    }
    else if(tokens[i].type == ')'){
      if(par == 0){
        return -1;
      }
      par--;
    }
    else if(par > 0){
      continue;
    }
    else{
      int tmp = 0;
      switch(tokens[i].type){
        case '*':
        case '/':
        tmp = 1;break;
        case '+':
        case '-':
        tmp = 2;break;
        default:assert(0);
      }
      if(tmp>=priority){
        priority = tmp;
        res = i;
      }
    }
  }
  if(par !=0) return -1;
  return res;
}

word_t eval(int p,int q,bool *confirm){
  *confirm = true;
  if(p > q){
    confirm = false;
    return 0;
    //bad expression
  }
  else if(p == q){
    if(tokens[p].type!= TK_NUM){
      confirm = false;
      return 0;
    }
    word_t result = strtol(tokens[p].str,NULL,10);
    return result;
    //single token
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
    word_t val1 = eval(p,dom-1,confirm);
    if(!*confirm) return 0;
    word_t val2 = eval(dom+1,q,confirm);
    if(!*confirm) return 0;

    switch(tokens[dom].type){
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': if(val2 == 0){
        *confirm = false;
        return 0;
      }
      return (sword_t)val1 / (sword_t)val2;
      default: assert(0);
    }

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
