/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 3 "grammar.y"


extern char *outP;
#include "std.h"
#include "compiler.h"
#include "lex.h"
#include "scratchpad.h"

#include "lpc_incl.h"
#include "simul_efun.h"
#include "generate.h"
#include "master.h"

/* gross. Necessary? - Beek */
#ifdef WIN32
#define MSDOS
#endif
#define YYSTACK_USE_ALLOCA 0
#line 20 "grammar.y.pre"
/*
 * This is the grammar definition of LPC, and its parse tree generator.
 */

/* down to one global :) 
   bits:
      SWITCH_CONTEXT     - we're inside a switch
      LOOP_CONTEXT       - we're inside a loop
      SWITCH_STRINGS     - a string case has been found
      SWITCH_NUMBERS     - a non-zero numeric case has been found
      SWITCH_RANGES      - a range has been found
      SWITCH_DEFAULT     - a default has been found
 */
int context;
int num_refs;
int func_present;
/*
 * bison & yacc don't prototype this in y.tab.h
 */
int yyparse (void);


#line 113 "y.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    L_STRING = 258,                /* L_STRING  */
    L_NUMBER = 259,                /* L_NUMBER  */
    L_REAL = 260,                  /* L_REAL  */
    L_BASIC_TYPE = 261,            /* L_BASIC_TYPE  */
    L_TYPE_MODIFIER = 262,         /* L_TYPE_MODIFIER  */
    L_DEFINED_NAME = 263,          /* L_DEFINED_NAME  */
    L_IDENTIFIER = 264,            /* L_IDENTIFIER  */
    L_EFUN = 265,                  /* L_EFUN  */
    L_INC = 266,                   /* L_INC  */
    L_DEC = 267,                   /* L_DEC  */
    L_ASSIGN = 268,                /* L_ASSIGN  */
    L_LAND = 269,                  /* L_LAND  */
    L_LOR = 270,                   /* L_LOR  */
    L_LSH = 271,                   /* L_LSH  */
    L_RSH = 272,                   /* L_RSH  */
    L_ORDER = 273,                 /* L_ORDER  */
    L_NOT = 274,                   /* L_NOT  */
    L_IF = 275,                    /* L_IF  */
    L_ELSE = 276,                  /* L_ELSE  */
    L_SWITCH = 277,                /* L_SWITCH  */
    L_CASE = 278,                  /* L_CASE  */
    L_DEFAULT = 279,               /* L_DEFAULT  */
    L_RANGE = 280,                 /* L_RANGE  */
    L_DOT_DOT_DOT = 281,           /* L_DOT_DOT_DOT  */
    L_WHILE = 282,                 /* L_WHILE  */
    L_DO = 283,                    /* L_DO  */
    L_FOR = 284,                   /* L_FOR  */
    L_FOREACH = 285,               /* L_FOREACH  */
    L_IN = 286,                    /* L_IN  */
    L_BREAK = 287,                 /* L_BREAK  */
    L_CONTINUE = 288,              /* L_CONTINUE  */
    L_RETURN = 289,                /* L_RETURN  */
    L_ARROW = 290,                 /* L_ARROW  */
    L_INHERIT = 291,               /* L_INHERIT  */
    L_COLON_COLON = 292,           /* L_COLON_COLON  */
    L_ARRAY_OPEN = 293,            /* L_ARRAY_OPEN  */
    L_MAPPING_OPEN = 294,          /* L_MAPPING_OPEN  */
    L_FUNCTION_OPEN = 295,         /* L_FUNCTION_OPEN  */
    L_NEW_FUNCTION_OPEN = 296,     /* L_NEW_FUNCTION_OPEN  */
    L_SSCANF = 297,                /* L_SSCANF  */
    L_CATCH = 298,                 /* L_CATCH  */
    L_PARSE_COMMAND = 299,         /* L_PARSE_COMMAND  */
    L_TIME_EXPRESSION = 300,       /* L_TIME_EXPRESSION  */
    L_CLASS = 301,                 /* L_CLASS  */
    L_NEW = 302,                   /* L_NEW  */
    L_PARAMETER = 303,             /* L_PARAMETER  */
    LOWER_THAN_ELSE = 304,         /* LOWER_THAN_ELSE  */
    L_EQ = 305,                    /* L_EQ  */
    L_NE = 306                     /* L_NE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define L_STRING 258
#define L_NUMBER 259
#define L_REAL 260
#define L_BASIC_TYPE 261
#define L_TYPE_MODIFIER 262
#define L_DEFINED_NAME 263
#define L_IDENTIFIER 264
#define L_EFUN 265
#define L_INC 266
#define L_DEC 267
#define L_ASSIGN 268
#define L_LAND 269
#define L_LOR 270
#define L_LSH 271
#define L_RSH 272
#define L_ORDER 273
#define L_NOT 274
#define L_IF 275
#define L_ELSE 276
#define L_SWITCH 277
#define L_CASE 278
#define L_DEFAULT 279
#define L_RANGE 280
#define L_DOT_DOT_DOT 281
#define L_WHILE 282
#define L_DO 283
#define L_FOR 284
#define L_FOREACH 285
#define L_IN 286
#define L_BREAK 287
#define L_CONTINUE 288
#define L_RETURN 289
#define L_ARROW 290
#define L_INHERIT 291
#define L_COLON_COLON 292
#define L_ARRAY_OPEN 293
#define L_MAPPING_OPEN 294
#define L_FUNCTION_OPEN 295
#define L_NEW_FUNCTION_OPEN 296
#define L_SSCANF 297
#define L_CATCH 298
#define L_PARSE_COMMAND 299
#define L_TIME_EXPRESSION 300
#define L_CLASS 301
#define L_NEW 302
#define L_PARAMETER 303
#define LOWER_THAN_ELSE 304
#define L_EQ 305
#define L_NE 306

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 122 "grammar.y"

    POINTER_INT pointer_int;
    long number;
    float real;
    char *string;
    int type;
    struct { short num_arg; char flags; } argument;
    ident_hash_elem_t *ihe;
    parse_node_t *node;
    function_context_t *contextp;
    struct {
	parse_node_t *node;
        char num;
    } decl; /* 5 */
    struct {
	char num_local;
	char max_num_locals; 
	short context; 
	short save_current_type; 
	short save_exact_types;
    } func_block; /* 8 */

#line 291 "y.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_L_STRING = 3,                   /* L_STRING  */
  YYSYMBOL_L_NUMBER = 4,                   /* L_NUMBER  */
  YYSYMBOL_L_REAL = 5,                     /* L_REAL  */
  YYSYMBOL_L_BASIC_TYPE = 6,               /* L_BASIC_TYPE  */
  YYSYMBOL_L_TYPE_MODIFIER = 7,            /* L_TYPE_MODIFIER  */
  YYSYMBOL_L_DEFINED_NAME = 8,             /* L_DEFINED_NAME  */
  YYSYMBOL_L_IDENTIFIER = 9,               /* L_IDENTIFIER  */
  YYSYMBOL_L_EFUN = 10,                    /* L_EFUN  */
  YYSYMBOL_L_INC = 11,                     /* L_INC  */
  YYSYMBOL_L_DEC = 12,                     /* L_DEC  */
  YYSYMBOL_L_ASSIGN = 13,                  /* L_ASSIGN  */
  YYSYMBOL_L_LAND = 14,                    /* L_LAND  */
  YYSYMBOL_L_LOR = 15,                     /* L_LOR  */
  YYSYMBOL_L_LSH = 16,                     /* L_LSH  */
  YYSYMBOL_L_RSH = 17,                     /* L_RSH  */
  YYSYMBOL_L_ORDER = 18,                   /* L_ORDER  */
  YYSYMBOL_L_NOT = 19,                     /* L_NOT  */
  YYSYMBOL_L_IF = 20,                      /* L_IF  */
  YYSYMBOL_L_ELSE = 21,                    /* L_ELSE  */
  YYSYMBOL_L_SWITCH = 22,                  /* L_SWITCH  */
  YYSYMBOL_L_CASE = 23,                    /* L_CASE  */
  YYSYMBOL_L_DEFAULT = 24,                 /* L_DEFAULT  */
  YYSYMBOL_L_RANGE = 25,                   /* L_RANGE  */
  YYSYMBOL_L_DOT_DOT_DOT = 26,             /* L_DOT_DOT_DOT  */
  YYSYMBOL_L_WHILE = 27,                   /* L_WHILE  */
  YYSYMBOL_L_DO = 28,                      /* L_DO  */
  YYSYMBOL_L_FOR = 29,                     /* L_FOR  */
  YYSYMBOL_L_FOREACH = 30,                 /* L_FOREACH  */
  YYSYMBOL_L_IN = 31,                      /* L_IN  */
  YYSYMBOL_L_BREAK = 32,                   /* L_BREAK  */
  YYSYMBOL_L_CONTINUE = 33,                /* L_CONTINUE  */
  YYSYMBOL_L_RETURN = 34,                  /* L_RETURN  */
  YYSYMBOL_L_ARROW = 35,                   /* L_ARROW  */
  YYSYMBOL_L_INHERIT = 36,                 /* L_INHERIT  */
  YYSYMBOL_L_COLON_COLON = 37,             /* L_COLON_COLON  */
  YYSYMBOL_L_ARRAY_OPEN = 38,              /* L_ARRAY_OPEN  */
  YYSYMBOL_L_MAPPING_OPEN = 39,            /* L_MAPPING_OPEN  */
  YYSYMBOL_L_FUNCTION_OPEN = 40,           /* L_FUNCTION_OPEN  */
  YYSYMBOL_L_NEW_FUNCTION_OPEN = 41,       /* L_NEW_FUNCTION_OPEN  */
  YYSYMBOL_L_SSCANF = 42,                  /* L_SSCANF  */
  YYSYMBOL_L_CATCH = 43,                   /* L_CATCH  */
  YYSYMBOL_L_PARSE_COMMAND = 44,           /* L_PARSE_COMMAND  */
  YYSYMBOL_L_TIME_EXPRESSION = 45,         /* L_TIME_EXPRESSION  */
  YYSYMBOL_L_CLASS = 46,                   /* L_CLASS  */
  YYSYMBOL_L_NEW = 47,                     /* L_NEW  */
  YYSYMBOL_L_PARAMETER = 48,               /* L_PARAMETER  */
  YYSYMBOL_LOWER_THAN_ELSE = 49,           /* LOWER_THAN_ELSE  */
  YYSYMBOL_50_ = 50,                       /* '?'  */
  YYSYMBOL_51_ = 51,                       /* '|'  */
  YYSYMBOL_52_ = 52,                       /* '^'  */
  YYSYMBOL_53_ = 53,                       /* '&'  */
  YYSYMBOL_L_EQ = 54,                      /* L_EQ  */
  YYSYMBOL_L_NE = 55,                      /* L_NE  */
  YYSYMBOL_56_ = 56,                       /* '<'  */
  YYSYMBOL_57_ = 57,                       /* '+'  */
  YYSYMBOL_58_ = 58,                       /* '-'  */
  YYSYMBOL_59_ = 59,                       /* '*'  */
  YYSYMBOL_60_ = 60,                       /* '%'  */
  YYSYMBOL_61_ = 61,                       /* '/'  */
  YYSYMBOL_62_ = 62,                       /* '~'  */
  YYSYMBOL_63_ = 63,                       /* ';'  */
  YYSYMBOL_64_ = 64,                       /* '('  */
  YYSYMBOL_65_ = 65,                       /* ')'  */
  YYSYMBOL_66_ = 66,                       /* ':'  */
  YYSYMBOL_67_ = 67,                       /* ','  */
  YYSYMBOL_68_ = 68,                       /* '{'  */
  YYSYMBOL_69_ = 69,                       /* '}'  */
  YYSYMBOL_70_ = 70,                       /* '$'  */
  YYSYMBOL_71_ = 71,                       /* '['  */
  YYSYMBOL_72_ = 72,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 73,                  /* $accept  */
  YYSYMBOL_all = 74,                       /* all  */
  YYSYMBOL_program = 75,                   /* program  */
  YYSYMBOL_possible_semi_colon = 76,       /* possible_semi_colon  */
  YYSYMBOL_inheritance = 77,               /* inheritance  */
  YYSYMBOL_real = 78,                      /* real  */
  YYSYMBOL_number = 79,                    /* number  */
  YYSYMBOL_optional_star = 80,             /* optional_star  */
  YYSYMBOL_block_or_semi = 81,             /* block_or_semi  */
  YYSYMBOL_identifier = 82,                /* identifier  */
  YYSYMBOL_def = 83,                       /* def  */
  YYSYMBOL_84_1 = 84,                      /* $@1  */
  YYSYMBOL_85_2 = 85,                      /* @2  */
  YYSYMBOL_modifier_change = 86,           /* modifier_change  */
  YYSYMBOL_member_name = 87,               /* member_name  */
  YYSYMBOL_member_name_list = 88,          /* member_name_list  */
  YYSYMBOL_member_list = 89,               /* member_list  */
  YYSYMBOL_90_3 = 90,                      /* $@3  */
  YYSYMBOL_type_decl = 91,                 /* type_decl  */
  YYSYMBOL_92_4 = 92,                      /* @4  */
  YYSYMBOL_new_local_name = 93,            /* new_local_name  */
  YYSYMBOL_atomic_type = 94,               /* atomic_type  */
  YYSYMBOL_basic_type = 95,                /* basic_type  */
  YYSYMBOL_arg_type = 96,                  /* arg_type  */
  YYSYMBOL_new_arg = 97,                   /* new_arg  */
  YYSYMBOL_argument = 98,                  /* argument  */
  YYSYMBOL_argument_list = 99,             /* argument_list  */
  YYSYMBOL_type_modifier_list = 100,       /* type_modifier_list  */
  YYSYMBOL_type = 101,                     /* type  */
  YYSYMBOL_cast = 102,                     /* cast  */
  YYSYMBOL_opt_basic_type = 103,           /* opt_basic_type  */
  YYSYMBOL_name_list = 104,                /* name_list  */
  YYSYMBOL_new_name = 105,                 /* new_name  */
  YYSYMBOL_block = 106,                    /* block  */
  YYSYMBOL_decl_block = 107,               /* decl_block  */
  YYSYMBOL_local_declarations = 108,       /* local_declarations  */
  YYSYMBOL_109_5 = 109,                    /* $@5  */
  YYSYMBOL_new_local_def = 110,            /* new_local_def  */
  YYSYMBOL_single_new_local_def = 111,     /* single_new_local_def  */
  YYSYMBOL_single_new_local_def_with_init = 112, /* single_new_local_def_with_init  */
  YYSYMBOL_local_name_list = 113,          /* local_name_list  */
  YYSYMBOL_statements = 114,               /* statements  */
  YYSYMBOL_statement = 115,                /* statement  */
  YYSYMBOL_while = 116,                    /* while  */
  YYSYMBOL_117_6 = 117,                    /* $@6  */
  YYSYMBOL_do = 118,                       /* do  */
  YYSYMBOL_119_7 = 119,                    /* $@7  */
  YYSYMBOL_for = 120,                      /* for  */
  YYSYMBOL_121_8 = 121,                    /* $@8  */
  YYSYMBOL_foreach_var = 122,              /* foreach_var  */
  YYSYMBOL_foreach_vars = 123,             /* foreach_vars  */
  YYSYMBOL_foreach = 124,                  /* foreach  */
  YYSYMBOL_125_9 = 125,                    /* $@9  */
  YYSYMBOL_for_expr = 126,                 /* for_expr  */
  YYSYMBOL_first_for_expr = 127,           /* first_for_expr  */
  YYSYMBOL_switch = 128,                   /* switch  */
  YYSYMBOL_129_10 = 129,                   /* $@10  */
  YYSYMBOL_switch_block = 130,             /* switch_block  */
  YYSYMBOL_case = 131,                     /* case  */
  YYSYMBOL_case_label = 132,               /* case_label  */
  YYSYMBOL_constant = 133,                 /* constant  */
  YYSYMBOL_comma_expr = 134,               /* comma_expr  */
  YYSYMBOL_expr0 = 135,                    /* expr0  */
  YYSYMBOL_return = 136,                   /* return  */
  YYSYMBOL_expr_list = 137,                /* expr_list  */
  YYSYMBOL_expr_list_node = 138,           /* expr_list_node  */
  YYSYMBOL_expr_list2 = 139,               /* expr_list2  */
  YYSYMBOL_expr_list3 = 140,               /* expr_list3  */
  YYSYMBOL_expr_list4 = 141,               /* expr_list4  */
  YYSYMBOL_assoc_pair = 142,               /* assoc_pair  */
  YYSYMBOL_lvalue = 143,                   /* lvalue  */
  YYSYMBOL_l_new_function_open = 144,      /* l_new_function_open  */
  YYSYMBOL_expr4 = 145,                    /* expr4  */
  YYSYMBOL_146_11 = 146,                   /* @11  */
  YYSYMBOL_147_12 = 147,                   /* @12  */
  YYSYMBOL_expr_or_block = 148,            /* expr_or_block  */
  YYSYMBOL_catch = 149,                    /* catch  */
  YYSYMBOL_150_13 = 150,                   /* @13  */
  YYSYMBOL_sscanf = 151,                   /* sscanf  */
  YYSYMBOL_parse_command = 152,            /* parse_command  */
  YYSYMBOL_time_expression = 153,          /* time_expression  */
  YYSYMBOL_154_14 = 154,                   /* @14  */
  YYSYMBOL_lvalue_list = 155,              /* lvalue_list  */
  YYSYMBOL_string = 156,                   /* string  */
  YYSYMBOL_string_con1 = 157,              /* string_con1  */
  YYSYMBOL_string_con2 = 158,              /* string_con2  */
  YYSYMBOL_class_init = 159,               /* class_init  */
  YYSYMBOL_opt_class_init = 160,           /* opt_class_init  */
  YYSYMBOL_function_call = 161,            /* function_call  */
  YYSYMBOL_162_15 = 162,                   /* @15  */
  YYSYMBOL_163_16 = 163,                   /* @16  */
  YYSYMBOL_164_17 = 164,                   /* @17  */
  YYSYMBOL_165_18 = 165,                   /* @18  */
  YYSYMBOL_166_19 = 166,                   /* @19  */
  YYSYMBOL_167_20 = 167,                   /* @20  */
  YYSYMBOL_efun_override = 168,            /* efun_override  */
  YYSYMBOL_function_name = 169,            /* function_name  */
  YYSYMBOL_cond = 170,                     /* cond  */
  YYSYMBOL_optional_else_part = 171        /* optional_else_part  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1728

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  73
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  99
/* YYNRULES -- Number of rules.  */
#define YYNRULES  251
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  479

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   306


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,    70,    60,    53,     2,
      64,    65,    59,    57,    67,    58,     2,    61,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    66,    63,
      56,     2,     2,    50,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    71,     2,    72,    52,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    68,    51,    69,    62,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    54,    55
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   211,   211,   219,   225,   231,   233,   243,   330,   338,
     347,   351,   359,   367,   372,   380,   385,   390,   416,   389,
     474,   481,   482,   483,   486,   504,   518,   519,   522,   525,
     524,   534,   533,   614,   615,   633,   634,   651,   668,   672,
     676,   683,   692,   706,   711,   712,   731,   743,   758,   762,
     772,   781,   789,   791,   798,   799,   803,   829,   885,   895,
     895,   895,   899,   905,   904,   926,   938,   973,   985,  1018,
    1024,  1036,  1040,  1047,  1055,  1068,  1069,  1070,  1071,  1072,
    1073,  1079,  1084,  1107,  1121,  1120,  1136,  1135,  1151,  1150,
    1176,  1198,  1209,  1227,  1233,  1245,  1244,  1266,  1270,  1274,
    1280,  1290,  1289,  1330,  1337,  1345,  1353,  1361,  1376,  1391,
    1405,  1422,  1437,  1454,  1459,  1464,  1469,  1474,  1479,  1488,
    1493,  1498,  1503,  1508,  1513,  1518,  1523,  1528,  1533,  1538,
    1543,  1548,  1556,  1561,  1570,  1596,  1602,  1627,  1634,  1641,
    1667,  1672,  1696,  1719,  1734,  1779,  1817,  1822,  1827,  1998,
    2093,  2174,  2179,  2275,  2297,  2319,  2342,  2352,  2364,  2389,
    2412,  2434,  2435,  2436,  2437,  2438,  2439,  2443,  2450,  2472,
    2476,  2481,  2489,  2494,  2502,  2509,  2523,  2528,  2533,  2541,
    2552,  2571,  2579,  2695,  2696,  2705,  2706,  2749,  2766,  2772,
    2771,  2803,  2828,  2836,  2841,  2849,  2857,  2862,  2867,  2913,
    2968,  2969,  2974,  2976,  2975,  3032,  3070,  3165,  3188,  3197,
    3209,  3214,  3223,  3222,  3238,  3248,  3260,  3259,  3275,  3281,
    3296,  3305,  3306,  3311,  3319,  3320,  3327,  3339,  3343,  3354,
    3353,  3369,  3368,  3383,  3419,  3439,  3438,  3508,  3507,  3581,
    3580,  3634,  3633,  3665,  3691,  3707,  3708,  3723,  3739,  3755,
    3790,  3795
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "L_STRING", "L_NUMBER",
  "L_REAL", "L_BASIC_TYPE", "L_TYPE_MODIFIER", "L_DEFINED_NAME",
  "L_IDENTIFIER", "L_EFUN", "L_INC", "L_DEC", "L_ASSIGN", "L_LAND",
  "L_LOR", "L_LSH", "L_RSH", "L_ORDER", "L_NOT", "L_IF", "L_ELSE",
  "L_SWITCH", "L_CASE", "L_DEFAULT", "L_RANGE", "L_DOT_DOT_DOT", "L_WHILE",
  "L_DO", "L_FOR", "L_FOREACH", "L_IN", "L_BREAK", "L_CONTINUE",
  "L_RETURN", "L_ARROW", "L_INHERIT", "L_COLON_COLON", "L_ARRAY_OPEN",
  "L_MAPPING_OPEN", "L_FUNCTION_OPEN", "L_NEW_FUNCTION_OPEN", "L_SSCANF",
  "L_CATCH", "L_PARSE_COMMAND", "L_TIME_EXPRESSION", "L_CLASS", "L_NEW",
  "L_PARAMETER", "LOWER_THAN_ELSE", "'?'", "'|'", "'^'", "'&'", "L_EQ",
  "L_NE", "'<'", "'+'", "'-'", "'*'", "'%'", "'/'", "'~'", "';'", "'('",
  "')'", "':'", "','", "'{'", "'}'", "'$'", "'['", "']'", "$accept", "all",
  "program", "possible_semi_colon", "inheritance", "real", "number",
  "optional_star", "block_or_semi", "identifier", "def", "$@1", "@2",
  "modifier_change", "member_name", "member_name_list", "member_list",
  "$@3", "type_decl", "@4", "new_local_name", "atomic_type", "basic_type",
  "arg_type", "new_arg", "argument", "argument_list", "type_modifier_list",
  "type", "cast", "opt_basic_type", "name_list", "new_name", "block",
  "decl_block", "local_declarations", "$@5", "new_local_def",
  "single_new_local_def", "single_new_local_def_with_init",
  "local_name_list", "statements", "statement", "while", "$@6", "do",
  "$@7", "for", "$@8", "foreach_var", "foreach_vars", "foreach", "$@9",
  "for_expr", "first_for_expr", "switch", "$@10", "switch_block", "case",
  "case_label", "constant", "comma_expr", "expr0", "return", "expr_list",
  "expr_list_node", "expr_list2", "expr_list3", "expr_list4", "assoc_pair",
  "lvalue", "l_new_function_open", "expr4", "@11", "@12", "expr_or_block",
  "catch", "@13", "sscanf", "parse_command", "time_expression", "@14",
  "lvalue_list", "string", "string_con1", "string_con2", "class_init",
  "opt_class_init", "function_call", "@15", "@16", "@17", "@18", "@19",
  "@20", "efun_override", "function_name", "cond", "optional_else_part", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-389)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-246)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -389,    42,   142,  -389,    49,  -389,    77,  -389,  -389,   129,
      29,  -389,  -389,  -389,  -389,    11,   190,  -389,  -389,  -389,
    -389,  -389,   214,    95,    85,  -389,    11,    -9,   187,   100,
     143,   201,  -389,  -389,     7,  -389,    29,   -16,    11,  -389,
    -389,  -389,  1515,   169,   214,  -389,  -389,  -389,  -389,   232,
    -389,  -389,   235,     9,    81,   243,   468,   468,  1515,   214,
    1037,   481,  1515,  -389,   254,  -389,   263,  -389,   268,  -389,
    1515,  1515,   209,   281,  -389,  -389,   247,  1515,  1281,   248,
     256,   121,  -389,  -389,  -389,  -389,  -389,   187,  -389,   297,
     313,   178,   300,    12,  1515,   214,   314,  -389,   180,  1107,
    -389,     1,  -389,  -389,  -389,  1350,   288,  -389,   312,   869,
     308,   315,  -389,   258,  1281,   297,  1515,   110,  1515,   110,
     340,  -389,  -389,    89,   331,  1515,    29,   -22,  -389,   214,
    -389,  1515,  1515,  1515,  1515,  1515,  1515,  1515,  1515,  1515,
    1515,  1515,  1515,  1515,  1515,  1515,  1515,  1515,  -389,  -389,
    1515,   323,  1515,   214,  1175,  -389,  -389,  -389,  -389,  -389,
    -389,    29,  -389,   326,    90,  -389,  -389,  1281,  -389,   178,
    1243,  -389,  -389,  -389,   327,   899,  1515,   328,   551,   329,
    1515,  1550,  1515,  -389,  -389,  -389,  1572,  -389,   339,  1243,
    -389,  -389,    -8,   341,  -389,  1515,  -389,   524,   384,   229,
     229,   177,  1007,  1211,  1482,  1490,   294,   294,   177,   217,
     217,  -389,  -389,  -389,  1281,  -389,   293,   332,  1515,    55,
    1243,  1243,   366,  -389,  -389,   178,    29,   343,   344,  -389,
    -389,  1281,  -389,  -389,  -389,  1281,  1515,   165,   691,  1515,
    -389,  -389,   348,   354,  -389,   175,  1515,   367,  1515,  -389,
     104,   325,  -389,   369,   381,  -389,    46,  -389,   214,   337,
     342,   322,  -389,  1598,  -389,    47,   383,   393,   396,  -389,
     397,   398,   370,   400,  1311,  -389,  -389,  -389,  -389,   362,
     761,  -389,  -389,  -389,  -389,  -389,   162,  -389,  -389,  1620,
     237,   238,  -389,  -389,  -389,  1281,  -389,  1243,   411,  -389,
    1515,  -389,    94,  -389,  -389,  -389,  -389,  -389,  -389,  -389,
      29,  -389,  -389,   468,   399,  -389,  1515,  1515,  1515,   831,
     969,   195,  -389,  -389,  -389,   207,    29,  -389,  -389,  -389,
    1515,  -389,   214,  -389,  1243,   401,  1515,  -389,    97,   105,
    -389,  -389,   403,  -389,   241,   242,   249,   438,    29,   455,
    -389,  -389,   409,   412,  -389,  -389,  -389,   413,   457,  -389,
     366,   427,   432,  1598,   430,  -389,   433,  -389,   124,  -389,
    -389,  -389,   831,  -389,  -389,   435,   366,  1515,  1379,   195,
    1515,   484,    29,  -389,   436,  1515,  -389,  -389,   482,   434,
     831,  1515,  -389,  1281,   441,  -389,  1145,  1515,  -389,  -389,
    1281,   831,  -389,  -389,  -389,   250,  1447,  -389,  1281,  -389,
     220,   447,   448,   831,    25,   446,   621,  -389,  -389,  -389,
    -389,   510,    66,   513,   523,    66,   113,  1076,   473,  -389,
     621,   462,   621,   831,  -389,   467,  -389,  -389,   938,    59,
    -389,   144,   144,   144,   144,   144,   144,   144,   144,   144,
     144,   144,   144,   144,   144,  -389,  -389,  -389,  -389,  -389,
    -389,  -389,   469,   144,   239,   239,   368,  1415,  1644,  1666,
    1218,  1218,   368,   233,   233,  -389,  -389,  -389,  -389
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,     0,    48,     1,    48,    21,     5,    23,    22,    53,
      10,    49,     6,     3,    35,     0,     0,    24,    38,    52,
      50,    11,     0,     0,    54,   224,     0,     0,   221,    36,
      37,     0,    15,    16,    56,    20,    10,     0,     0,     7,
     225,    31,     0,     0,     0,    55,   222,   223,    28,     0,
       9,     8,   203,   186,   187,     0,     0,     0,     0,     0,
       0,     0,     0,   183,     0,   212,     0,   216,     0,   188,
       0,     0,     0,     0,   166,   165,     0,     0,    57,     0,
       0,   161,   202,   162,   163,   164,   200,   220,   185,     0,
       0,    43,    56,     0,     0,     0,     0,   235,     0,     0,
     154,   182,   155,   156,   246,   172,     0,   174,   170,     0,
       0,   177,   179,     0,   132,   184,     0,     0,     0,     0,
     231,   158,   157,    35,     0,     0,    10,     0,   189,     0,
     153,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   159,   160,
       0,     0,     0,     0,     0,   229,   237,    34,    33,    42,
      39,    10,    46,     0,    44,    32,    29,   135,   247,    43,
       0,   244,   243,   173,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    62,   210,   213,     0,   217,     0,     0,
      36,    37,     0,     0,   201,     0,   248,   138,   137,   146,
     147,   144,     0,   139,   140,   141,   142,   143,   145,   148,
     149,   150,   151,   152,   134,   205,     0,   191,     0,     0,
       0,     0,    40,    18,    45,     0,    10,     0,     0,   209,
     175,   181,   208,   180,   207,   133,     0,     0,     0,     0,
     227,   227,     0,     0,    51,     0,     0,     0,     0,   239,
       0,     0,   199,     0,     0,    41,     0,    47,     0,    26,
       0,     0,   236,   218,   211,     0,     0,     0,     0,    86,
       0,     0,     0,     0,     0,    81,    63,    59,    80,     0,
       0,    76,    77,    60,    61,    78,     0,    79,    75,     0,
       0,     0,   232,   241,   190,   136,   206,     0,     0,   198,
       0,   196,     0,   230,   238,    14,    13,    19,    12,    25,
      10,    30,   204,     0,     0,    73,     0,     0,     0,     0,
       0,     0,    82,    83,   167,     0,    10,    58,    72,    74,
       0,   233,     0,   234,     0,     0,     0,   197,     0,     0,
     192,    27,   218,   214,     0,     0,     0,     0,    10,     0,
     100,    99,     0,    98,    90,    92,    91,    93,     0,   168,
       0,    69,     0,   218,     0,   228,     0,   240,     0,   193,
     195,   219,     0,   101,    84,     0,     0,     0,     0,     0,
       0,    65,    10,    64,     0,     0,   242,   194,   250,     0,
       0,     0,    67,    68,     0,    94,     0,     0,    70,   215,
     226,     0,   249,    62,    85,     0,     0,    95,    66,   251,
       0,     0,     0,     0,     0,     0,     0,    87,    88,    96,
     128,     0,     0,     0,     0,     0,     0,   111,   112,   110,
       0,     0,     0,     0,   130,     0,   129,   131,     0,     0,
     106,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   104,   102,   103,    89,   109,
     127,   108,     0,     0,   120,   121,   118,   113,   114,   115,
     116,   117,   119,   122,   123,   124,   125,   126,   107
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -389,  -389,  -389,  -389,  -389,  -389,  -389,    -6,  -389,    17,
    -389,  -389,  -389,  -389,  -389,   224,  -389,  -389,  -389,  -389,
    -209,  -389,    -7,  -295,   311,   375,  -389,   533,  -389,  -389,
    -389,   511,  -389,  -102,  -389,   145,  -389,  -389,   226,  -389,
     167,   270,  -228,  -389,  -389,  -389,  -389,  -389,  -389,   179,
    -389,  -389,  -389,  -338,  -389,  -389,  -389,  -111,   154,  -388,
    1265,   -35,   -39,  -389,  -154,  -166,   414,  -389,  -389,   387,
     -51,  -389,   -49,  -389,  -389,   449,  -389,  -389,  -389,  -389,
    -389,  -389,  -289,  -389,   -14,   -15,  -389,   330,  -389,  -389,
    -389,  -389,  -389,  -389,  -389,   505,  -389,  -389,  -389
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,     2,    13,     5,    74,    75,   258,   307,    76,
       6,    43,   256,     7,   259,   260,    93,   226,     8,    48,
     159,    18,   160,   161,   162,   163,   164,     9,    10,    77,
      20,    23,    24,   277,   278,   238,   326,   361,   356,   350,
     362,   279,   430,   281,   390,   282,   319,   283,   433,   357,
     358,   284,   413,   351,   352,   285,   389,   431,   432,   426,
     427,   286,   114,   287,   106,   107,   108,   110,   111,   112,
      79,    80,    81,   195,    96,   185,    82,   117,    83,    84,
      85,   119,   314,    86,   428,    87,   365,   290,    88,   220,
     189,   170,   221,   297,   334,    89,    90,   288,   402
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      28,    27,    19,    78,    22,   100,   102,   101,   101,   230,
     280,    28,    37,   255,    25,   184,   228,   184,    14,   103,
      42,   105,   109,    28,    47,   348,   348,   113,    25,   420,
      44,   121,   122,    31,   435,   242,   153,   127,   130,    34,
     394,    38,     3,   194,   421,   180,   -15,   305,    38,    46,
     422,   462,   280,   371,    39,   167,     4,   243,   124,   180,
      94,    92,    25,   420,   127,   126,   253,   254,   412,    25,
     420,   -17,   154,    97,   384,    26,   104,   181,   421,   186,
     251,   165,   230,   423,   348,   421,   166,   424,    21,   425,
     192,   347,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   306,
     315,   214,   168,   105,   183,   172,   224,   423,   -16,   219,
     193,   424,   180,   425,   423,   461,    95,   252,   424,   298,
     425,   105,  -182,  -182,  -182,    14,   105,   231,   439,   109,
      12,   235,    -2,   335,   388,  -245,   196,   237,   420,     4,
     105,   381,    36,  -203,   308,   222,   153,   225,    35,   312,
     245,   180,   404,   421,   180,    15,   340,   392,   -15,   369,
     217,   180,   180,   409,   182,    16,   299,   370,   183,   440,
     366,   105,   105,   250,    14,   419,   157,   158,    32,    33,
      40,   180,   154,   133,   134,    17,   387,   263,    29,    30,
     289,    14,   423,   354,   355,   458,   424,   295,   463,   105,
      49,   -16,    25,    50,    51,   123,   302,    53,    54,    55,
      56,    57,    32,    33,   124,   329,    14,   171,    58,   180,
     264,   276,   180,    91,   143,   144,   145,   146,   147,   325,
     294,   124,   180,   414,   415,    94,    59,    60,    61,    62,
      63,    64,    65,    66,    67,   124,    68,    69,   105,   148,
     149,   150,   342,   338,   101,   339,   124,    70,   125,    41,
     359,    71,    95,    72,   180,   309,   145,   146,   147,    73,
      98,   344,   345,   346,   129,   353,   143,   144,   145,   146,
     147,   363,   452,   453,   454,   105,   450,   451,   452,   453,
     454,   368,   331,   333,   332,   332,   372,   373,   180,   180,
     133,   134,   135,    42,   374,   411,   180,   180,   116,   455,
     360,   457,   151,   152,   179,   180,    49,   118,    25,    50,
      51,    52,   120,    53,    54,    55,    56,    57,   393,   190,
     191,   396,   376,   353,    58,   128,   400,   240,   241,   364,
     142,   143,   144,   145,   146,   147,   405,   174,   408,   247,
     248,   155,    59,    60,    61,    62,    63,    64,    65,    66,
      67,   353,    68,    69,   157,   158,   360,   156,   169,   175,
     177,   300,   178,    70,   441,   442,   188,    71,   215,    72,
     183,   223,   229,   232,   234,    73,   249,   301,   131,    28,
     133,   134,   135,   276,   310,   311,   244,    28,   261,   262,
      28,    37,    49,   292,    25,    50,    51,    52,   293,    53,
      54,    55,    56,    57,    28,   450,   451,   452,   453,   454,
      58,   327,   296,   322,   303,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   304,   316,    59,    60,
      61,    62,    63,    64,    65,    66,    67,   317,    68,    69,
     318,   320,   321,   323,   343,   375,   367,   336,   377,    70,
     313,    25,   378,    71,    52,    72,    53,    54,    55,   180,
     379,    73,    49,   337,    25,    50,    51,    52,   380,    53,
      54,    55,    56,    57,   382,   383,   385,   397,   386,   391,
      58,   399,   403,   401,   406,    59,    60,    61,    62,    63,
     417,    65,   429,   418,   434,    68,    69,   436,    59,    60,
      61,    62,    63,    64,    65,    66,    67,   437,    68,    69,
      38,   456,    99,   459,   341,   478,   257,    11,    73,    70,
     133,   134,   135,    71,   227,    72,   349,    45,   410,   398,
     328,    73,    49,  -176,    25,    50,    51,    52,   395,    53,
      54,    55,    56,    57,   416,   233,   216,   115,   187,     0,
      58,   291,     0,     0,     0,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,     0,     0,    59,    60,
      61,    62,    63,    64,    65,    66,    67,     0,    68,    69,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    70,
       0,     0,     0,    71,     0,    72,     0,     0,     0,     0,
       0,    73,    49,  -178,    25,    50,    51,    52,     0,    53,
      54,    55,    56,    57,     0,     0,     0,     0,     0,     0,
      58,   266,     0,   267,   414,   415,     0,     0,   268,   269,
     270,   271,     0,   272,   273,   274,     0,     0,    59,    60,
      61,    62,    63,    64,    65,    66,    67,     0,    68,    69,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    70,
       0,     0,     0,    71,   275,    72,     0,     0,     0,   183,
    -105,    73,   265,     0,    25,    50,    51,   123,     0,    53,
      54,    55,    56,    57,     0,     0,     0,     0,     0,     0,
      58,   266,     0,   267,     0,     0,     0,     0,   268,   269,
     270,   271,     0,   272,   273,   274,     0,     0,    59,    60,
      61,    62,    63,    64,    65,    66,    67,   124,    68,    69,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    70,
       0,     0,     0,    71,   275,    72,     0,     0,     0,   183,
     -71,    73,   265,     0,    25,    50,    51,    52,     0,    53,
      54,    55,    56,    57,     0,     0,     0,     0,     0,     0,
      58,   266,     0,   267,     0,     0,     0,     0,   268,   269,
     270,   271,     0,   272,   273,   274,     0,     0,    59,    60,
      61,    62,    63,    64,    65,    66,    67,     0,    68,    69,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    70,
       0,     0,     0,    71,   275,    72,     0,     0,     0,   183,
     -71,    73,    49,     0,    25,    50,    51,    52,     0,    53,
      54,    55,    56,    57,     0,     0,     0,     0,     0,     0,
      58,   266,     0,   267,     0,     0,     0,     0,   268,   269,
     270,   271,     0,   272,   273,   274,     0,     0,    59,    60,
      61,    62,    63,    64,    65,    66,    67,     0,    68,    69,
       0,     0,     0,   131,   132,   133,   134,   135,     0,    70,
       0,     0,     0,    71,   275,    72,     0,     0,     0,   183,
      49,    73,    25,    50,    51,    52,     0,    53,    54,    55,
      56,    57,     0,     0,     0,     0,     0,     0,    58,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,     0,     0,     0,     0,   176,    59,    60,    61,    62,
      63,    64,    65,    66,    67,     0,    68,    69,     0,     0,
       0,     0,     0,     0,   441,   442,   443,    70,     0,     0,
       0,    71,     0,    72,  -171,     0,     0,     0,  -171,    73,
      49,     0,    25,    50,    51,   123,     0,    53,    54,    55,
      56,    57,     0,     0,     0,     0,     0,     0,    58,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
       0,     0,     0,   460,     0,     0,    59,    60,    61,    62,
      63,    64,    65,    66,    67,   124,    68,    69,     0,     0,
       0,   131,   132,   133,   134,   135,     0,    70,     0,     0,
       0,    71,   -97,    72,     0,     0,     0,     0,    49,    73,
      25,    50,    51,    52,     0,    53,    54,    55,    56,    57,
       0,     0,     0,     0,     0,     0,    58,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,     0,
       0,     0,     0,   246,    59,    60,    61,    62,    63,    64,
      65,    66,    67,     0,    68,    69,     0,     0,     0,     0,
       0,     0,   441,   442,   443,    70,     0,     0,     0,    71,
       0,    72,     0,     0,     0,     0,  -169,    73,    49,     0,
      25,    50,    51,    52,     0,    53,    54,    55,    56,    57,
       0,     0,     0,     0,     0,     0,    58,   444,   445,   446,
     447,   448,   449,   450,   451,   452,   453,   454,     0,     0,
       0,     0,     0,     0,    59,    60,    61,    62,    63,    64,
      65,    66,    67,     0,    68,    69,     0,     0,     0,   131,
     132,   133,   134,   135,     0,    70,   125,     0,     0,    71,
       0,    72,     0,     0,     0,     0,    49,    73,    25,    50,
      51,    52,     0,    53,    54,    55,    56,    57,     0,     0,
       0,     0,     0,     0,    58,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,     0,     0,     0,
     407,     0,    59,    60,    61,    62,    63,    64,    65,    66,
      67,     0,    68,    69,     0,     0,     0,   133,   134,   135,
       0,   218,     0,    70,   441,   442,   443,    71,     0,    72,
       0,     0,     0,     0,    49,    73,    25,    50,    51,    52,
       0,    53,    54,    55,    56,    57,     0,     0,     0,     0,
       0,     0,    58,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,     0,   449,   450,   451,   452,   453,   454,
      59,    60,    61,    62,    63,    64,    65,    66,    67,     0,
      68,    69,     0,     0,     0,   131,   132,   133,   134,   135,
       0,    70,     0,     0,     0,    71,     0,    72,  -169,     0,
       0,     0,    49,    73,    25,    50,    51,    52,     0,    53,
      54,    55,    56,    57,     0,     0,     0,     0,     0,     0,
      58,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,     0,     0,     0,     0,     0,    59,    60,
      61,    62,    63,    64,    65,    66,    67,     0,    68,    69,
       0,     0,     0,     0,   131,   132,   133,   134,   135,    70,
       0,     0,     0,    71,   324,    72,   173,     0,     0,     0,
      49,    73,    25,    50,    51,    52,     0,    53,    54,    55,
      56,    57,     0,     0,     0,     0,     0,     0,    58,     0,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,     0,     0,     0,     0,    59,    60,    61,    62,
      63,    64,    65,    66,    67,     0,    68,    69,     0,     0,
       0,   441,   442,   443,     0,     0,     0,    70,     0,     0,
       0,    71,   -97,    72,     0,     0,     0,     0,    49,    73,
      25,    50,    51,    52,     0,    53,    54,    55,    56,    57,
       0,     0,     0,     0,     0,     0,    58,   445,   446,   447,
     448,   449,   450,   451,   452,   453,   454,     0,     0,     0,
       0,     0,     0,     0,    59,    60,    61,    62,    63,    64,
      65,    66,    67,     0,    68,    69,     0,     0,   133,   134,
     135,     0,     0,     0,     0,    70,   133,   134,   135,    71,
       0,    72,   -97,     0,     0,     0,    49,    73,    25,    50,
      51,    52,     0,    53,    54,    55,    56,    57,     0,     0,
       0,     0,     0,     0,    58,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   140,   141,   142,   143,   144,   145,
     146,   147,    59,    60,    61,    62,    63,    64,    65,    66,
      67,     0,    68,    69,   131,   132,   133,   134,   135,     0,
       0,     0,     0,    70,     0,     0,     0,    71,     0,    72,
       0,     0,     0,     0,     0,    73,   131,   132,   133,   134,
     135,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   131,   132,   133,   134,   135,   236,     0,     0,
       0,     0,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   131,   132,   133,   134,   135,   239,
       0,     0,     0,     0,     0,     0,     0,     0,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     441,   442,   443,     0,     0,   313,     0,     0,     0,     0,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   441,   442,   443,     0,     0,   330,     0,     0,
     438,     0,     0,     0,     0,     0,     0,   446,   447,   448,
     449,   450,   451,   452,   453,   454,   464,   465,   466,   467,
     468,   469,   470,   471,   472,   473,   474,   475,   476,   477,
     447,   448,   449,   450,   451,   452,   453,   454,   438
};

