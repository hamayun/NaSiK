
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ESINT64VAL = 258,
     EUINT64VAL = 259,
     ESAPINTVAL = 260,
     EUAPINTVAL = 261,
     LOCALVAL_ID = 262,
     GLOBALVAL_ID = 263,
     FPVAL = 264,
     VOID = 265,
     INTTYPE = 266,
     FLOAT = 267,
     DOUBLE = 268,
     X86_FP80 = 269,
     FP128 = 270,
     PPC_FP128 = 271,
     LABEL = 272,
     TYPE = 273,
     LOCALVAR = 274,
     GLOBALVAR = 275,
     LABELSTR = 276,
     STRINGCONSTANT = 277,
     ATSTRINGCONSTANT = 278,
     PCTSTRINGCONSTANT = 279,
     ZEROINITIALIZER = 280,
     TRUETOK = 281,
     FALSETOK = 282,
     BEGINTOK = 283,
     ENDTOK = 284,
     DECLARE = 285,
     DEFINE = 286,
     GLOBAL = 287,
     CONSTANT = 288,
     SECTION = 289,
     ALIAS = 290,
     VOLATILE = 291,
     THREAD_LOCAL = 292,
     TO = 293,
     DOTDOTDOT = 294,
     NULL_TOK = 295,
     UNDEF = 296,
     INTERNAL = 297,
     LINKONCE = 298,
     WEAK = 299,
     APPENDING = 300,
     DLLIMPORT = 301,
     DLLEXPORT = 302,
     EXTERN_WEAK = 303,
     COMMON = 304,
     OPAQUE = 305,
     EXTERNAL = 306,
     TARGET = 307,
     TRIPLE = 308,
     ALIGN = 309,
     ADDRSPACE = 310,
     DEPLIBS = 311,
     CALL = 312,
     TAIL = 313,
     ASM_TOK = 314,
     MODULE = 315,
     SIDEEFFECT = 316,
     CC_TOK = 317,
     CCC_TOK = 318,
     FASTCC_TOK = 319,
     COLDCC_TOK = 320,
     X86_STDCALLCC_TOK = 321,
     X86_FASTCALLCC_TOK = 322,
     X86_SSECALLCC_TOK = 323,
     DATALAYOUT = 324,
     RET = 325,
     BR = 326,
     SWITCH = 327,
     INVOKE = 328,
     UNWIND = 329,
     UNREACHABLE = 330,
     ADD = 331,
     SUB = 332,
     MUL = 333,
     UDIV = 334,
     SDIV = 335,
     FDIV = 336,
     UREM = 337,
     SREM = 338,
     FREM = 339,
     AND = 340,
     OR = 341,
     XOR = 342,
     SHL = 343,
     LSHR = 344,
     ASHR = 345,
     ICMP = 346,
     FCMP = 347,
     VICMP = 348,
     VFCMP = 349,
     EQ = 350,
     NE = 351,
     SLT = 352,
     SGT = 353,
     SLE = 354,
     SGE = 355,
     ULT = 356,
     UGT = 357,
     ULE = 358,
     UGE = 359,
     OEQ = 360,
     ONE = 361,
     OLT = 362,
     OGT = 363,
     OLE = 364,
     OGE = 365,
     ORD = 366,
     UNO = 367,
     UEQ = 368,
     UNE = 369,
     MALLOC = 370,
     ALLOCA = 371,
     FREE = 372,
     LOAD = 373,
     STORE = 374,
     GETELEMENTPTR = 375,
     TRUNC = 376,
     ZEXT = 377,
     SEXT = 378,
     FPTRUNC = 379,
     FPEXT = 380,
     BITCAST = 381,
     UITOFP = 382,
     SITOFP = 383,
     FPTOUI = 384,
     FPTOSI = 385,
     INTTOPTR = 386,
     PTRTOINT = 387,
     PHI_TOK = 388,
     SELECT = 389,
     VAARG = 390,
     EXTRACTELEMENT = 391,
     INSERTELEMENT = 392,
     SHUFFLEVECTOR = 393,
     GETRESULT = 394,
     EXTRACTVALUE = 395,
     INSERTVALUE = 396,
     SIGNEXT = 397,
     ZEROEXT = 398,
     NORETURN = 399,
     INREG = 400,
     SRET = 401,
     NOUNWIND = 402,
     NOALIAS = 403,
     BYVAL = 404,
     NEST = 405,
     READNONE = 406,
     READONLY = 407,
     GC = 408,
     FNNOTE = 409,
     INLINE = 410,
     ALWAYS = 411,
     NEVER = 412,
     OPTIMIZEFORSIZE = 413,
     DEFAULT = 414,
     HIDDEN = 415,
     PROTECTED = 416
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 970 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"

  llvm::Module                           *ModuleVal;
  llvm::Function                         *FunctionVal;
  llvm::BasicBlock                       *BasicBlockVal;
  llvm::TerminatorInst                   *TermInstVal;
  llvm::Instruction                      *InstVal;
  llvm::Constant                         *ConstVal;

  const llvm::Type                       *PrimType;
  std::list<llvm::PATypeHolder>          *TypeList;
  llvm::PATypeHolder                     *TypeVal;
  llvm::Value                            *ValueVal;
  std::vector<llvm::Value*>              *ValueList;
  std::vector<unsigned>                  *ConstantList;
  llvm::ArgListType                      *ArgList;
  llvm::TypeWithAttrs                     TypeWithAttrs;
  llvm::TypeWithAttrsList                *TypeWithAttrsList;
  llvm::ParamList                        *ParamList;

  // Represent the RHS of PHI node
  std::list<std::pair<llvm::Value*,
                      llvm::BasicBlock*> > *PHIList;
  std::vector<std::pair<llvm::Constant*, llvm::BasicBlock*> > *JumpTable;
  std::vector<llvm::Constant*>           *ConstVector;

  llvm::GlobalValue::LinkageTypes         Linkage;
  llvm::GlobalValue::VisibilityTypes      Visibility;
  llvm::ParameterAttributes         ParamAttrs;
  llvm::FunctionNotes               FunctionNotes;
  llvm::APInt                       *APIntVal;
  int64_t                           SInt64Val;
  uint64_t                          UInt64Val;
  int                               SIntVal;
  unsigned                          UIntVal;
  llvm::APFloat                    *FPVal;
  bool                              BoolVal;

  std::string                      *StrVal;   // This memory must be deleted
  llvm::ValID                       ValIDVal;

  llvm::Instruction::BinaryOps      BinaryOpVal;
  llvm::Instruction::TermOps        TermOpVal;
  llvm::Instruction::MemoryOps      MemOpVal;
  llvm::Instruction::CastOps        CastOpVal;
  llvm::Instruction::OtherOps       OtherOpVal;
  llvm::ICmpInst::Predicate         IPredicate;
  llvm::FCmpInst::Predicate         FPredicate;



/* Line 1676 of yacc.c  */
#line 264 "llvmAsmParser.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE llvmAsmlval;


