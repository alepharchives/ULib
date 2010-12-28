%token_prefix U_TK_
%token_type { UString* }
%extra_argument { UString* result }
%name expressionParser

%include {
#include <ulib/tokenizer.h>
#include <ulib/utility/string_ext.h>

extern void* U_EXPORT expressionParserAlloc(void* (*mallocProc)(size_t));
extern void  U_EXPORT expressionParserFree(void* p, void (*freeProc)(void*));
extern void  U_EXPORT expressionParserTrace(FILE* stream, char* zPrefix);
extern void  U_EXPORT expressionParser(void* yyp, int yymajor, UString* yyminor, UString* result);

extern void token_destructor(UString* token);

void token_destructor(UString* token) {
	U_TRACE(0, "::token_destructor(%.*S)", U_STRING_TO_TRACE(*token))

	delete token;
}
} /* %include */

%token_destructor { token_destructor($$); }

%parse_accept {
	U_TRACE(0, "::parse_accept()")
	U_INTERNAL_ASSERT_POINTER(result)
	U_INTERNAL_DUMP("result = %.*S", U_STRING_TO_TRACE(*result))
}
%parse_failure {
	U_TRACE(0, "::parse_failure()")
	U_INTERNAL_ASSERT_POINTER(result)
	result->clear();
	U_WARNING("Parse failure!");
}
%syntax_error {
	U_TRACE(0, "::syntax_error()")
	U_INTERNAL_ASSERT_POINTER(result)
	result->clear();
	U_WARNING("Syntax error!");
}
%stack_overflow {
	U_TRACE(0, "::stack_overflow()")
	U_WARNING("Parse stack overflow");
}

%left AND.
%left OR.
%nonassoc EQ NE GT GE LT LE.
%left PLUS MINUS.
%left MULT DIV MOD.
%right NOT.

%type cond { int }
%type value { UString* }

%type booleanExpression { UString* }
%type equalityExpression { UString* }
%type relationalExpression { UString* }
%type additiveExpression { UString* }
%type multiplicativeExpression { UString* }
%type unaryExpression { UString* }
%type primaryExpression { UString* }