static const yytype_int16 yycheck[] =
{
      15,    15,     9,    42,    10,    56,    57,    56,    57,   175,
     238,    26,    26,   222,     3,   117,   170,   119,     6,    58,
      13,    60,    61,    38,    38,   320,   321,    62,     3,     4,
      36,    70,    71,    16,   422,   189,    35,    72,    77,    22,
     378,    57,     0,    65,    19,    67,    37,     1,    57,    65,
      25,   439,   280,   342,    63,    94,     7,    65,    46,    67,
      13,    44,     3,     4,    99,    72,   220,   221,   406,     3,
       4,    64,    71,    64,   363,    64,    59,   116,    19,   118,
      25,    69,   248,    58,   379,    19,    93,    62,    59,    64,
     125,   319,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,    63,
      63,   150,    95,   152,    68,    98,    26,    58,    37,   154,
     126,    62,    67,    64,    58,    66,    37,    72,    62,    25,
      64,   170,    11,    12,    13,     6,   175,   176,    25,   178,
      63,   180,     0,   297,   372,    64,   129,   182,     4,     7,
     189,   360,    67,    64,   256,   161,    35,    67,    63,   261,
     195,    67,   390,    19,    67,    36,    72,   376,    68,    72,
     153,    67,    67,   401,    64,    46,    72,    72,    68,    66,
     334,   220,   221,   218,     6,   413,     8,     9,     8,     9,
       3,    67,    71,    16,    17,    66,    72,   236,     8,     9,
     239,     6,    58,     8,     9,   433,    62,   246,    64,   248,
       1,    68,     3,     4,     5,     6,   251,     8,     9,    10,
      11,    12,     8,     9,    46,    63,     6,    47,    19,    67,
      65,   238,    67,    64,    57,    58,    59,    60,    61,   274,
      65,    46,    67,    23,    24,    13,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,   297,    11,
      12,    13,   313,   298,   313,   300,    46,    58,    59,    68,
      63,    62,    37,    64,    67,   258,    59,    60,    61,    70,
      37,   316,   317,   318,    37,   320,    57,    58,    59,    60,
      61,   330,    59,    60,    61,   334,    57,    58,    59,    60,
      61,   336,    65,    65,    67,    67,    65,    65,    67,    67,
      16,    17,    18,    13,    65,    65,    67,    67,    64,   430,
     326,   432,    66,    67,    66,    67,     1,    64,     3,     4,
       5,     6,    64,     8,     9,    10,    11,    12,   377,     8,
       9,   380,   348,   378,    19,    64,   385,     8,     9,   332,
      56,    57,    58,    59,    60,    61,   391,    69,   397,    66,
      67,    64,    37,    38,    39,    40,    41,    42,    43,    44,
      45,   406,    47,    48,     8,     9,   382,    64,    64,    67,
      72,    56,    67,    58,    16,    17,    46,    62,    65,    64,
      68,    65,    65,    65,    65,    70,    64,    72,    14,   414,
      16,    17,    18,   410,    67,    63,    65,   422,    65,    65,
     425,   425,     1,    65,     3,     4,     5,     6,    64,     8,
       9,    10,    11,    12,   439,    57,    58,    59,    60,    61,
      19,    69,    65,    63,    65,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    65,    64,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    64,    47,    48,
      64,    64,    64,    63,    65,    27,    65,    56,    13,    58,
      67,     3,    63,    62,     6,    64,     8,     9,    10,    67,
      67,    70,     1,    72,     3,     4,     5,     6,    31,     8,
       9,    10,    11,    12,    67,    63,    66,    13,    65,    64,
      19,    65,    68,    21,    63,    37,    38,    39,    40,    41,
      63,    43,    66,    65,     4,    47,    48,     4,    37,    38,
      39,    40,    41,    42,    43,    44,    45,     4,    47,    48,
      57,    69,    64,    66,   310,    66,   225,     4,    70,    58,
      16,    17,    18,    62,   169,    64,   320,    36,   403,   382,
     280,    70,     1,    72,     3,     4,     5,     6,   379,     8,
       9,    10,    11,    12,   410,   178,   152,    62,   119,    -1,
      19,   241,    -1,    -1,    -1,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    -1,    -1,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    -1,    47,    48,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      -1,    -1,    -1,    62,    -1,    64,    -1,    -1,    -1,    -1,
      -1,    70,     1,    72,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    -1,    22,    23,    24,    -1,    -1,    27,    28,
      29,    30,    -1,    32,    33,    34,    -1,    -1,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    -1,    47,    48,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      -1,    -1,    -1,    62,    63,    64,    -1,    -1,    -1,    68,
      69,    70,     1,    -1,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    -1,    22,    -1,    -1,    -1,    -1,    27,    28,
      29,    30,    -1,    32,    33,    34,    -1,    -1,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      -1,    -1,    -1,    62,    63,    64,    -1,    -1,    -1,    68,
      69,    70,     1,    -1,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    -1,    22,    -1,    -1,    -1,    -1,    27,    28,
      29,    30,    -1,    32,    33,    34,    -1,    -1,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    -1,    47,    48,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      -1,    -1,    -1,    62,    63,    64,    -1,    -1,    -1,    68,
      69,    70,     1,    -1,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    -1,    22,    -1,    -1,    -1,    -1,    27,    28,
      29,    30,    -1,    32,    33,    34,    -1,    -1,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    -1,    47,    48,
      -1,    -1,    -1,    14,    15,    16,    17,    18,    -1,    58,
      -1,    -1,    -1,    62,    63,    64,    -1,    -1,    -1,    68,
       1,    70,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    19,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    -1,    -1,    -1,    -1,    66,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    -1,    47,    48,    -1,    -1,
      -1,    -1,    -1,    -1,    16,    17,    18,    58,    -1,    -1,
      -1,    62,    -1,    64,    65,    -1,    -1,    -1,    69,    70,
       1,    -1,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    19,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      -1,    -1,    -1,    65,    -1,    -1,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    -1,    -1,
      -1,    14,    15,    16,    17,    18,    -1,    58,    -1,    -1,
      -1,    62,    63,    64,    -1,    -1,    -1,    -1,     1,    70,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
      -1,    -1,    -1,    66,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    -1,    47,    48,    -1,    -1,    -1,    -1,
      -1,    -1,    16,    17,    18,    58,    -1,    -1,    -1,    62,
      -1,    64,    -1,    -1,    -1,    -1,    69,    70,     1,    -1,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    -1,    -1,
      -1,    -1,    -1,    -1,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    -1,    47,    48,    -1,    -1,    -1,    14,
      15,    16,    17,    18,    -1,    58,    59,    -1,    -1,    62,
      -1,    64,    -1,    -1,    -1,    -1,     1,    70,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    -1,    -1,
      65,    -1,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    -1,    47,    48,    -1,    -1,    -1,    16,    17,    18,
      -1,    56,    -1,    58,    16,    17,    18,    62,    -1,    64,
      -1,    -1,    -1,    -1,     1,    70,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    19,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    -1,    56,    57,    58,    59,    60,    61,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    -1,
      47,    48,    -1,    -1,    -1,    14,    15,    16,    17,    18,
      -1,    58,    -1,    -1,    -1,    62,    -1,    64,    65,    -1,
      -1,    -1,     1,    70,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    -1,    -1,    -1,    -1,    -1,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    -1,    47,    48,
      -1,    -1,    -1,    -1,    14,    15,    16,    17,    18,    58,
      -1,    -1,    -1,    62,    63,    64,    26,    -1,    -1,    -1,
       1,    70,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    -1,    -1,    -1,    -1,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    -1,    47,    48,    -1,    -1,
      -1,    16,    17,    18,    -1,    -1,    -1,    58,    -1,    -1,
      -1,    62,    63,    64,    -1,    -1,    -1,    -1,     1,    70,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    -1,    47,    48,    -1,    -1,    16,    17,
      18,    -1,    -1,    -1,    -1,    58,    16,    17,    18,    62,
      -1,    64,    65,    -1,    -1,    -1,     1,    70,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    54,    55,    56,    57,    58,    59,
      60,    61,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    -1,    47,    48,    14,    15,    16,    17,    18,    -1,
      -1,    -1,    -1,    58,    -1,    -1,    -1,    62,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    70,    14,    15,    16,    17,
      18,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    14,    15,    16,    17,    18,    67,    -1,    -1,
      -1,    -1,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    14,    15,    16,    17,    18,    67,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      16,    17,    18,    -1,    -1,    67,    -1,    -1,    -1,    -1,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    16,    17,    18,    -1,    -1,    67,    -1,    -1,
     425,    -1,    -1,    -1,    -1,    -1,    -1,    53,    54,    55,
      56,    57,    58,    59,    60,    61,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
      54,    55,    56,    57,    58,    59,    60,    61,   463
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    74,    75,     0,     7,    77,    83,    86,    91,   100,
     101,   100,    63,    76,     6,    36,    46,    66,    94,    95,
     103,    59,    80,   104,   105,     3,    64,   157,   158,     8,
       9,    82,     8,     9,    82,    63,    67,   157,    57,    63,
       3,    68,    13,    84,    80,   104,    65,   157,    92,     1,
       4,     5,     6,     8,     9,    10,    11,    12,    19,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    47,    48,
      58,    62,    64,    70,    78,    79,    82,   102,   135,   143,
     144,   145,   149,   151,   152,   153,   156,   158,   161,   168,
     169,    64,    82,    89,    13,    37,   147,    64,    37,    64,
     143,   145,   143,   135,    82,   135,   137,   138,   139,   135,
     140,   141,   142,   134,   135,   168,    64,   150,    64,   154,
      64,   135,   135,     6,    46,    59,    95,   134,    64,    37,
     135,    14,    15,    16,    17,    18,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    11,    12,
      13,    66,    67,    35,    71,    64,    64,     8,     9,    93,
      95,    96,    97,    98,    99,    69,    95,   135,    82,    64,
     164,    47,    82,    26,    69,    67,    66,    72,    67,    66,
      67,   135,    64,    68,   106,   148,   135,   148,    46,   163,
       8,     9,   134,    80,    65,   146,    82,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,    65,   139,    82,    56,   134,
     162,   165,    80,    65,    26,    67,    90,    98,   137,    65,
     138,   135,    65,   142,    65,   135,    67,   134,   108,    67,
       8,     9,   137,    65,    65,   134,    66,    66,    67,    64,
     134,    25,    72,   137,   137,    93,    85,    97,    80,    87,
      88,    65,    65,   135,    65,     1,    20,    22,    27,    28,
      29,    30,    32,    33,    34,    63,    95,   106,   107,   114,
     115,   116,   118,   120,   124,   128,   134,   136,   170,   135,
     160,   160,    65,    64,    65,   135,    65,   166,    25,    72,
      56,    72,   134,    65,    65,     1,    63,    81,   106,    82,
      67,    63,   106,    67,   155,    63,    64,    64,    64,   119,
      64,    64,    63,    63,    63,   134,   109,    69,   114,    63,
      67,    65,    67,    65,   167,   137,    56,    72,   134,   134,
      72,    88,   143,    65,   134,   134,   134,   115,    96,   111,
     112,   126,   127,   134,     8,     9,   111,   122,   123,    63,
      80,   110,   113,   135,    82,   159,   137,    65,   134,    72,
      72,   155,    65,    65,    65,    27,    80,    13,    63,    67,
      31,    93,    67,    63,   155,    66,    65,    72,   115,   129,
     117,    64,    93,   135,   126,   122,   135,    13,   113,    65,
     135,    21,   171,    68,   115,   134,    63,    65,   135,   115,
     108,    65,   126,   125,    23,    24,   131,    63,    65,   115,
       4,    19,    25,    58,    62,    64,   132,   133,   157,    66,
     115,   130,   131,   121,     4,   132,     4,     4,   133,    25,
      66,    16,    17,    18,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,   130,    69,   130,   115,    66,
      65,    66,   132,    64,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   133,    66
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    73,    74,    75,    75,    76,    76,    77,    78,    79,
      80,    80,    81,    81,    81,    82,    82,    84,    85,    83,
      83,    83,    83,    83,    86,    87,    88,    88,    89,    90,
      89,    92,    91,    93,    93,    94,    94,    94,    95,    96,
      97,    97,    97,    98,    98,    98,    99,    99,   100,   100,
     101,   102,   103,   103,   104,   104,   105,   105,   106,   107,
     107,   107,   108,   109,   108,   110,   110,   111,   112,   113,
     113,   114,   114,   114,   115,   115,   115,   115,   115,   115,
     115,   115,   115,   115,   117,   116,   119,   118,   121,   120,
     122,   122,   122,   123,   123,   125,   124,   126,   126,   127,
     127,   129,   128,   130,   130,   130,   131,   131,   131,   131,
     131,   132,   132,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   134,   134,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   136,   136,   137,
     137,   137,   138,   138,   139,   139,   140,   140,   140,   141,
     141,   142,   143,   144,   144,   145,   145,   145,   145,   146,
     145,   145,   145,   145,   145,   145,   145,   145,   145,   145,
     145,   145,   145,   147,   145,   145,   145,   145,   145,   145,
     148,   148,   150,   149,   151,   152,   154,   153,   155,   155,
     156,   157,   157,   157,   158,   158,   159,   160,   160,   162,
     161,   163,   161,   161,   161,   164,   161,   165,   161,   166,
     161,   167,   161,   168,   168,   169,   169,   169,   169,   170,
     171,   171
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     3,     0,     0,     1,     4,     1,     1,
       0,     1,     1,     1,     1,     1,     1,     0,     0,     9,
       3,     1,     1,     1,     2,     2,     1,     3,     0,     0,
       5,     0,     7,     1,     1,     1,     2,     2,     1,     1,
       2,     3,     1,     0,     1,     2,     1,     3,     0,     2,
       2,     4,     1,     0,     1,     3,     2,     4,     4,     1,
       1,     1,     0,     0,     5,     2,     4,     3,     3,     1,
       3,     0,     2,     2,     2,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     0,     6,     0,     8,     0,    10,
       1,     1,     1,     1,     3,     0,     8,     0,     1,     1,
       1,     0,    10,     2,     2,     0,     3,     5,     4,     4,
       2,     1,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     1,     2,
       2,     2,     1,     3,     3,     3,     5,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     2,     2,     2,     2,     2,     2,
       2,     1,     1,     1,     1,     1,     1,     2,     3,     0,
       1,     2,     1,     2,     1,     3,     0,     1,     2,     1,
       3,     3,     1,     1,     2,     1,     1,     1,     1,     0,
       5,     3,     6,     7,     8,     7,     5,     6,     5,     4,
       1,     3,     1,     0,     6,     3,     5,     4,     4,     4,
       1,     3,     0,     3,     7,     9,     0,     3,     0,     3,
       1,     1,     3,     3,     1,     2,     3,     0,     3,     0,
       5,     0,     5,     6,     6,     0,     5,     0,     5,     0,
       7,     0,     8,     3,     3,     1,     2,     3,     3,     6,
       0,     2
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* all: program  */
#line 212 "grammar.y"
        {
#line 231 "grammar.y.pre"
	    comp_trees[TREE_MAIN] = (yyval.node);
	}
#line 2041 "y.tab.c"
    break;

  case 3: /* program: program def possible_semi_colon  */
#line 220 "grammar.y"
        {
#line 238 "grammar.y.pre"
	    CREATE_TWO_VALUES((yyval.node), 0, (yyvsp[-2].node), (yyvsp[-1].node));
	}
#line 2050 "y.tab.c"
    break;

  case 4: /* program: %empty  */
#line 225 "grammar.y"
        {
#line 242 "grammar.y.pre"
	    (yyval.node) = 0;
	}
#line 2059 "y.tab.c"
    break;

  case 6: /* possible_semi_colon: ';'  */
#line 234 "grammar.y"
            {
#line 250 "grammar.y.pre"

		yywarn("Extra ';'. Ignored.");
	    }
#line 2069 "y.tab.c"
    break;

  case 7: /* inheritance: type_modifier_list L_INHERIT string_con1 ';'  */
#line 244 "grammar.y"
            {
#line 259 "grammar.y.pre"
		object_t *ob;
		inherit_t inherit;
		int initializer;
		
		(yyvsp[-3].type) |= global_modifiers;

		
		if (!((yyvsp[-3].type) & DECL_ACCESS)) (yyvsp[-3].type) |= DECL_PUBLIC;
#ifndef ALLOW_INHERIT_AFTER_FUNCTION
		if (func_present)
		    yyerror("Illegal to inherit after defining functions.");
#endif
#ifndef ALLOW_INHERIT_AFTER_GLOBAL_VARIABLES
		if (var_defined)
		    yyerror("Illegal to inherit after defining global variables.");
#endif
#ifndef ALLOW_INHERIT_AFTER_FUNCTION
		if (func_present){
		  inherit_file = 0;
		  YYACCEPT;
		}
#endif
#ifndef ALLOW_INHERIT_AFTER_GLOBAL_VARIABLES
               if (var_defined && inherit_file){
                  inherit_file = 0;
                  YYACCEPT;
                }
#endif
		ob = find_object2((yyvsp[-1].string));
		if (ob == 0) {
		    inherit_file = alloc_cstring((yyvsp[-1].string), "inherit");
		    /* Return back to load_object() */
		    YYACCEPT;
		}
		scratch_free((yyvsp[-1].string));
		inherit.prog = ob->prog;

		if (mem_block[A_INHERITS].current_size){
		    inherit_t *prev_inherit = INHERIT(NUM_INHERITS - 1);
		    
		    inherit.function_index_offset 
			= prev_inherit->function_index_offset
			+ prev_inherit->prog->num_functions_defined
			+ prev_inherit->prog->last_inherited;
		    if (prev_inherit->prog->num_functions_defined &&
			prev_inherit->prog->function_table[prev_inherit->prog->num_functions_defined - 1].funcname[0] == APPLY___INIT_SPECIAL_CHAR)
			inherit.function_index_offset--;
		} else inherit.function_index_offset = 0;
		
		inherit.variable_index_offset =
		    mem_block[A_VAR_TEMP].current_size /
		    sizeof (variable_t);
		inherit.type_mod = (yyvsp[-3].type);
		add_to_mem_block(A_INHERITS, (char *)&inherit, sizeof inherit);

		/* The following has to come before copy_vars - Sym */
		copy_structures(ob->prog);
		copy_variables(ob->prog, (yyvsp[-3].type));
		initializer = copy_functions(ob->prog, (yyvsp[-3].type));
		if (initializer >= 0) {
		    parse_node_t *node, *newnode;
		    /* initializer is an index into the object we're
		       inheriting's function table; this finds the
		       appropriate entry in our table and generates
		       a call to it */
		    node = new_node_no_line();
		    node->kind = NODE_CALL_2;
		    node->r.expr = 0;
		    node->v.number = F_CALL_INHERITED;
		    node->l.number = initializer | ((NUM_INHERITS - 1) << 16);
		    node->type = TYPE_ANY;
		    
		    /* The following illustrates a distinction between */
		    /* macros and funcs...newnode is needed here - Sym */
		    newnode = comp_trees[TREE_INIT];
		    CREATE_TWO_VALUES(comp_trees[TREE_INIT],0, newnode, node);
		    comp_trees[TREE_INIT] = pop_value(comp_trees[TREE_INIT]);
		    
		} 
		(yyval.node) = 0;
	    }
#line 2157 "y.tab.c"
    break;

  case 8: /* real: L_REAL  */
#line 331 "grammar.y"
            {
#line 361 "grammar.y.pre"
		CREATE_REAL((yyval.node), (yyvsp[0].real));
	    }
#line 2166 "y.tab.c"
    break;

  case 9: /* number: L_NUMBER  */
#line 339 "grammar.y"
            {
#line 368 "grammar.y.pre"
		CREATE_NUMBER((yyval.node), (yyvsp[0].number));
	    }
#line 2175 "y.tab.c"
    break;

  case 10: /* optional_star: %empty  */
#line 347 "grammar.y"
            {
#line 375 "grammar.y.pre"
		(yyval.type) = 0;
	    }
#line 2184 "y.tab.c"
    break;

  case 11: /* optional_star: '*'  */
#line 352 "grammar.y"
{
#line 379 "grammar.y.pre"
		(yyval.type) = TYPE_MOD_ARRAY;
	    }
#line 2193 "y.tab.c"
    break;

  case 12: /* block_or_semi: block  */
#line 360 "grammar.y"
            {
#line 386 "grammar.y.pre"
		(yyval.node) = (yyvsp[0].decl).node;
		if (!(yyval.node)) {
		    CREATE_RETURN((yyval.node), 0);
		}
            }
#line 2205 "y.tab.c"
    break;

  case 13: /* block_or_semi: ';'  */
#line 368 "grammar.y"
            {
#line 393 "grammar.y.pre"
		(yyval.node) = 0;
	    }
#line 2214 "y.tab.c"
    break;

  case 14: /* block_or_semi: error  */
#line 373 "grammar.y"
            {
#line 397 "grammar.y.pre"
		(yyval.node) = 0;
	    }
#line 2223 "y.tab.c"
    break;

  case 15: /* identifier: L_DEFINED_NAME  */
#line 381 "grammar.y"
            {
#line 404 "grammar.y.pre"
		(yyval.string) = scratch_copy((yyvsp[0].ihe)->name);
	    }
#line 2232 "y.tab.c"
    break;

  case 17: /* $@1: %empty  */
#line 390 "grammar.y"
            {
#line 412 "grammar.y.pre"
		int flags;
                func_present = 1;
		flags = ((yyvsp[-2].type) >> 16);
		
		flags |= global_modifiers;


		if (!(flags & DECL_ACCESS)) flags |= DECL_PUBLIC;
                (yyvsp[-2].type) = (flags << 16) | ((yyvsp[-2].type) & 0xffff);
		/* Handle type checking here so we know whether to typecheck
		   'argument' */
		if ((yyvsp[-2].type) & 0xffff) {
		    exact_types = ((yyvsp[-2].type)& 0xffff) | (yyvsp[-1].type);
		} else {
		    if (pragmas & PRAGMA_STRICT_TYPES) {
			if (strcmp((yyvsp[0].string), "create") != 0)
			    yyerror("\"#pragma strict_types\" requires type of function");
			else
			    exact_types = TYPE_VOID; /* default for create() */
		    } else
			exact_types = 0;
		}
	    }
#line 2262 "y.tab.c"
    break;

  case 18: /* @2: %empty  */
#line 416 "grammar.y"
            {
#line 463 "grammar.y.pre"
		char *p = (yyvsp[-4].string);
		(yyvsp[-4].string) = make_shared_string((yyvsp[-4].string));
		scratch_free(p);

		/* If we had nested functions, we would need to check */
		/* here if we have enough space for locals */
		
		/*
		 * Define a prototype. If it is a real function, then the
		 * prototype will be replaced below.
		 */

		(yyval.number) = FUNC_PROTOTYPE;
		if ((yyvsp[-1].argument).flags & ARG_IS_VARARGS) {
		    (yyval.number) |= (FUNC_TRUE_VARARGS | FUNC_VARARGS);
		}
		(yyval.number) |= ((yyvsp[-6].type) >> 16);

		define_new_function((yyvsp[-4].string), (yyvsp[-1].argument).num_arg, 0, (yyval.number), ((yyvsp[-6].type) & 0xffff)| (yyvsp[-5].type));
		/* This is safe since it is guaranteed to be in the
		   function table, so it can't be dangling */
		free_string((yyvsp[-4].string)); 
		context = 0;
	    }
#line 2293 "y.tab.c"
    break;

  case 19: /* def: type optional_star identifier $@1 '(' argument ')' @2 block_or_semi  */
#line 443 "grammar.y"
            {
#line 489 "grammar.y.pre"
		/* Either a prototype or a block */
		if ((yyvsp[0].node)) {
		    int fun;

		    (yyvsp[-1].number) &= ~FUNC_PROTOTYPE;
		    if ((yyvsp[0].node)->kind != NODE_RETURN &&
			((yyvsp[0].node)->kind != NODE_TWO_VALUES
			 || (yyvsp[0].node)->r.expr->kind != NODE_RETURN)) {
			parse_node_t *replacement;
			CREATE_STATEMENTS(replacement, (yyvsp[0].node), 0);
			CREATE_RETURN(replacement->r.expr, 0);
			(yyvsp[0].node) = replacement;
		    }

		    fun = define_new_function((yyvsp[-6].string), (yyvsp[-3].argument).num_arg, 
					      max_num_locals - (yyvsp[-3].argument).num_arg,
					      (yyvsp[-1].number), ((yyvsp[-8].type) & 0xffff) | (yyvsp[-7].type));
		    if (fun != -1) {
			(yyval.node) = new_node_no_line();
			(yyval.node)->kind = NODE_FUNCTION;
			(yyval.node)->v.number = fun;
			(yyval.node)->l.number = max_num_locals;
			(yyval.node)->r.expr = (yyvsp[0].node);
		    } else 
			(yyval.node) = 0;
		} else
		    (yyval.node) = 0;
		free_all_local_names(!!(yyvsp[0].node));
	    }
#line 2329 "y.tab.c"
    break;

  case 20: /* def: type name_list ';'  */
#line 475 "grammar.y"
            {
#line 520 "grammar.y.pre"
		if (!((yyvsp[-2].type) & ~(DECL_MODS)) && (pragmas & PRAGMA_STRICT_TYPES))
		    yyerror("Missing type for global variable declaration");
		(yyval.node) = 0;
	    }
#line 2340 "y.tab.c"
    break;

  case 24: /* modifier_change: type_modifier_list ':'  */
#line 487 "grammar.y"
            {
#line 531 "grammar.y.pre"
		if (!(yyvsp[-1].type)) 
		    yyerror("modifier list may not be empty.");
		
		if ((yyvsp[-1].type) & FUNC_VARARGS) {
		    yyerror("Illegal modifier 'varargs' in global modifier list.");
		    (yyvsp[-1].type) &= ~FUNC_VARARGS;
		}

		if (!((yyvsp[-1].type) & DECL_ACCESS)) (yyvsp[-1].type) |= DECL_PUBLIC;
		global_modifiers = (yyvsp[-1].type);
		(yyval.node) = 0;
	    }
#line 2359 "y.tab.c"
    break;

  case 25: /* member_name: optional_star identifier  */
#line 505 "grammar.y"
            {
#line 548 "grammar.y.pre"
		/* At this point, the current_type here is only a basic_type */
		/* and cannot be unused yet - Sym */
		
		if (current_type == TYPE_VOID)
		    yyerror("Illegal to declare class member of type void.");
		add_local_name((yyvsp[0].string), current_type | (yyvsp[-1].type));
		scratch_free((yyvsp[0].string));
	    }
#line 2374 "y.tab.c"
    break;

  case 29: /* $@3: %empty  */
#line 525 "grammar.y"
          {
#line 567 "grammar.y.pre"
	      current_type = (yyvsp[0].type);
	  }
#line 2383 "y.tab.c"
    break;

  case 31: /* @4: %empty  */
#line 534 "grammar.y"
            {
#line 575 "grammar.y.pre"
		ident_hash_elem_t *ihe;

		ihe = find_or_add_ident(
			   PROG_STRING((yyval.number) = store_prog_string((yyvsp[-1].string))),
			   FOA_GLOBAL_SCOPE);
		if (ihe->dn.class_num == -1) {
		    ihe->sem_value++;
		    ihe->dn.class_num = mem_block[A_CLASS_DEF].current_size / sizeof(class_def_t);
		    if (ihe->dn.class_num > CLASS_NUM_MASK){
			char buf[256];
			char *p;

			p = buf;
			sprintf(p, "Too many classes, max is %d.\n", CLASS_NUM_MASK + 1);
			yyerror(buf);
		    }

		    scratch_free((yyvsp[-1].string));
		    (yyvsp[-2].ihe) = 0;
		}
		else {
		    (yyvsp[-2].ihe) = ihe;
		}
	    }
#line 2414 "y.tab.c"
    break;

  case 32: /* type_decl: type_modifier_list L_CLASS identifier '{' @4 member_list '}'  */
#line 561 "grammar.y"
            {
#line 601 "grammar.y.pre"
		class_def_t *sd;
		class_member_entry_t *sme;
		int i, raise_error = 0;
		
		/* check for a redefinition */
		if ((yyvsp[-5].ihe) != 0) {
		    sd = CLASS((yyvsp[-5].ihe)->dn.class_num);
		    if (sd->size != current_number_of_locals)
			raise_error = 1;
		    else {
			i = sd->size;
			sme = (class_member_entry_t *)mem_block[A_CLASS_MEMBER].block + sd->index;
			while (i--) {
			    /* check for matching names and types */
			    if (strcmp(PROG_STRING(sme[i].membername), locals_ptr[i].ihe->name) != 0 ||
				sme[i].type != (type_of_locals_ptr[i] & ~LOCAL_MODS)) {
				raise_error = 1;
				break;
			    }
			}
		    }
		}

		if (raise_error) {
		    char buf[512];
		    char *end = EndOf(buf);
		    char *p;

		    p = strput(buf, end, "Illegal to redefine class ");
		    p = strput(p, end, PROG_STRING((yyval.number)));
		    yyerror(buf);
		} else {
		    sd = (class_def_t *)allocate_in_mem_block(A_CLASS_DEF, sizeof(class_def_t));
		    i = sd->size = current_number_of_locals;
		    sd->index = mem_block[A_CLASS_MEMBER].current_size / sizeof(class_member_entry_t);
		    sd->classname = (yyvsp[-2].number);

		    sme = (class_member_entry_t *)allocate_in_mem_block(A_CLASS_MEMBER, sizeof(class_member_entry_t) * current_number_of_locals);

		    while (i--) {
			sme[i].membername = store_prog_string(locals_ptr[i].ihe->name);
			sme[i].type = type_of_locals_ptr[i] & ~LOCAL_MODS;
		    }
		}

		free_all_local_names(0);
		(yyval.node) = 0;
	    }
#line 2469 "y.tab.c"
    break;

  case 34: /* new_local_name: L_DEFINED_NAME  */
#line 616 "grammar.y"
            {
#line 655 "grammar.y.pre"
		if ((yyvsp[0].ihe)->dn.local_num != -1) {
		    char buff[256];
		    char *end = EndOf(buff);
		    char *p;
		    
		    p = strput(buff, end, "Illegal to redeclare local name '");
		    p = strput(p, end, (yyvsp[0].ihe)->name);
		    p = strput(p, end, "'");
		    yyerror(buff);
		}
		(yyval.string) = scratch_copy((yyvsp[0].ihe)->name);
	    }
#line 2488 "y.tab.c"
    break;

  case 36: /* atomic_type: L_CLASS L_DEFINED_NAME  */
#line 635 "grammar.y"
            {
#line 673 "grammar.y.pre"
		if ((yyvsp[0].ihe)->dn.class_num == -1) {
		    char buf[256];
		    char *end = EndOf(buf);
		    char *p;
		    
		    p = strput(buf, end, "Undefined class '");
		    p = strput(p, end, (yyvsp[0].ihe)->name);
		    p = strput(p, end, "'");
		    yyerror(buf);
		    (yyval.type) = TYPE_ANY;
		} else {
		    (yyval.type) = (yyvsp[0].ihe)->dn.class_num | TYPE_MOD_CLASS;
		}
	    }
#line 2509 "y.tab.c"
    break;

  case 37: /* atomic_type: L_CLASS L_IDENTIFIER  */
#line 652 "grammar.y"
            {
#line 689 "grammar.y.pre"
		char buf[256];
		char *end = EndOf(buf);
		char *p;

		p = strput(buf, end, "Undefined class '");
		p = strput(p, end, (yyvsp[0].string));
		p = strput(p, end, "'");
		yyerror(buf);
		(yyval.type) = TYPE_ANY;
	    }
#line 2526 "y.tab.c"
    break;

  case 40: /* new_arg: arg_type optional_star  */
#line 677 "grammar.y"
            {
#line 734 "grammar.y.pre"
                (yyval.number) = (yyvsp[-1].type) | (yyvsp[0].type);
		if ((yyvsp[-1].type) != TYPE_VOID)
		    add_local_name("", (yyvsp[-1].type) | (yyvsp[0].type));
            }
#line 2537 "y.tab.c"
    break;

  case 41: /* new_arg: arg_type optional_star new_local_name  */
#line 684 "grammar.y"
            {
#line 740 "grammar.y.pre"
		if ((yyvsp[-2].type) == TYPE_VOID)
		    yyerror("Illegal to declare argument of type void.");
                add_local_name((yyvsp[0].string), (yyvsp[-2].type) | (yyvsp[-1].type));
		scratch_free((yyvsp[0].string));
                (yyval.number) = (yyvsp[-2].type) | (yyvsp[-1].type);
	    }
#line 2550 "y.tab.c"
    break;

  case 42: /* new_arg: new_local_name  */
#line 693 "grammar.y"
            {
#line 748 "grammar.y.pre"
		if (exact_types) {
		    yyerror("Missing type for argument");
		}
		add_local_name((yyvsp[0].string), TYPE_ANY);
		scratch_free((yyvsp[0].string));
		(yyval.number) = TYPE_ANY;
            }
#line 2564 "y.tab.c"
    break;

  case 43: /* argument: %empty  */
#line 706 "grammar.y"
            {
#line 760 "grammar.y.pre"
		(yyval.argument).num_arg = 0;
                (yyval.argument).flags = 0;
	    }
#line 2574 "y.tab.c"
    break;

  case 45: /* argument: argument_list L_DOT_DOT_DOT  */
#line 713 "grammar.y"
            {
#line 766 "grammar.y.pre"
		int x = type_of_locals_ptr[max_num_locals-1];
		int lt = x & ~LOCAL_MODS;
		
		(yyval.argument) = (yyvsp[-1].argument);
		(yyval.argument).flags |= ARG_IS_VARARGS;

		if (x & LOCAL_MOD_REF) {
		    yyerror("Variable to hold remainder of args may not be a reference");
		    x &= ~LOCAL_MOD_REF;
		}
		if (lt != TYPE_ANY && !(lt & TYPE_MOD_ARRAY))
		    yywarn("Variable to hold remainder of arguments should be an array.");
	    }
#line 2594 "y.tab.c"
    break;

  case 46: /* argument_list: new_arg  */
#line 732 "grammar.y"
            {
#line 784 "grammar.y.pre"
		if (((yyvsp[0].number) & TYPE_MASK) == TYPE_VOID && !((yyvsp[0].number) & TYPE_MOD_CLASS)) {
		    if ((yyvsp[0].number) & ~TYPE_MASK)
			yyerror("Illegal to declare argument of type void.");
		    (yyval.argument).num_arg = 0;
		} else {
		    (yyval.argument).num_arg = 1;
		}
                (yyval.argument).flags = 0;
	    }
#line 2610 "y.tab.c"
    break;

  case 47: /* argument_list: argument_list ',' new_arg  */
#line 744 "grammar.y"
            {
#line 795 "grammar.y.pre"
		if (!(yyval.argument).num_arg)    /* first arg was void w/no name */
		    yyerror("argument of type void must be the only argument.");
		if (((yyvsp[0].number) & TYPE_MASK) == TYPE_VOID && !((yyvsp[0].number) & TYPE_MOD_CLASS))
		    yyerror("Illegal to declare argument of type void.");

                (yyval.argument) = (yyvsp[-2].argument);
		(yyval.argument).num_arg++;
	    }
#line 2625 "y.tab.c"
    break;

  case 48: /* type_modifier_list: %empty  */
#line 758 "grammar.y"
            {
#line 808 "grammar.y.pre"
		(yyval.type) = 0;
	    }
#line 2634 "y.tab.c"
    break;

  case 49: /* type_modifier_list: L_TYPE_MODIFIER type_modifier_list  */
#line 763 "grammar.y"
            {
#line 812 "grammar.y.pre"
		
		(yyval.type) = (yyvsp[-1].type) | (yyvsp[0].type);
		
	    }
#line 2645 "y.tab.c"
    break;

  case 50: /* type: type_modifier_list opt_basic_type  */
#line 773 "grammar.y"
            {
#line 838 "grammar.y.pre"
		(yyval.type) = ((yyvsp[-1].type) << 16) | (yyvsp[0].type);
		current_type = (yyval.type);
	    }
#line 2655 "y.tab.c"
    break;

  case 51: /* cast: '(' basic_type optional_star ')'  */
#line 782 "grammar.y"
            {
#line 846 "grammar.y.pre"
		(yyval.type) = (yyvsp[-2].type) | (yyvsp[-1].type);
	    }
#line 2664 "y.tab.c"
    break;

  case 53: /* opt_basic_type: %empty  */
#line 791 "grammar.y"
            {
#line 854 "grammar.y.pre"
		(yyval.type) = TYPE_UNKNOWN;
	    }
#line 2673 "y.tab.c"
    break;

  case 56: /* new_name: optional_star identifier  */
#line 804 "grammar.y"
            {
#line 866 "grammar.y.pre"
		if (current_type & (FUNC_VARARGS << 16)){
		    yyerror("Illegal to declare varargs variable.");
		    current_type &= ~(FUNC_VARARGS << 16);
		}
		/* Now it is ok to merge the two
		 * remember that class_num and varargs was the reason for above
		 * Do the merging once only per row of decls
		 */

		if (current_type & 0xffff0000){
		    current_type = (current_type >> 16) | (current_type & 0xffff);
		}

		current_type |= global_modifiers;

		if (!(current_type & DECL_ACCESS)) current_type |= DECL_PUBLIC;

		if ((current_type & ~DECL_MODS) == TYPE_VOID)
		    yyerror("Illegal to declare global variable of type void.");

		define_new_variable((yyvsp[0].string), current_type | (yyvsp[-1].type));
		scratch_free((yyvsp[0].string));
	    }
#line 2703 "y.tab.c"
    break;

  case 57: /* new_name: optional_star identifier L_ASSIGN expr0  */
#line 830 "grammar.y"
            {
#line 891 "grammar.y.pre"
		parse_node_t *expr, *newnode;
		int type;

		if (current_type & (FUNC_VARARGS << 16)){
		    yyerror("Illegal to declare varargs variable.");
		    current_type &= ~(FUNC_VARARGS << 16);
		}
		
		if (current_type & 0xffff0000){
		    current_type = (current_type >> 16) | (current_type & 0xffff);
		}

		current_type |= global_modifiers;

		if (!(current_type & DECL_ACCESS)) current_type |= DECL_PUBLIC;

		if ((current_type & ~DECL_MODS) == TYPE_VOID)
		    yyerror("Illegal to declare global variable of type void.");

		if ((yyvsp[-1].number) != F_ASSIGN)
		    yyerror("Only '=' is legal in initializers.");

		/* ignore current_type == 0, which gets a missing type error
		   later anyway */
		if (current_type) {
		    type = (current_type | (yyvsp[-3].type)) & ~DECL_MODS;
		    if ((current_type & ~DECL_MODS) == TYPE_VOID)
			yyerror("Illegal to declare global variable of type void.");
		    if (!compatible_types(type, (yyvsp[0].node)->type)) {
			char buff[256];
			char *end = EndOf(buff);
			char *p;
			
			p = strput(buff, end, "Type mismatch ");
			p = get_two_types(p, end, type, (yyvsp[0].node)->type);
			p = strput(p, end, " when initializing ");
			p = strput(p, end, (yyvsp[-2].string));
			yyerror(buff);
		    }
		} else type = 0;
		(yyvsp[0].node) = do_promotions((yyvsp[0].node), type);

		CREATE_BINARY_OP(expr, F_VOID_ASSIGN, 0, (yyvsp[0].node), 0);
		CREATE_OPCODE_1(expr->r.expr, F_GLOBAL_LVALUE, 0,
				define_new_variable((yyvsp[-2].string), current_type | (yyvsp[-3].type)));
		newnode = comp_trees[TREE_INIT];
		CREATE_TWO_VALUES(comp_trees[TREE_INIT], 0,
				  newnode, expr);
		scratch_free((yyvsp[-2].string));
	    }
#line 2760 "y.tab.c"
    break;

  case 58: /* block: '{' local_declarations statements '}'  */
#line 886 "grammar.y"
            {
#line 946 "grammar.y.pre"
		if ((yyvsp[-2].decl).node && (yyvsp[-1].node)) {
		    CREATE_STATEMENTS((yyval.decl).node, (yyvsp[-2].decl).node, (yyvsp[-1].node));
		} else (yyval.decl).node = ((yyvsp[-2].decl).node ? (yyvsp[-2].decl).node : (yyvsp[-1].node));
                (yyval.decl).num = (yyvsp[-2].decl).num;
            }
#line 2772 "y.tab.c"
    break;

  case 62: /* local_declarations: %empty  */
#line 899 "grammar.y"
            {
#line 958 "grammar.y.pre"
                (yyval.decl).node = 0;
                (yyval.decl).num = 0;
            }
#line 2782 "y.tab.c"
    break;

  case 63: /* $@5: %empty  */
#line 905 "grammar.y"
            {
#line 963 "grammar.y.pre"
		if ((yyvsp[0].type) == TYPE_VOID)
		    yyerror("Illegal to declare local variable of type void.");
                /* can't do this in basic_type b/c local_name_list contains
                 * expr0 which contains cast which contains basic_type
                 */
                current_type = (yyvsp[0].type);
            }
#line 2796 "y.tab.c"
    break;

  case 64: /* local_declarations: local_declarations basic_type $@5 local_name_list ';'  */
#line 915 "grammar.y"
            {
#line 972 "grammar.y.pre"
                if ((yyvsp[-4].decl).node && (yyvsp[-1].decl).node) {
		    CREATE_STATEMENTS((yyval.decl).node, (yyvsp[-4].decl).node, (yyvsp[-1].decl).node);
                } else (yyval.decl).node = ((yyvsp[-4].decl).node ? (yyvsp[-4].decl).node : (yyvsp[-1].decl).node);
                (yyval.decl).num = (yyvsp[-4].decl).num + (yyvsp[-1].decl).num;
            }
#line 2808 "y.tab.c"
    break;

  case 65: /* new_local_def: optional_star new_local_name  */
#line 927 "grammar.y"
            {
#line 983 "grammar.y.pre"
		if (current_type & LOCAL_MOD_REF) {
		    yyerror("Illegal to declare local variable as reference");
		    current_type &= ~LOCAL_MOD_REF;
		}
		add_local_name((yyvsp[0].string), current_type | (yyvsp[-1].type) | LOCAL_MOD_UNUSED);

		scratch_free((yyvsp[0].string));
		(yyval.node) = 0;
	    }
#line 2824 "y.tab.c"
    break;

  case 66: /* new_local_def: optional_star new_local_name L_ASSIGN expr0  */
#line 939 "grammar.y"
            {
#line 994 "grammar.y.pre"
		int type = (current_type | (yyvsp[-3].type)) & ~DECL_MODS;

		if (current_type & LOCAL_MOD_REF) {
		    yyerror("Illegal to declare local variable as reference");
		    current_type &= ~LOCAL_MOD_REF;
		    type &= ~LOCAL_MOD_REF;
		}

		if ((yyvsp[-1].number) != F_ASSIGN)
		    yyerror("Only '=' is allowed in initializers.");
		if (!compatible_types((yyvsp[0].node)->type, type)) {
		    char buff[256];
		    char *end = EndOf(buff);
		    char *p;
		    
		    p = strput(buff, end, "Type mismatch ");
		    p = get_two_types(p, end, type, (yyvsp[0].node)->type);
		    p = strput(p, end, " when initializing ");
		    p = strput(p, end, (yyvsp[-2].string));

		    yyerror(buff);
		}
		
		(yyvsp[0].node) = do_promotions((yyvsp[0].node), type);

		CREATE_UNARY_OP_1((yyval.node), F_VOID_ASSIGN_LOCAL, 0, (yyvsp[0].node),
				  add_local_name((yyvsp[-2].string), current_type | (yyvsp[-3].type) | LOCAL_MOD_UNUSED));
		scratch_free((yyvsp[-2].string));
	    }
#line 2860 "y.tab.c"
    break;

  case 67: /* single_new_local_def: arg_type optional_star new_local_name  */
#line 974 "grammar.y"
            {
#line 1028 "grammar.y.pre"
		if ((yyvsp[-2].type) == TYPE_VOID)
		    yyerror("Illegal to declare local variable of type void.");

		(yyval.number) = add_local_name((yyvsp[0].string), (yyvsp[-2].type) | (yyvsp[-1].type));
		scratch_free((yyvsp[0].string));
	    }
#line 2873 "y.tab.c"
    break;

  case 68: /* single_new_local_def_with_init: single_new_local_def L_ASSIGN expr0  */
#line 986 "grammar.y"
            {
#line 1039 "grammar.y.pre"
                int type = type_of_locals_ptr[(yyvsp[-2].number)];

		if (type & LOCAL_MOD_REF) {
		    yyerror("Illegal to declare local variable as reference");
		    type_of_locals_ptr[(yyvsp[-2].number)] &= ~LOCAL_MOD_REF;
		}
		type &= ~LOCAL_MODS;

		if ((yyvsp[-1].number) != F_ASSIGN)
		    yyerror("Only '=' is allowed in initializers.");
		if (!compatible_types((yyvsp[0].node)->type, type)) {
		    char buff[256];
		    char *end = EndOf(buff);
		    char *p;
		    
		    p = strput(buff, end, "Type mismatch ");
		    p = get_two_types(p, end, type, (yyvsp[0].node)->type);
		    p = strput(p, end, " when initializing.");
		    yyerror(buff);
		}

		(yyvsp[0].node) = do_promotions((yyvsp[0].node), type);

		/* this is an expression */
		CREATE_BINARY_OP((yyval.node), F_ASSIGN, 0, (yyvsp[0].node), 0);
                CREATE_OPCODE_1((yyval.node)->r.expr, F_LOCAL_LVALUE, 0, (yyvsp[-2].number));
	    }
#line 2907 "y.tab.c"
    break;

  case 69: /* local_name_list: new_local_def  */
#line 1019 "grammar.y"
            {
#line 1071 "grammar.y.pre"
                (yyval.decl).node = (yyvsp[0].node);
                (yyval.decl).num = 1;
            }
#line 2917 "y.tab.c"
    break;

  case 70: /* local_name_list: new_local_def ',' local_name_list  */
#line 1025 "grammar.y"
            {
#line 1076 "grammar.y.pre"
                if ((yyvsp[-2].node) && (yyvsp[0].decl).node) {
		    CREATE_STATEMENTS((yyval.decl).node, (yyvsp[-2].node), (yyvsp[0].decl).node);
                } else (yyval.decl).node = ((yyvsp[-2].node) ? (yyvsp[-2].node) : (yyvsp[0].decl).node);
                (yyval.decl).num = 1 + (yyvsp[0].decl).num;
            }
#line 2929 "y.tab.c"
    break;

  case 71: /* statements: %empty  */
#line 1036 "grammar.y"
            {
#line 1086 "grammar.y.pre"
		(yyval.node) = 0;
	    }
#line 2938 "y.tab.c"
    break;

  case 72: /* statements: statement statements  */
#line 1041 "grammar.y"
            {
#line 1090 "grammar.y.pre"
		if ((yyvsp[-1].node) && (yyvsp[0].node)) {
		    CREATE_STATEMENTS((yyval.node), (yyvsp[-1].node), (yyvsp[0].node));
		} else (yyval.node) = ((yyvsp[-1].node) ? (yyvsp[-1].node) : (yyvsp[0].node));
            }
#line 2949 "y.tab.c"
    break;

  case 73: /* statements: error ';'  */
#line 1048 "grammar.y"
            {
#line 1096 "grammar.y.pre"
		(yyval.node) = 0;
            }
#line 2958 "y.tab.c"
    break;

  case 74: /* statement: comma_expr ';'  */
#line 1056 "grammar.y"
            {
#line 1103 "grammar.y.pre"
		(yyval.node) = pop_value((yyvsp[-1].node));
#ifdef DEBUG
		{
		    parse_node_t *replacement;
		    CREATE_STATEMENTS(replacement, (yyval.node), 0);
		    CREATE_OPCODE(replacement->r.expr, F_BREAK_POINT, 0);
		    (yyval.node) = replacement;
		}
#endif
	    }
#line 2975 "y.tab.c"
    break;

  case 80: /* statement: decl_block  */
#line 1074 "grammar.y"
           {
#line 1120 "grammar.y.pre"
                (yyval.node) = (yyvsp[0].decl).node;
                pop_n_locals((yyvsp[0].decl).num);
            }
#line 2985 "y.tab.c"
    break;

  case 81: /* statement: ';'  */
#line 1080 "grammar.y"
            {
#line 1125 "grammar.y.pre"
		(yyval.node) = 0;
	    }
#line 2994 "y.tab.c"
    break;

  case 82: /* statement: L_BREAK ';'  */
#line 1085 "grammar.y"
            {
#line 1129 "grammar.y.pre"
		if (context & SPECIAL_CONTEXT) {
		    yyerror("Cannot break out of catch { } or time_expression { }");
		    (yyval.node) = 0;
		} else
		if (context & SWITCH_CONTEXT) {
		    CREATE_CONTROL_JUMP((yyval.node), CJ_BREAK_SWITCH);
		} else
		if (context & LOOP_CONTEXT) {
		    CREATE_CONTROL_JUMP((yyval.node), CJ_BREAK);
		    if (context & LOOP_FOREACH) {
			parse_node_t *replace;
			CREATE_STATEMENTS(replace, 0, (yyval.node));
			CREATE_OPCODE(replace->l.expr, F_EXIT_FOREACH, 0);
			(yyval.node) = replace;
		    }
		} else {
		    yyerror("break statement outside loop");
		    (yyval.node) = 0;
		}
	    }
#line 3021 "y.tab.c"
    break;

  case 83: /* statement: L_CONTINUE ';'  */
#line 1108 "grammar.y"
            {
#line 1151 "grammar.y.pre"
		if (context & SPECIAL_CONTEXT)
		    yyerror("Cannot continue out of catch { } or time_expression { }");
		else
		if (!(context & LOOP_CONTEXT))
		    yyerror("continue statement outside loop");
		CREATE_CONTROL_JUMP((yyval.node), CJ_CONTINUE);
	    }
#line 3035 "y.tab.c"
    break;

  case 84: /* $@6: %empty  */
#line 1121 "grammar.y"
            {
#line 1163 "grammar.y.pre"
		(yyvsp[-3].number) = context;
		context = LOOP_CONTEXT;
	    }
#line 3045 "y.tab.c"
    break;

  case 85: /* while: L_WHILE '(' comma_expr ')' $@6 statement  */
#line 1127 "grammar.y"
            {
#line 1168 "grammar.y.pre"
		CREATE_LOOP((yyval.node), 1, (yyvsp[0].node), 0, optimize_loop_test((yyvsp[-3].node)));
		context = (yyvsp[-5].number);
	    }
#line 3055 "y.tab.c"
    break;

  case 86: /* $@7: %empty  */
#line 1136 "grammar.y"
            {
#line 1176 "grammar.y.pre"
		(yyvsp[0].number) = context;
		context = LOOP_CONTEXT;
	    }
#line 3065 "y.tab.c"
    break;

  case 87: /* do: L_DO $@7 statement L_WHILE '(' comma_expr ')' ';'  */
#line 1142 "grammar.y"
            {
#line 1181 "grammar.y.pre"
		CREATE_LOOP((yyval.node), 0, (yyvsp[-5].node), 0, optimize_loop_test((yyvsp[-2].node)));
		context = (yyvsp[-7].number);
	    }
#line 3075 "y.tab.c"
    break;

  case 88: /* $@8: %empty  */
#line 1151 "grammar.y"
            {
#line 1189 "grammar.y.pre"
		(yyvsp[-5].decl).node = pop_value((yyvsp[-5].decl).node);
		(yyvsp[-7].number) = context;
		context = LOOP_CONTEXT;
	    }
#line 3086 "y.tab.c"
    break;

  case 89: /* for: L_FOR '(' first_for_expr ';' for_expr ';' for_expr ')' $@8 statement  */
#line 1158 "grammar.y"
            {
#line 1195 "grammar.y.pre"
		(yyval.decl).num = (yyvsp[-7].decl).num; /* number of declarations (0/1) */
		
		(yyvsp[-3].node) = pop_value((yyvsp[-3].node));
		if ((yyvsp[-3].node) && IS_NODE((yyvsp[-3].node), NODE_UNARY_OP, F_INC)
		    && IS_NODE((yyvsp[-3].node)->r.expr, NODE_OPCODE_1, F_LOCAL_LVALUE)) {
		    long lvar = (yyvsp[-3].node)->r.expr->l.number;
		    CREATE_OPCODE_1((yyvsp[-3].node), F_LOOP_INCR, 0, lvar);
		}

		CREATE_STATEMENTS((yyval.decl).node, (yyvsp[-7].decl).node, 0);
		CREATE_LOOP((yyval.decl).node->r.expr, 1, (yyvsp[0].node), (yyvsp[-3].node), optimize_loop_test((yyvsp[-5].node)));

		context = (yyvsp[-9].number);
	      }
#line 3107 "y.tab.c"
    break;

  case 90: /* foreach_var: L_DEFINED_NAME  */
#line 1177 "grammar.y"
            {
#line 1213 "grammar.y.pre"
		if ((yyvsp[0].ihe)->dn.local_num != -1) {
		    CREATE_OPCODE_1((yyval.decl).node, F_LOCAL_LVALUE, 0, (yyvsp[0].ihe)->dn.local_num);
		    type_of_locals_ptr[(yyvsp[0].ihe)->dn.local_num] &= ~LOCAL_MOD_UNUSED;
		} else
		if ((yyvsp[0].ihe)->dn.global_num != -1) {
		    CREATE_OPCODE_1((yyval.decl).node, F_GLOBAL_LVALUE, 0, (yyvsp[0].ihe)->dn.global_num);
		} else {
		    char buf[256];
		    char *end = EndOf(buf);
		    char *p;

		    p = strput(buf, end, "'");
		    p = strput(p, end, (yyvsp[0].ihe)->name);
		    p = strput(p, end, "' is not a local or a global variable.");
		    yyerror(buf);
		    CREATE_OPCODE_1((yyval.decl).node, F_GLOBAL_LVALUE, 0, 0);
		}
		(yyval.decl).num = 0;
	    }
#line 3133 "y.tab.c"
    break;

  case 91: /* foreach_var: single_new_local_def  */
#line 1199 "grammar.y"
            {
#line 1234 "grammar.y.pre"
		if (type_of_locals_ptr[(yyvsp[0].number)] & LOCAL_MOD_REF) {
		    CREATE_OPCODE_1((yyval.decl).node, F_REF_LVALUE, 0, (yyvsp[0].number));
		} else {
		    CREATE_OPCODE_1((yyval.decl).node, F_LOCAL_LVALUE, 0, (yyvsp[0].number));
		    type_of_locals_ptr[(yyvsp[0].number)] &= ~LOCAL_MOD_UNUSED;
		}
		(yyval.decl).num = 1;
            }
#line 3148 "y.tab.c"
    break;

  case 92: /* foreach_var: L_IDENTIFIER  */
#line 1210 "grammar.y"
            {
#line 1244 "grammar.y.pre"
		char buf[256];
		char *end = EndOf(buf);
		char *p;
		
		p = strput(buf, end, "'");
		p = strput(p, end, (yyvsp[0].string));
		p = strput(p, end, "' is not a local or a global variable.");
		yyerror(buf);
		CREATE_OPCODE_1((yyval.decl).node, F_GLOBAL_LVALUE, 0, 0);
		scratch_free((yyvsp[0].string));
		(yyval.decl).num = 0;
	    }
#line 3167 "y.tab.c"
    break;

  case 93: /* foreach_vars: foreach_var  */
#line 1228 "grammar.y"
            {
#line 1261 "grammar.y.pre"
		CREATE_FOREACH((yyval.decl).node, (yyvsp[0].decl).node, 0);
		(yyval.decl).num = (yyvsp[0].decl).num;
            }
#line 3177 "y.tab.c"
    break;

  case 94: /* foreach_vars: foreach_var ',' foreach_var  */
#line 1234 "grammar.y"
            {
#line 1266 "grammar.y.pre"
		CREATE_FOREACH((yyval.decl).node, (yyvsp[-2].decl).node, (yyvsp[0].decl).node);
		(yyval.decl).num = (yyvsp[-2].decl).num + (yyvsp[0].decl).num;
		if ((yyvsp[-2].decl).node->v.number == F_REF_LVALUE)
		    yyerror("Mapping key may not be a reference in foreach()");
            }
#line 3189 "y.tab.c"
    break;

  case 95: /* $@9: %empty  */
#line 1245 "grammar.y"
            {
#line 1276 "grammar.y.pre"
		(yyvsp[-3].decl).node->v.expr = (yyvsp[-1].node);
		(yyvsp[-5].number) = context;
		context = LOOP_CONTEXT | LOOP_FOREACH;
            }
#line 3200 "y.tab.c"
    break;

  case 96: /* foreach: L_FOREACH '(' foreach_vars L_IN expr0 ')' $@9 statement  */
#line 1252 "grammar.y"
            {
#line 1282 "grammar.y.pre"
		(yyval.decl).num = (yyvsp[-5].decl).num;

		CREATE_STATEMENTS((yyval.decl).node, (yyvsp[-5].decl).node, 0);
		CREATE_LOOP((yyval.decl).node->r.expr, 2, (yyvsp[0].node), 0, 0);
		CREATE_OPCODE((yyval.decl).node->r.expr->r.expr, F_NEXT_FOREACH, 0);
		
		context = (yyvsp[-7].number);
	    }
#line 3215 "y.tab.c"
    break;

  case 97: /* for_expr: %empty  */
#line 1266 "grammar.y"
            {
#line 1295 "grammar.y.pre"
		(yyval.node) = 0;
	    }
#line 3224 "y.tab.c"
    break;

  case 99: /* first_for_expr: for_expr  */
#line 1275 "grammar.y"
            {
#line 1303 "grammar.y.pre"
	 	(yyval.decl).node = (yyvsp[0].node);
		(yyval.decl).num = 0;
	    }
#line 3234 "y.tab.c"
    break;

  case 100: /* first_for_expr: single_new_local_def_with_init  */
#line 1281 "grammar.y"
            {
#line 1308 "grammar.y.pre"
		(yyval.decl).node = (yyvsp[0].node);
		(yyval.decl).num = 1;
	    }
#line 3244 "y.tab.c"
    break;

  case 101: /* $@10: %empty  */
#line 1290 "grammar.y"
            {
#line 1316 "grammar.y.pre"
                (yyvsp[-3].number) = context;
                context &= LOOP_CONTEXT;
                context |= SWITCH_CONTEXT;
                (yyvsp[-2].number) = mem_block[A_CASES].current_size;
            }
#line 3256 "y.tab.c"
    break;

  case 102: /* switch: L_SWITCH '(' comma_expr ')' $@10 '{' local_declarations case switch_block '}'  */
#line 1298 "grammar.y"
            {
#line 1323 "grammar.y.pre"
                parse_node_t *node1, *node2;

                if ((yyvsp[-1].node)) {
		    CREATE_STATEMENTS(node1, (yyvsp[-2].node), (yyvsp[-1].node));
                } else node1 = (yyvsp[-2].node);

                if (context & SWITCH_STRINGS) {
                    NODE_NO_LINE(node2, NODE_SWITCH_STRINGS);
                } else if (context & SWITCH_RANGES) {
		    NODE_NO_LINE(node2, NODE_SWITCH_RANGES);
		} else if ((context & SWITCH_NUMBERS) ||
			   (context & SWITCH_NOT_EMPTY)) {
                    NODE_NO_LINE(node2, NODE_SWITCH_NUMBERS);
                } else {
		    // to prevent crashing during the remaining parsing bits
		    NODE_NO_LINE(node2, NODE_SWITCH_NUMBERS);

		    yyerror("need case statements in switch/case, not just default:"); //just a default case present
		}

                node2->l.expr = (yyvsp[-7].node);
                node2->r.expr = node1;
                prepare_cases(node2, (yyvsp[-8].number));
                context = (yyvsp[-9].number);
		(yyval.node) = node2;
		pop_n_locals((yyvsp[-3].decl).num);
            }
#line 3290 "y.tab.c"
    break;

  case 103: /* switch_block: case switch_block  */
#line 1331 "grammar.y"
          {
#line 1355 "grammar.y.pre"
               if ((yyvsp[0].node)){
		   CREATE_STATEMENTS((yyval.node), (yyvsp[-1].node), (yyvsp[0].node));
               } else (yyval.node) = (yyvsp[-1].node);
           }
#line 3301 "y.tab.c"
    break;

  case 104: /* switch_block: statement switch_block  */
#line 1338 "grammar.y"
           {
#line 1361 "grammar.y.pre"
               if ((yyvsp[0].node)){
		   CREATE_STATEMENTS((yyval.node), (yyvsp[-1].node), (yyvsp[0].node));
               } else (yyval.node) = (yyvsp[-1].node);
           }
#line 3312 "y.tab.c"
    break;

  case 105: /* switch_block: %empty  */
#line 1345 "grammar.y"
           {
#line 1367 "grammar.y.pre"
               (yyval.node) = 0;
           }
#line 3321 "y.tab.c"
    break;

  case 106: /* case: L_CASE case_label ':'  */
#line 1354 "grammar.y"
            {
#line 1375 "grammar.y.pre"
                (yyval.node) = (yyvsp[-1].node);
                (yyval.node)->v.expr = 0;

                add_to_mem_block(A_CASES, (char *)&((yyvsp[-1].node)), sizeof((yyvsp[-1].node)));
            }
#line 3333 "y.tab.c"
    break;

  case 107: /* case: L_CASE case_label L_RANGE case_label ':'  */
#line 1362 "grammar.y"
            {
#line 1382 "grammar.y.pre"
                if ( (yyvsp[-3].node)->kind != NODE_CASE_NUMBER
                    || (yyvsp[-1].node)->kind != NODE_CASE_NUMBER )
                    yyerror("String case labels not allowed as range bounds");
                if ((yyvsp[-3].node)->r.number > (yyvsp[-1].node)->r.number) break;

		context |= SWITCH_RANGES;

                (yyval.node) = (yyvsp[-3].node);
                (yyval.node)->v.expr = (yyvsp[-1].node);

                add_to_mem_block(A_CASES, (char *)&((yyvsp[-3].node)), sizeof((yyvsp[-3].node)));
            }
#line 3352 "y.tab.c"
    break;

  case 108: /* case: L_CASE case_label L_RANGE ':'  */
#line 1377 "grammar.y"
            {
#line 1396 "grammar.y.pre"
	        if ( (yyvsp[-2].node)->kind != NODE_CASE_NUMBER )
                    yyerror("String case labels not allowed as range bounds");

		context |= SWITCH_RANGES;

                (yyval.node) = (yyvsp[-2].node);
                (yyval.node)->v.expr = new_node();
		(yyval.node)->v.expr->kind = NODE_CASE_NUMBER;
		(yyval.node)->v.expr->r.number = ((unsigned long)-1)/2; //maxint

                add_to_mem_block(A_CASES, (char *)&((yyvsp[-2].node)), sizeof((yyvsp[-2].node)));
            }
#line 3371 "y.tab.c"
    break;

  case 109: /* case: L_CASE L_RANGE case_label ':'  */
#line 1392 "grammar.y"
            {
#line 1410 "grammar.y.pre"
	      if ( (yyvsp[-1].node)->kind != NODE_CASE_NUMBER )
                    yyerror("String case labels not allowed as range bounds");

		context |= SWITCH_RANGES;
		(yyval.node) = new_node();
		(yyval.node)->kind = NODE_CASE_NUMBER;
                (yyval.node)->r.number = (long) 1+ ((unsigned long)-1)/2; //maxint +1 wraps to min_int, on all computers i know, just not in the C standard iirc 
                (yyval.node)->v.expr = (yyvsp[-1].node);

                add_to_mem_block(A_CASES, (char *)&((yyval.node)), sizeof((yyval.node)));
            }
#line 3389 "y.tab.c"
    break;

  case 110: /* case: L_DEFAULT ':'  */
#line 1406 "grammar.y"
            {
#line 1423 "grammar.y.pre"
                if (context & SWITCH_DEFAULT) {
                    yyerror("Duplicate default");
                    (yyval.node) = 0;
                    break;
                }
		(yyval.node) = new_node();
		(yyval.node)->kind = NODE_DEFAULT;
                (yyval.node)->v.expr = 0;
                add_to_mem_block(A_CASES, (char *)&((yyval.node)), sizeof((yyval.node)));
                context |= SWITCH_DEFAULT;
            }
#line 3407 "y.tab.c"
    break;

  case 111: /* case_label: constant  */
#line 1423 "grammar.y"
            {
#line 1439 "grammar.y.pre"
                if ((context & SWITCH_STRINGS) && (yyvsp[0].pointer_int))
                    yyerror("Mixed case label list not allowed");

                if ((yyvsp[0].pointer_int))
		  context |= SWITCH_NUMBERS;
		else
		  context |= SWITCH_NOT_EMPTY;

		(yyval.node) = new_node();
		(yyval.node)->kind = NODE_CASE_NUMBER;
                (yyval.node)->r.expr = (parse_node_t *)(yyvsp[0].pointer_int);
            }
#line 3426 "y.tab.c"
    break;

  case 112: /* case_label: string_con1  */
#line 1438 "grammar.y"
            {
#line 1453 "grammar.y.pre"
		int str;
		
		str = store_prog_string((yyvsp[0].string));
                scratch_free((yyvsp[0].string));
                if (context & SWITCH_NUMBERS)
                    yyerror("Mixed case label list not allowed");
                context |= SWITCH_STRINGS;
		(yyval.node) = new_node();
		(yyval.node)->kind = NODE_CASE_STRING;
                (yyval.node)->r.number = str;
            }
#line 3444 "y.tab.c"
    break;

  case 113: /* constant: constant '|' constant  */
#line 1455 "grammar.y"
            {
#line 1469 "grammar.y.pre"
                (yyval.pointer_int) = (yyvsp[-2].pointer_int) | (yyvsp[0].pointer_int);
            }
#line 3453 "y.tab.c"
    break;

  case 114: /* constant: constant '^' constant  */
#line 1460 "grammar.y"
            {
#line 1473 "grammar.y.pre"
                (yyval.pointer_int) = (yyvsp[-2].pointer_int) ^ (yyvsp[0].pointer_int);
            }
#line 3462 "y.tab.c"
    break;

  case 115: /* constant: constant '&' constant  */
#line 1465 "grammar.y"
            {
#line 1477 "grammar.y.pre"
                (yyval.pointer_int) = (yyvsp[-2].pointer_int) & (yyvsp[0].pointer_int);
            }
#line 3471 "y.tab.c"
    break;

  case 116: /* constant: constant L_EQ constant  */
#line 1470 "grammar.y"
            {
#line 1481 "grammar.y.pre"
                (yyval.pointer_int) = (yyvsp[-2].pointer_int) == (yyvsp[0].pointer_int);
            }
#line 3480 "y.tab.c"
    break;

  case 117: /* constant: constant L_NE constant  */
#line 1475 "grammar.y"
            {
#line 1485 "grammar.y.pre"
                (yyval.pointer_int) = (yyvsp[-2].pointer_int) != (yyvsp[0].pointer_int);
            }
#line 3489 "y.tab.c"
    break;

  case 118: /* constant: constant L_ORDER constant  */
#line 1480 "grammar.y"
            {
#line 1489 "grammar.y.pre"
                switch((yyvsp[-1].number)){
                    case F_GE: (yyval.pointer_int) = (yyvsp[-2].pointer_int) >= (yyvsp[0].pointer_int); break;
                    case F_LE: (yyval.pointer_int) = (yyvsp[-2].pointer_int) <= (yyvsp[0].pointer_int); break;
                    case F_GT: (yyval.pointer_int) = (yyvsp[-2].pointer_int) >  (yyvsp[0].pointer_int); break;
                }
            }
#line 3502 "y.tab.c"
    break;

  case 119: /* constant: constant '<' constant  */
#line 1489 "grammar.y"
            {
#line 1497 "grammar.y.pre"
                (yyval.pointer_int) = (yyvsp[-2].pointer_int) < (yyvsp[0].pointer_int);
            }
#line 3511 "y.tab.c"
    break;

  case 120: /* constant: constant L_LSH constant  */
#line 1494 "grammar.y"
            {
#line 1501 "grammar.y.pre"
                (yyval.pointer_int) = (yyvsp[-2].pointer_int) << (yyvsp[0].pointer_int);
            }
#line 3520 "y.tab.c"
    break;

  case 121: /* constant: constant L_RSH constant  */
#line 1499 "grammar.y"
            {
#line 1505 "grammar.y.pre"
                (yyval.pointer_int) = (yyvsp[-2].pointer_int) >> (yyvsp[0].pointer_int);
            }
#line 3529 "y.tab.c"
    break;

  case 122: /* constant: constant '+' constant  */
#line 1504 "grammar.y"
            {
#line 1509 "grammar.y.pre"
                (yyval.pointer_int) = (yyvsp[-2].pointer_int) + (yyvsp[0].pointer_int);
            }
#line 3538 "y.tab.c"
    break;

  case 123: /* constant: constant '-' constant  */
#line 1509 "grammar.y"
            {
#line 1513 "grammar.y.pre"
                (yyval.pointer_int) = (yyvsp[-2].pointer_int) - (yyvsp[0].pointer_int);
            }
#line 3547 "y.tab.c"
    break;

  case 124: /* constant: constant '*' constant  */
#line 1514 "grammar.y"
            {
#line 1517 "grammar.y.pre"
                (yyval.pointer_int) = (yyvsp[-2].pointer_int) * (yyvsp[0].pointer_int);
            }
#line 3556 "y.tab.c"
    break;

  case 125: /* constant: constant '%' constant  */
#line 1519 "grammar.y"
            {
#line 1521 "grammar.y.pre"
                if ((yyvsp[0].pointer_int)) (yyval.pointer_int) = (yyvsp[-2].pointer_int) % (yyvsp[0].pointer_int); else yyerror("Modulo by zero");
            }
#line 3565 "y.tab.c"
    break;

  case 126: /* constant: constant '/' constant  */
#line 1524 "grammar.y"
            {
#line 1525 "grammar.y.pre"
                if ((yyvsp[0].pointer_int)) (yyval.pointer_int) = (yyvsp[-2].pointer_int) / (yyvsp[0].pointer_int); else yyerror("Division by zero");
            }
#line 3574 "y.tab.c"
    break;

  case 127: /* constant: '(' constant ')'  */
#line 1529 "grammar.y"
            {
#line 1529 "grammar.y.pre"
                (yyval.pointer_int) = (yyvsp[-1].pointer_int);
            }
#line 3583 "y.tab.c"
    break;

  case 128: /* constant: L_NUMBER  */
#line 1534 "grammar.y"
            {
#line 1533 "grammar.y.pre"
		(yyval.pointer_int) = (yyvsp[0].number);
	    }
#line 3592 "y.tab.c"
    break;

  case 129: /* constant: '-' L_NUMBER  */
#line 1539 "grammar.y"
            {
#line 1537 "grammar.y.pre"
                (yyval.pointer_int) = -(yyvsp[0].number);
            }
#line 3601 "y.tab.c"
    break;

  case 130: /* constant: L_NOT L_NUMBER  */
#line 1544 "grammar.y"
            {
#line 1541 "grammar.y.pre"
                (yyval.pointer_int) = !(yyvsp[0].number);
            }
#line 3610 "y.tab.c"
    break;

  case 131: /* constant: '~' L_NUMBER  */
#line 1549 "grammar.y"
            {
#line 1545 "grammar.y.pre"
                (yyval.pointer_int) = ~(yyvsp[0].number);
            }
#line 3619 "y.tab.c"
    break;

  case 132: /* comma_expr: expr0  */
#line 1557 "grammar.y"
            {
#line 1552 "grammar.y.pre"
		(yyval.node) = (yyvsp[0].node);
	    }
#line 3628 "y.tab.c"
    break;

  case 133: /* comma_expr: comma_expr ',' expr0  */
#line 1562 "grammar.y"
            {
#line 1556 "grammar.y.pre"
		CREATE_TWO_VALUES((yyval.node), (yyvsp[0].node)->type, pop_value((yyvsp[-2].node)), (yyvsp[0].node));
	    }
#line 3637 "y.tab.c"
    break;

  case 134: /* expr0: lvalue L_ASSIGN expr0  */
#line 1571 "grammar.y"
            {
#line 1602 "grammar.y.pre"
	        parse_node_t *l = (yyvsp[-2].node), *r = (yyvsp[0].node);
		/* set this up here so we can change it below */
		/* assignments are backwards; rhs is evaluated before
		   lhs, so put the RIGHT hand side on the LEFT hand
		   side of the tree node. */
		CREATE_BINARY_OP((yyval.node), (yyvsp[-1].number), r->type, r, l);

		if (exact_types && !compatible_types(r->type, l->type) &&
		    !((yyvsp[-1].number) == F_ADD_EQ
		      && l->type == TYPE_STRING && 
		      (COMP_TYPE(r->type, TYPE_NUMBER))||r->type == TYPE_OBJECT)) {
		    char buf[256];
		    char *end = EndOf(buf);
		    char *p;
		    p = strput(buf, end, "Bad assignment ");
		    p = get_two_types(p, end, l->type, r->type);
		    p = strput(p, end, ".");
		    yyerror(buf);
		}
		
		if ((yyvsp[-1].number) == F_ASSIGN)
		    (yyval.node)->l.expr = do_promotions(r, l->type);
	    }
#line 3667 "y.tab.c"
    break;

  case 135: /* expr0: error L_ASSIGN expr0  */
#line 1597 "grammar.y"
            {
#line 1627 "grammar.y.pre"
		yyerror("Illegal LHS");
		CREATE_ERROR((yyval.node));
	    }
#line 3677 "y.tab.c"
    break;

  case 136: /* expr0: expr0 '?' expr0 ':' expr0  */
#line 1603 "grammar.y"
            {
#line 1632 "grammar.y.pre"
		parse_node_t *p1 = (yyvsp[-2].node), *p2 = (yyvsp[0].node);

		if (exact_types && !compatible_types2(p1->type, p2->type)) {
		    char buf[256];
		    char *end = EndOf(buf);
		    char *p;
		    
		    p = strput(buf, end, "Types in ?: do not match ");
		    p = get_two_types(p, end, p1->type, p2->type);
		    p = strput(p, end, ".");
		    yywarn(buf);
		}

		/* optimize if last expression did F_NOT */
		if (IS_NODE((yyvsp[-4].node), NODE_UNARY_OP, F_NOT)) {
		    /* !a ? b : c  --> a ? c : b */
		    CREATE_IF((yyval.node), (yyvsp[-4].node)->r.expr, p2, p1);
		} else {
		    CREATE_IF((yyval.node), (yyvsp[-4].node), p1, p2);
		}
		(yyval.node)->type = ((p1->type == p2->type) ? p1->type : TYPE_ANY);
	    }
#line 3706 "y.tab.c"
    break;

  case 137: /* expr0: expr0 L_LOR expr0  */
#line 1628 "grammar.y"
            {
#line 1656 "grammar.y.pre"
		CREATE_LAND_LOR((yyval.node), F_LOR, (yyvsp[-2].node), (yyvsp[0].node));
		if (IS_NODE((yyvsp[-2].node), NODE_LAND_LOR, F_LOR))
		    (yyvsp[-2].node)->kind = NODE_BRANCH_LINK;
	    }
#line 3717 "y.tab.c"
    break;

  case 138: /* expr0: expr0 L_LAND expr0  */
#line 1635 "grammar.y"
            {
#line 1662 "grammar.y.pre"
		CREATE_LAND_LOR((yyval.node), F_LAND, (yyvsp[-2].node), (yyvsp[0].node));
		if (IS_NODE((yyvsp[-2].node), NODE_LAND_LOR, F_LAND))
		    (yyvsp[-2].node)->kind = NODE_BRANCH_LINK;
	    }
#line 3728 "y.tab.c"
    break;

  case 139: /* expr0: expr0 '|' expr0  */
#line 1642 "grammar.y"
            {
#line 1668 "grammar.y.pre"
		int t1 = (yyvsp[-2].node)->type, t3 = (yyvsp[0].node)->type;
		
		if (is_boolean((yyvsp[-2].node)) && is_boolean((yyvsp[0].node)))
		    yywarn("bitwise operation on boolean values.");
		if ((t1 & TYPE_MOD_ARRAY) || (t3 & TYPE_MOD_ARRAY)) {
		    if (t1 != t3) {
			if ((t1 != TYPE_ANY) && (t3 != TYPE_ANY) &&
			    !(t1 & t3 & TYPE_MOD_ARRAY)) {
			    char buf[256];
			    char *end = EndOf(buf);
			    char *p;

			    p = strput(buf, end, "Incompatible types for | ");
			    p = get_two_types(p, end, t1, t3);
			    p = strput(p, end, ".");
			    yyerror(buf);
			}
			t1 = TYPE_ANY | TYPE_MOD_ARRAY;
		    }
		    CREATE_BINARY_OP((yyval.node), F_OR, t1, (yyvsp[-2].node), (yyvsp[0].node));
		}
		else (yyval.node) = binary_int_op((yyvsp[-2].node), (yyvsp[0].node), F_OR, "|");		
	    }
#line 3758 "y.tab.c"
    break;

  case 140: /* expr0: expr0 '^' expr0  */
#line 1668 "grammar.y"
            {
#line 1693 "grammar.y.pre"
		(yyval.node) = binary_int_op((yyvsp[-2].node), (yyvsp[0].node), F_XOR, "^");
	    }
#line 3767 "y.tab.c"
    break;

  case 141: /* expr0: expr0 '&' expr0  */
#line 1673 "grammar.y"
            {
#line 1697 "grammar.y.pre"
		int t1 = (yyvsp[-2].node)->type, t3 = (yyvsp[0].node)->type;
		if (is_boolean((yyvsp[-2].node)) && is_boolean((yyvsp[0].node)))
		    yywarn("bitwise operation on boolean values.");
		if ((t1 & TYPE_MOD_ARRAY) || (t3 & TYPE_MOD_ARRAY)) {
		    if (t1 != t3) {
			if ((t1 != TYPE_ANY) && (t3 != TYPE_ANY) &&
			    !(t1 & t3 & TYPE_MOD_ARRAY)) {
			    char buf[256];
			    char *end = EndOf(buf);
			    char *p;
			    
			    p = strput(buf, end, "Incompatible types for & ");
			    p = get_two_types(p, end, t1, t3);
			    p = strput(p, end, ".");
			    yyerror(buf);
			}
			t1 = TYPE_ANY | TYPE_MOD_ARRAY;
		    } 
		    CREATE_BINARY_OP((yyval.node), F_AND, t1, (yyvsp[-2].node), (yyvsp[0].node));
		} else (yyval.node) = binary_int_op((yyvsp[-2].node), (yyvsp[0].node), F_AND, "&");
	    }
#line 3795 "y.tab.c"
    break;

  case 142: /* expr0: expr0 L_EQ expr0  */
#line 1697 "grammar.y"
            {
#line 1720 "grammar.y.pre"
		if (exact_types && !compatible_types2((yyvsp[-2].node)->type, (yyvsp[0].node)->type)){
		    char buf[256];
		    char *end = EndOf(buf);
		    char *p;
		    
		    p = strput(buf, end, "== always false because of incompatible types ");
		    p = get_two_types(p, end, (yyvsp[-2].node)->type, (yyvsp[0].node)->type);
		    p = strput(p, end, ".");
		    yyerror(buf);
		}
		/* x == 0 -> !x */
		if (IS_NODE((yyvsp[-2].node), NODE_NUMBER, 0)) {
		    CREATE_UNARY_OP((yyval.node), F_NOT, TYPE_NUMBER, (yyvsp[0].node));
		} else
		if (IS_NODE((yyvsp[0].node), NODE_NUMBER, 0)) {
		    CREATE_UNARY_OP((yyval.node), F_NOT, TYPE_NUMBER, (yyvsp[-2].node));
		} else {
		    CREATE_BINARY_OP((yyval.node), F_EQ, TYPE_NUMBER, (yyvsp[-2].node), (yyvsp[0].node));
		}
	    }
#line 3822 "y.tab.c"
    break;

  case 143: /* expr0: expr0 L_NE expr0  */
#line 1720 "grammar.y"
            {
#line 1742 "grammar.y.pre"
		if (exact_types && !compatible_types2((yyvsp[-2].node)->type, (yyvsp[0].node)->type)){
		    char buf[256];
		    char *end = EndOf(buf);
		    char *p;

		    p = strput(buf, end, "!= always true because of incompatible types ");
		    p = get_two_types(p, end, (yyvsp[-2].node)->type, (yyvsp[0].node)->type);
		    p = strput(p, end, ".");
		    yyerror(buf);
		}
                CREATE_BINARY_OP((yyval.node), F_NE, TYPE_NUMBER, (yyvsp[-2].node), (yyvsp[0].node));
	    }
#line 3841 "y.tab.c"
    break;

  case 144: /* expr0: expr0 L_ORDER expr0  */
#line 1735 "grammar.y"
            {
#line 1756 "grammar.y.pre"
		if (exact_types) {
		    int t1 = (yyvsp[-2].node)->type;
		    int t3 = (yyvsp[0].node)->type;

		    if (!COMP_TYPE(t1, TYPE_NUMBER) 
			&& !COMP_TYPE(t1, TYPE_STRING)) {
			char buf[256];
			char *end = EndOf(buf);
			char *p;
			
			p = strput(buf, end, "Bad left argument to '");
			p = strput(p, end, query_instr_name((yyvsp[-1].number)));
			p = strput(p, end, "' : \"");
			p = get_type_name(p, end, t1);
			p = strput(p, end, "\"");
			yyerror(buf);
		    } else if (!COMP_TYPE(t3, TYPE_NUMBER) 
			       && !COMP_TYPE(t3, TYPE_STRING)) {
                        char buf[256];
			char *end = EndOf(buf);
			char *p;
			
                        p = strput(buf, end, "Bad right argument to '");
                        p = strput(p, end, query_instr_name((yyvsp[-1].number)));
                        p = strput(p, end, "' : \"");
                        p = get_type_name(p, end, t3);
			p = strput(p, end, "\"");
			yyerror(buf);
		    } else if (!compatible_types2(t1,t3)) {
			char buf[256];
			char *end = EndOf(buf);
			char *p;
			
			p = strput(buf, end, "Arguments to ");
			p = strput(p, end, query_instr_name((yyvsp[-1].number)));
			p = strput(p, end, " do not have compatible types : ");
			p = get_two_types(p, end, t1, t3);
			yyerror(buf);
		    }
		}
                CREATE_BINARY_OP((yyval.node), (yyvsp[-1].number), TYPE_NUMBER, (yyvsp[-2].node), (yyvsp[0].node));
	    }
#line 3890 "y.tab.c"
    break;

  case 145: /* expr0: expr0 '<' expr0  */
#line 1780 "grammar.y"
            {
#line 1800 "grammar.y.pre"
                if (exact_types) {
                    int t1 = (yyvsp[-2].node)->type, t3 = (yyvsp[0].node)->type;

                    if (!COMP_TYPE(t1, TYPE_NUMBER) 
			&& !COMP_TYPE(t1, TYPE_STRING)) {
                        char buf[256];
			char *end = EndOf(buf);
			char *p;
			
			p = strput(buf, end, "Bad left argument to '<' : \"");
                        p = get_type_name(p, end, t1);
			p = strput(p, end, "\"");
                        yyerror(buf);
                    } else if (!COMP_TYPE(t3, TYPE_NUMBER)
			       && !COMP_TYPE(t3, TYPE_STRING)) {
                        char buf[200];
			char *end = EndOf(buf);
			char *p;
			
                        p = strput(buf, end, "Bad right argument to '<' : \"");
                        p = get_type_name(p, end, t3);
                        p = strput(p, end, "\"");
                        yyerror(buf);
                    } else if (!compatible_types2(t1,t3)) {
                        char buf[256];
			char *end = EndOf(buf);
			char *p;
			
			p = strput(buf, end, "Arguments to < do not have compatible types : ");
			p = get_two_types(p, end, t1, t3);
                        yyerror(buf);
                    }
                }
                CREATE_BINARY_OP((yyval.node), F_LT, TYPE_NUMBER, (yyvsp[-2].node), (yyvsp[0].node));
            }
#line 3932 "y.tab.c"
    break;

  case 146: /* expr0: expr0 L_LSH expr0  */
#line 1818 "grammar.y"
            {
#line 1837 "grammar.y.pre"
		(yyval.node) = binary_int_op((yyvsp[-2].node), (yyvsp[0].node), F_LSH, "<<");
	    }
#line 3941 "y.tab.c"
    break;

  case 147: /* expr0: expr0 L_RSH expr0  */
#line 1823 "grammar.y"
            {
#line 1841 "grammar.y.pre"
		(yyval.node) = binary_int_op((yyvsp[-2].node), (yyvsp[0].node), F_RSH, ">>");
	    }
#line 3950 "y.tab.c"
    break;

  case 148: /* expr0: expr0 '+' expr0  */
#line 1828 "grammar.y"
            {
#line 1845 "grammar.y.pre"
		int result_type;

		if (exact_types) {
		    int t1 = (yyvsp[-2].node)->type, t3 = (yyvsp[0].node)->type;

		    if (t1 == t3){
#ifdef CAST_CALL_OTHERS
			if (t1 == TYPE_UNKNOWN){
			    yyerror("Bad arguments to '+' (unknown vs unknown)");
			    result_type = TYPE_ANY;
			} else
#endif
			    result_type = t1;
		    }
		    else if (t1 == TYPE_ANY) {
			if (t3 == TYPE_FUNCTION) {
			    yyerror("Bad right argument to '+' (function)");
			    result_type = TYPE_ANY;
			} else result_type = t3;
		    } else if (t3 == TYPE_ANY) {
			if (t1 == TYPE_FUNCTION) {
			    yyerror("Bad left argument to '+' (function)");
			    result_type = TYPE_ANY;
			} else result_type = t1;
		    } else {
			switch(t1) {
			case TYPE_OBJECT:
			  if(t3 == TYPE_STRING){
			    result_type = TYPE_STRING;
			  } else goto add_error;
			  break;
			case TYPE_STRING:
			  {
			    if (t3 == TYPE_REAL || t3 == TYPE_NUMBER || t3 == TYPE_OBJECT){
			      result_type = TYPE_STRING;
			    } else goto add_error;
			    break;
			  }
			case TYPE_NUMBER:
			  {
			    if (t3 == TYPE_REAL || t3 == TYPE_STRING)
			      result_type = t3;
			    else goto add_error;
			    break;
			  }
			case TYPE_REAL:
			  {
			    if (t3 == TYPE_NUMBER) result_type = TYPE_REAL;
			    else if (t3 == TYPE_STRING) result_type = TYPE_STRING;
			    else goto add_error;
			    break;
			  }
			default:
			  {
			    if (t1 & t3 & TYPE_MOD_ARRAY) {
			      result_type = TYPE_ANY|TYPE_MOD_ARRAY;
			      break;
			    }
			  add_error:
			    {
			      char buf[256];
			      char *end = EndOf(buf);
			      char *p;
			      
			      p = strput(buf, end, "Invalid argument types to '+' ");
			      p = get_two_types(p, end, t1, t3);
			      yyerror(buf);
			      result_type = TYPE_ANY;
			    }
			  }
			}
		    }
		} else 
		    result_type = TYPE_ANY;

		/* TODO: perhaps we should do (string)+(number) and
		 * (number)+(string) constant folding as well.
		 *
		 * codefor string x = "foo" + 1;
		 *
		 * 0000: push string 13, number 1
		 * 0004: +
		 * 0005: (void)assign_local LV0
		 */
		switch ((yyvsp[-2].node)->kind) {
		case NODE_NUMBER:
		    /* 0 + X */
		    if ((yyvsp[-2].node)->v.number == 0 &&
			((yyvsp[0].node)->type == TYPE_NUMBER || (yyvsp[0].node)->type == TYPE_REAL)) {
			(yyval.node) = (yyvsp[0].node);
			break;
		    }
		    if ((yyvsp[0].node)->kind == NODE_NUMBER) {
			(yyval.node) = (yyvsp[-2].node);
			(yyvsp[-2].node)->v.number += (yyvsp[0].node)->v.number;
			break;
		    }
		    if ((yyvsp[0].node)->kind == NODE_REAL) {
			(yyval.node) = (yyvsp[0].node);
			(yyvsp[0].node)->v.real += (yyvsp[-2].node)->v.number;
			break;
		    }
		    /* swapping the nodes may help later constant folding */
		    if ((yyvsp[0].node)->type != TYPE_STRING && (yyvsp[0].node)->type != TYPE_ANY)
			CREATE_BINARY_OP((yyval.node), F_ADD, result_type, (yyvsp[0].node), (yyvsp[-2].node));
		    else
			CREATE_BINARY_OP((yyval.node), F_ADD, result_type, (yyvsp[-2].node), (yyvsp[0].node));
		    break;
		case NODE_REAL:
		    if ((yyvsp[0].node)->kind == NODE_NUMBER) {
			(yyval.node) = (yyvsp[-2].node);
			(yyvsp[-2].node)->v.real += (yyvsp[0].node)->v.number;
			break;
		    }
		    if ((yyvsp[0].node)->kind == NODE_REAL) {
			(yyval.node) = (yyvsp[-2].node);
			(yyvsp[-2].node)->v.real += (yyvsp[0].node)->v.real;
			break;
		    }
		    /* swapping the nodes may help later constant folding */
		    if ((yyvsp[0].node)->type != TYPE_STRING && (yyvsp[0].node)->type != TYPE_ANY)
			CREATE_BINARY_OP((yyval.node), F_ADD, result_type, (yyvsp[0].node), (yyvsp[-2].node));
		    else
			CREATE_BINARY_OP((yyval.node), F_ADD, result_type, (yyvsp[-2].node), (yyvsp[0].node));
		    break;
		case NODE_STRING:
		    if ((yyvsp[0].node)->kind == NODE_STRING) {
			/* Combine strings */
			long n1, n2;
			const char *s1, *s2;
			char *new;
			int l;

			n1 = (yyvsp[-2].node)->v.number;
			n2 = (yyvsp[0].node)->v.number;
			s1 = PROG_STRING(n1);
			s2 = PROG_STRING(n2);
			new = (char *)DXALLOC( (l = strlen(s1))+strlen(s2)+1, TAG_COMPILER, "combine string" );
			strcpy(new, s1);
			strcat(new + l, s2);
			/* free old strings (ordering may help shrink table) */
			if (n1 > n2) {
			    free_prog_string(n1); free_prog_string(n2);
			} else {
			    free_prog_string(n2); free_prog_string(n1);
			}
			(yyval.node) = (yyvsp[-2].node);
			(yyval.node)->v.number = store_prog_string(new);
			FREE(new);
			break;
		    }
		    /* Yes, this can actually happen for absurd code like:
		     * (int)"foo" + 0
		     * for which I guess we ought to generate (int)"foo"
		     * in order to be consistent.  Then shoot the coder.
		     */
		    /* FALLTHROUGH */
		default:
		    /* X + 0 */
		    if (IS_NODE((yyvsp[0].node), NODE_NUMBER, 0) &&
			((yyvsp[-2].node)->type == TYPE_NUMBER || (yyvsp[-2].node)->type == TYPE_REAL)) {
			(yyval.node) = (yyvsp[-2].node);
			break;
		    }
		    CREATE_BINARY_OP((yyval.node), F_ADD, result_type, (yyvsp[-2].node), (yyvsp[0].node));
		    break;
		}
	    }
#line 4125 "y.tab.c"
    break;

  case 149: /* expr0: expr0 '-' expr0  */
#line 1999 "grammar.y"
            {
#line 2015 "grammar.y.pre"
		int result_type;

		if (exact_types) {
		    int t1 = (yyvsp[-2].node)->type, t3 = (yyvsp[0].node)->type;

		    if (t1 == t3){
			switch(t1){
			    case TYPE_ANY:
			    case TYPE_NUMBER:
			    case TYPE_REAL:
			        result_type = t1;
				break;
			    default:
				if (!(t1 & TYPE_MOD_ARRAY)){
				    type_error("Bad argument number 1 to '-'", t1);
				    result_type = TYPE_ANY;
				} else result_type = t1;
			}
		    } else if (t1 == TYPE_ANY){
			switch(t3){
			    case TYPE_REAL:
			    case TYPE_NUMBER:
			        result_type = t3;
				break;
			    default:
				if (!(t3 & TYPE_MOD_ARRAY)){
				    type_error("Bad argument number 2 to '-'", t3);
				    result_type = TYPE_ANY;
				} else result_type = t3;
			}
		    } else if (t3 == TYPE_ANY){
			switch(t1){
			    case TYPE_REAL:
			    case TYPE_NUMBER:
			        result_type = t1;
				break;
			    default:
				if (!(t1 & TYPE_MOD_ARRAY)){
				    type_error("Bad argument number 1 to '-'", t1);
				    result_type = TYPE_ANY;
				} else result_type = t1;
			}
		    } else if ((t1 == TYPE_REAL && t3 == TYPE_NUMBER) ||
			       (t3 == TYPE_REAL && t1 == TYPE_NUMBER)){
			result_type = TYPE_REAL;
		    } else if (t1 & t3 & TYPE_MOD_ARRAY){
			result_type = TYPE_MOD_ARRAY|TYPE_ANY;
		    } else {
			char buf[256];
			char *end = EndOf(buf);
			char *p;
			
			p = strput(buf, end, "Invalid types to '-' ");
			p = get_two_types(p, end, t1, t3);
			yyerror(buf);
			result_type = TYPE_ANY;
		    }
		} else result_type = TYPE_ANY;
		
		switch ((yyvsp[-2].node)->kind) {
		case NODE_NUMBER:
		    if ((yyvsp[-2].node)->v.number == 0) {
			CREATE_UNARY_OP((yyval.node), F_NEGATE, (yyvsp[0].node)->type, (yyvsp[0].node));
		    } else if ((yyvsp[0].node)->kind == NODE_NUMBER) {
			(yyval.node) = (yyvsp[-2].node);
			(yyvsp[-2].node)->v.number -= (yyvsp[0].node)->v.number;
		    } else if ((yyvsp[0].node)->kind == NODE_REAL) {
			(yyval.node) = (yyvsp[0].node);
			(yyvsp[0].node)->v.real = (yyvsp[-2].node)->v.number - (yyvsp[0].node)->v.real;
		    } else {
			CREATE_BINARY_OP((yyval.node), F_SUBTRACT, result_type, (yyvsp[-2].node), (yyvsp[0].node));
		    }
		    break;
		case NODE_REAL:
		    if ((yyvsp[0].node)->kind == NODE_NUMBER) {
			(yyval.node) = (yyvsp[-2].node);
			(yyvsp[-2].node)->v.real -= (yyvsp[0].node)->v.number;
		    } else if ((yyvsp[0].node)->kind == NODE_REAL) {
			(yyval.node) = (yyvsp[-2].node);
			(yyvsp[-2].node)->v.real -= (yyvsp[0].node)->v.real;
		    } else {
			CREATE_BINARY_OP((yyval.node), F_SUBTRACT, result_type, (yyvsp[-2].node), (yyvsp[0].node));
		    }
		    break;
		default:
		    /* optimize X-0 */
		    if (IS_NODE((yyvsp[0].node), NODE_NUMBER, 0)) {
			(yyval.node) = (yyvsp[-2].node);
		    } 
		    CREATE_BINARY_OP((yyval.node), F_SUBTRACT, result_type, (yyvsp[-2].node), (yyvsp[0].node));
		}
	    }
#line 4224 "y.tab.c"
    break;

  case 150: /* expr0: expr0 '*' expr0  */
#line 2094 "grammar.y"
            {
#line 2109 "grammar.y.pre"
		int result_type;

		if (exact_types){
		    int t1 = (yyvsp[-2].node)->type, t3 = (yyvsp[0].node)->type;

		    if (t1 == t3){
			switch(t1){
			    case TYPE_MAPPING:
			    case TYPE_ANY:
			    case TYPE_NUMBER:
			    case TYPE_REAL:
			        result_type = t1;
				break;
			default:
				type_error("Bad argument number 1 to '*'", t1);
				result_type = TYPE_ANY;
			}
		    } else if (t1 == TYPE_ANY || t3 == TYPE_ANY){
			int t = (t1 == TYPE_ANY) ? t3 : t1;
			switch(t){
			    case TYPE_NUMBER:
			    case TYPE_REAL:
			    case TYPE_MAPPING:
			        result_type = t;
				break;
			    default:
				type_error((t1 == TYPE_ANY) ?
					   "Bad argument number 2 to '*'" :
					   "Bad argument number 1 to '*'",
					   t);
				result_type = TYPE_ANY;
			}
		    } else if ((t1 == TYPE_NUMBER && t3 == TYPE_REAL) ||
			       (t1 == TYPE_REAL && t3 == TYPE_NUMBER)){
			result_type = TYPE_REAL;
		    } else {
			char buf[256];
			char *end = EndOf(buf);
			char *p;
			
			p = strput(buf, end, "Invalid types to '*' ");
			p = get_two_types(p, end, t1, t3);
			yyerror(buf);
			result_type = TYPE_ANY;
		    }
		} else result_type = TYPE_ANY;

		switch ((yyvsp[-2].node)->kind) {
		case NODE_NUMBER:
		    if ((yyvsp[0].node)->kind == NODE_NUMBER) {
			(yyval.node) = (yyvsp[-2].node);
			(yyval.node)->v.number *= (yyvsp[0].node)->v.number;
			break;
		    }
		    if ((yyvsp[0].node)->kind == NODE_REAL) {
			(yyval.node) = (yyvsp[0].node);
			(yyvsp[0].node)->v.real *= (yyvsp[-2].node)->v.number;
			break;
		    }
		    CREATE_BINARY_OP((yyval.node), F_MULTIPLY, result_type, (yyvsp[0].node), (yyvsp[-2].node));
		    break;
		case NODE_REAL:
		    if ((yyvsp[0].node)->kind == NODE_NUMBER) {
			(yyval.node) = (yyvsp[-2].node);
			(yyvsp[-2].node)->v.real *= (yyvsp[0].node)->v.number;
			break;
		    }
		    if ((yyvsp[0].node)->kind == NODE_REAL) {
			(yyval.node) = (yyvsp[-2].node);
			(yyvsp[-2].node)->v.real *= (yyvsp[0].node)->v.real;
			break;
		    }
		    CREATE_BINARY_OP((yyval.node), F_MULTIPLY, result_type, (yyvsp[0].node), (yyvsp[-2].node));
		    break;
		default:
		    CREATE_BINARY_OP((yyval.node), F_MULTIPLY, result_type, (yyvsp[-2].node), (yyvsp[0].node));
		}
	    }
#line 4309 "y.tab.c"
    break;

  case 151: /* expr0: expr0 '%' expr0  */
#line 2175 "grammar.y"
            {
#line 2189 "grammar.y.pre"
		(yyval.node) = binary_int_op((yyvsp[-2].node), (yyvsp[0].node), F_MOD, "%");
	    }
#line 4318 "y.tab.c"
    break;

  case 152: /* expr0: expr0 '/' expr0  */
#line 2180 "grammar.y"
            {
#line 2193 "grammar.y.pre"
		int result_type;

		if (exact_types){
		    int t1 = (yyvsp[-2].node)->type, t3 = (yyvsp[0].node)->type;

		    if (t1 == t3){
			switch(t1){
			    case TYPE_NUMBER:
			    case TYPE_REAL:
			case TYPE_ANY:
			        result_type = t1;
				break;
			    default:
				type_error("Bad argument 1 to '/'", t1);
				result_type = TYPE_ANY;
			}
		    } else if (t1 == TYPE_ANY || t3 == TYPE_ANY){
			int t = (t1 == TYPE_ANY) ? t3 : t1;
			if (t == TYPE_REAL || t == TYPE_NUMBER)
			    result_type = t; 
			else {
			    type_error(t1 == TYPE_ANY ?
				       "Bad argument 2 to '/'" :
				       "Bad argument 1 to '/'", t);
			    result_type = TYPE_ANY;
			}
		    } else if ((t1 == TYPE_NUMBER && t3 == TYPE_REAL) ||
			       (t1 == TYPE_REAL && t3 == TYPE_NUMBER)) {
			result_type = TYPE_REAL;
		    } else {
			char buf[256];
			char *end = EndOf(buf);
			char *p;
			
			p = strput(buf, end, "Invalid types to '/' ");
			p = get_two_types(p, end, t1, t3);
			yyerror(buf);
			result_type = TYPE_ANY;
		    }
		} else result_type = TYPE_ANY;		    

		/* constant expressions */
		switch ((yyvsp[-2].node)->kind) {
		case NODE_NUMBER:
		    if ((yyvsp[0].node)->kind == NODE_NUMBER) {
			if ((yyvsp[0].node)->v.number == 0) {
			    yyerror("Divide by zero in constant");
			    (yyval.node) = (yyvsp[-2].node);
			    break;
			}
			(yyval.node) = (yyvsp[-2].node);
			(yyvsp[-2].node)->v.number /= (yyvsp[0].node)->v.number;
			break;
		    }
		    if ((yyvsp[0].node)->kind == NODE_REAL) {
			if ((yyvsp[0].node)->v.real == 0.0) {
			    yyerror("Divide by zero in constant");
			    (yyval.node) = (yyvsp[-2].node);
			    break;
			}
			(yyval.node) = (yyvsp[0].node);
			(yyvsp[0].node)->v.real = ((yyvsp[-2].node)->v.number / (yyvsp[0].node)->v.real);
			break;
		    }
		    CREATE_BINARY_OP((yyval.node), F_DIVIDE, result_type, (yyvsp[-2].node), (yyvsp[0].node));
		    break;
		case NODE_REAL:
		    if ((yyvsp[0].node)->kind == NODE_NUMBER) {
			if ((yyvsp[0].node)->v.number == 0) {
			    yyerror("Divide by zero in constant");
			    (yyval.node) = (yyvsp[-2].node);
			    break;
			}
			(yyval.node) = (yyvsp[-2].node);
			(yyvsp[-2].node)->v.real /= (yyvsp[0].node)->v.number;
			break;
		    }
		    if ((yyvsp[0].node)->kind == NODE_REAL) {
			if ((yyvsp[0].node)->v.real == 0.0) {
			    yyerror("Divide by zero in constant");
			    (yyval.node) = (yyvsp[-2].node);
			    break;
			}
			(yyval.node) = (yyvsp[-2].node);
			(yyvsp[-2].node)->v.real /= (yyvsp[0].node)->v.real;
			break;
		    }
		    CREATE_BINARY_OP((yyval.node), F_DIVIDE, result_type, (yyvsp[-2].node), (yyvsp[0].node));
		    break;
		default:
		    CREATE_BINARY_OP((yyval.node), F_DIVIDE, result_type, (yyvsp[-2].node), (yyvsp[0].node));
		}
	    }
#line 4418 "y.tab.c"
    break;

  case 153: /* expr0: cast expr0  */
#line 2276 "grammar.y"
            {
#line 2288 "grammar.y.pre"
		(yyval.node) = (yyvsp[0].node);
		(yyval.node)->type = (yyvsp[-1].type);

		if (exact_types &&
		    (yyvsp[0].node)->type != (yyvsp[-1].type) &&
		    (yyvsp[0].node)->type != TYPE_ANY && 
		    (yyvsp[0].node)->type != TYPE_UNKNOWN &&
		    (yyvsp[-1].type) != TYPE_VOID) {
		    char buf[256];
		    char *end = EndOf(buf);
		    char *p;
		    
		    p = strput(buf, end, "Cannot cast ");
		    p = get_type_name(p, end, (yyvsp[0].node)->type);
		    p = strput(p, end, "to ");
		    p = get_type_name(p, end, (yyvsp[-1].type));
		    yyerror(buf);
		}
	    }
#line 4444 "y.tab.c"
    break;

  case 154: /* expr0: L_INC lvalue  */
#line 2298 "grammar.y"
            {
#line 2309 "grammar.y.pre"
		CREATE_UNARY_OP((yyval.node), F_PRE_INC, 0, (yyvsp[0].node));
                if (exact_types){
                    switch((yyvsp[0].node)->type){
                        case TYPE_NUMBER:
                        case TYPE_ANY:
                        case TYPE_REAL:
                        {
                            (yyval.node)->type = (yyvsp[0].node)->type;
                            break;
                        }

                        default:
                        {
                            (yyval.node)->type = TYPE_ANY;
                            type_error("Bad argument 1 to ++x", (yyvsp[0].node)->type);
                        }
                    }
                } else (yyval.node)->type = TYPE_ANY;
	    }
#line 4470 "y.tab.c"
    break;

  case 155: /* expr0: L_DEC lvalue  */
#line 2320 "grammar.y"
            {
#line 2330 "grammar.y.pre"
		CREATE_UNARY_OP((yyval.node), F_PRE_DEC, 0, (yyvsp[0].node));
                if (exact_types){
                    switch((yyvsp[0].node)->type){
                        case TYPE_NUMBER:
                        case TYPE_ANY:
                        case TYPE_REAL:
                        {
                            (yyval.node)->type = (yyvsp[0].node)->type;
                            break;
                        }

                        default:
                        {
                            (yyval.node)->type = TYPE_ANY;
                            type_error("Bad argument 1 to --x", (yyvsp[0].node)->type);
                        }
                    }
                } else (yyval.node)->type = TYPE_ANY;

	    }
#line 4497 "y.tab.c"
    break;

  case 156: /* expr0: L_NOT expr0  */
#line 2343 "grammar.y"
            {
#line 2352 "grammar.y.pre"
		if ((yyvsp[0].node)->kind == NODE_NUMBER) {
		    (yyval.node) = (yyvsp[0].node);
		    (yyval.node)->v.number = !((yyval.node)->v.number);
		} else {
		    CREATE_UNARY_OP((yyval.node), F_NOT, TYPE_NUMBER, (yyvsp[0].node));
		}
	    }
#line 4511 "y.tab.c"
    break;

  case 157: /* expr0: '~' expr0  */
#line 2353 "grammar.y"
            {
#line 2361 "grammar.y.pre"
		if (exact_types && !IS_TYPE((yyvsp[0].node)->type, TYPE_NUMBER))
		    type_error("Bad argument to ~", (yyvsp[0].node)->type);
		if ((yyvsp[0].node)->kind == NODE_NUMBER) {
		    (yyval.node) = (yyvsp[0].node);
		    (yyval.node)->v.number = ~(yyval.node)->v.number;
		} else {
		    CREATE_UNARY_OP((yyval.node), F_COMPL, TYPE_NUMBER, (yyvsp[0].node));
		}
	    }
#line 4527 "y.tab.c"
    break;

  case 158: /* expr0: '-' expr0  */
#line 2365 "grammar.y"
            {
#line 2372 "grammar.y.pre"
		int result_type;
                if (exact_types){
		    int t = (yyvsp[0].node)->type;
		    if (!COMP_TYPE(t, TYPE_NUMBER)){
			type_error("Bad argument to unary '-'", t);
			result_type = TYPE_ANY;
		    } else result_type = t;
		} else result_type = TYPE_ANY;

		switch ((yyvsp[0].node)->kind) {
		case NODE_NUMBER:
		    (yyval.node) = (yyvsp[0].node);
		    (yyval.node)->v.number = -(yyval.node)->v.number;
		    break;
		case NODE_REAL:
		    (yyval.node) = (yyvsp[0].node);
		    (yyval.node)->v.real = -(yyval.node)->v.real;
		    break;
		default:
		    CREATE_UNARY_OP((yyval.node), F_NEGATE, result_type, (yyvsp[0].node));
		}
	    }
#line 4556 "y.tab.c"
    break;

  case 159: /* expr0: lvalue L_INC  */
#line 2390 "grammar.y"
            {
#line 2396 "grammar.y.pre"
		CREATE_UNARY_OP((yyval.node), F_POST_INC, 0, (yyvsp[-1].node));
		(yyval.node)->v.number = F_POST_INC;
                if (exact_types){
                    switch((yyvsp[-1].node)->type){
                        case TYPE_NUMBER:
		    case TYPE_ANY:
                        case TYPE_REAL:
                        {
                            (yyval.node)->type = (yyvsp[-1].node)->type;
                            break;
                        }

                        default:
                        {
                            (yyval.node)->type = TYPE_ANY;
                            type_error("Bad argument 1 to x++", (yyvsp[-1].node)->type);
                        }
                    }
                } else (yyval.node)->type = TYPE_ANY;
	    }
#line 4583 "y.tab.c"
    break;

  case 160: /* expr0: lvalue L_DEC  */
#line 2413 "grammar.y"
            {
#line 2418 "grammar.y.pre"
		CREATE_UNARY_OP((yyval.node), F_POST_DEC, 0, (yyvsp[-1].node));
                if (exact_types){
                    switch((yyvsp[-1].node)->type){
		    case TYPE_NUMBER:
		    case TYPE_ANY:
		    case TYPE_REAL:
		    {
			(yyval.node)->type = (yyvsp[-1].node)->type;
			break;
		    }

		    default:
		    {
			(yyval.node)->type = TYPE_ANY;
			type_error("Bad argument 1 to x--", (yyvsp[-1].node)->type);
		    }
                    }
                } else (yyval.node)->type = TYPE_ANY;
	    }
#line 4609 "y.tab.c"
    break;

  case 167: /* return: L_RETURN ';'  */
#line 2444 "grammar.y"
            {
#line 2448 "grammar.y.pre"
		if (exact_types && !IS_TYPE(exact_types, TYPE_VOID))
		    yywarn("Non-void functions must return a value.");
		CREATE_RETURN((yyval.node), 0);
	    }
#line 4620 "y.tab.c"
    break;

  case 168: /* return: L_RETURN comma_expr ';'  */
#line 2451 "grammar.y"
            {
#line 2454 "grammar.y.pre"
		if (exact_types && !compatible_types((yyvsp[-1].node)->type, exact_types)) {
		    char buf[256];
		    char *end = EndOf(buf);
		    char *p;
		    
		    p = strput(buf, end, "Type of returned value doesn't match function return type ");
		    p = get_two_types(p, end, (yyvsp[-1].node)->type, exact_types);
		    yyerror(buf);
		}
		if (IS_NODE((yyvsp[-1].node), NODE_NUMBER, 0)) {
		    CREATE_RETURN((yyval.node), 0);
		} else {
		    CREATE_RETURN((yyval.node), (yyvsp[-1].node));
		}
	    }
#line 4642 "y.tab.c"
    break;

  case 169: /* expr_list: %empty  */
#line 2472 "grammar.y"
            {
#line 2474 "grammar.y.pre"
		CREATE_EXPR_LIST((yyval.node), 0);
	    }
#line 4651 "y.tab.c"
    break;

  case 170: /* expr_list: expr_list2  */
#line 2477 "grammar.y"
            {
#line 2478 "grammar.y.pre"
		CREATE_EXPR_LIST((yyval.node), (yyvsp[0].node));
	    }
#line 4660 "y.tab.c"
    break;

  case 171: /* expr_list: expr_list2 ','  */
#line 2482 "grammar.y"
            {
#line 2482 "grammar.y.pre"
		CREATE_EXPR_LIST((yyval.node), (yyvsp[-1].node));
	    }
#line 4669 "y.tab.c"
    break;

  case 172: /* expr_list_node: expr0  */
#line 2490 "grammar.y"
            {
#line 2489 "grammar.y.pre"
		CREATE_EXPR_NODE((yyval.node), (yyvsp[0].node), 0);
	    }
#line 4678 "y.tab.c"
    break;

  case 173: /* expr_list_node: expr0 L_DOT_DOT_DOT  */
#line 2495 "grammar.y"
            {
#line 2493 "grammar.y.pre"
		CREATE_EXPR_NODE((yyval.node), (yyvsp[-1].node), 1);
	    }
#line 4687 "y.tab.c"
    break;

  case 174: /* expr_list2: expr_list_node  */
#line 2503 "grammar.y"
            {
#line 2500 "grammar.y.pre"
		(yyvsp[0].node)->kind = 1;

		(yyval.node) = (yyvsp[0].node);
	    }
#line 4698 "y.tab.c"
    break;

  case 175: /* expr_list2: expr_list2 ',' expr_list_node  */
#line 2510 "grammar.y"
            {
#line 2506 "grammar.y.pre"
		(yyvsp[0].node)->kind = 0;

		(yyval.node) = (yyvsp[-2].node);
		(yyval.node)->kind++;
		(yyval.node)->l.expr->r.expr = (yyvsp[0].node);
		(yyval.node)->l.expr = (yyvsp[0].node);
	    }
#line 4712 "y.tab.c"
    break;

  case 176: /* expr_list3: %empty  */
#line 2523 "grammar.y"
            {
#line 2518 "grammar.y.pre"
		/* this is a dummy node */
		CREATE_EXPR_LIST((yyval.node), 0);
	    }
#line 4722 "y.tab.c"
    break;

  case 177: /* expr_list3: expr_list4  */
#line 2529 "grammar.y"
            {
#line 2523 "grammar.y.pre"
		CREATE_EXPR_LIST((yyval.node), (yyvsp[0].node));
	    }
#line 4731 "y.tab.c"
    break;

  case 178: /* expr_list3: expr_list4 ','  */
#line 2534 "grammar.y"
            {
#line 2527 "grammar.y.pre"
		CREATE_EXPR_LIST((yyval.node), (yyvsp[-1].node));
	    }
#line 4740 "y.tab.c"
    break;

  case 179: /* expr_list4: assoc_pair  */
#line 2542 "grammar.y"
            {
#line 2534 "grammar.y.pre"
		(yyval.node) = new_node_no_line();
		(yyval.node)->kind = 2;
		(yyval.node)->v.expr = (yyvsp[0].node);
		(yyval.node)->r.expr = 0;
		(yyval.node)->type = 0;
		/* we keep track of the end of the chain in the left nodes */
		(yyval.node)->l.expr = (yyval.node);
            }
#line 4755 "y.tab.c"
    break;

  case 180: /* expr_list4: expr_list4 ',' assoc_pair  */
#line 2553 "grammar.y"
            {
#line 2544 "grammar.y.pre"
		parse_node_t *expr;

		expr = new_node_no_line();
		expr->kind = 0;
		expr->v.expr = (yyvsp[0].node);
		expr->r.expr = 0;
		expr->type = 0;
		
		(yyvsp[-2].node)->l.expr->r.expr = expr;
		(yyvsp[-2].node)->l.expr = expr;
		(yyvsp[-2].node)->kind += 2;
		(yyval.node) = (yyvsp[-2].node);
	    }
#line 4775 "y.tab.c"
    break;

  case 181: /* assoc_pair: expr0 ':' expr0  */
#line 2572 "grammar.y"
            {
#line 2562 "grammar.y.pre"
		CREATE_TWO_VALUES((yyval.node), 0, (yyvsp[-2].node), (yyvsp[0].node));
            }
#line 4784 "y.tab.c"
    break;

  case 182: /* lvalue: expr4  */
#line 2580 "grammar.y"
            {
#line 2569 "grammar.y.pre"
#define LV_ILLEGAL 1
#define LV_RANGE 2
#define LV_INDEX 4
                /* Restrictive lvalues, but I think they make more sense :) */
                (yyval.node) = (yyvsp[0].node);
		if((yyval.node)->kind == NODE_BINARY_OP && (yyval.node)->v.number == F_TYPE_CHECK) 
		    (yyval.node) = (yyval.node)->l.expr;
                switch((yyval.node)->kind) {
		default:
		    yyerror("Illegal lvalue");
		    break;
		case NODE_PARAMETER:
		    (yyval.node)->kind = NODE_PARAMETER_LVALUE;
		    break;
		case NODE_TERNARY_OP:
		    (yyval.node)->v.number = (yyval.node)->r.expr->v.number;
		case NODE_OPCODE_1:
		case NODE_UNARY_OP_1:
		case NODE_BINARY_OP:
		    if ((yyval.node)->v.number >= F_LOCAL && (yyval.node)->v.number <= F_MEMBER)
			(yyval.node)->v.number++; /* make it an lvalue */
		    else if ((yyval.node)->v.number >= F_INDEX 
			     && (yyval.node)->v.number <= F_RE_RANGE) {
                        parse_node_t *node = (yyval.node);
                        int flag = 0;
                        do {
                            switch(node->kind) {
			    case NODE_PARAMETER:
				node->kind = NODE_PARAMETER_LVALUE;
				flag |= LV_ILLEGAL;
				break;
			    case NODE_TERNARY_OP:
				node->v.number = node->r.expr->v.number;
			    case NODE_OPCODE_1:
			    case NODE_UNARY_OP_1:
			    case NODE_BINARY_OP:
			        if(node->kind == NODE_BINARY_OP && 
				   node->v.number == F_TYPE_CHECK) {
				    node = node->l.expr;
				    continue;
				}

				if (node->v.number >= F_LOCAL 
				    && node->v.number <= F_MEMBER) {
				    node->v.number++;
				    flag |= LV_ILLEGAL;
				    break;
				} else if (node->v.number == F_INDEX ||
					 node->v.number == F_RINDEX) {
				    node->v.number++;
				    flag |= LV_INDEX;
				    break;
				} else if (node->v.number >= F_ADD_EQ
					   && node->v.number <= F_ASSIGN) {
				    if (!(flag & LV_INDEX)) {
					yyerror("Illegal lvalue, a possible lvalue is (x <assign> y)[a]");
				    }
				    if (node->r.expr->kind == NODE_BINARY_OP||
					node->r.expr->kind == NODE_TERNARY_OP){
					if (node->r.expr->v.number >= F_NN_RANGE_LVALUE && node->r.expr->v.number <= F_NR_RANGE_LVALUE)
					    yyerror("Illegal to have (x[a..b] <assign> y) to be the beginning of an lvalue");
				    }
				    flag = LV_ILLEGAL;
				    break;
				} else if (node->v.number >= F_NN_RANGE
					 && node->v.number <= F_RE_RANGE) {
				    if (flag & LV_RANGE) {
					yyerror("Can't do range lvalue of range lvalue.");
					flag |= LV_ILLEGAL;
					break;
				    }
                                    if (flag & LV_INDEX){
					yyerror("Can't do indexed lvalue of range lvalue.");
					flag |= LV_ILLEGAL;
					break;
				    }
				    if (node->v.number == F_NE_RANGE) {
					/* x[foo..] -> x[foo..<1] */
					parse_node_t *rchild = node->r.expr;
					node->kind = NODE_TERNARY_OP;
					CREATE_BINARY_OP(node->r.expr,
							 F_NR_RANGE_LVALUE,
							 0, 0, rchild);
					CREATE_NUMBER(node->r.expr->l.expr, 1);
				    } else if (node->v.number == F_RE_RANGE) {
					/* x[<foo..] -> x[<foo..<1] */
					parse_node_t *rchild = node->r.expr;
					node->kind = NODE_TERNARY_OP;
					CREATE_BINARY_OP(node->r.expr,
							 F_RR_RANGE_LVALUE,
							 0, 0, rchild);
					CREATE_NUMBER(node->r.expr->l.expr, 1);
				    } else
					node->r.expr->v.number++;
				    flag |= LV_RANGE;
				    node = node->r.expr->r.expr;
				    continue;
				}
			    default:
				yyerror("Illegal lvalue");
				flag = LV_ILLEGAL;
				break;
			    }   
                            if ((flag & LV_ILLEGAL) || !(node = node->r.expr)) break;
                        } while (1);
                        break;
		    } else 
			yyerror("Illegal lvalue");
		    break;
                }
            }
#line 4902 "y.tab.c"
    break;

  case 184: /* l_new_function_open: L_FUNCTION_OPEN efun_override  */
#line 2697 "grammar.y"
            {
#line 2685 "grammar.y.pre"
		(yyval.number) = ((yyvsp[0].number) << 8) | FP_EFUN;
	    }
#line 4911 "y.tab.c"
    break;

  case 186: /* expr4: L_DEFINED_NAME  */
#line 2707 "grammar.y"
            {
#line 2717 "grammar.y.pre"
              int i;
              if ((i = (yyvsp[0].ihe)->dn.local_num) != -1) {
		  type_of_locals_ptr[i] &= ~LOCAL_MOD_UNUSED;
		  if (type_of_locals_ptr[i] & LOCAL_MOD_REF)
		      CREATE_OPCODE_1((yyval.node), F_REF, type_of_locals_ptr[i] & ~LOCAL_MOD_REF,i & 0xff);
		  else
		      CREATE_OPCODE_1((yyval.node), F_LOCAL, type_of_locals_ptr[i], i & 0xff);
		  if (current_function_context)
		      current_function_context->num_locals++;
              } else
		  if ((i = (yyvsp[0].ihe)->dn.global_num) != -1) {
		      if (current_function_context)
			  current_function_context->bindable = FP_NOT_BINDABLE;
                          CREATE_OPCODE_1((yyval.node), F_GLOBAL,
				      VAR_TEMP(i)->type & ~DECL_MODS, i);
		      if (VAR_TEMP(i)->type & DECL_HIDDEN) {
			  char buf[256];
			  char *end = EndOf(buf);
			  char *p;

			  p = strput(buf, end, "Illegal to use private variable '");
			  p = strput(p, end, (yyvsp[0].ihe)->name);
			  p = strput(p, end, "'");
			  yyerror(buf);
		      }
		  } else {
		      char buf[256];
		      char *end = EndOf(buf);
		      char *p;
		      
		      p = strput(buf, end, "Undefined variable '");
		      p = strput(p, end, (yyvsp[0].ihe)->name);
		      p = strput(p, end, "'");
		      if (current_number_of_locals < CFG_MAX_LOCAL_VARIABLES) {
			  add_local_name((yyvsp[0].ihe)->name, TYPE_ANY);
		      }
		      CREATE_ERROR((yyval.node));
		      yyerror(buf);
		  }
	    }
#line 4958 "y.tab.c"
    break;

  case 187: /* expr4: L_IDENTIFIER  */
#line 2750 "grammar.y"
            {
#line 2759 "grammar.y.pre"
		char buf[256];
		char *end = EndOf(buf);
		char *p;
		
		p = strput(buf, end, "Undefined variable '");
		p = strput(p, end, (yyvsp[0].string));
		p = strput(p, end, "'");
                if (current_number_of_locals < CFG_MAX_LOCAL_VARIABLES) {
                    add_local_name((yyvsp[0].string), TYPE_ANY);
                }
                CREATE_ERROR((yyval.node));
                yyerror(buf);
                scratch_free((yyvsp[0].string));
            }
#line 4979 "y.tab.c"
    break;

  case 188: /* expr4: L_PARAMETER  */
#line 2767 "grammar.y"
            {
#line 2775 "grammar.y.pre"
		CREATE_PARAMETER((yyval.node), TYPE_ANY, (yyvsp[0].number));
            }
#line 4988 "y.tab.c"
    break;

  case 189: /* @11: %empty  */
#line 2772 "grammar.y"
            {
#line 2779 "grammar.y.pre"
		(yyval.contextp) = current_function_context;
		/* already flagged as an error */
		if (current_function_context)
		    current_function_context = current_function_context->parent;
            }
#line 5000 "y.tab.c"
    break;

  case 190: /* expr4: '$' '(' @11 comma_expr ')'  */
#line 2780 "grammar.y"
            {
#line 2786 "grammar.y.pre"
		parse_node_t *node;

		current_function_context = (yyvsp[-2].contextp);

		if (!current_function_context || current_function_context->num_parameters == -2) {
		    /* This was illegal, and error'ed when the '$' token
		     * was returned.
		     */
		    CREATE_ERROR((yyval.node));
		} else {
		    CREATE_OPCODE_1((yyval.node), F_LOCAL, (yyvsp[-1].node)->type,
				    current_function_context->values_list->kind++);

		    node = new_node_no_line();
		    node->type = 0;
		    current_function_context->values_list->l.expr->r.expr = node;
		    current_function_context->values_list->l.expr = node;
		    node->r.expr = 0;
		    node->v.expr = (yyvsp[-1].node);
		}
	    }
#line 5028 "y.tab.c"
    break;

  case 191: /* expr4: expr4 L_ARROW identifier  */
#line 2804 "grammar.y"
            {
#line 2809 "grammar.y.pre"
		if ((yyvsp[-2].node)->type == TYPE_ANY) {
		    int cmi;
		    unsigned char tp;
		    
		    if ((cmi = lookup_any_class_member((yyvsp[0].string), &tp)) != -1) {
			CREATE_UNARY_OP_1((yyval.node), F_MEMBER, tp, (yyvsp[-2].node), 0);
			(yyval.node)->l.number = cmi;
		    } else {
			CREATE_ERROR((yyval.node));
		    }
		} else if (!IS_CLASS((yyvsp[-2].node)->type)) {
		    yyerror("Left argument of -> is not a class");
		    CREATE_ERROR((yyval.node));
		} else {
		    CREATE_UNARY_OP_1((yyval.node), F_MEMBER, 0, (yyvsp[-2].node), 0);
		    (yyval.node)->l.number = lookup_class_member(CLASS_IDX((yyvsp[-2].node)->type),
						       (yyvsp[0].string),
						       &((yyval.node)->type));
		}
		    
		scratch_free((yyvsp[0].string));
            }
#line 5057 "y.tab.c"
    break;

  case 192: /* expr4: expr4 '[' comma_expr L_RANGE comma_expr ']'  */
#line 2829 "grammar.y"
            {
#line 2833 "grammar.y.pre"
                if ((yyvsp[-5].node)->type != TYPE_MAPPING && 
		    (yyvsp[-1].node)->kind == NODE_NUMBER && (yyvsp[-1].node)->v.number < 0)
		    yywarn("A negative constant as the second element of arr[x..y] no longer means indexing from the end.  Use arr[x..<y]");
                (yyval.node) = make_range_node(F_NN_RANGE, (yyvsp[-5].node), (yyvsp[-3].node), (yyvsp[-1].node));
            }
#line 5069 "y.tab.c"
    break;

  case 193: /* expr4: expr4 '[' '<' comma_expr L_RANGE comma_expr ']'  */
#line 2837 "grammar.y"
            {
#line 2842 "grammar.y.pre"
                (yyval.node) = make_range_node(F_RN_RANGE, (yyvsp[-6].node), (yyvsp[-3].node), (yyvsp[-1].node));
            }
#line 5078 "y.tab.c"
    break;

  case 194: /* expr4: expr4 '[' '<' comma_expr L_RANGE '<' comma_expr ']'  */
#line 2842 "grammar.y"
            {
#line 2846 "grammar.y.pre"
		if ((yyvsp[-1].node)->kind == NODE_NUMBER && (yyvsp[-1].node)->v.number <= 1)
		    (yyval.node) = make_range_node(F_RE_RANGE, (yyvsp[-7].node), (yyvsp[-4].node), 0);
		else
		    (yyval.node) = make_range_node(F_RR_RANGE, (yyvsp[-7].node), (yyvsp[-4].node), (yyvsp[-1].node));
            }
#line 5090 "y.tab.c"
    break;

  case 195: /* expr4: expr4 '[' comma_expr L_RANGE '<' comma_expr ']'  */
#line 2850 "grammar.y"
            {
#line 2853 "grammar.y.pre"
		if ((yyvsp[-1].node)->kind == NODE_NUMBER && (yyvsp[-1].node)->v.number <= 1)
		    (yyval.node) = make_range_node(F_NE_RANGE, (yyvsp[-6].node), (yyvsp[-4].node), 0);
		else
		    (yyval.node) = make_range_node(F_NR_RANGE, (yyvsp[-6].node), (yyvsp[-4].node), (yyvsp[-1].node));
            }
#line 5102 "y.tab.c"
    break;

  case 196: /* expr4: expr4 '[' comma_expr L_RANGE ']'  */
#line 2858 "grammar.y"
            {
#line 2860 "grammar.y.pre"
                (yyval.node) = make_range_node(F_NE_RANGE, (yyvsp[-4].node), (yyvsp[-2].node), 0);
            }
#line 5111 "y.tab.c"
    break;

  case 197: /* expr4: expr4 '[' '<' comma_expr L_RANGE ']'  */
#line 2863 "grammar.y"
            {
#line 2864 "grammar.y.pre"
                (yyval.node) = make_range_node(F_RE_RANGE, (yyvsp[-5].node), (yyvsp[-2].node), 0);
            }
#line 5120 "y.tab.c"
    break;

  case 198: /* expr4: expr4 '[' '<' comma_expr ']'  */
#line 2868 "grammar.y"
            {
#line 2868 "grammar.y.pre"
                if (IS_NODE((yyvsp[-4].node), NODE_CALL, F_AGGREGATE)
		    && (yyvsp[-1].node)->kind == NODE_NUMBER) {
                    int i = (yyvsp[-1].node)->v.number;
                    if (i < 1 || i > (yyvsp[-4].node)->l.number)
                        yyerror("Illegal index to array constant.");
                    else {
                        parse_node_t *node = (yyvsp[-4].node)->r.expr;
                        i = (yyvsp[-4].node)->l.number - i;
                        while (i--)
                            node = node->r.expr;
                        (yyval.node) = node->v.expr;
                        break;
                    }
                }
		CREATE_BINARY_OP((yyval.node), F_RINDEX, 0, (yyvsp[-1].node), (yyvsp[-4].node));
                if (exact_types) {
		    switch((yyvsp[-4].node)->type) {
		    case TYPE_MAPPING:
			yyerror("Illegal index for mapping.");
		    case TYPE_ANY:
			(yyval.node)->type = TYPE_ANY;
			break;
		    case TYPE_STRING:
		    case TYPE_BUFFER:
			(yyval.node)->type = TYPE_NUMBER;
			if (!IS_TYPE((yyvsp[-1].node)->type,TYPE_NUMBER))
			    type_error("Bad type of index", (yyvsp[-1].node)->type);
			break;
			
		    default:
			if ((yyvsp[-4].node)->type & TYPE_MOD_ARRAY) {
			    (yyval.node)->type = (yyvsp[-4].node)->type & ~TYPE_MOD_ARRAY;
			    if ((yyval.node)->type != TYPE_ANY)
			        (yyval.node) = add_type_check((yyval.node), (yyval.node)->type); 
			    if (!IS_TYPE((yyvsp[-1].node)->type,TYPE_NUMBER))
				type_error("Bad type of index", (yyvsp[-1].node)->type);
			} else {
			    type_error("Value indexed has a bad type ", (yyvsp[-4].node)->type);
			    (yyval.node)->type = TYPE_ANY;
			}
		    }
		} else (yyval.node)->type = TYPE_ANY;
            }
#line 5170 "y.tab.c"
    break;

  case 199: /* expr4: expr4 '[' comma_expr ']'  */
#line 2914 "grammar.y"
            {
#line 2913 "grammar.y.pre"
		/* Something stupid like ({ 1, 2, 3 })[1]; we take the
		 * time to optimize this because people who don't understand
		 * the preprocessor often write things like:
		 *
		 * #define MY_ARRAY ({ "foo", "bar", "bazz" })
		 * ...
		 * ... MY_ARRAY[1] ...
		 *
		 * which of course expands to the above.
		 */
                if (IS_NODE((yyvsp[-3].node), NODE_CALL, F_AGGREGATE) && (yyvsp[-1].node)->kind == NODE_NUMBER) {
                    int i = (yyvsp[-1].node)->v.number;
                    if (i < 0 || i >= (yyvsp[-3].node)->l.number)
                        yyerror("Illegal index to array constant.");
                    else {
                        parse_node_t *node = (yyvsp[-3].node)->r.expr;
                        while (i--)
                            node = node->r.expr;
                        (yyval.node) = node->v.expr;
                        break;
                    }
                }
                if ((yyvsp[-1].node)->kind == NODE_NUMBER && (yyvsp[-1].node)->v.number < 0)
		    yywarn("A negative constant in arr[x] no longer means indexing from the end.  Use arr[<x]");
                CREATE_BINARY_OP((yyval.node), F_INDEX, 0, (yyvsp[-1].node), (yyvsp[-3].node));
                if (exact_types) {
		    switch((yyvsp[-3].node)->type) {
		    case TYPE_MAPPING:
		    case TYPE_ANY:
			(yyval.node)->type = TYPE_ANY;
			break;
		    case TYPE_STRING:
		    case TYPE_BUFFER:
			(yyval.node)->type = TYPE_NUMBER;
			if (!IS_TYPE((yyvsp[-1].node)->type,TYPE_NUMBER))
			    type_error("Bad type of index", (yyvsp[-1].node)->type);
			break;
			
		    default:
			if ((yyvsp[-3].node)->type & TYPE_MOD_ARRAY) {
			    (yyval.node)->type = (yyvsp[-3].node)->type & ~TYPE_MOD_ARRAY;
			    if((yyval.node)->type != TYPE_ANY)
			        (yyval.node) = add_type_check((yyval.node), (yyval.node)->type);
			    if (!IS_TYPE((yyvsp[-1].node)->type,TYPE_NUMBER))
				type_error("Bad type of index", (yyvsp[-1].node)->type);
			} else {
			    type_error("Value indexed has a bad type ", (yyvsp[-3].node)->type);
			    (yyval.node)->type = TYPE_ANY;
			}
                    }
                } else (yyval.node)->type = TYPE_ANY;
            }
#line 5229 "y.tab.c"
    break;

  case 201: /* expr4: '(' comma_expr ')'  */
#line 2970 "grammar.y"
            {
#line 2970 "grammar.y.pre"
		(yyval.node) = (yyvsp[-1].node);
	    }
#line 5238 "y.tab.c"
    break;

  case 203: /* @12: %empty  */
#line 2976 "grammar.y"
            {
#line 2978 "grammar.y.pre"
	        if ((yyvsp[0].type) != TYPE_FUNCTION) yyerror("Reserved type name unexpected.");
		(yyval.func_block).num_local = current_number_of_locals;
		(yyval.func_block).max_num_locals = max_num_locals;
		(yyval.func_block).context = context;
		(yyval.func_block).save_current_type = current_type;
		(yyval.func_block).save_exact_types = exact_types;
	        if (type_of_locals_ptr + max_num_locals + CFG_MAX_LOCAL_VARIABLES >= &type_of_locals[type_of_locals_size])
		    reallocate_locals();
		deactivate_current_locals();
		locals_ptr += current_number_of_locals;
		type_of_locals_ptr += max_num_locals;
		max_num_locals = current_number_of_locals = 0;
		push_function_context();
		current_function_context->num_parameters = -1;
		exact_types = TYPE_ANY;
		context = 0;
            }
#line 5262 "y.tab.c"
    break;

  case 204: /* expr4: L_BASIC_TYPE @12 '(' argument ')' block  */
#line 2996 "grammar.y"
            {
#line 2997 "grammar.y.pre"
		if ((yyvsp[-2].argument).flags & ARG_IS_VARARGS) {
		    yyerror("Anonymous varargs functions aren't implemented");
		}
		if (!(yyvsp[0].decl).node) {
		    CREATE_RETURN((yyvsp[0].decl).node, 0);
		} else if ((yyvsp[0].decl).node->kind != NODE_RETURN &&
			   ((yyvsp[0].decl).node->kind != NODE_TWO_VALUES || (yyvsp[0].decl).node->r.expr->kind != NODE_RETURN)) {
		    parse_node_t *replacement;
		    CREATE_STATEMENTS(replacement, (yyvsp[0].decl).node, 0);
		    CREATE_RETURN(replacement->r.expr, 0);
		    (yyvsp[0].decl).node = replacement;
		}
		
		(yyval.node) = new_node();
		(yyval.node)->kind = NODE_ANON_FUNC;
		(yyval.node)->type = TYPE_FUNCTION;
		(yyval.node)->l.number = (max_num_locals - (yyvsp[-2].argument).num_arg);
		(yyval.node)->r.expr = (yyvsp[0].decl).node;
		(yyval.node)->v.number = (yyvsp[-2].argument).num_arg;
		if (current_function_context->bindable)
		    (yyval.node)->v.number |= 0x10000;
		free_all_local_names(1);
		
		current_number_of_locals = (yyvsp[-4].func_block).num_local;
		max_num_locals = (yyvsp[-4].func_block).max_num_locals;
		context = (yyvsp[-4].func_block).context;
		current_type = (yyvsp[-4].func_block).save_current_type;
		exact_types = (yyvsp[-4].func_block).save_exact_types;
		pop_function_context();
		
		locals_ptr -= current_number_of_locals;
		type_of_locals_ptr -= max_num_locals;
		reactivate_current_locals();
	    }
#line 5303 "y.tab.c"
    break;

  case 205: /* expr4: l_new_function_open ':' ')'  */
#line 3033 "grammar.y"
            {
#line 3037 "grammar.y.pre"
#ifdef WOMBLES
	        if(*(outP-2) != ':')
		  yyerror("End of functional not found");
#endif
		(yyval.node) = new_node();
		(yyval.node)->kind = NODE_FUNCTION_CONSTRUCTOR;
		(yyval.node)->type = TYPE_FUNCTION;
		(yyval.node)->r.expr = 0;
		switch ((yyvsp[-2].number) & 0xff) {
		case FP_L_VAR:
		    yyerror("Illegal to use local variable in a functional.");
		    CREATE_NUMBER((yyval.node)->l.expr, 0);
		    (yyval.node)->l.expr->r.expr = 0;
		    (yyval.node)->l.expr->l.expr = 0;
		    (yyval.node)->v.number = FP_FUNCTIONAL;
		    break;
		case FP_G_VAR:
		    CREATE_OPCODE_1((yyval.node)->l.expr, F_GLOBAL, 0, (yyvsp[-2].number) >> 8);
		    (yyval.node)->v.number = FP_FUNCTIONAL | FP_NOT_BINDABLE;
		    if (VAR_TEMP((yyval.node)->l.expr->l.number)->type & DECL_HIDDEN) {
			char buf[256];
			char *end = EndOf(buf);
			char *p;
			
			p = strput(buf, end, "Illegal to use private variable '");
			p = strput(p, end, VAR_TEMP((yyval.node)->l.expr->l.number)->name);
			p = strput(p, end, "'");
			yyerror(buf);
		    }
		    break;
		default:
		    (yyval.node)->v.number = (yyvsp[-2].number);
		    break;
		}
	    }
#line 5345 "y.tab.c"
    break;

  case 206: /* expr4: l_new_function_open ',' expr_list2 ':' ')'  */
#line 3071 "grammar.y"
            {
#line 3074 "grammar.y.pre"
#ifdef WOMBLES
	        if(*(outP-2) != ':')
		  yyerror("End of functional not found");
#endif
		(yyval.node) = new_node();
		(yyval.node)->kind = NODE_FUNCTION_CONSTRUCTOR;
		(yyval.node)->type = TYPE_FUNCTION;
		(yyval.node)->v.number = (yyvsp[-4].number);
		(yyval.node)->r.expr = (yyvsp[-2].node);
		
		switch ((yyvsp[-4].number) & 0xff) {
		case FP_EFUN: {
		    int *argp;
		    int f = (yyvsp[-4].number) >>8;
		    int num = (yyvsp[-2].node)->kind;
		    int max_arg = predefs[f].max_args;
		    if(f!=-1){
		      if (num > max_arg && max_arg != -1) {
			parse_node_t *pn = (yyvsp[-2].node);
			
			while (pn) {
			    if (pn->type & 1) break;
			    pn = pn->r.expr;
			}
			
			if (!pn) {
			    char bff[256];
			    char *end = EndOf(bff);
			    char *p;
			    
			    p = strput(bff, end, "Too many arguments to ");
			    p = strput(p, end, predefs[f].word);
			    yyerror(bff);
			}
		      } else if (max_arg != -1 && exact_types) {
			/*
			 * Now check all types of arguments to efuns.
			 */
			int i, argn, tmp;
			parse_node_t *enode = (yyvsp[-2].node);
			argp = &efun_arg_types[predefs[f].arg_index];
			
			for (argn = 0; argn < num; argn++) {
			    if (enode->type & 1) break;
			    
			    tmp = enode->v.expr->type;
			    for (i=0; !compatible_types(tmp, argp[i])
				 && argp[i] != 0; i++)
				;
			    if (argp[i] == 0) {
				char buf[256];
				char *end = EndOf(buf);
				char *p;

				p = strput(buf, end, "Bad argument ");
				p = strput_int(p, end, argn+1);
				p = strput(p, end, " to efun ");
				p = strput(p, end, predefs[f].word);
				p = strput(p, end, "()");
				yyerror(buf);
			    } else {
				/* this little section necessary b/c in the
				   case float | int we dont want to do
				   promoting. */
				if (tmp == TYPE_NUMBER && argp[i] == TYPE_REAL) {
				    for (i++; argp[i] && argp[i] != TYPE_NUMBER; i++)
					;
				    if (!argp[i])
					enode->v.expr = promote_to_float(enode->v.expr);
				}
				if (tmp == TYPE_REAL && argp[i] == TYPE_NUMBER) {
				    for (i++; argp[i] && argp[i] != TYPE_REAL; i++)
					;
				    if (!argp[i])
					enode->v.expr = promote_to_int(enode->v.expr);
				}
			    }
			    while (argp[i] != 0)
				i++;
			    argp += i + 1;
			    enode = enode->r.expr;
			}
		      }
		    }
		    break;
		}
		case FP_L_VAR:
		case FP_G_VAR:
		    yyerror("Can't give parameters to functional.");
		    break;
		}
	    }
#line 5444 "y.tab.c"
    break;

  case 207: /* expr4: L_FUNCTION_OPEN comma_expr ':' ')'  */
#line 3166 "grammar.y"
             {
#line 3168 "grammar.y.pre"
#ifdef WOMBLES
	         if(*(outP-2) != ':')
		   yyerror("End of functional not found");
#endif
		 if (current_function_context->num_locals)
		     yyerror("Illegal to use local variable in functional.");
		 if (current_function_context->values_list->r.expr)
		     current_function_context->values_list->r.expr->kind = current_function_context->values_list->kind;
		 
		 (yyval.node) = new_node();
		 (yyval.node)->kind = NODE_FUNCTION_CONSTRUCTOR;
		 (yyval.node)->type = TYPE_FUNCTION;
		 (yyval.node)->l.expr = (yyvsp[-2].node);
		 if ((yyvsp[-2].node)->kind == NODE_STRING)
		     yywarn("Function pointer returning string constant is NOT a function call");
		 (yyval.node)->r.expr = current_function_context->values_list->r.expr;
		 (yyval.node)->v.number = FP_FUNCTIONAL + current_function_context->bindable
		     + (current_function_context->num_parameters << 8);
		 pop_function_context();
             }
#line 5471 "y.tab.c"
    break;

  case 208: /* expr4: L_MAPPING_OPEN expr_list3 ']' ')'  */
#line 3189 "grammar.y"
            {
#line 3190 "grammar.y.pre"
#ifdef WOMBLES
	        if(*(outP-2) != ']')
		  yyerror("End of mapping not found");
#endif
		CREATE_CALL((yyval.node), F_AGGREGATE_ASSOC, TYPE_MAPPING, (yyvsp[-2].node));
	    }
#line 5484 "y.tab.c"
    break;

  case 209: /* expr4: L_ARRAY_OPEN expr_list '}' ')'  */
#line 3198 "grammar.y"
            {
#line 3198 "grammar.y.pre"
#ifdef WOMBLES
	        if(*(outP-2) != '}')
		  yyerror("End of array not found");
#endif  
		CREATE_CALL((yyval.node), F_AGGREGATE, TYPE_ANY | TYPE_MOD_ARRAY, (yyvsp[-2].node));
	    }
#line 5497 "y.tab.c"
    break;

  case 210: /* expr_or_block: block  */
#line 3210 "grammar.y"
            {
#line 3209 "grammar.y.pre"
		(yyval.node) = (yyvsp[0].decl).node;
	    }
#line 5506 "y.tab.c"
    break;

  case 211: /* expr_or_block: '(' comma_expr ')'  */
#line 3215 "grammar.y"
            {
#line 3213 "grammar.y.pre"
		(yyval.node) = insert_pop_value((yyvsp[-1].node));
	    }
#line 5515 "y.tab.c"
    break;

  case 212: /* @13: %empty  */
#line 3223 "grammar.y"
            {
#line 3220 "grammar.y.pre"
		(yyval.number) = context;
		context = SPECIAL_CONTEXT;
	    }
#line 5525 "y.tab.c"
    break;

  case 213: /* catch: L_CATCH @13 expr_or_block  */
#line 3229 "grammar.y"
            {
#line 3225 "grammar.y.pre"
		CREATE_CATCH((yyval.node), (yyvsp[0].node));
		context = (yyvsp[-1].number);
	    }
#line 5535 "y.tab.c"
    break;

  case 214: /* sscanf: L_SSCANF '(' expr0 ',' expr0 lvalue_list ')'  */
#line 3239 "grammar.y"
            {
#line 3249 "grammar.y.pre"
		int p = (yyvsp[-1].node)->v.number;
		CREATE_LVALUE_EFUN((yyval.node), TYPE_NUMBER, (yyvsp[-1].node));
		CREATE_BINARY_OP_1((yyval.node)->l.expr, F_SSCANF, 0, (yyvsp[-4].node), (yyvsp[-2].node), p);
	    }
#line 5546 "y.tab.c"
    break;

  case 215: /* parse_command: L_PARSE_COMMAND '(' expr0 ',' expr0 ',' expr0 lvalue_list ')'  */
#line 3249 "grammar.y"
            {
#line 3258 "grammar.y.pre"
		int p = (yyvsp[-1].node)->v.number;
		CREATE_LVALUE_EFUN((yyval.node), TYPE_NUMBER, (yyvsp[-1].node));
		CREATE_TERNARY_OP_1((yyval.node)->l.expr, F_PARSE_COMMAND, 0, 
				    (yyvsp[-6].node), (yyvsp[-4].node), (yyvsp[-2].node), p);
	    }
#line 5558 "y.tab.c"
    break;

  case 216: /* @14: %empty  */
#line 3260 "grammar.y"
            {
#line 3268 "grammar.y.pre"
		(yyval.number) = context;
		context = SPECIAL_CONTEXT;
	    }
#line 5568 "y.tab.c"
    break;

  case 217: /* time_expression: L_TIME_EXPRESSION @14 expr_or_block  */
#line 3266 "grammar.y"
            {
#line 3273 "grammar.y.pre"
		CREATE_TIME_EXPRESSION((yyval.node), (yyvsp[0].node));
		context = (yyvsp[-1].number);
	    }
#line 5578 "y.tab.c"
    break;

  case 218: /* lvalue_list: %empty  */
#line 3275 "grammar.y"
            {
#line 3281 "grammar.y.pre"
	        (yyval.node) = new_node_no_line();
		(yyval.node)->r.expr = 0;
	        (yyval.node)->v.number = 0;
	    }
#line 5589 "y.tab.c"
    break;

  case 219: /* lvalue_list: ',' lvalue lvalue_list  */
#line 3282 "grammar.y"
            {
#line 3287 "grammar.y.pre"
		parse_node_t *insert;
		
		(yyval.node) = (yyvsp[0].node);
		insert = new_node_no_line();
		insert->r.expr = (yyvsp[0].node)->r.expr;
		insert->l.expr = (yyvsp[-1].node);
		(yyvsp[0].node)->r.expr = insert;
		(yyval.node)->v.number++;
	    }
#line 5605 "y.tab.c"
    break;

  case 220: /* string: string_con2  */
#line 3297 "grammar.y"
            {
#line 3301 "grammar.y.pre"
		CREATE_STRING((yyval.node), (yyvsp[0].string));
		scratch_free((yyvsp[0].string));
	    }
#line 5615 "y.tab.c"
    break;

  case 222: /* string_con1: '(' string_con1 ')'  */
#line 3307 "grammar.y"
            {
#line 3310 "grammar.y.pre"
		(yyval.string) = (yyvsp[-1].string);
	    }
#line 5624 "y.tab.c"
    break;

  case 223: /* string_con1: string_con1 '+' string_con1  */
#line 3312 "grammar.y"
            {
#line 3314 "grammar.y.pre"
		(yyval.string) = scratch_join((yyvsp[-2].string), (yyvsp[0].string));
	    }
#line 5633 "y.tab.c"
    break;

  case 225: /* string_con2: string_con2 L_STRING  */
#line 3321 "grammar.y"
            {
#line 3322 "grammar.y.pre"
		(yyval.string) = scratch_join((yyvsp[-1].string), (yyvsp[0].string));
	    }
#line 5642 "y.tab.c"
    break;

  case 226: /* class_init: identifier ':' expr0  */
#line 3328 "grammar.y"
    {
#line 3328 "grammar.y.pre"
	(yyval.node) = new_node();
	(yyval.node)->l.expr = (parse_node_t *)(yyvsp[-2].string);
	(yyval.node)->v.expr = (yyvsp[0].node);
	(yyval.node)->r.expr = 0;
    }
#line 5654 "y.tab.c"
    break;

  case 227: /* opt_class_init: %empty  */
#line 3339 "grammar.y"
    {
#line 3338 "grammar.y.pre"
	(yyval.node) = 0;
    }
#line 5663 "y.tab.c"
    break;

  case 228: /* opt_class_init: opt_class_init ',' class_init  */
#line 3344 "grammar.y"
    {
#line 3342 "grammar.y.pre"
	(yyval.node) = (yyvsp[0].node);
	(yyval.node)->r.expr = (yyvsp[-2].node);
    }
#line 5673 "y.tab.c"
    break;

  case 229: /* @15: %empty  */
#line 3354 "grammar.y"
            {
#line 3351 "grammar.y.pre"
		(yyval.number) = context;
		(yyvsp[0].number) = num_refs;
		context |= ARG_LIST; 
	    }
#line 5684 "y.tab.c"
    break;

  case 230: /* function_call: efun_override '(' @15 expr_list ')'  */
#line 3361 "grammar.y"
            {
#line 3357 "grammar.y.pre"
		context = (yyvsp[-2].number);
		(yyval.node) = validate_efun_call((yyvsp[-4].number),(yyvsp[-1].node));
		(yyval.node) = check_refs(num_refs - (yyvsp[-3].number), (yyvsp[-1].node), (yyval.node));
		num_refs = (yyvsp[-3].number);
	    }
#line 5696 "y.tab.c"
    break;

  case 231: /* @16: %empty  */
#line 3369 "grammar.y"
            {
#line 3364 "grammar.y.pre"
		(yyval.number) = context;
		(yyvsp[0].number) = num_refs;
		context |= ARG_LIST;
	    }
#line 5707 "y.tab.c"
    break;

  case 232: /* function_call: L_NEW '(' @16 expr_list ')'  */
#line 3376 "grammar.y"
            {
#line 3370 "grammar.y.pre"
		context = (yyvsp[-2].number);
		(yyval.node) = validate_efun_call(new_efun, (yyvsp[-1].node));
		(yyval.node) = check_refs(num_refs - (yyvsp[-3].number), (yyvsp[-1].node), (yyval.node));
		num_refs = (yyvsp[-3].number);
            }
#line 5719 "y.tab.c"
    break;

  case 233: /* function_call: L_NEW '(' L_CLASS L_DEFINED_NAME opt_class_init ')'  */
#line 3384 "grammar.y"
            {
#line 3377 "grammar.y.pre"
		parse_node_t *node;
		
		if ((yyvsp[-2].ihe)->dn.class_num == -1) {
		    char buf[256];
		    char *end = EndOf(buf);
		    char *p;
		    
		    p = strput(buf, end, "Undefined class '");
		    p = strput(p, end, (yyvsp[-2].ihe)->name);
		    p = strput(p, end, "'");
		    yyerror(buf);
		    CREATE_ERROR((yyval.node));
		    node = (yyvsp[-1].node);
		    while (node) {
			scratch_free((char *)node->l.expr);
			node = node->r.expr;
		    }
		} else {
		    int type = (yyvsp[-2].ihe)->dn.class_num | TYPE_MOD_CLASS;
		    
		    if ((node = (yyvsp[-1].node))) {
			CREATE_TWO_VALUES((yyval.node), type, 0, 0);
			(yyval.node)->l.expr = reorder_class_values((yyvsp[-2].ihe)->dn.class_num,
							node);
			CREATE_OPCODE_1((yyval.node)->r.expr, F_NEW_CLASS,
					type, (yyvsp[-2].ihe)->dn.class_num);
			
		    } else {
			CREATE_OPCODE_1((yyval.node), F_NEW_EMPTY_CLASS,
					type, (yyvsp[-2].ihe)->dn.class_num);
		    }
		}
            }
#line 5759 "y.tab.c"
    break;

  case 234: /* function_call: L_NEW '(' L_CLASS L_IDENTIFIER opt_class_init ')'  */
#line 3420 "grammar.y"
            {
#line 3412 "grammar.y.pre"
		parse_node_t *node;
		char buf[256];
		char *end = EndOf(buf);
		char *p;

		p = strput(buf, end, "Undefined class '");
		p = strput(p, end, (yyvsp[-2].string));
		p = strput(p, end, "'");
		yyerror(buf);
		CREATE_ERROR((yyval.node));
		node = (yyvsp[-1].node);
		while (node) {
		    scratch_free((char *)node->l.expr);
		    node = node->r.expr;
		}
	    }
#line 5782 "y.tab.c"
    break;

  case 235: /* @17: %empty  */
#line 3439 "grammar.y"
            {
#line 3430 "grammar.y.pre"
		(yyval.number) = context;
		(yyvsp[0].number) = num_refs;
		context |= ARG_LIST;
	    }
#line 5793 "y.tab.c"
    break;

  case 236: /* function_call: L_DEFINED_NAME '(' @17 expr_list ')'  */
#line 3446 "grammar.y"
            {
#line 3436 "grammar.y.pre"
		int f;
		
		context = (yyvsp[-2].number);
		(yyval.node) = (yyvsp[-1].node);
		if ((f = (yyvsp[-4].ihe)->dn.function_num) != -1) {
		    if (current_function_context)
			current_function_context->bindable = FP_NOT_BINDABLE;
		    
		    (yyval.node)->kind = NODE_CALL_1;
		    (yyval.node)->v.number = F_CALL_FUNCTION_BY_ADDRESS;
		    (yyval.node)->l.number = f;
		    (yyval.node)->type = validate_function_call(f, (yyvsp[-1].node)->r.expr);
		} else if ((f=(yyvsp[-4].ihe)->dn.simul_num) != -1) {
		    (yyval.node)->kind = NODE_CALL_1;
		    (yyval.node)->v.number = F_SIMUL_EFUN;
		    (yyval.node)->l.number = f;
		    (yyval.node)->type = (SIMUL(f)->type) & ~DECL_MODS;
		} else if ((f=(yyvsp[-4].ihe)->dn.efun_num) != -1) {
		    (yyval.node) = validate_efun_call(f, (yyvsp[-1].node));
		} else {
		    /* This here is a really nasty case that only occurs with
		     * exact_types off.  The user has done something gross like:
		     *
		     * func() { int f; f(); } // if f was prototyped we wouldn't
		     * f() { }                // need this case
		     *
		     * Don't complain, just grok it.
		     */
		    
		    if (current_function_context)
			current_function_context->bindable = FP_NOT_BINDABLE;
		    
		    f = define_new_function((yyvsp[-4].ihe)->name, 0, 0, 
					    DECL_PUBLIC|FUNC_UNDEFINED, TYPE_ANY);
		    (yyval.node)->kind = NODE_CALL_1;
		    (yyval.node)->v.number = F_CALL_FUNCTION_BY_ADDRESS;
		    (yyval.node)->l.number = f;
		    (yyval.node)->type = TYPE_ANY; /* just a guess */
		    if (exact_types) {
			char buf[256];
			char *end = EndOf(buf);
			char *p;
			const char *n = (yyvsp[-4].ihe)->name;
			if (*n == ':') n++;
			/* prevent some errors; by making it look like an
			 * inherited function we prevent redeclaration errors
			 * if it shows up later
			 */
			
			FUNCTION_FLAGS(f) &= ~FUNC_UNDEFINED;
			FUNCTION_FLAGS(f) |= (FUNC_INHERITED | FUNC_VARARGS);
			p = strput(buf, end, "Undefined function ");
			p = strput(p, end, n);
			yyerror(buf);
		    }
		}
		(yyval.node) = check_refs(num_refs - (yyvsp[-3].number), (yyvsp[-1].node), (yyval.node));
		num_refs = (yyvsp[-3].number);
	    }
#line 5859 "y.tab.c"
    break;

  case 237: /* @18: %empty  */
#line 3508 "grammar.y"
            {
#line 3497 "grammar.y.pre"
		(yyval.number) = context;
		(yyvsp[0].number) = num_refs;
		context |= ARG_LIST;
	    }
#line 5870 "y.tab.c"
    break;

  case 238: /* function_call: function_name '(' @18 expr_list ')'  */
#line 3515 "grammar.y"
            {
#line 3503 "grammar.y.pre"
	      char *name = (yyvsp[-4].string);

	      context = (yyvsp[-2].number);
	      (yyval.node) = (yyvsp[-1].node);
	      
	      if (current_function_context)
		  current_function_context->bindable = FP_NOT_BINDABLE;

	      if (*name == ':') {
		  int f;
		  
		  if ((f = arrange_call_inherited(name + 1, (yyval.node))) != -1)
		      /* Can't do this; f may not be the correct function
			 entry.  It might be overloaded.
			 
		      validate_function_call(f, $$->r.expr)
		      */
		      ;
	      } else {
		  int f;
		  ident_hash_elem_t *ihe;
		  
		  f = (ihe = lookup_ident(name)) ? ihe->dn.function_num : -1;
		  (yyval.node)->kind = NODE_CALL_1;
		  (yyval.node)->v.number = F_CALL_FUNCTION_BY_ADDRESS;
		  if (f!=-1) {
		      /* The only way this can happen is if function_name
		       * below made the function name.  The lexer would
		       * return L_DEFINED_NAME instead.
		       */
		      (yyval.node)->type = validate_function_call(f, (yyvsp[-1].node)->r.expr);
		  } else {
		      f = define_new_function(name, 0, 0, 
					      DECL_PUBLIC|FUNC_UNDEFINED, TYPE_ANY);
		  }
		  (yyval.node)->l.number = f;
		  /*
		   * Check if this function has been defined.
		   * But, don't complain yet about functions defined
		   * by inheritance.
		   */
		  if (exact_types && (FUNCTION_FLAGS(f) & FUNC_UNDEFINED)) {
		      char buf[256];
		      char *end = EndOf(buf);
		      char *p;
		      char *n = (yyvsp[-4].string);
		      if (*n == ':') n++;
		      /* prevent some errors */
		      FUNCTION_FLAGS(f) &= ~FUNC_UNDEFINED;
		      FUNCTION_FLAGS(f) |= (FUNC_INHERITED | FUNC_VARARGS);
		      p = strput(buf, end, "Undefined function ");
		      p = strput(p, end, n);
		      yyerror(buf);
		  }
		  if (!(FUNCTION_FLAGS(f) & FUNC_UNDEFINED))
		      (yyval.node)->type = FUNCTION_DEF(f)->type;
		  else
		      (yyval.node)->type = TYPE_ANY;  /* Just a guess */
	      }
	      (yyval.node) = check_refs(num_refs - (yyvsp[-3].number), (yyvsp[-1].node), (yyval.node));
	      num_refs = (yyvsp[-3].number);
	      scratch_free(name);
	  }
#line 5940 "y.tab.c"
    break;

  case 239: /* @19: %empty  */
#line 3581 "grammar.y"
            {
#line 3568 "grammar.y.pre"
		(yyval.number) = context;
		(yyvsp[0].number) = num_refs;
		context |= ARG_LIST;
	    }
#line 5951 "y.tab.c"
    break;

  case 240: /* function_call: expr4 L_ARROW identifier '(' @19 expr_list ')'  */
#line 3588 "grammar.y"
            {
#line 3574 "grammar.y.pre"
		ident_hash_elem_t *ihe;
		int f;
		parse_node_t *pn1, *pn2;
		
		(yyvsp[-1].node)->v.number += 2;

		pn1 = new_node_no_line();
		pn1->type = 0;
		pn1->v.expr = (yyvsp[-6].node);
		pn1->kind = (yyvsp[-1].node)->v.number;
		
		pn2 = new_node_no_line();
		pn2->type = 0;
		CREATE_STRING(pn2->v.expr, (yyvsp[-4].string));
		scratch_free((yyvsp[-4].string));
		
		/* insert the two nodes */
		pn2->r.expr = (yyvsp[-1].node)->r.expr;
		pn1->r.expr = pn2;
		(yyvsp[-1].node)->r.expr = pn1;
		
		if (!(yyvsp[-1].node)->l.expr) (yyvsp[-1].node)->l.expr = pn2;
		    
		context = (yyvsp[-2].number);
		ihe = lookup_ident("call_other");

		if ((f = ihe->dn.simul_num) != -1) {
		    (yyval.node) = (yyvsp[-1].node);
		    (yyval.node)->kind = NODE_CALL_1;
		    (yyval.node)->v.number = F_SIMUL_EFUN;
		    (yyval.node)->l.number = f;
		    (yyval.node)->type = (SIMUL(f)->type) & ~DECL_MODS;
		} else {
		    (yyval.node) = validate_efun_call(arrow_efun, (yyvsp[-1].node));
#ifdef CAST_CALL_OTHERS
		    (yyval.node)->type = TYPE_UNKNOWN;
#else
		    (yyval.node)->type = TYPE_ANY;
#endif		  
		}
		(yyval.node) = check_refs(num_refs - (yyvsp[-3].number), (yyvsp[-1].node), (yyval.node));
		num_refs = (yyvsp[-3].number);
	    }
#line 6001 "y.tab.c"
    break;

  case 241: /* @20: %empty  */
#line 3634 "grammar.y"
            {
#line 3619 "grammar.y.pre"
		(yyval.number) = context;
		(yyvsp[0].number) = num_refs;
		context |= ARG_LIST;
	    }
#line 6012 "y.tab.c"
    break;

  case 242: /* function_call: '(' '*' comma_expr ')' '(' @20 expr_list ')'  */
#line 3641 "grammar.y"
            {
#line 3625 "grammar.y.pre"
	        parse_node_t *expr;

		context = (yyvsp[-2].number);
		(yyval.node) = (yyvsp[-1].node);
		(yyval.node)->kind = NODE_EFUN;
		(yyval.node)->l.number = (yyval.node)->v.number + 1;
		(yyval.node)->v.number = predefs[evaluate_efun].token;
#ifdef CAST_CALL_OTHERS
		(yyval.node)->type = TYPE_UNKNOWN;
#else
		(yyval.node)->type = TYPE_ANY;
#endif
		expr = new_node_no_line();
		expr->type = 0;
		expr->v.expr = (yyvsp[-5].node);
		expr->r.expr = (yyval.node)->r.expr;
		(yyval.node)->r.expr = expr;
		(yyval.node) = check_refs(num_refs - (yyvsp[-3].number), (yyvsp[-1].node), (yyval.node));
		num_refs = (yyvsp[-3].number);
	    }
#line 6039 "y.tab.c"
    break;

  case 243: /* efun_override: L_EFUN L_COLON_COLON identifier  */
#line 3665 "grammar.y"
                                               {
#line 3648 "grammar.y.pre"
	svalue_t *res;
	ident_hash_elem_t *ihe;

	(yyval.number) = (ihe = lookup_ident((yyvsp[0].string))) ? ihe->dn.efun_num : -1;
	if ((yyval.number) == -1) {
	    char buf[256];
	    char *end = EndOf(buf);
	    char *p;
	    
	    p = strput(buf, end, "Unknown efun: ");
	    p = strput(p, end, (yyvsp[0].string));
	    yyerror(buf);
	} else {
	    push_malloced_string(the_file_name(current_file));
	    share_and_push_string((yyvsp[0].string));
	    push_malloced_string(add_slash(main_file_name()));
	    res = safe_apply_master_ob(APPLY_VALID_OVERRIDE, 3);
	    if (!MASTER_APPROVED(res)) {
		yyerror("Invalid simulated efunction override");
		(yyval.number) = -1;
	    }
	}
	scratch_free((yyvsp[0].string));
      }
#line 6070 "y.tab.c"
    break;

  case 244: /* efun_override: L_EFUN L_COLON_COLON L_NEW  */
#line 3691 "grammar.y"
                                 {
#line 3673 "grammar.y.pre"
	svalue_t *res;
	
	push_malloced_string(the_file_name(current_file));
	push_constant_string("new");
	push_malloced_string(add_slash(main_file_name()));
	res = safe_apply_master_ob(APPLY_VALID_OVERRIDE, 3);
	if (!MASTER_APPROVED(res)) {
	    yyerror("Invalid simulated efunction override");
	    (yyval.number) = -1;
	} else (yyval.number) = new_efun;
      }
#line 6088 "y.tab.c"
    break;

  case 246: /* function_name: L_COLON_COLON identifier  */
#line 3709 "grammar.y"
            {
#line 3690 "grammar.y.pre"
		int l = strlen((yyvsp[0].string)) + 1;
		char *p;
		/* here we be a bit cute.  we put a : on the front so we
		 * don't have to strchr for it.  Here we do:
		 * "name" -> ":::name"
		 */
		(yyval.string) = scratch_realloc((yyvsp[0].string), l + 3);
		p = (yyval.string) + l;
		while (p--,l--)
		    *(p+3) = *p;
		strncpy((yyval.string), ":::", 3);
	    }
#line 6107 "y.tab.c"
    break;

  case 247: /* function_name: L_BASIC_TYPE L_COLON_COLON identifier  */
#line 3724 "grammar.y"
            {
#line 3704 "grammar.y.pre"
		int z, l = strlen((yyvsp[0].string)) + 1;
		char *p;
		/* <type> and "name" -> ":type::name" */
		z = strlen(compiler_type_names[(yyvsp[-2].type)]) + 3; /* length of :type:: */
		(yyval.string) = scratch_realloc((yyvsp[0].string), l + z);
		p = (yyval.string) + l;
		while (p--,l--)
		    *(p+z) = *p;
		(yyval.string)[0] = ':';
		strncpy((yyval.string) + 1, compiler_type_names[(yyvsp[-2].type)], z - 3);
		(yyval.string)[z-2] = ':';
		(yyval.string)[z-1] = ':';
	    }
#line 6127 "y.tab.c"
    break;

  case 248: /* function_name: identifier L_COLON_COLON identifier  */
#line 3740 "grammar.y"
            {
#line 3719 "grammar.y.pre"
		int l = strlen((yyvsp[-2].string));
		/* "ob" and "name" -> ":ob::name" */
		(yyval.string) = scratch_alloc(l + strlen((yyvsp[0].string)) + 4);
		*((yyval.string)) = ':';
		strcpy((yyval.string) + 1, (yyvsp[-2].string));
		strcpy((yyval.string) + l + 1, "::");
		strcpy((yyval.string) + l + 3, (yyvsp[0].string));
		scratch_free((yyvsp[-2].string));
		scratch_free((yyvsp[0].string));
	    }
#line 6144 "y.tab.c"
    break;

  case 249: /* cond: L_IF '(' comma_expr ')' statement optional_else_part  */
#line 3756 "grammar.y"
            {
#line 3734 "grammar.y.pre"
		/* x != 0 -> x */
		if (IS_NODE((yyvsp[-3].node), NODE_BINARY_OP, F_NE)) {
		    if (IS_NODE((yyvsp[-3].node)->r.expr, NODE_NUMBER, 0))
			(yyvsp[-3].node) = (yyvsp[-3].node)->l.expr;
		    else if (IS_NODE((yyvsp[-3].node)->l.expr, NODE_NUMBER, 0))
			     (yyvsp[-3].node) = (yyvsp[-3].node)->r.expr;
		}

		/* TODO: should optimize if (0), if (1) here.  
		 * Also generalize this.
		 */

		if ((yyvsp[-1].node) == 0) {
		    if ((yyvsp[0].node) == 0) {
			/* if (x) ; -> x; */
			(yyval.node) = pop_value((yyvsp[-3].node));
			break;
		    } else {
			/* if (x) {} else y; -> if (!x) y; */
			parse_node_t *repl;
			
			CREATE_UNARY_OP(repl, F_NOT, TYPE_NUMBER, (yyvsp[-3].node));
			(yyvsp[-3].node) = repl;
			(yyvsp[-1].node) = (yyvsp[0].node);
			(yyvsp[0].node) = 0;
		    }
		}
		CREATE_IF((yyval.node), (yyvsp[-3].node), (yyvsp[-1].node), (yyvsp[0].node));
	    }
#line 6180 "y.tab.c"
    break;

  case 250: /* optional_else_part: %empty  */
#line 3791 "grammar.y"
            {
#line 3768 "grammar.y.pre"
		(yyval.node) = 0;
	    }
#line 6189 "y.tab.c"
    break;

  case 251: /* optional_else_part: L_ELSE statement  */
#line 3796 "grammar.y"
            {
#line 3772 "grammar.y.pre"
		(yyval.node) = (yyvsp[0].node);
            }
#line 6198 "y.tab.c"
    break;


#line 6202 "y.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 3801 "grammar.y"



#line 3777 "grammar.y.pre"
