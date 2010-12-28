/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is include which follows the "include" declaration
** in the input file. */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#line 6 "expression.y"

#include <ulib/tokenizer.h>
#include <ulib/utility/string_ext.h>

extern void* expressionParserAlloc(void* (*mallocProc)(size_t));
extern void  expressionParserFree(void* p, void (*freeProc)(void*));
extern void  expressionParserTrace(FILE* stream, char* zPrefix);
extern void  expressionParser(void* yyp, int yymajor, UString* yyminor, UString* result);

extern void token_destructor(UString* token);

void token_destructor(UString* token) {
   U_TRACE(0, "::token_destructor(%.*S)", U_STRING_TO_TRACE(*token))

   delete token;
}
#line 28 "expression.c"
/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/*
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands.
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    expressionParserTOKENTYPE     is the data type used for minor tokens given
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is expressionParserTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    expressionParserARG_SDECL     A static variable declaration for the %extra_argument
**    expressionParserARG_PDECL     A parameter declaration for the %extra_argument
**    expressionParserARG_STORE     Code to store %extra_argument into yypParser
**    expressionParserARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 30
#define YYACTIONTYPE unsigned char
#define expressionParserTOKENTYPE  UString* 
typedef union {
  expressionParserTOKENTYPE yy0;
  UString* yy11;
  int yy46;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define expressionParserARG_SDECL  UString* result ;
#define expressionParserARG_PDECL , UString* result 
#define expressionParserARG_FETCH  UString* result  = yypParser->result 
#define expressionParserARG_STORE yypParser->result  = result 
#define YYNSTATE 42
#define YYNRULE 25
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* Next are that tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static const YYACTIONTYPE yy_action[] = {
 /*     0 */    32,   19,   31,   18,   30,   23,   40,   36,   68,   32,
 /*    10 */    21,   31,   18,   30,   23,   40,   36,   32,   37,   24,
 /*    20 */    18,   30,   23,   40,   36,   32,   69,   26,   18,   30,
 /*    30 */    23,   40,   36,   32,   69,   32,   16,   30,   23,   40,
 /*    40 */    36,   32,   34,   69,   17,   30,   23,   40,   36,   32,
 /*    50 */     4,    5,   32,   25,   23,   40,   36,   32,   33,   36,
 /*    60 */    32,   27,   23,   40,   36,   32,   35,   36,   69,   28,
 /*    70 */    23,   40,   36,    1,   32,   38,   69,   32,   29,   23,
 /*    80 */    40,   36,   20,   40,   36,   32,    8,    9,    6,    7,
 /*    90 */    22,   40,   36,   15,    1,   32,   38,   42,    3,    2,
 /*   100 */    69,   39,   36,   12,   13,   14,    3,    2,   10,   11,
 /*   110 */    69,   69,   69,   69,   69,   69,   69,   69,   69,   69,
 /*   120 */    69,   41,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    20,   21,   22,   23,   24,   25,   26,   27,   28,   20,
 /*    10 */    21,   22,   23,   24,   25,   26,   27,   20,   17,   22,
 /*    20 */    23,   24,   25,   26,   27,   20,   29,   22,   23,   24,
 /*    30 */    25,   26,   27,   20,   29,   20,   23,   24,   25,   26,
 /*    40 */    27,   20,   27,   29,   23,   24,   25,   26,   27,   20,
 /*    50 */     3,    4,   20,   24,   25,   26,   27,   20,   26,   27,
 /*    60 */    20,   24,   25,   26,   27,   20,   26,   27,   29,   24,
 /*    70 */    25,   26,   27,   15,   20,   17,   29,   20,   24,   25,
 /*    80 */    26,   27,   25,   26,   27,   20,    5,    6,    7,    8,
 /*    90 */    25,   26,   27,   14,   15,   20,   17,    0,    1,    2,
 /*   100 */    29,   26,   27,   11,   12,   13,    1,    2,    9,   10,
 /*   110 */    29,   29,   29,   29,   29,   29,   29,   29,   29,   29,
 /*   120 */    29,   16,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 32