input ::= booleanExpression(B). {
	U_TRACE(0, "input ::= booleanExpression(B)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(result)

	U_INTERNAL_DUMP("B = %.*S result = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*result))

	*result = *B;

	delete B;
}

/*
booleanExpression(A) ::= booleanExpression(B) cond(C) equalityExpression(D). {
   U_TRACE(0, "booleanExpression(A) ::= booleanExpression(B) cond(C) equalityExpression(D)")

   U_INTERNAL_ASSERT_POINTER(B)
   U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S C = %d D = %.*S", U_STRING_TO_TRACE(*B), C, U_STRING_TO_TRACE(*D))

   bool Bbo = (B->empty() == false),
        Dbo = (D->empty() == false),
		   bo = (C == U_TK_AND ? Bbo && Dbo
									  : Bbo || Dbo);

	A = (bo ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}
*/

booleanExpression(A) ::= booleanExpression(B) OR equalityExpression(C). {
	U_TRACE(0, "booleanExpression(A) ::= booleanExpression(B) OR equalityExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	A = (B->empty() == false || C->empty() == false ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

booleanExpression(A) ::= booleanExpression(B) AND equalityExpression(C). {
	U_TRACE(0, "booleanExpression(A) ::= booleanExpression(B) AND equalityExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	A = (B->empty() == false && C->empty() == false ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

booleanExpression(A) ::= equalityExpression(B). {
	U_TRACE(0, "booleanExpression(A) ::= equalityExpression(B)")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

/*
equalityExpression(A) ::= equalityExpression(B) cond(C) relationalExpression(D). {
   U_TRACE(0, "equalityExpression(A) ::= equalityExpression(B) cond(C) relationalExpression(D)")

   U_INTERNAL_ASSERT_POINTER(B)
   U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S C = %d D = %.*S", U_STRING_TO_TRACE(*B), C, U_STRING_TO_TRACE(*D))

   bool bo, Bbo = (B->empty() == false),
				Dbo = (D->empty() == false);

	int cmp = (Bbo && Dbo ? B->compare(D->rep) : Bbo - Dbo);

   switch (C)
      {
      case U_TK_EQ: bo = (cmp == 0); break;
      case U_TK_NE: bo = (cmp != 0); break;
      }

   U_INTERNAL_DUMP("bo = %b cmp = %d", bo, cmp)

	A = (bo ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}
*/

equalityExpression(A) ::= equalityExpression(B) EQ relationalExpression(C). {
	U_TRACE(0, "equalityExpression(A) ::= equalityExpression(B) EQ relationalExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	bool Bbo = (B->empty() == false),
		  Cbo = (C->empty() == false);

	int cmp = (Bbo && Cbo ? B->compare(C->rep) : Bbo - Cbo);

	U_INTERNAL_DUMP("cmp = %d", cmp)

	A = (cmp == 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

equalityExpression(A) ::= equalityExpression(B) NE relationalExpression(C). {
	U_TRACE(0, "equalityExpression(A) ::= equalityExpression(B) NE relationalExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	bool Bbo = (B->empty() == false),
		  Cbo = (C->empty() == false);

	int cmp = (Bbo && Cbo ? B->compare(C->rep) : Bbo - Cbo);

	U_INTERNAL_DUMP("cmp = %d", cmp)

	A = (cmp != 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

equalityExpression(A) ::= relationalExpression(B). {
	U_TRACE(0, "equalityExpression(A) ::= relationalExpression(B)")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

/*
relationalExpression(A) ::= relationalExpression(B) cond(C) additiveExpression(D). {
   U_TRACE(0, "relationalExpression(A) ::= relationalExpression(B) cond(C) additiveExpression(D)")

   U_INTERNAL_ASSERT_POINTER(B)
   U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S C = %d D = %.*S", U_STRING_TO_TRACE(*B), C, U_STRING_TO_TRACE(*D))

   bool bo, Bbo = (B->empty() == false),
				Dbo = (D->empty() == false);

	int cmp = (Bbo && Dbo ? B->compare(D->rep) : Bbo - Dbo);

   switch (C)
      {
      case U_TK_LT: bo = (cmp <  0); break;
      case U_TK_LE: bo = (cmp <= 0); break;
      case U_TK_GT: bo = (cmp >  0); break;
      case U_TK_GE: bo = (cmp >= 0); break;
      }

   U_INTERNAL_DUMP("bo = %b cmp = %d", bo, cmp)

	A = (bo ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}
*/

relationalExpression(A) ::= relationalExpression(B) LT additiveExpression(C). {
	U_TRACE(0, "relationalExpression(A) ::= relationalExpression(B) LT additiveExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	bool Bbo = (B->empty() == false),
		  Cbo = (C->empty() == false);

	int cmp = (Bbo && Cbo ? B->strtol() - C->strtol() : Bbo - Cbo);

	U_INTERNAL_DUMP("cmp = %d", cmp)

	A = (cmp < 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

relationalExpression(A) ::= relationalExpression(B) LE additiveExpression(C). {
	U_TRACE(0, "relationalExpression(A) ::= relationalExpression(B) LE additiveExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	bool Bbo = (B->empty() == false),
		  Cbo = (C->empty() == false);

	int cmp = (Bbo && Cbo ? B->strtol() - C->strtol() : Bbo - Cbo);

	U_INTERNAL_DUMP("cmp = %d", cmp)

	A = (cmp <= 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

relationalExpression(A) ::= relationalExpression(B) GT additiveExpression(C). {
	U_TRACE(0, "relationalExpression(A) ::= relationalExpression(B) GT additiveExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	bool Bbo = (B->empty() == false),
		  Cbo = (C->empty() == false);

	int cmp = (Bbo && Cbo ? B->strtol() - C->strtol() : Bbo - Cbo);

	U_INTERNAL_DUMP("cmp = %d", cmp)

	A = (cmp > 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

relationalExpression(A) ::= relationalExpression(B) GE additiveExpression(C). {
	U_TRACE(0, "relationalExpression(A) ::= relationalExpression(B) GE additiveExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	bool Bbo = (B->empty() == false),
		  Cbo = (C->empty() == false);

	int cmp = (Bbo && Cbo ? B->strtol() - C->strtol() : Bbo - Cbo);

	U_INTERNAL_DUMP("cmp = %d", cmp)

	A = (cmp >= 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

relationalExpression(A) ::= additiveExpression(B). {
	U_TRACE(0, "relationalExpression(A) ::= additiveExpression(B)")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

/*
additiveExpression(A) ::= additiveExpression(B) cond(C) multiplicativeExpression(D). {
   U_TRACE(0, "additiveExpression(A) ::= additiveExpression(B) cond(C) multiplicativeExpression(D)")

   U_INTERNAL_ASSERT_POINTER(B)
   U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S C = %d D = %.*S", U_STRING_TO_TRACE(*B), C, U_STRING_TO_TRACE(*D))

   long Blo = B->strtol(),
        Dlo = D->strtol(),
		   lo = (C == U_TK_PLUS ? Blo + Dlo
									   : Blo - Dlo);

	A = U_NEW(UString(UStringExt::numberToString(lo)));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}
*/

additiveExpression(A) ::= additiveExpression(B) PLUS multiplicativeExpression(C). {
	U_TRACE(0, "additiveExpression(A) ::= additiveExpression(B) PLUS multiplicativeExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	A = U_NEW(UString(UStringExt::numberToString(B->strtol() + C->strtol())));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

additiveExpression(A) ::= additiveExpression(B) MINUS multiplicativeExpression(C). {
	U_TRACE(0, "additiveExpression(A) ::= additiveExpression(B) MINUS multiplicativeExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	A = U_NEW(UString(UStringExt::numberToString(B->strtol() - C->strtol())));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

additiveExpression(A) ::= multiplicativeExpression(B). {
	U_TRACE(0, "additiveExpression(A) ::= multiplicativeExpression(B)")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

/*
multiplicativeExpression(A) ::= multiplicativeExpression(B) cond(C) unaryExpression(D). {
   U_TRACE(0, "multiplicativeExpression(A) ::= multiplicativeExpression(B) cond(C) unaryExpression(D)")

   U_INTERNAL_ASSERT_POINTER(B)
   U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S C = %d D = %.*S", U_STRING_TO_TRACE(*B), C, U_STRING_TO_TRACE(*D))

   long Blo = B->strtol(),
        Dlo = D->strtol(),
		   lo = (C == U_TK_MULT ? Blo * Dlo :
				   C == U_TK_DIV  ? Blo / Dlo :
										  Blo % Dlo);

	A = U_NEW(UString(UStringExt::numberToString(lo)));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}
*/

multiplicativeExpression(A) ::= multiplicativeExpression(B) MULT unaryExpression(C). {
	U_TRACE(0, "multiplicativeExpression(A) ::= multiplicativeExpression(B) MULT unaryExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	A = U_NEW(UString(UStringExt::numberToString(B->strtol() * C->strtol())));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

multiplicativeExpression(A) ::= multiplicativeExpression(B) DIV unaryExpression(C). {
	U_TRACE(0, "multiplicativeExpression(A) ::= multiplicativeExpression(B) DIV unaryExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	A = U_NEW(UString(UStringExt::numberToString(B->strtol() / C->strtol())));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

multiplicativeExpression(A) ::= multiplicativeExpression(B) MOD unaryExpression(C). {
	U_TRACE(0, "multiplicativeExpression(A) ::= multiplicativeExpression(B) MOD unaryExpression(C)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	U_INTERNAL_DUMP("B = %.*S C = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*C))

	A = U_NEW(UString(UStringExt::numberToString(B->strtol() % C->strtol())));

	delete B;
	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

multiplicativeExpression(A) ::= unaryExpression(B). {
	U_TRACE(0, "multiplicativeExpression(A) ::= unaryExpression(B)")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

unaryExpression(A) ::= NOT primaryExpression(B). {
	U_TRACE(0, "unaryExpression(A) ::= NOT primaryExpression(B)")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = (B->empty() ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

unaryExpression(A) ::= primaryExpression(B). {
	U_TRACE(0, "unaryExpression(A) ::= primaryExpression(B)")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

primaryExpression(A) ::= LPAREN booleanExpression(B) RPAREN. {
	U_TRACE(0, "primaryExpression(A) ::= LPAREN booleanExpression(B) RPAREN")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

primaryExpression(A) ::= value(B). {
	U_TRACE(0, "primaryExpression(A) ::= value(B)")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

value(A) ::= VALUE(B). {
	U_TRACE(0, "value(A) ::= VALUE(B)")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

value(A) ::= value(B) VALUE(C). {
	U_TRACE(0, "value(A) ::= value(B) VALUE(C)")

	U_INTERNAL_DUMP("A = %p B = %p C = %p", A, B, C)

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(C)

	 A  =  B;
	*A += *C;

	delete C;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

/*
cond(A) ::= AND. {
   U_TRACE(0, "cond(A) ::= AND")
   A = U_TK_AND;
}
cond(A) ::= OR. {
   U_TRACE(0, "cond(A) ::= OR")
   A = U_TK_OR;
}
cond(A) ::= EQ. {
   U_TRACE(0, "cond(A) ::= EQ")
   A = U_TK_EQ;
}
cond(A) ::= NE. {
   U_TRACE(0, "cond(A) ::= NE")
   A = U_TK_NE;
}
cond(A) ::= GT. {
   U_TRACE(0, "cond(A) ::= GT")
   A = U_TK_GT;
}
cond(A) ::= GE. {
   U_TRACE(0, "cond(A) ::= GE")
   A = U_TK_GE;
}
cond(A) ::= LT. {
   U_TRACE(0, "cond(A) ::= LT")
   A = U_TK_LT;
}
cond(A) ::= LE. {
   U_TRACE(0, "cond(A) ::= LE")
   A = U_TK_LE;
}
cond(A) ::= PLUS. {
   U_TRACE(0, "cond(A) ::= PLUS")
   A = U_TK_PLUS;
}
cond(A) ::= MINUS. {
   U_TRACE(0, "cond(A) ::= MINUS")
   A = U_TK_MINUS;
}
cond(A) ::= MULT. {
   U_TRACE(0, "cond(A) ::= MULT")
   A = U_TK_MULT;
}
cond(A) ::= DIV. {
   U_TRACE(0, "cond(A) ::= DIV")
   A = U_TK_DIV;
}
cond(A) ::= MOD. {
   U_TRACE(0, "cond(A) ::= MOD")
   A = U_TK_MOD;
}
*/

/*
%type expr { UString* }
%type exprline { UString* }

input ::= exprline(B). {
	U_TRACE(0, "input ::= exprline(B)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(result)

	U_INTERNAL_DUMP("B = %.*S result = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*result))

	*result = *B;

	delete B;
}

exprline(A) ::= exprline(B) AND expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) AND expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

	A = (B->empty() == false && D->empty() == false ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= exprline(B) OR expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) OR expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

	A = (B->empty() == false || D->empty() == false ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= exprline(B) EQ expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) EQ expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

   bool Bbo = (B->empty() == false),
        Dbo = (D->empty() == false);

   int cmp = (Bbo && Dbo ? B->compare(D->rep) : Bbo - Dbo);

   U_INTERNAL_DUMP("cmp = %d", cmp)

   A = (cmp == 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= exprline(B) NE expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) NE expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

   bool Bbo = (B->empty() == false),
        Dbo = (D->empty() == false);

   int cmp = (Bbo && Dbo ? B->compare(D->rep) : Bbo - Dbo);

   U_INTERNAL_DUMP("cmp = %d", cmp)

   A = (cmp != 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= exprline(B) GT expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) GT expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

   bool Bbo = (B->empty() == false),
        Dbo = (D->empty() == false);

   int cmp = (Bbo && Dbo ? B->strtol() - D->strtol() : Bbo - Dbo);

   U_INTERNAL_DUMP("cmp = %d", cmp)

   A = (cmp > 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= exprline(B) GE expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) GE expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

   bool Bbo = (B->empty() == false),
        Dbo = (D->empty() == false);

   int cmp = (Bbo && Dbo ? B->strtol() - D->strtol() : Bbo - Dbo);

   U_INTERNAL_DUMP("cmp = %d", cmp)

   A = (cmp >= 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= exprline(B) LT expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) LT expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

   bool Bbo = (B->empty() == false),
        Dbo = (D->empty() == false);

   int cmp = (Bbo && Dbo ? B->strtol() - D->strtol() : Bbo - Dbo);

   U_INTERNAL_DUMP("cmp = %d", cmp)

   A = (cmp < 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= exprline(B) LE expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) LE expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

   bool Bbo = (B->empty() == false),
        Dbo = (D->empty() == false);

   int cmp = (Bbo && Dbo ? B->strtol() - D->strtol() : Bbo - Dbo);

   U_INTERNAL_DUMP("cmp = %d", cmp)

   A = (cmp <= 0 ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= exprline(B) PLUS expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) PLUS expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

	A = U_NEW(UString(UStringExt::numberToString(B->strtol() + D->strtol())));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= exprline(B) MINUS expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) MINUS expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

	A = U_NEW(UString(UStringExt::numberToString(B->strtol() - D->strtol())));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= exprline(B) MULT expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) MULT expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

	A = U_NEW(UString(UStringExt::numberToString(B->strtol() * D->strtol())));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= exprline(B) DIV expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) DIV expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

	A = U_NEW(UString(UStringExt::numberToString(B->strtol() / D->strtol())));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= exprline(B) MOD expr(D). {
	U_TRACE(0, "exprline(A) ::= exprline(B) MOD expr(D)")

	U_INTERNAL_ASSERT_POINTER(B)
	U_INTERNAL_ASSERT_POINTER(D)

	U_INTERNAL_DUMP("B = %.*S D = %.*S", U_STRING_TO_TRACE(*B), U_STRING_TO_TRACE(*D))

	A = U_NEW(UString(UStringExt::numberToString(B->strtol() % D->strtol())));

	delete B;
	delete D;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

exprline(A) ::= expr(B). {
	U_TRACE(0, "exprline(A) ::= expr(B)")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

expr(A) ::= NOT expr(B). {
	U_TRACE(0, "expr(A) ::= NOT expr(B)")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = (B->empty() ? U_NEW(UString(U_CONSTANT_TO_PARAM("true"))) : U_NEW(UString));

	delete B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

expr(A) ::= LPAREN exprline(B) RPAREN. {
	U_TRACE(0, "expr(A) ::= LPAREN exprline(B) RPAREN")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}

expr(A) ::= value(B). {
	U_TRACE(0, "expr(A) ::= value(B)")

	U_INTERNAL_DUMP("A = %p B = %p", A, B)

	U_INTERNAL_ASSERT_POINTER(B)

	A = B;

	U_INTERNAL_DUMP("A = %.*S", U_STRING_TO_TRACE(*A))
}
*/