static const signed char yy_shift_ofst[] = {
 /*     0 */    79,   79,   79,   79,   79,   79,   79,   79,   79,   79,
 /*    10 */    79,   79,   79,   79,   79,   58,   81,   81,   81,   97,
 /*    20 */    92,  105,   92,   92,   47,   99,   47,   99,   99,   99,
 /*    30 */    99,   47,    1,
};
#define YY_REDUCE_USE_DFLT (-21)
#define YY_REDUCE_MAX 15
static const signed char yy_reduce_ofst[] = {
 /*     0 */   -20,  -11,   -3,    5,   13,   21,   29,   37,   45,   54,
 /*    10 */    57,   65,   32,   40,   75,   15,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */    67,   67,   67,   67,   67,   67,   67,   67,   67,   67,
 /*    10 */    67,   67,   67,   67,   67,   67,   46,   47,   48,   67,
 /*    20 */    54,   67,   55,   56,   43,   49,   44,   50,   51,   52,
 /*    30 */    53,   45,   64,   57,   61,   58,   62,   66,   65,   59,
 /*    40 */    60,   63,
};
#define YY_SZ_ACTTAB (int)(sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
**
**      %fallback ID X Y Z.
**
** appears in the grammer, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  int stateno;       /* The state-number */
  int major;         /* The major token value.  This is the code
                     ** number for the token at this stack level */
  YYMINORTYPE minor; /* The user-supplied minor token value.  This
                     ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
  int yyerrcnt;                 /* Shifts left before out of the error */
  expressionParserARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/*
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void expressionParserTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = {
  "$",             "AND",           "OR",            "EQ",          
  "NE",            "GT",            "GE",            "LT",          
  "LE",            "PLUS",          "MINUS",         "MULT",        
  "DIV",           "MOD",           "NOT",           "LPAREN",      
  "RPAREN",        "VALUE",         "error",         "cond",        
  "value",         "booleanExpression",  "equalityExpression",  "relationalExpression",
  "additiveExpression",  "multiplicativeExpression",  "unaryExpression",  "primaryExpression",
  "input",       
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "input ::= booleanExpression",
 /*   1 */ "booleanExpression ::= booleanExpression OR equalityExpression",
 /*   2 */ "booleanExpression ::= booleanExpression AND equalityExpression",
 /*   3 */ "booleanExpression ::= equalityExpression",
 /*   4 */ "equalityExpression ::= equalityExpression EQ relationalExpression",
 /*   5 */ "equalityExpression ::= equalityExpression NE relationalExpression",
 /*   6 */ "equalityExpression ::= relationalExpression",
 /*   7 */ "relationalExpression ::= relationalExpression LT additiveExpression",
 /*   8 */ "relationalExpression ::= relationalExpression LE additiveExpression",
 /*   9 */ "relationalExpression ::= relationalExpression GT additiveExpression",
 /*  10 */ "relationalExpression ::= relationalExpression GE additiveExpression",
 /*  11 */ "relationalExpression ::= additiveExpression",
 /*  12 */ "additiveExpression ::= additiveExpression PLUS multiplicativeExpression",
 /*  13 */ "additiveExpression ::= additiveExpression MINUS multiplicativeExpression",
 /*  14 */ "additiveExpression ::= multiplicativeExpression",
 /*  15 */ "multiplicativeExpression ::= multiplicativeExpression MULT unaryExpression",
 /*  16 */ "multiplicativeExpression ::= multiplicativeExpression DIV unaryExpression",
 /*  17 */ "multiplicativeExpression ::= multiplicativeExpression MOD unaryExpression",
 /*  18 */ "multiplicativeExpression ::= unaryExpression",
 /*  19 */ "unaryExpression ::= NOT primaryExpression",
 /*  20 */ "unaryExpression ::= primaryExpression",
 /*  21 */ "primaryExpression ::= LPAREN booleanExpression RPAREN",
 /*  22 */ "primaryExpression ::= value",
 /*  23 */ "value ::= VALUE",
 /*  24 */ "value ::= value VALUE",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/*
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to expressionParser and expressionParserFree.
*/
void *expressionParserAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#if YYSTACKDEPTH<=0
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(YYCODETYPE yymajor, YYMINORTYPE *yypminor){
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    case 1: /* AND */
    case 2: /* OR */
    case 3: /* EQ */
    case 4: /* NE */
    case 5: /* GT */
    case 6: /* GE */
    case 7: /* LT */
    case 8: /* LE */
    case 9: /* PLUS */
    case 10: /* MINUS */
    case 11: /* MULT */
    case 12: /* DIV */
    case 13: /* MOD */
    case 14: /* NOT */
    case 15: /* LPAREN */
    case 16: /* RPAREN */
    case 17: /* VALUE */
#line 24 "expression.y"
{ token_destructor((yypminor->yy0)); }
#line 414 "expression.c"
      break;
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor( yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/*
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from expressionParserAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void expressionParserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;

  if( stateno>YY_SHIFT_MAX || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      int iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  assert( stateno<=YY_REDUCE_MAX );
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  assert( i>=0 && i<YY_SZ_ACTTAB );
  assert( yy_lookahead[i]==iLookAhead );
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
   expressionParserARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
#line 43 "expression.y"

   U_TRACE(0, "::stack_overflow()")
   U_WARNING("Parse stack overflow");
#line 569 "expression.c"
   expressionParserARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer ot the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#if YYSTACKDEPTH>0
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = yyNewState;
  yytos->major = yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 28, 1 },
  { 21, 3 },
  { 21, 3 },
  { 21, 1 },
  { 22, 3 },
  { 22, 3 },
  { 22, 1 },
  { 23, 3 },
  { 23, 3 },
  { 23, 3 },
  { 23, 3 },
  { 23, 1 },
  { 24, 3 },
  { 24, 3 },
  { 24, 1 },
  { 25, 3 },
  { 25, 3 },
  { 25, 3 },
  { 25, 1 },
  { 26, 2 },
  { 26, 1 },
  { 27, 3 },
  { 27, 1 },
  { 20, 1 },
  { 20, 2 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  expressionParserARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  memset(&yygotominor, 0, sizeof(yygotominor));


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 0: /* input ::= booleanExpression */
#line 66 "expression.y"
{
   U_TRACE(0, "input ::= booleanExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(result)

   U_INTERNAL_DUMP("yymsp[0].minor.yy11 = %.*S result = %.*S", U_STRING_TO_TRACE(*yymsp[0].minor.yy11), U_STRING_TO_TRACE(*result))

   *result = *yymsp[0].minor.yy11;

   delete yymsp[0].minor.yy11;
}
#line 713 "expression.c"
        break;
      case 1: /* booleanExpression ::= booleanExpression OR equalityExpression */
#line 102 "expression.y"
{
   U_TRACE(0, "booleanExpression(yygotominor.yy11) ::= booleanExpression(yymsp[-2].minor.yy11) OR equalityExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   yygotominor.yy11 = (yymsp[-2].minor.yy11->empty() == false || yymsp[0].minor.yy11->empty() == false ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(2,&yymsp[-1].minor);
}
#line 733 "expression.c"
        break;
      case 2: /* booleanExpression ::= booleanExpression AND equalityExpression */
#line 118 "expression.y"
{
   U_TRACE(0, "booleanExpression(yygotominor.yy11) ::= booleanExpression(yymsp[-2].minor.yy11) AND equalityExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   yygotominor.yy11 = (yymsp[-2].minor.yy11->empty() == false && yymsp[0].minor.yy11->empty() == false ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(1,&yymsp[-1].minor);
}
#line 753 "expression.c"
        break;
      case 3: /* booleanExpression ::= equalityExpression */
#line 134 "expression.y"
{
   U_TRACE(0, "booleanExpression(yygotominor.yy11) ::= equalityExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_DUMP("yygotominor.yy11 = %p yymsp[0].minor.yy11 = %p", yygotominor.yy11, yymsp[0].minor.yy11)

   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   yygotominor.yy11 = yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
}
#line 768 "expression.c"
        break;
      case 4: /* equalityExpression ::= equalityExpression EQ relationalExpression */
#line 177 "expression.y"
{
   U_TRACE(0, "equalityExpression(yygotominor.yy11) ::= equalityExpression(yymsp[-2].minor.yy11) EQ relationalExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   bool Bbo = (yymsp[-2].minor.yy11->empty() == false),
        Cbo = (yymsp[0].minor.yy11->empty() == false);

   int cmp = (Bbo && Cbo ? yymsp[-2].minor.yy11->compare(yymsp[0].minor.yy11->rep) : Bbo - Cbo);

   U_INTERNAL_DUMP("cmp = %d", cmp)

   yygotominor.yy11 = (cmp == 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(3,&yymsp[-1].minor);
}
#line 795 "expression.c"
        break;
      case 5: /* equalityExpression ::= equalityExpression NE relationalExpression */
#line 200 "expression.y"
{
   U_TRACE(0, "equalityExpression(yygotominor.yy11) ::= equalityExpression(yymsp[-2].minor.yy11) NE relationalExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   bool Bbo = (yymsp[-2].minor.yy11->empty() == false),
        Cbo = (yymsp[0].minor.yy11->empty() == false);

   int cmp = (Bbo && Cbo ? yymsp[-2].minor.yy11->compare(yymsp[0].minor.yy11->rep) : Bbo - Cbo);

   U_INTERNAL_DUMP("cmp = %d", cmp)

   yygotominor.yy11 = (cmp != 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(4,&yymsp[-1].minor);
}
#line 822 "expression.c"
        break;
      case 6: /* equalityExpression ::= relationalExpression */
#line 223 "expression.y"
{
   U_TRACE(0, "equalityExpression(yygotominor.yy11) ::= relationalExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_DUMP("yygotominor.yy11 = %p yymsp[0].minor.yy11 = %p", yygotominor.yy11, yymsp[0].minor.yy11)

   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   yygotominor.yy11 = yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
}
#line 837 "expression.c"
        break;
      case 7: /* relationalExpression ::= relationalExpression LT additiveExpression */
#line 268 "expression.y"
{
   U_TRACE(0, "relationalExpression(yygotominor.yy11) ::= relationalExpression(yymsp[-2].minor.yy11) LT additiveExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   bool Bbo = (yymsp[-2].minor.yy11->empty() == false),
        Cbo = (yymsp[0].minor.yy11->empty() == false);

   int cmp = (Bbo && Cbo ? yymsp[-2].minor.yy11->strtol() - yymsp[0].minor.yy11->strtol() : Bbo - Cbo);

   U_INTERNAL_DUMP("cmp = %d", cmp)

   yygotominor.yy11 = (cmp < 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(7,&yymsp[-1].minor);
}
#line 864 "expression.c"
        break;
      case 8: /* relationalExpression ::= relationalExpression LE additiveExpression */
#line 291 "expression.y"
{
   U_TRACE(0, "relationalExpression(yygotominor.yy11) ::= relationalExpression(yymsp[-2].minor.yy11) LE additiveExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   bool Bbo = (yymsp[-2].minor.yy11->empty() == false),
        Cbo = (yymsp[0].minor.yy11->empty() == false);

   int cmp = (Bbo && Cbo ? yymsp[-2].minor.yy11->strtol() - yymsp[0].minor.yy11->strtol() : Bbo - Cbo);

   U_INTERNAL_DUMP("cmp = %d", cmp)

   yygotominor.yy11 = (cmp <= 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(8,&yymsp[-1].minor);
}
#line 891 "expression.c"
        break;
      case 9: /* relationalExpression ::= relationalExpression GT additiveExpression */
#line 314 "expression.y"
{
   U_TRACE(0, "relationalExpression(yygotominor.yy11) ::= relationalExpression(yymsp[-2].minor.yy11) GT additiveExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   bool Bbo = (yymsp[-2].minor.yy11->empty() == false),
        Cbo = (yymsp[0].minor.yy11->empty() == false);

   int cmp = (Bbo && Cbo ? yymsp[-2].minor.yy11->strtol() - yymsp[0].minor.yy11->strtol() : Bbo - Cbo);

   U_INTERNAL_DUMP("cmp = %d", cmp)

   yygotominor.yy11 = (cmp > 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(5,&yymsp[-1].minor);
}
#line 918 "expression.c"
        break;
      case 10: /* relationalExpression ::= relationalExpression GE additiveExpression */
#line 337 "expression.y"
{
   U_TRACE(0, "relationalExpression(yygotominor.yy11) ::= relationalExpression(yymsp[-2].minor.yy11) GE additiveExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   bool Bbo = (yymsp[-2].minor.yy11->empty() == false),
        Cbo = (yymsp[0].minor.yy11->empty() == false);

   int cmp = (Bbo && Cbo ? yymsp[-2].minor.yy11->strtol() - yymsp[0].minor.yy11->strtol() : Bbo - Cbo);

   U_INTERNAL_DUMP("cmp = %d", cmp)

   yygotominor.yy11 = (cmp >= 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(6,&yymsp[-1].minor);
}
#line 945 "expression.c"
        break;
      case 11: /* relationalExpression ::= additiveExpression */
#line 360 "expression.y"
{
   U_TRACE(0, "relationalExpression(yygotominor.yy11) ::= additiveExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_DUMP("yygotominor.yy11 = %p yymsp[0].minor.yy11 = %p", yygotominor.yy11, yymsp[0].minor.yy11)

   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   yygotominor.yy11 = yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
}
#line 960 "expression.c"
        break;
      case 12: /* additiveExpression ::= additiveExpression PLUS multiplicativeExpression */
#line 395 "expression.y"
{
   U_TRACE(0, "additiveExpression(yygotominor.yy11) ::= additiveExpression(yymsp[-2].minor.yy11) PLUS multiplicativeExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   yygotominor.yy11 = U_NEW(UString(UStringExt::numberToString(yymsp[-2].minor.yy11->strtol() + yymsp[0].minor.yy11->strtol())));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(9,&yymsp[-1].minor);
}
#line 980 "expression.c"
        break;
      case 13: /* additiveExpression ::= additiveExpression MINUS multiplicativeExpression */
#line 411 "expression.y"
{
   U_TRACE(0, "additiveExpression(yygotominor.yy11) ::= additiveExpression(yymsp[-2].minor.yy11) MINUS multiplicativeExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   yygotominor.yy11 = U_NEW(UString(UStringExt::numberToString(yymsp[-2].minor.yy11->strtol() - yymsp[0].minor.yy11->strtol())));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(10,&yymsp[-1].minor);
}
#line 1000 "expression.c"
        break;
      case 14: /* additiveExpression ::= multiplicativeExpression */
#line 427 "expression.y"
{
   U_TRACE(0, "additiveExpression(yygotominor.yy11) ::= multiplicativeExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_DUMP("yygotominor.yy11 = %p yymsp[0].minor.yy11 = %p", yygotominor.yy11, yymsp[0].minor.yy11)

   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   yygotominor.yy11 = yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
}
#line 1015 "expression.c"
        break;
      case 15: /* multiplicativeExpression ::= multiplicativeExpression MULT unaryExpression */
#line 463 "expression.y"
{
   U_TRACE(0, "multiplicativeExpression(yygotominor.yy11) ::= multiplicativeExpression(yymsp[-2].minor.yy11) MULT unaryExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   yygotominor.yy11 = U_NEW(UString(UStringExt::numberToString(yymsp[-2].minor.yy11->strtol() * yymsp[0].minor.yy11->strtol())));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(11,&yymsp[-1].minor);
}
#line 1035 "expression.c"
        break;
      case 16: /* multiplicativeExpression ::= multiplicativeExpression DIV unaryExpression */
#line 479 "expression.y"
{
   U_TRACE(0, "multiplicativeExpression(yygotominor.yy11) ::= multiplicativeExpression(yymsp[-2].minor.yy11) DIV unaryExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   yygotominor.yy11 = U_NEW(UString(UStringExt::numberToString(yymsp[-2].minor.yy11->strtol() / yymsp[0].minor.yy11->strtol())));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(12,&yymsp[-1].minor);
}
#line 1055 "expression.c"
        break;
      case 17: /* multiplicativeExpression ::= multiplicativeExpression MOD unaryExpression */
#line 495 "expression.y"
{
   U_TRACE(0, "multiplicativeExpression(yygotominor.yy11) ::= multiplicativeExpression(yymsp[-2].minor.yy11) MOD unaryExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_ASSERT_POINTER(yymsp[-2].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   U_INTERNAL_DUMP("yymsp[-2].minor.yy11 = %.*S yymsp[0].minor.yy11 = %.*S", U_STRING_TO_TRACE(*yymsp[-2].minor.yy11), U_STRING_TO_TRACE(*yymsp[0].minor.yy11))

   yygotominor.yy11 = U_NEW(UString(UStringExt::numberToString(yymsp[-2].minor.yy11->strtol() % yymsp[0].minor.yy11->strtol())));

   delete yymsp[-2].minor.yy11;
   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(13,&yymsp[-1].minor);
}
#line 1075 "expression.c"
        break;
      case 18: /* multiplicativeExpression ::= unaryExpression */
#line 511 "expression.y"
{
   U_TRACE(0, "multiplicativeExpression(yygotominor.yy11) ::= unaryExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_DUMP("yygotominor.yy11 = %p yymsp[0].minor.yy11 = %p", yygotominor.yy11, yymsp[0].minor.yy11)

   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   yygotominor.yy11 = yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
}
#line 1090 "expression.c"
        break;
      case 19: /* unaryExpression ::= NOT primaryExpression */
#line 523 "expression.y"
{
   U_TRACE(0, "unaryExpression(yygotominor.yy11) ::= NOT primaryExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_DUMP("yygotominor.yy11 = %p yymsp[0].minor.yy11 = %p", yygotominor.yy11, yymsp[0].minor.yy11)

   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   yygotominor.yy11 = (yymsp[0].minor.yy11->empty() ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

   delete yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(14,&yymsp[-1].minor);
}
#line 1108 "expression.c"
        break;
      case 20: /* unaryExpression ::= primaryExpression */
#line 537 "expression.y"
{
   U_TRACE(0, "unaryExpression(yygotominor.yy11) ::= primaryExpression(yymsp[0].minor.yy11)")

   U_INTERNAL_DUMP("yygotominor.yy11 = %p yymsp[0].minor.yy11 = %p", yygotominor.yy11, yymsp[0].minor.yy11)

   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   yygotominor.yy11 = yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
}
#line 1123 "expression.c"
        break;
      case 21: /* primaryExpression ::= LPAREN booleanExpression RPAREN */
#line 549 "expression.y"
{
   U_TRACE(0, "primaryExpression(yygotominor.yy11) ::= LPAREN booleanExpression(yymsp[-1].minor.yy11) RPAREN")

   U_INTERNAL_DUMP("yygotominor.yy11 = %p yymsp[-1].minor.yy11 = %p", yygotominor.yy11, yymsp[-1].minor.yy11)

   U_INTERNAL_ASSERT_POINTER(yymsp[-1].minor.yy11)

   yygotominor.yy11 = yymsp[-1].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
  yy_destructor(15,&yymsp[-2].minor);
  yy_destructor(16,&yymsp[0].minor);
}
#line 1140 "expression.c"
        break;
      case 22: /* primaryExpression ::= value */
#line 561 "expression.y"
{
   U_TRACE(0, "primaryExpression(yygotominor.yy11) ::= value(yymsp[0].minor.yy11)")

   U_INTERNAL_DUMP("yygotominor.yy11 = %p yymsp[0].minor.yy11 = %p", yygotominor.yy11, yymsp[0].minor.yy11)

   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy11)

   yygotominor.yy11 = yymsp[0].minor.yy11;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
}
#line 1155 "expression.c"
        break;
      case 23: /* value ::= VALUE */
#line 573 "expression.y"
{
   U_TRACE(0, "value(yygotominor.yy11) ::= VALUE(yymsp[0].minor.yy0)")

   U_INTERNAL_DUMP("yygotominor.yy11 = %p yymsp[0].minor.yy0 = %p", yygotominor.yy11, yymsp[0].minor.yy0)

   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy0)

   yygotominor.yy11 = yymsp[0].minor.yy0;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
}
#line 1170 "expression.c"
        break;
      case 24: /* value ::= value VALUE */
#line 585 "expression.y"
{
   U_TRACE(0, "value(yygotominor.yy11) ::= value(yymsp[-1].minor.yy11) VALUE(yymsp[0].minor.yy0)")

   U_INTERNAL_DUMP("yygotominor.yy11 = %p yymsp[-1].minor.yy11 = %p yymsp[0].minor.yy0 = %p", yygotominor.yy11, yymsp[-1].minor.yy11, yymsp[0].minor.yy0)

   U_INTERNAL_ASSERT_POINTER(yymsp[-1].minor.yy11)
   U_INTERNAL_ASSERT_POINTER(yymsp[0].minor.yy0)

    yygotominor.yy11  =  yymsp[-1].minor.yy11;
   *yygotominor.yy11 += *yymsp[0].minor.yy0;

   delete yymsp[0].minor.yy0;

   U_INTERNAL_DUMP("yygotominor.yy11 = %.*S", U_STRING_TO_TRACE(*yygotominor.yy11))
}
#line 1189 "expression.c"
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = yyact;
      yymsp->major = yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  expressionParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
#line 31 "expression.y"

   U_TRACE(0, "::parse_failure()")
   U_INTERNAL_ASSERT_POINTER(result)
   result->clear();
   U_WARNING("Parse failure!");
#line 1241 "expression.c"
  expressionParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  expressionParserARG_FETCH;
#define TOKEN (yyminor.yy0)
#line 37 "expression.y"

   U_TRACE(0, "::syntax_error()")
   U_INTERNAL_ASSERT_POINTER(result)
   result->clear();
   U_WARNING("Syntax error!");
#line 1262 "expression.c"
  expressionParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  expressionParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
#line 26 "expression.y"

   U_TRACE(0, "::parse_accept()")
   U_INTERNAL_ASSERT_POINTER(result)
   U_INTERNAL_DUMP("result = %.*S", U_STRING_TO_TRACE(*result))
#line 1287 "expression.c"
  expressionParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "expressionParserAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void expressionParser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  expressionParserTOKENTYPE yyminor       /* The value for the token */
  expressionParserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      memset(&yyminorunion, 0, sizeof(yyminorunion));
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  expressionParserARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
