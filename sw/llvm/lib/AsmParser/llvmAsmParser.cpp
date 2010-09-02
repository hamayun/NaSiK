
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         llvmAsmparse
#define yylex           llvmAsmlex
#define yyerror         llvmAsmerror
#define yylval          llvmAsmlval
#define yychar          llvmAsmchar
#define yydebug         llvmAsmdebug
#define yynerrs         llvmAsmnerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 14 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"

#include "ParserInternals.h"
#include "llvm/CallingConv.h"
#include "llvm/InlineAsm.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/ValueSymbolTable.h"
#include "llvm/AutoUpgrade.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/Streams.h"
#include <algorithm>
#include <list>
#include <map>
#include <utility>

// The following is a gross hack. In order to rid the libAsmParser library of
// exceptions, we have to have a way of getting the yyparse function to go into
// an error situation. So, whenever we want an error to occur, the GenerateError
// function (see bottom of file) sets TriggerError. Then, at the end of each 
// production in the grammer we use CHECK_FOR_ERROR which will invoke YYERROR 
// (a goto) to put YACC in error state. Furthermore, several calls to 
// GenerateError are made from inside productions and they must simulate the
// previous exception behavior by exiting the production immediately. We have
// replaced these with the GEN_ERROR macro which calls GeneratError and then
// immediately invokes YYERROR. This would be so much cleaner if it was a 
// recursive descent parser.
static bool TriggerError = false;
#define CHECK_FOR_ERROR { if (TriggerError) { TriggerError = false; YYABORT; } }
#define GEN_ERROR(msg) { GenerateError(msg); YYERROR; }

int yyerror(const char *ErrorMsg); // Forward declarations to prevent "implicit
int yylex();                       // declaration" of xxx warnings.
int yyparse();
using namespace llvm;

static Module *ParserResult;

// DEBUG_UPREFS - Define this symbol if you want to enable debugging output
// relating to upreferences in the input stream.
//
//#define DEBUG_UPREFS 1
#ifdef DEBUG_UPREFS
#define UR_OUT(X) cerr << X
#else
#define UR_OUT(X)
#endif

#define YYERROR_VERBOSE 1

static GlobalVariable *CurGV;


// This contains info used when building the body of a function.  It is
// destroyed when the function is completed.
//
typedef std::vector<Value *> ValueList;           // Numbered defs

static void 
ResolveDefinitions(ValueList &LateResolvers, ValueList *FutureLateResolvers=0);

static struct PerModuleInfo {
  Module *CurrentModule;
  ValueList Values; // Module level numbered definitions
  ValueList LateResolveValues;
  std::vector<PATypeHolder>    Types;
  std::map<ValID, PATypeHolder> LateResolveTypes;

  /// PlaceHolderInfo - When temporary placeholder objects are created, remember
  /// how they were referenced and on which line of the input they came from so
  /// that we can resolve them later and print error messages as appropriate.
  std::map<Value*, std::pair<ValID, int> > PlaceHolderInfo;

  // GlobalRefs - This maintains a mapping between <Type, ValID>'s and forward
  // references to global values.  Global values may be referenced before they
  // are defined, and if so, the temporary object that they represent is held
  // here.  This is used for forward references of GlobalValues.
  //
  typedef std::map<std::pair<const PointerType *,
                             ValID>, GlobalValue*> GlobalRefsType;
  GlobalRefsType GlobalRefs;

  void ModuleDone() {
    // If we could not resolve some functions at function compilation time
    // (calls to functions before they are defined), resolve them now...  Types
    // are resolved when the constant pool has been completely parsed.
    //
    ResolveDefinitions(LateResolveValues);
    if (TriggerError)
      return;

    // Check to make sure that all global value forward references have been
    // resolved!
    //
    if (!GlobalRefs.empty()) {
      std::string UndefinedReferences = "Unresolved global references exist:\n";

      for (GlobalRefsType::iterator I = GlobalRefs.begin(), E =GlobalRefs.end();
           I != E; ++I) {
        UndefinedReferences += "  " + I->first.first->getDescription() + " " +
                               I->first.second.getName() + "\n";
      }
      GenerateError(UndefinedReferences);
      return;
    }

    // Look for intrinsic functions and CallInst that need to be upgraded
    for (Module::iterator FI = CurrentModule->begin(),
         FE = CurrentModule->end(); FI != FE; )
      UpgradeCallsToIntrinsic(FI++); // must be post-increment, as we remove

    Values.clear();         // Clear out function local definitions
    Types.clear();
    CurrentModule = 0;
  }

  // GetForwardRefForGlobal - Check to see if there is a forward reference
  // for this global.  If so, remove it from the GlobalRefs map and return it.
  // If not, just return null.
  GlobalValue *GetForwardRefForGlobal(const PointerType *PTy, ValID ID) {
    // Check to see if there is a forward reference to this global variable...
    // if there is, eliminate it and patch the reference to use the new def'n.
    GlobalRefsType::iterator I = GlobalRefs.find(std::make_pair(PTy, ID));
    GlobalValue *Ret = 0;
    if (I != GlobalRefs.end()) {
      Ret = I->second;
      GlobalRefs.erase(I);
    }
    return Ret;
  }

  bool TypeIsUnresolved(PATypeHolder* PATy) {
    // If it isn't abstract, its resolved
    const Type* Ty = PATy->get();
    if (!Ty->isAbstract())
      return false;
    // Traverse the type looking for abstract types. If it isn't abstract then
    // we don't need to traverse that leg of the type. 
    std::vector<const Type*> WorkList, SeenList;
    WorkList.push_back(Ty);
    while (!WorkList.empty()) {
      const Type* Ty = WorkList.back();
      SeenList.push_back(Ty);
      WorkList.pop_back();
      if (const OpaqueType* OpTy = dyn_cast<OpaqueType>(Ty)) {
        // Check to see if this is an unresolved type
        std::map<ValID, PATypeHolder>::iterator I = LateResolveTypes.begin();
        std::map<ValID, PATypeHolder>::iterator E = LateResolveTypes.end();
        for ( ; I != E; ++I) {
          if (I->second.get() == OpTy)
            return true;
        }
      } else if (const SequentialType* SeqTy = dyn_cast<SequentialType>(Ty)) {
        const Type* TheTy = SeqTy->getElementType();
        if (TheTy->isAbstract() && TheTy != Ty) {
          std::vector<const Type*>::iterator I = SeenList.begin(), 
                                             E = SeenList.end();
          for ( ; I != E; ++I)
            if (*I == TheTy)
              break;
          if (I == E)
            WorkList.push_back(TheTy);
        }
      } else if (const StructType* StrTy = dyn_cast<StructType>(Ty)) {
        for (unsigned i = 0; i < StrTy->getNumElements(); ++i) {
          const Type* TheTy = StrTy->getElementType(i);
          if (TheTy->isAbstract() && TheTy != Ty) {
            std::vector<const Type*>::iterator I = SeenList.begin(), 
                                               E = SeenList.end();
            for ( ; I != E; ++I)
              if (*I == TheTy)
                break;
            if (I == E)
              WorkList.push_back(TheTy);
          }
        }
      }
    }
    return false;
  }
} CurModule;

static struct PerFunctionInfo {
  Function *CurrentFunction;     // Pointer to current function being created

  ValueList Values; // Keep track of #'d definitions
  unsigned NextValNum;
  ValueList LateResolveValues;
  bool isDeclare;                   // Is this function a forward declararation?
  GlobalValue::LinkageTypes Linkage; // Linkage for forward declaration.
  GlobalValue::VisibilityTypes Visibility;

  /// BBForwardRefs - When we see forward references to basic blocks, keep
  /// track of them here.
  std::map<ValID, BasicBlock*> BBForwardRefs;

  inline PerFunctionInfo() {
    CurrentFunction = 0;
    isDeclare = false;
    Linkage = GlobalValue::ExternalLinkage;
    Visibility = GlobalValue::DefaultVisibility;
  }

  inline void FunctionStart(Function *M) {
    CurrentFunction = M;
    NextValNum = 0;
  }

  void FunctionDone() {
    // Any forward referenced blocks left?
    if (!BBForwardRefs.empty()) {
      GenerateError("Undefined reference to label " +
                     BBForwardRefs.begin()->second->getName());
      return;
    }

    // Resolve all forward references now.
    ResolveDefinitions(LateResolveValues, &CurModule.LateResolveValues);

    Values.clear();         // Clear out function local definitions
    BBForwardRefs.clear();
    CurrentFunction = 0;
    isDeclare = false;
    Linkage = GlobalValue::ExternalLinkage;
    Visibility = GlobalValue::DefaultVisibility;
  }
} CurFun;  // Info for the current function...

static bool inFunctionScope() { return CurFun.CurrentFunction != 0; }


//===----------------------------------------------------------------------===//
//               Code to handle definitions of all the types
//===----------------------------------------------------------------------===//

/// InsertValue - Insert a value into the value table.  If it is named, this
/// returns -1, otherwise it returns the slot number for the value.
static int InsertValue(Value *V, ValueList &ValueTab = CurFun.Values) {
  // Things that have names or are void typed don't get slot numbers
  if (V->hasName() || (V->getType() == Type::VoidTy))
    return -1;

  // In the case of function values, we have to allow for the forward reference
  // of basic blocks, which are included in the numbering. Consequently, we keep
  // track of the next insertion location with NextValNum. When a BB gets 
  // inserted, it could change the size of the CurFun.Values vector.
  if (&ValueTab == &CurFun.Values) {
    if (ValueTab.size() <= CurFun.NextValNum)
      ValueTab.resize(CurFun.NextValNum+1);
    ValueTab[CurFun.NextValNum++] = V;
    return CurFun.NextValNum-1;
  } 
  // For all other lists, its okay to just tack it on the back of the vector.
  ValueTab.push_back(V);
  return ValueTab.size()-1;
}

static const Type *getTypeVal(const ValID &D, bool DoNotImprovise = false) {
  switch (D.Type) {
  case ValID::LocalID:               // Is it a numbered definition?
    // Module constants occupy the lowest numbered slots...
    if (D.Num < CurModule.Types.size())
      return CurModule.Types[D.Num];
    break;
  case ValID::LocalName:                 // Is it a named definition?
    if (const Type *N = CurModule.CurrentModule->getTypeByName(D.getName())) {
      D.destroy();  // Free old strdup'd memory...
      return N;
    }
    break;
  default:
    GenerateError("Internal parser error: Invalid symbol type reference");
    return 0;
  }

  // If we reached here, we referenced either a symbol that we don't know about
  // or an id number that hasn't been read yet.  We may be referencing something
  // forward, so just create an entry to be resolved later and get to it...
  //
  if (DoNotImprovise) return 0;  // Do we just want a null to be returned?


  if (inFunctionScope()) {
    if (D.Type == ValID::LocalName) {
      GenerateError("Reference to an undefined type: '" + D.getName() + "'");
      return 0;
    } else {
      GenerateError("Reference to an undefined type: #" + utostr(D.Num));
      return 0;
    }
  }

  std::map<ValID, PATypeHolder>::iterator I =CurModule.LateResolveTypes.find(D);
  if (I != CurModule.LateResolveTypes.end())
    return I->second;

  Type *Typ = OpaqueType::get();
  CurModule.LateResolveTypes.insert(std::make_pair(D, Typ));
  return Typ;
 }

// getExistingVal - Look up the value specified by the provided type and
// the provided ValID.  If the value exists and has already been defined, return
// it.  Otherwise return null.
//
static Value *getExistingVal(const Type *Ty, const ValID &D) {
  if (isa<FunctionType>(Ty)) {
    GenerateError("Functions are not values and "
                   "must be referenced as pointers");
    return 0;
  }

  switch (D.Type) {
  case ValID::LocalID: {                 // Is it a numbered definition?
    // Check that the number is within bounds.
    if (D.Num >= CurFun.Values.size()) 
      return 0;
    Value *Result = CurFun.Values[D.Num];
    if (Ty != Result->getType()) {
      GenerateError("Numbered value (%" + utostr(D.Num) + ") of type '" +
                    Result->getType()->getDescription() + "' does not match " 
                    "expected type, '" + Ty->getDescription() + "'");
      return 0;
    }
    return Result;
  }
  case ValID::GlobalID: {                 // Is it a numbered definition?
    if (D.Num >= CurModule.Values.size()) 
      return 0;
    Value *Result = CurModule.Values[D.Num];
    if (Ty != Result->getType()) {
      GenerateError("Numbered value (@" + utostr(D.Num) + ") of type '" +
                    Result->getType()->getDescription() + "' does not match " 
                    "expected type, '" + Ty->getDescription() + "'");
      return 0;
    }
    return Result;
  }
    
  case ValID::LocalName: {                // Is it a named definition?
    if (!inFunctionScope()) 
      return 0;
    ValueSymbolTable &SymTab = CurFun.CurrentFunction->getValueSymbolTable();
    Value *N = SymTab.lookup(D.getName());
    if (N == 0) 
      return 0;
    if (N->getType() != Ty)
      return 0;
    
    D.destroy();  // Free old strdup'd memory...
    return N;
  }
  case ValID::GlobalName: {                // Is it a named definition?
    ValueSymbolTable &SymTab = CurModule.CurrentModule->getValueSymbolTable();
    Value *N = SymTab.lookup(D.getName());
    if (N == 0) 
      return 0;
    if (N->getType() != Ty)
      return 0;

    D.destroy();  // Free old strdup'd memory...
    return N;
  }

  // Check to make sure that "Ty" is an integral type, and that our
  // value will fit into the specified type...
  case ValID::ConstSIntVal:    // Is it a constant pool reference??
    if (!isa<IntegerType>(Ty) ||
        !ConstantInt::isValueValidForType(Ty, D.ConstPool64)) {
      GenerateError("Signed integral constant '" +
                     itostr(D.ConstPool64) + "' is invalid for type '" +
                     Ty->getDescription() + "'");
      return 0;
    }
    return ConstantInt::get(Ty, D.ConstPool64, true);

  case ValID::ConstUIntVal:     // Is it an unsigned const pool reference?
    if (isa<IntegerType>(Ty) &&
        ConstantInt::isValueValidForType(Ty, D.UConstPool64))
      return ConstantInt::get(Ty, D.UConstPool64);

    if (!isa<IntegerType>(Ty) ||
        !ConstantInt::isValueValidForType(Ty, D.ConstPool64)) {
      GenerateError("Integral constant '" + utostr(D.UConstPool64) +
                    "' is invalid or out of range for type '" +
                    Ty->getDescription() + "'");
      return 0;
    }
    // This is really a signed reference.  Transmogrify.
    return ConstantInt::get(Ty, D.ConstPool64, true);

  case ValID::ConstAPInt:     // Is it an unsigned const pool reference?
    if (!isa<IntegerType>(Ty)) {
      GenerateError("Integral constant '" + D.getName() +
                    "' is invalid or out of range for type '" +
                    Ty->getDescription() + "'");
      return 0;
    }
      
    {
      APSInt Tmp = *D.ConstPoolInt;
      Tmp.extOrTrunc(Ty->getPrimitiveSizeInBits());
      return ConstantInt::get(Tmp);
    }
      
  case ValID::ConstFPVal:        // Is it a floating point const pool reference?
    if (!Ty->isFloatingPoint() ||
        !ConstantFP::isValueValidForType(Ty, *D.ConstPoolFP)) {
      GenerateError("FP constant invalid for type");
      return 0;
    }
    // Lexer has no type info, so builds all float and double FP constants 
    // as double.  Fix this here.  Long double does not need this.
    if (&D.ConstPoolFP->getSemantics() == &APFloat::IEEEdouble &&
        Ty==Type::FloatTy)
      D.ConstPoolFP->convert(APFloat::IEEEsingle, APFloat::rmNearestTiesToEven);
    return ConstantFP::get(*D.ConstPoolFP);

  case ValID::ConstNullVal:      // Is it a null value?
    if (!isa<PointerType>(Ty)) {
      GenerateError("Cannot create a a non pointer null");
      return 0;
    }
    return ConstantPointerNull::get(cast<PointerType>(Ty));

  case ValID::ConstUndefVal:      // Is it an undef value?
    return UndefValue::get(Ty);

  case ValID::ConstZeroVal:      // Is it a zero value?
    return Constant::getNullValue(Ty);
    
  case ValID::ConstantVal:       // Fully resolved constant?
    if (D.ConstantValue->getType() != Ty) {
      GenerateError("Constant expression type different from required type");
      return 0;
    }
    return D.ConstantValue;

  case ValID::InlineAsmVal: {    // Inline asm expression
    const PointerType *PTy = dyn_cast<PointerType>(Ty);
    const FunctionType *FTy =
      PTy ? dyn_cast<FunctionType>(PTy->getElementType()) : 0;
    if (!FTy || !InlineAsm::Verify(FTy, D.IAD->Constraints)) {
      GenerateError("Invalid type for asm constraint string");
      return 0;
    }
    InlineAsm *IA = InlineAsm::get(FTy, D.IAD->AsmString, D.IAD->Constraints,
                                   D.IAD->HasSideEffects);
    D.destroy();   // Free InlineAsmDescriptor.
    return IA;
  }
  default:
    assert(0 && "Unhandled case!");
    return 0;
  }   // End of switch

  assert(0 && "Unhandled case!");
  return 0;
}

// getVal - This function is identical to getExistingVal, except that if a
// value is not already defined, it "improvises" by creating a placeholder var
// that looks and acts just like the requested variable.  When the value is
// defined later, all uses of the placeholder variable are replaced with the
// real thing.
//
static Value *getVal(const Type *Ty, const ValID &ID) {
  if (Ty == Type::LabelTy) {
    GenerateError("Cannot use a basic block here");
    return 0;
  }

  // See if the value has already been defined.
  Value *V = getExistingVal(Ty, ID);
  if (V) return V;
  if (TriggerError) return 0;

  if (!Ty->isFirstClassType() && !isa<OpaqueType>(Ty)) {
    GenerateError("Invalid use of a non-first-class type");
    return 0;
  }

  // If we reached here, we referenced either a symbol that we don't know about
  // or an id number that hasn't been read yet.  We may be referencing something
  // forward, so just create an entry to be resolved later and get to it...
  //
  switch (ID.Type) {
  case ValID::GlobalName:
  case ValID::GlobalID: {
   const PointerType *PTy = dyn_cast<PointerType>(Ty);
   if (!PTy) {
     GenerateError("Invalid type for reference to global" );
     return 0;
   }
   const Type* ElTy = PTy->getElementType();
   if (const FunctionType *FTy = dyn_cast<FunctionType>(ElTy))
     V = Function::Create(FTy, GlobalValue::ExternalLinkage);
   else
     V = new GlobalVariable(ElTy, false, GlobalValue::ExternalLinkage, 0, "",
                            (Module*)0, false, PTy->getAddressSpace());
   break;
  }
  default:
   V = new Argument(Ty);
  }
  
  // Remember where this forward reference came from.  FIXME, shouldn't we try
  // to recycle these things??
  CurModule.PlaceHolderInfo.insert(std::make_pair(V, std::make_pair(ID,
                                                              LLLgetLineNo())));

  if (inFunctionScope())
    InsertValue(V, CurFun.LateResolveValues);
  else
    InsertValue(V, CurModule.LateResolveValues);
  return V;
}

/// defineBBVal - This is a definition of a new basic block with the specified
/// identifier which must be the same as CurFun.NextValNum, if its numeric.
static BasicBlock *defineBBVal(const ValID &ID) {
  assert(inFunctionScope() && "Can't get basic block at global scope!");

  BasicBlock *BB = 0;

  // First, see if this was forward referenced

  std::map<ValID, BasicBlock*>::iterator BBI = CurFun.BBForwardRefs.find(ID);
  if (BBI != CurFun.BBForwardRefs.end()) {
    BB = BBI->second;
    // The forward declaration could have been inserted anywhere in the
    // function: insert it into the correct place now.
    CurFun.CurrentFunction->getBasicBlockList().remove(BB);
    CurFun.CurrentFunction->getBasicBlockList().push_back(BB);

    // We're about to erase the entry, save the key so we can clean it up.
    ValID Tmp = BBI->first;

    // Erase the forward ref from the map as its no longer "forward"
    CurFun.BBForwardRefs.erase(ID);

    // The key has been removed from the map but so we don't want to leave 
    // strdup'd memory around so destroy it too.
    Tmp.destroy();

    // If its a numbered definition, bump the number and set the BB value.
    if (ID.Type == ValID::LocalID) {
      assert(ID.Num == CurFun.NextValNum && "Invalid new block number");
      InsertValue(BB);
    }
  } else { 
    // We haven't seen this BB before and its first mention is a definition. 
    // Just create it and return it.
    std::string Name (ID.Type == ValID::LocalName ? ID.getName() : "");
    BB = BasicBlock::Create(Name, CurFun.CurrentFunction);
    if (ID.Type == ValID::LocalID) {
      assert(ID.Num == CurFun.NextValNum && "Invalid new block number");
      InsertValue(BB);
    }
  }

  ID.destroy();
  return BB;
}

/// getBBVal - get an existing BB value or create a forward reference for it.
/// 
static BasicBlock *getBBVal(const ValID &ID) {
  assert(inFunctionScope() && "Can't get basic block at global scope!");

  BasicBlock *BB =  0;

  std::map<ValID, BasicBlock*>::iterator BBI = CurFun.BBForwardRefs.find(ID);
  if (BBI != CurFun.BBForwardRefs.end()) {
    BB = BBI->second;
  } if (ID.Type == ValID::LocalName) {
    std::string Name = ID.getName();
    Value *N = CurFun.CurrentFunction->getValueSymbolTable().lookup(Name);
    if (N) {
      if (N->getType()->getTypeID() == Type::LabelTyID)
        BB = cast<BasicBlock>(N);
      else
        GenerateError("Reference to label '" + Name + "' is actually of type '"+
          N->getType()->getDescription() + "'");
    }
  } else if (ID.Type == ValID::LocalID) {
    if (ID.Num < CurFun.NextValNum && ID.Num < CurFun.Values.size()) {
      if (CurFun.Values[ID.Num]->getType()->getTypeID() == Type::LabelTyID)
        BB = cast<BasicBlock>(CurFun.Values[ID.Num]);
      else
        GenerateError("Reference to label '%" + utostr(ID.Num) + 
          "' is actually of type '"+ 
          CurFun.Values[ID.Num]->getType()->getDescription() + "'");
    }
  } else {
    GenerateError("Illegal label reference " + ID.getName());
    return 0;
  }

  // If its already been defined, return it now.
  if (BB) {
    ID.destroy(); // Free strdup'd memory.
    return BB;
  }

  // Otherwise, this block has not been seen before, create it.
  std::string Name;
  if (ID.Type == ValID::LocalName)
    Name = ID.getName();
  BB = BasicBlock::Create(Name, CurFun.CurrentFunction);

  // Insert it in the forward refs map.
  CurFun.BBForwardRefs[ID] = BB;

  return BB;
}


//===----------------------------------------------------------------------===//
//              Code to handle forward references in instructions
//===----------------------------------------------------------------------===//
//
// This code handles the late binding needed with statements that reference
// values not defined yet... for example, a forward branch, or the PHI node for
// a loop body.
//
// This keeps a table (CurFun.LateResolveValues) of all such forward references
// and back patchs after we are done.
//

// ResolveDefinitions - If we could not resolve some defs at parsing
// time (forward branches, phi functions for loops, etc...) resolve the
// defs now...
//
static void 
ResolveDefinitions(ValueList &LateResolvers, ValueList *FutureLateResolvers) {
  // Loop over LateResolveDefs fixing up stuff that couldn't be resolved
  while (!LateResolvers.empty()) {
    Value *V = LateResolvers.back();
    LateResolvers.pop_back();

    std::map<Value*, std::pair<ValID, int> >::iterator PHI =
      CurModule.PlaceHolderInfo.find(V);
    assert(PHI != CurModule.PlaceHolderInfo.end() && "Placeholder error!");

    ValID &DID = PHI->second.first;

    Value *TheRealValue = getExistingVal(V->getType(), DID);
    if (TriggerError)
      return;
    if (TheRealValue) {
      V->replaceAllUsesWith(TheRealValue);
      delete V;
      CurModule.PlaceHolderInfo.erase(PHI);
    } else if (FutureLateResolvers) {
      // Functions have their unresolved items forwarded to the module late
      // resolver table
      InsertValue(V, *FutureLateResolvers);
    } else {
      if (DID.Type == ValID::LocalName || DID.Type == ValID::GlobalName) {
        GenerateError("Reference to an invalid definition: '" +DID.getName()+
                       "' of type '" + V->getType()->getDescription() + "'",
                       PHI->second.second);
        return;
      } else {
        GenerateError("Reference to an invalid definition: #" +
                       itostr(DID.Num) + " of type '" +
                       V->getType()->getDescription() + "'",
                       PHI->second.second);
        return;
      }
    }
  }
  LateResolvers.clear();
}

// ResolveTypeTo - A brand new type was just declared.  This means that (if
// name is not null) things referencing Name can be resolved.  Otherwise, things
// refering to the number can be resolved.  Do this now.
//
static void ResolveTypeTo(std::string *Name, const Type *ToTy) {
  ValID D;
  if (Name)
    D = ValID::createLocalName(*Name);
  else      
    D = ValID::createLocalID(CurModule.Types.size());

  std::map<ValID, PATypeHolder>::iterator I =
    CurModule.LateResolveTypes.find(D);
  if (I != CurModule.LateResolveTypes.end()) {
    ((DerivedType*)I->second.get())->refineAbstractTypeTo(ToTy);
    CurModule.LateResolveTypes.erase(I);
  }
}

// setValueName - Set the specified value to the name given.  The name may be
// null potentially, in which case this is a noop.  The string passed in is
// assumed to be a malloc'd string buffer, and is free'd by this function.
//
static void setValueName(Value *V, std::string *NameStr) {
  if (!NameStr) return;
  std::string Name(*NameStr);      // Copy string
  delete NameStr;                  // Free old string

  if (V->getType() == Type::VoidTy) {
    GenerateError("Can't assign name '" + Name+"' to value with void type");
    return;
  }

  assert(inFunctionScope() && "Must be in function scope!");
  ValueSymbolTable &ST = CurFun.CurrentFunction->getValueSymbolTable();
  if (ST.lookup(Name)) {
    GenerateError("Redefinition of value '" + Name + "' of type '" +
                   V->getType()->getDescription() + "'");
    return;
  }

  // Set the name.
  V->setName(Name);
}

/// ParseGlobalVariable - Handle parsing of a global.  If Initializer is null,
/// this is a declaration, otherwise it is a definition.
static GlobalVariable *
ParseGlobalVariable(std::string *NameStr,
                    GlobalValue::LinkageTypes Linkage,
                    GlobalValue::VisibilityTypes Visibility,
                    bool isConstantGlobal, const Type *Ty,
                    Constant *Initializer, bool IsThreadLocal,
                    unsigned AddressSpace = 0) {
  if (isa<FunctionType>(Ty)) {
    GenerateError("Cannot declare global vars of function type");
    return 0;
  }
  if (Ty == Type::LabelTy) {
    GenerateError("Cannot declare global vars of label type");
    return 0;
  }

  const PointerType *PTy = PointerType::get(Ty, AddressSpace);

  std::string Name;
  if (NameStr) {
    Name = *NameStr;      // Copy string
    delete NameStr;       // Free old string
  }

  // See if this global value was forward referenced.  If so, recycle the
  // object.
  ValID ID;
  if (!Name.empty()) {
    ID = ValID::createGlobalName(Name);
  } else {
    ID = ValID::createGlobalID(CurModule.Values.size());
  }

  if (GlobalValue *FWGV = CurModule.GetForwardRefForGlobal(PTy, ID)) {
    // Move the global to the end of the list, from whereever it was
    // previously inserted.
    GlobalVariable *GV = cast<GlobalVariable>(FWGV);
    CurModule.CurrentModule->getGlobalList().remove(GV);
    CurModule.CurrentModule->getGlobalList().push_back(GV);
    GV->setInitializer(Initializer);
    GV->setLinkage(Linkage);
    GV->setVisibility(Visibility);
    GV->setConstant(isConstantGlobal);
    GV->setThreadLocal(IsThreadLocal);
    InsertValue(GV, CurModule.Values);
    return GV;
  }

  // If this global has a name
  if (!Name.empty()) {
    // if the global we're parsing has an initializer (is a definition) and
    // has external linkage.
    if (Initializer && Linkage != GlobalValue::InternalLinkage)
      // If there is already a global with external linkage with this name
      if (CurModule.CurrentModule->getGlobalVariable(Name, false)) {
        // If we allow this GVar to get created, it will be renamed in the
        // symbol table because it conflicts with an existing GVar. We can't
        // allow redefinition of GVars whose linking indicates that their name
        // must stay the same. Issue the error.
        GenerateError("Redefinition of global variable named '" + Name +
                       "' of type '" + Ty->getDescription() + "'");
        return 0;
      }
  }

  // Otherwise there is no existing GV to use, create one now.
  GlobalVariable *GV =
    new GlobalVariable(Ty, isConstantGlobal, Linkage, Initializer, Name,
                       CurModule.CurrentModule, IsThreadLocal, AddressSpace);
  GV->setVisibility(Visibility);
  InsertValue(GV, CurModule.Values);
  return GV;
}

// setTypeName - Set the specified type to the name given.  The name may be
// null potentially, in which case this is a noop.  The string passed in is
// assumed to be a malloc'd string buffer, and is freed by this function.
//
// This function returns true if the type has already been defined, but is
// allowed to be redefined in the specified context.  If the name is a new name
// for the type plane, it is inserted and false is returned.
static bool setTypeName(const Type *T, std::string *NameStr) {
  assert(!inFunctionScope() && "Can't give types function-local names!");
  if (NameStr == 0) return false;
 
  std::string Name(*NameStr);      // Copy string
  delete NameStr;                  // Free old string

  // We don't allow assigning names to void type
  if (T == Type::VoidTy) {
    GenerateError("Can't assign name '" + Name + "' to the void type");
    return false;
  }

  // Set the type name, checking for conflicts as we do so.
  bool AlreadyExists = CurModule.CurrentModule->addTypeName(Name, T);

  if (AlreadyExists) {   // Inserting a name that is already defined???
    const Type *Existing = CurModule.CurrentModule->getTypeByName(Name);
    assert(Existing && "Conflict but no matching type?!");

    // There is only one case where this is allowed: when we are refining an
    // opaque type.  In this case, Existing will be an opaque type.
    if (const OpaqueType *OpTy = dyn_cast<OpaqueType>(Existing)) {
      // We ARE replacing an opaque type!
      const_cast<OpaqueType*>(OpTy)->refineAbstractTypeTo(T);
      return true;
    }

    // Otherwise, this is an attempt to redefine a type. That's okay if
    // the redefinition is identical to the original. This will be so if
    // Existing and T point to the same Type object. In this one case we
    // allow the equivalent redefinition.
    if (Existing == T) return true;  // Yes, it's equal.

    // Any other kind of (non-equivalent) redefinition is an error.
    GenerateError("Redefinition of type named '" + Name + "' of type '" +
                   T->getDescription() + "'");
  }

  return false;
}

//===----------------------------------------------------------------------===//
// Code for handling upreferences in type names...
//

// TypeContains - Returns true if Ty directly contains E in it.
//
static bool TypeContains(const Type *Ty, const Type *E) {
  return std::find(Ty->subtype_begin(), Ty->subtype_end(),
                   E) != Ty->subtype_end();
}

namespace {
  struct UpRefRecord {
    // NestingLevel - The number of nesting levels that need to be popped before
    // this type is resolved.
    unsigned NestingLevel;

    // LastContainedTy - This is the type at the current binding level for the
    // type.  Every time we reduce the nesting level, this gets updated.
    const Type *LastContainedTy;

    // UpRefTy - This is the actual opaque type that the upreference is
    // represented with.
    OpaqueType *UpRefTy;

    UpRefRecord(unsigned NL, OpaqueType *URTy)
      : NestingLevel(NL), LastContainedTy(URTy), UpRefTy(URTy) {}
  };
}

// UpRefs - A list of the outstanding upreferences that need to be resolved.
static std::vector<UpRefRecord> UpRefs;

/// HandleUpRefs - Every time we finish a new layer of types, this function is
/// called.  It loops through the UpRefs vector, which is a list of the
/// currently active types.  For each type, if the up reference is contained in
/// the newly completed type, we decrement the level count.  When the level
/// count reaches zero, the upreferenced type is the type that is passed in:
/// thus we can complete the cycle.
///
static PATypeHolder HandleUpRefs(const Type *ty) {
  // If Ty isn't abstract, or if there are no up-references in it, then there is
  // nothing to resolve here.
  if (!ty->isAbstract() || UpRefs.empty()) return ty;
  
  PATypeHolder Ty(ty);
  UR_OUT("Type '" << Ty->getDescription() <<
         "' newly formed.  Resolving upreferences.\n" <<
         UpRefs.size() << " upreferences active!\n");

  // If we find any resolvable upreferences (i.e., those whose NestingLevel goes
  // to zero), we resolve them all together before we resolve them to Ty.  At
  // the end of the loop, if there is anything to resolve to Ty, it will be in
  // this variable.
  OpaqueType *TypeToResolve = 0;

  for (unsigned i = 0; i != UpRefs.size(); ++i) {
    UR_OUT("  UR#" << i << " - TypeContains(" << Ty->getDescription() << ", "
           << UpRefs[i].second->getDescription() << ") = "
           << (TypeContains(Ty, UpRefs[i].second) ? "true" : "false") << "\n");
    if (TypeContains(Ty, UpRefs[i].LastContainedTy)) {
      // Decrement level of upreference
      unsigned Level = --UpRefs[i].NestingLevel;
      UpRefs[i].LastContainedTy = Ty;
      UR_OUT("  Uplevel Ref Level = " << Level << "\n");
      if (Level == 0) {                     // Upreference should be resolved!
        if (!TypeToResolve) {
          TypeToResolve = UpRefs[i].UpRefTy;
        } else {
          UR_OUT("  * Resolving upreference for "
                 << UpRefs[i].second->getDescription() << "\n";
                 std::string OldName = UpRefs[i].UpRefTy->getDescription());
          UpRefs[i].UpRefTy->refineAbstractTypeTo(TypeToResolve);
          UR_OUT("  * Type '" << OldName << "' refined upreference to: "
                 << (const void*)Ty << ", " << Ty->getDescription() << "\n");
        }
        UpRefs.erase(UpRefs.begin()+i);     // Remove from upreference list...
        --i;                                // Do not skip the next element...
      }
    }
  }

  if (TypeToResolve) {
    UR_OUT("  * Resolving upreference for "
           << UpRefs[i].second->getDescription() << "\n";
           std::string OldName = TypeToResolve->getDescription());
    TypeToResolve->refineAbstractTypeTo(Ty);
  }

  return Ty;
}

//===----------------------------------------------------------------------===//
//            RunVMAsmParser - Define an interface to this parser
//===----------------------------------------------------------------------===//
//
static Module* RunParser(Module * M);

Module *llvm::RunVMAsmParser(llvm::MemoryBuffer *MB) {
  InitLLLexer(MB);
  Module *M = RunParser(new Module(LLLgetFilename()));
  FreeLexer();
  return M;
}



/* Line 189 of yacc.c  */
#line 1038 "llvmAsmParser.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


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

/* Line 214 of yacc.c  */
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



/* Line 214 of yacc.c  */
#line 1286 "llvmAsmParser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1298 "llvmAsmParser.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  44
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2421

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  176
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  90
/* YYNRULES -- Number of rules.  */
#define YYNRULES  353
/* YYNRULES -- Number of states.  */
#define YYNSTATES  720

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   416

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     162,   163,   166,     2,   165,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     171,   164,   172,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   168,   167,   170,     2,     2,     2,     2,     2,   175,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     169,     2,     2,   173,     2,   174,     2,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      19,    21,    23,    25,    27,    29,    31,    33,    35,    37,
      39,    41,    43,    45,    47,    49,    51,    53,    55,    57,
      59,    61,    63,    65,    67,    69,    71,    73,    75,    77,
      79,    81,    83,    85,    87,    89,    91,    93,    95,    97,
      99,   101,   103,   105,   107,   109,   111,   113,   115,   117,
     119,   121,   123,   125,   127,   129,   130,   135,   136,   139,
     140,   143,   145,   147,   149,   150,   153,   155,   157,   159,
     161,   163,   165,   167,   169,   171,   172,   174,   176,   178,
     179,   181,   183,   184,   186,   188,   190,   192,   193,   195,
     197,   198,   200,   202,   204,   206,   208,   210,   213,   215,
     217,   219,   221,   223,   225,   227,   229,   231,   234,   235,
     238,   240,   242,   244,   246,   248,   250,   251,   254,   256,
     260,   264,   268,   270,   271,   276,   277,   280,   281,   284,
     285,   289,   292,   293,   295,   296,   300,   302,   305,   307,
     309,   311,   313,   315,   317,   319,   321,   323,   327,   329,
     332,   338,   344,   350,   356,   360,   363,   369,   374,   377,
     379,   381,   383,   387,   389,   393,   395,   396,   398,   402,
     407,   411,   415,   420,   425,   429,   436,   442,   445,   448,
     451,   454,   457,   460,   463,   466,   469,   472,   475,   478,
     485,   491,   500,   507,   514,   522,   530,   538,   546,   553,
     562,   571,   577,   585,   589,   591,   593,   595,   597,   598,
     601,   608,   610,   611,   613,   616,   617,   621,   622,   626,
     630,   634,   638,   639,   648,   649,   659,   660,   670,   676,
     679,   683,   685,   689,   693,   697,   701,   703,   704,   710,
     714,   716,   720,   722,   723,   735,   737,   739,   744,   746,
     748,   751,   755,   756,   758,   760,   762,   764,   766,   768,
     770,   772,   774,   776,   778,   782,   786,   789,   792,   796,
     799,   805,   810,   812,   818,   820,   822,   824,   826,   828,
     830,   833,   835,   839,   842,   845,   849,   853,   856,   857,
     859,   862,   865,   869,   879,   889,   898,   913,   915,   917,
     924,   930,   933,   936,   943,   951,   956,   961,   968,   975,
     976,   977,   981,   984,   988,   991,   993,   999,  1005,  1012,
    1019,  1026,  1033,  1038,  1045,  1050,  1055,  1062,  1069,  1072,
    1081,  1083,  1085,  1086,  1090,  1097,  1101,  1108,  1111,  1117,
    1125,  1131,  1136,  1141
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     226,     0,    -1,    76,    -1,    77,    -1,    78,    -1,    79,
      -1,    80,    -1,    81,    -1,    82,    -1,    83,    -1,    84,
      -1,    88,    -1,    89,    -1,    90,    -1,    85,    -1,    86,
      -1,    87,    -1,   121,    -1,   122,    -1,   123,    -1,   124,
      -1,   125,    -1,   126,    -1,   127,    -1,   128,    -1,   129,
      -1,   130,    -1,   131,    -1,   132,    -1,    95,    -1,    96,
      -1,    97,    -1,    98,    -1,    99,    -1,   100,    -1,   101,
      -1,   102,    -1,   103,    -1,   104,    -1,   105,    -1,   106,
      -1,   107,    -1,   108,    -1,   109,    -1,   110,    -1,   111,
      -1,   112,    -1,   113,    -1,   114,    -1,   101,    -1,   102,
      -1,   103,    -1,   104,    -1,    26,    -1,    27,    -1,    11,
      -1,    12,    -1,    13,    -1,    16,    -1,    15,    -1,    14,
      -1,    19,    -1,    22,    -1,    24,    -1,   184,    -1,    -1,
      55,   162,     4,   163,    -1,    -1,   184,   164,    -1,    -1,
       7,   164,    -1,    20,    -1,    23,    -1,   191,    -1,    -1,
     189,   164,    -1,    42,    -1,    44,    -1,    43,    -1,    45,
      -1,    47,    -1,    49,    -1,    46,    -1,    48,    -1,    51,
      -1,    -1,   159,    -1,   160,    -1,   161,    -1,    -1,    46,
      -1,    48,    -1,    -1,    42,    -1,    43,    -1,    44,    -1,
      47,    -1,    -1,    44,    -1,    42,    -1,    -1,    63,    -1,
      64,    -1,    65,    -1,    66,    -1,    67,    -1,    68,    -1,
      62,     4,    -1,   143,    -1,   122,    -1,   142,    -1,   123,
      -1,   145,    -1,   146,    -1,   148,    -1,   149,    -1,   150,
      -1,    54,     4,    -1,    -1,   200,   199,    -1,   144,    -1,
     147,    -1,   143,    -1,   142,    -1,   151,    -1,   152,    -1,
      -1,   202,   201,    -1,   204,    -1,   203,   165,   204,    -1,
     155,   164,   157,    -1,   155,   164,   156,    -1,   158,    -1,
      -1,   154,   162,   203,   163,    -1,    -1,   153,    22,    -1,
      -1,    54,     4,    -1,    -1,   165,    54,     4,    -1,    34,
      22,    -1,    -1,   209,    -1,    -1,   165,   212,   211,    -1,
     209,    -1,    54,     4,    -1,    11,    -1,    12,    -1,    13,
      -1,    16,    -1,    15,    -1,    14,    -1,    17,    -1,    50,
      -1,   213,    -1,   214,   186,   166,    -1,   248,    -1,   167,
       4,    -1,   214,   162,   218,   163,   202,    -1,    10,   162,
     218,   163,   202,    -1,   168,     4,   169,   214,   170,    -1,
     171,     4,   169,   214,   172,    -1,   173,   219,   174,    -1,
     173,   174,    -1,   171,   173,   219,   174,   172,    -1,   171,
     173,   174,   172,    -1,   214,   200,    -1,   214,    -1,    10,
      -1,   215,    -1,   217,   165,   215,    -1,   217,    -1,   217,
     165,    39,    -1,    39,    -1,    -1,   214,    -1,   219,   165,
     214,    -1,   214,   168,   222,   170,    -1,   214,   168,   170,
      -1,   214,   175,    22,    -1,   214,   171,   222,   172,    -1,
     214,   173,   222,   174,    -1,   214,   173,   174,    -1,   214,
     171,   173,   222,   174,   172,    -1,   214,   171,   173,   174,
     172,    -1,   214,    40,    -1,   214,    41,    -1,   214,   248,
      -1,   214,   221,    -1,   214,    25,    -1,   182,     3,    -1,
     182,     5,    -1,   182,     4,    -1,   182,     6,    -1,    11,
      26,    -1,    11,    27,    -1,   183,     9,    -1,   179,   162,
     220,    38,   214,   163,    -1,   120,   162,   220,   260,   163,
      -1,   134,   162,   220,   165,   220,   165,   220,   163,    -1,
     177,   162,   220,   165,   220,   163,    -1,   178,   162,   220,
     165,   220,   163,    -1,    91,   180,   162,   220,   165,   220,
     163,    -1,    92,   181,   162,   220,   165,   220,   163,    -1,
      93,   180,   162,   220,   165,   220,   163,    -1,    94,   181,
     162,   220,   165,   220,   163,    -1,   136,   162,   220,   165,
     220,   163,    -1,   137,   162,   220,   165,   220,   165,   220,
     163,    -1,   138,   162,   220,   165,   220,   165,   220,   163,
      -1,   140,   162,   220,   261,   163,    -1,   141,   162,   220,
     165,   220,   261,   163,    -1,   222,   165,   220,    -1,   220,
      -1,    32,    -1,    33,    -1,    37,    -1,    -1,   216,   248,
      -1,   126,   162,   225,    38,   214,   163,    -1,   227,    -1,
      -1,   228,    -1,   227,   228,    -1,    -1,    31,   229,   244,
      -1,    -1,    30,   230,   245,    -1,    60,    59,   234,    -1,
     187,    18,   214,    -1,   187,    18,    10,    -1,    -1,   190,
     194,   224,   223,   220,   186,   231,   211,    -1,    -1,   190,
     192,   194,   224,   223,   220,   186,   232,   211,    -1,    -1,
     190,   193,   194,   224,   223,   214,   186,   233,   211,    -1,
     190,   194,    35,   197,   225,    -1,    52,   235,    -1,    56,
     164,   236,    -1,    22,    -1,    53,   164,    22,    -1,    69,
     164,    22,    -1,   168,   237,   170,    -1,   237,   165,    22,
      -1,    22,    -1,    -1,   238,   165,   214,   200,   185,    -1,
     214,   200,   185,    -1,   238,    -1,   238,   165,    39,    -1,
      39,    -1,    -1,   198,   216,   189,   162,   239,   163,   202,
     210,   207,   206,   205,    -1,    28,    -1,   173,    -1,   196,
     194,   240,   241,    -1,    29,    -1,   174,    -1,   252,   243,
      -1,   195,   194,   240,    -1,    -1,    61,    -1,     3,    -1,
       4,    -1,     5,    -1,     6,    -1,     9,    -1,    26,    -1,
      27,    -1,    40,    -1,    41,    -1,    25,    -1,   171,   222,
     172,    -1,   168,   222,   170,    -1,   168,   170,    -1,   175,
      22,    -1,   173,   222,   174,    -1,   173,   174,    -1,   171,
     173,   222,   174,   172,    -1,   171,   173,   174,   172,    -1,
     221,    -1,    59,   246,    22,   165,    22,    -1,     7,    -1,
       8,    -1,   184,    -1,   189,    -1,   248,    -1,   247,    -1,
     214,   249,    -1,   250,    -1,   251,   165,   250,    -1,   252,
     253,    -1,   242,   253,    -1,   254,   187,   255,    -1,   254,
     188,   255,    -1,   254,   257,    -1,    -1,    21,    -1,    70,
     251,    -1,    70,    10,    -1,    71,    17,   249,    -1,    71,
      11,   249,   165,    17,   249,   165,    17,   249,    -1,    72,
     182,   249,   165,    17,   249,   168,   256,   170,    -1,    72,
     182,   249,   165,    17,   249,   168,   170,    -1,    73,   198,
     216,   249,   162,   259,   163,   202,    38,    17,   249,    74,
      17,   249,    -1,    74,    -1,    75,    -1,   256,   182,   247,
     165,    17,   249,    -1,   182,   247,   165,    17,   249,    -1,
     187,   263,    -1,   188,   263,    -1,   214,   168,   249,   165,
     249,   170,    -1,   258,   165,   168,   249,   165,   249,   170,
      -1,   214,   200,   249,   200,    -1,    17,   200,   249,   200,
      -1,   259,   165,   214,   200,   249,   200,    -1,   259,   165,
      17,   200,   249,   200,    -1,    -1,    -1,   260,   165,   250,
      -1,   165,     4,    -1,   261,   165,     4,    -1,    58,    57,
      -1,    57,    -1,   177,   214,   249,   165,   249,    -1,   178,
     214,   249,   165,   249,    -1,    91,   180,   214,   249,   165,
     249,    -1,    92,   181,   214,   249,   165,   249,    -1,    93,
     180,   214,   249,   165,   249,    -1,    94,   181,   214,   249,
     165,   249,    -1,   179,   250,    38,   214,    -1,   134,   250,
     165,   250,   165,   250,    -1,   135,   250,   165,   214,    -1,
     136,   250,   165,   250,    -1,   137,   250,   165,   250,   165,
     250,    -1,   138,   250,   165,   250,   165,   250,    -1,   133,
     258,    -1,   262,   198,   216,   249,   162,   259,   163,   202,
      -1,   265,    -1,    36,    -1,    -1,   115,   214,   208,    -1,
     115,   214,   165,    11,   249,   208,    -1,   116,   214,   208,
      -1,   116,   214,   165,    11,   249,   208,    -1,   117,   250,
      -1,   264,   118,   214,   249,   208,    -1,   264,   119,   250,
     165,   214,   249,   208,    -1,   139,   214,   249,   165,     4,
      -1,   120,   214,   249,   260,    -1,   140,   214,   249,   261,
      -1,   141,   214,   249,   165,   214,   249,   261,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1142,  1142,  1142,  1142,  1142,  1142,  1142,  1142,  1142,
    1142,  1143,  1143,  1143,  1143,  1143,  1143,  1144,  1144,  1144,
    1144,  1144,  1144,  1145,  1145,  1145,  1145,  1145,  1145,  1148,
    1148,  1149,  1149,  1150,  1150,  1151,  1151,  1152,  1152,  1156,
    1156,  1157,  1157,  1158,  1158,  1159,  1159,  1160,  1160,  1161,
    1161,  1162,  1162,  1163,  1164,  1169,  1170,  1170,  1170,  1170,
    1170,  1172,  1172,  1172,  1173,  1173,  1175,  1176,  1180,  1184,
    1189,  1195,  1195,  1197,  1198,  1203,  1209,  1210,  1211,  1212,
    1213,  1214,  1218,  1219,  1220,  1224,  1225,  1226,  1227,  1231,
    1232,  1233,  1237,  1238,  1239,  1240,  1241,  1245,  1246,  1247,
    1250,  1251,  1252,  1253,  1254,  1255,  1256,  1257,  1264,  1265,
    1266,  1267,  1268,  1269,  1270,  1271,  1272,  1273,  1277,  1278,
    1283,  1284,  1285,  1286,  1287,  1288,  1291,  1292,  1297,  1298,
    1309,  1310,  1311,  1314,  1315,  1320,  1321,  1328,  1329,  1335,
    1336,  1345,  1353,  1354,  1359,  1360,  1361,  1366,  1379,  1379,
    1379,  1379,  1379,  1379,  1379,  1382,  1386,  1390,  1397,  1402,
    1410,  1439,  1464,  1469,  1479,  1489,  1493,  1503,  1510,  1519,
    1526,  1531,  1536,  1543,  1544,  1551,  1558,  1566,  1572,  1584,
    1612,  1628,  1655,  1683,  1709,  1729,  1755,  1775,  1787,  1794,
    1860,  1870,  1880,  1886,  1896,  1902,  1912,  1918,  1924,  1937,
    1949,  1970,  1978,  1984,  1995,  2000,  2005,  2010,  2015,  2021,
    2027,  2033,  2041,  2052,  2056,  2064,  2064,  2067,  2067,  2070,
    2082,  2103,  2108,  2116,  2117,  2121,  2121,  2125,  2125,  2128,
    2131,  2155,  2167,  2166,  2178,  2177,  2187,  2186,  2197,  2237,
    2240,  2246,  2256,  2260,  2265,  2267,  2272,  2277,  2286,  2296,
    2307,  2311,  2320,  2329,  2334,  2466,  2466,  2468,  2477,  2477,
    2479,  2484,  2496,  2500,  2505,  2509,  2513,  2518,  2523,  2527,
    2531,  2535,  2539,  2543,  2547,  2569,  2591,  2597,  2610,  2622,
    2627,  2639,  2645,  2649,  2659,  2663,  2667,  2672,  2679,  2679,
    2685,  2694,  2699,  2704,  2708,  2717,  2726,  2739,  2748,  2752,
    2760,  2780,  2784,  2789,  2800,  2819,  2828,  2914,  2918,  2925,
    2936,  2949,  2958,  2971,  2982,  2992,  3003,  3011,  3021,  3028,
    3031,  3032,  3040,  3046,  3055,  3059,  3064,  3080,  3097,  3111,
    3125,  3139,  3153,  3165,  3173,  3180,  3186,  3192,  3198,  3213,
    3303,  3308,  3312,  3319,  3326,  3336,  3343,  3353,  3361,  3375,
    3392,  3406,  3421,  3436
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ESINT64VAL", "EUINT64VAL", "ESAPINTVAL",
  "EUAPINTVAL", "LOCALVAL_ID", "GLOBALVAL_ID", "FPVAL", "VOID", "INTTYPE",
  "FLOAT", "DOUBLE", "X86_FP80", "FP128", "PPC_FP128", "LABEL", "TYPE",
  "LOCALVAR", "GLOBALVAR", "LABELSTR", "STRINGCONSTANT",
  "ATSTRINGCONSTANT", "PCTSTRINGCONSTANT", "ZEROINITIALIZER", "TRUETOK",
  "FALSETOK", "BEGINTOK", "ENDTOK", "DECLARE", "DEFINE", "GLOBAL",
  "CONSTANT", "SECTION", "ALIAS", "VOLATILE", "THREAD_LOCAL", "TO",
  "DOTDOTDOT", "NULL_TOK", "UNDEF", "INTERNAL", "LINKONCE", "WEAK",
  "APPENDING", "DLLIMPORT", "DLLEXPORT", "EXTERN_WEAK", "COMMON", "OPAQUE",
  "EXTERNAL", "TARGET", "TRIPLE", "ALIGN", "ADDRSPACE", "DEPLIBS", "CALL",
  "TAIL", "ASM_TOK", "MODULE", "SIDEEFFECT", "CC_TOK", "CCC_TOK",
  "FASTCC_TOK", "COLDCC_TOK", "X86_STDCALLCC_TOK", "X86_FASTCALLCC_TOK",
  "X86_SSECALLCC_TOK", "DATALAYOUT", "RET", "BR", "SWITCH", "INVOKE",
  "UNWIND", "UNREACHABLE", "ADD", "SUB", "MUL", "UDIV", "SDIV", "FDIV",
  "UREM", "SREM", "FREM", "AND", "OR", "XOR", "SHL", "LSHR", "ASHR",
  "ICMP", "FCMP", "VICMP", "VFCMP", "EQ", "NE", "SLT", "SGT", "SLE", "SGE",
  "ULT", "UGT", "ULE", "UGE", "OEQ", "ONE", "OLT", "OGT", "OLE", "OGE",
  "ORD", "UNO", "UEQ", "UNE", "MALLOC", "ALLOCA", "FREE", "LOAD", "STORE",
  "GETELEMENTPTR", "TRUNC", "ZEXT", "SEXT", "FPTRUNC", "FPEXT", "BITCAST",
  "UITOFP", "SITOFP", "FPTOUI", "FPTOSI", "INTTOPTR", "PTRTOINT",
  "PHI_TOK", "SELECT", "VAARG", "EXTRACTELEMENT", "INSERTELEMENT",
  "SHUFFLEVECTOR", "GETRESULT", "EXTRACTVALUE", "INSERTVALUE", "SIGNEXT",
  "ZEROEXT", "NORETURN", "INREG", "SRET", "NOUNWIND", "NOALIAS", "BYVAL",
  "NEST", "READNONE", "READONLY", "GC", "FNNOTE", "INLINE", "ALWAYS",
  "NEVER", "OPTIMIZEFORSIZE", "DEFAULT", "HIDDEN", "PROTECTED", "'('",
  "')'", "'='", "','", "'*'", "'\\\\'", "'['", "'x'", "']'", "'<'", "'>'",
  "'{'", "'}'", "'c'", "$accept", "ArithmeticOps", "LogicalOps", "CastOps",
  "IPredicates", "FPredicates", "IntType", "FPType", "LocalName",
  "OptLocalName", "OptAddrSpace", "OptLocalAssign", "LocalNumber",
  "GlobalName", "OptGlobalAssign", "GlobalAssign", "GVInternalLinkage",
  "GVExternalLinkage", "GVVisibilityStyle", "FunctionDeclareLinkage",
  "FunctionDefineLinkage", "AliasLinkage", "OptCallingConv", "ParamAttr",
  "OptParamAttrs", "FuncAttr", "OptFuncAttrs", "FuncNoteList", "FuncNote",
  "OptFuncNotes", "OptGC", "OptAlign", "OptCAlign", "SectionString",
  "OptSection", "GlobalVarAttributes", "GlobalVarAttribute", "PrimType",
  "Types", "ArgType", "ResultTypes", "ArgTypeList", "ArgTypeListI",
  "TypeListI", "ConstVal", "ConstExpr", "ConstVector", "GlobalType",
  "ThreadLocal", "AliaseeRef", "Module", "DefinitionList", "Definition",
  "$@1", "$@2", "$@3", "$@4", "$@5", "AsmBlock", "TargetDefinition",
  "LibrariesDefinition", "LibList", "ArgListH", "ArgList",
  "FunctionHeaderH", "BEGIN", "FunctionHeader", "END", "Function",
  "FunctionProto", "OptSideEffect", "ConstValueRef", "SymbolicValueRef",
  "ValueRef", "ResolvedVal", "ReturnedVal", "BasicBlockList", "BasicBlock",
  "InstructionList", "BBTerminatorInst", "JumpTable", "Inst", "PHIList",
  "ParamList", "IndexList", "ConstantIndexList", "OptTailCall", "InstVal",
  "OptVolatile", "MemoryInst", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,    40,    41,    61,    44,    42,    92,    91,   120,
      93,    60,    62,   123,   125,    99
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   176,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   178,   178,   178,   178,   178,   178,   179,   179,   179,
     179,   179,   179,   179,   179,   179,   179,   179,   179,   180,
     180,   180,   180,   180,   180,   180,   180,   180,   180,   181,
     181,   181,   181,   181,   181,   181,   181,   181,   181,   181,
     181,   181,   181,   181,   181,   182,   183,   183,   183,   183,
     183,   184,   184,   184,   185,   185,   186,   186,   187,   187,
     188,   189,   189,   190,   190,   191,   192,   192,   192,   192,
     192,   192,   193,   193,   193,   194,   194,   194,   194,   195,
     195,   195,   196,   196,   196,   196,   196,   197,   197,   197,
     198,   198,   198,   198,   198,   198,   198,   198,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   200,   200,
     201,   201,   201,   201,   201,   201,   202,   202,   203,   203,
     204,   204,   204,   205,   205,   206,   206,   207,   207,   208,
     208,   209,   210,   210,   211,   211,   212,   212,   213,   213,
     213,   213,   213,   213,   213,   214,   214,   214,   214,   214,
     214,   214,   214,   214,   214,   214,   214,   214,   215,   216,
     216,   217,   217,   218,   218,   218,   218,   219,   219,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   221,
     221,   221,   221,   221,   221,   221,   221,   221,   221,   221,
     221,   221,   221,   222,   222,   223,   223,   224,   224,   225,
     225,   226,   226,   227,   227,   229,   228,   230,   228,   228,
     228,   228,   231,   228,   232,   228,   233,   228,   228,   228,
     228,   234,   235,   235,   236,   237,   237,   237,   238,   238,
     239,   239,   239,   239,   240,   241,   241,   242,   243,   243,
     244,   245,   246,   246,   247,   247,   247,   247,   247,   247,
     247,   247,   247,   247,   247,   247,   247,   247,   247,   247,
     247,   247,   247,   247,   248,   248,   248,   248,   249,   249,
     250,   251,   251,   252,   252,   253,   253,   254,   254,   254,
     255,   255,   255,   255,   255,   255,   255,   255,   255,   256,
     256,   257,   257,   258,   258,   259,   259,   259,   259,   259,
     260,   260,   261,   261,   262,   262,   263,   263,   263,   263,
     263,   263,   263,   263,   263,   263,   263,   263,   263,   263,
     263,   264,   264,   265,   265,   265,   265,   265,   265,   265,
     265,   265,   265,   265
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     4,     0,     2,     0,
       2,     1,     1,     1,     0,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     1,     1,     1,     0,
       1,     1,     0,     1,     1,     1,     1,     0,     1,     1,
       0,     1,     1,     1,     1,     1,     1,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     0,     2,
       1,     1,     1,     1,     1,     1,     0,     2,     1,     3,
       3,     3,     1,     0,     4,     0,     2,     0,     2,     0,
       3,     2,     0,     1,     0,     3,     1,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     1,     2,
       5,     5,     5,     5,     3,     2,     5,     4,     2,     1,
       1,     1,     3,     1,     3,     1,     0,     1,     3,     4,
       3,     3,     4,     4,     3,     6,     5,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     6,
       5,     8,     6,     6,     7,     7,     7,     7,     6,     8,
       8,     5,     7,     3,     1,     1,     1,     1,     0,     2,
       6,     1,     0,     1,     2,     0,     3,     0,     3,     3,
       3,     3,     0,     8,     0,     9,     0,     9,     5,     2,
       3,     1,     3,     3,     3,     3,     1,     0,     5,     3,
       1,     3,     1,     0,    11,     1,     1,     4,     1,     1,
       2,     3,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     2,     2,     3,     2,
       5,     4,     1,     5,     1,     1,     1,     1,     1,     1,
       2,     1,     3,     2,     2,     3,     3,     2,     0,     1,
       2,     2,     3,     9,     9,     8,    14,     1,     1,     6,
       5,     2,     2,     6,     7,     4,     4,     6,     6,     0,
       0,     3,     2,     3,     2,     1,     5,     5,     6,     6,
       6,     6,     4,     6,     4,     4,     6,     6,     2,     8,
       1,     1,     0,     3,     6,     3,     6,     2,     5,     7,
       5,     4,     4,     7
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
      74,    61,    71,    62,    72,    63,   227,   225,     0,     0,
       0,     0,     0,     0,    85,    73,     0,    74,   223,    89,
      92,     0,     0,   239,     0,     0,    68,     0,    75,    76,
      78,    77,    79,    82,    80,    83,    81,    84,    86,    87,
      88,    85,    85,   218,     1,   224,    90,    91,    85,   228,
      93,    94,    95,    96,    85,   298,   226,   298,     0,     0,
     247,   240,   241,   229,   284,   285,   231,   148,   149,   150,
     153,   152,   151,   154,   155,     0,     0,     0,     0,   286,
     287,   156,   230,   158,   218,   218,    97,   217,     0,   100,
     100,   299,   294,    69,   258,   259,   260,   293,   242,   243,
     246,     0,   176,   159,     0,     0,     0,     0,   165,   177,
       0,     0,   176,     0,     0,     0,    99,    98,     0,   215,
     216,     0,     0,   101,   102,   103,   104,   105,   106,     0,
     261,     0,     0,   342,   342,   297,     0,   244,   175,   118,
     171,   173,     0,     0,     0,     0,     0,     0,   164,     0,
       0,   157,     0,     0,   170,     0,   169,     0,   238,   148,
     149,   150,   153,   152,   151,     0,     0,    67,    67,   107,
       0,   255,   256,   257,    70,   341,   325,     0,     0,     0,
       0,   100,   307,   308,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    14,    15,    16,    11,    12,    13,     0,
       0,     0,     0,     0,     0,     0,     0,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   295,   100,   311,     0,   340,   296,   312,   245,   168,
       0,   126,    67,    67,   167,     0,   178,     0,   126,    67,
      67,     0,   219,   196,   197,   192,   194,   193,   195,   198,
     191,   187,   188,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   190,   189,   232,     0,   324,   301,    67,   291,   300,
       0,     0,    55,     0,     0,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,     0,    53,    54,    49,    50,
      51,    52,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,     0,     0,     0,   139,   139,   347,    67,    67,
     338,     0,     0,     0,     0,     0,    67,    67,    67,    67,
      67,     0,     0,     0,     0,     0,   109,   111,   110,   108,
     112,   113,   114,   115,   116,   119,   174,   172,   161,   162,
     163,   166,    66,   160,   234,   236,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   180,   214,
       0,     0,     0,   184,     0,   181,     0,     0,     0,   144,
     253,   264,   265,   266,   267,   268,   273,   269,   270,   271,
     272,   262,     0,     0,     0,     0,   282,   289,   288,   290,
       0,     0,   302,     0,     0,    67,    67,    67,    67,     0,
     343,     0,   345,   320,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    67,     0,
     117,   123,   122,   120,   121,   124,   125,   127,   144,   144,
       0,     0,     0,     0,     0,   320,     0,     0,     0,     0,
       0,     0,     0,   179,   165,   177,     0,   182,   183,     0,
       0,     0,     0,   233,   252,   118,   250,     0,   263,     0,
     276,     0,     0,     0,   279,     0,   277,   292,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   351,     0,
       0,     0,   334,   335,     0,     0,     0,     0,   352,     0,
       0,     0,   332,     0,   139,     0,   235,   237,    67,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     213,   186,     0,     0,     0,     0,     0,     0,   146,   144,
      65,     0,   126,     0,   275,   165,     0,   274,   278,     0,
       0,   319,     0,     0,     0,     0,   139,   140,   139,     0,
       0,     0,     0,     0,     0,   350,   322,     0,    67,   326,
     327,   319,     0,   348,    67,   220,     0,     0,     0,     0,
     200,     0,     0,     0,     0,   211,     0,   185,     0,     0,
      67,   141,   147,   145,    64,   249,   251,   118,   142,     0,
     281,     0,     0,     0,   118,   118,     0,   328,   329,   330,
     331,   344,   346,   321,     0,     0,   333,   336,   337,   323,
       0,     0,   139,     0,     0,     0,     0,     0,   208,     0,
       0,     0,   202,   203,   199,    65,   143,   137,   283,   280,
       0,     0,     0,     0,   126,     0,   313,     0,   353,   126,
     349,   204,   205,   206,   207,     0,     0,     0,   212,   248,
       0,   135,     0,   305,     0,     0,   109,   111,   118,   118,
       0,   118,   118,   314,   339,   201,   209,   210,   138,     0,
     133,   303,     0,   304,     0,   316,   315,     0,     0,     0,
     136,     0,   254,     0,     0,     0,   118,   118,     0,     0,
       0,     0,   318,   317,     0,   132,     0,   128,   310,     0,
       0,     0,   134,     0,   309,     0,   131,   130,   129,   306
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,   278,   279,   280,   305,   322,   165,   166,    79,   595,
     113,    12,   134,    80,    14,    15,    41,    42,    43,    48,
      54,   118,   129,   355,   239,   447,   358,   706,   707,   692,
     680,   661,   420,   538,   637,   473,   539,    81,   167,   140,
     157,   141,   142,   110,   379,   406,   380,   121,    88,   158,
      16,    17,    18,    20,    19,   389,   448,   449,    63,    23,
      61,   101,   476,   477,   130,   173,    55,    96,    56,    49,
     479,   407,    83,   409,   288,   289,    57,    92,    93,   231,
     665,   135,   330,   606,   498,   508,   232,   233,   234,   235
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -630
static const yytype_int16 yypact[] =
{
     247,  -630,  -630,  -630,  -630,  -630,  -630,  -630,    30,  -108,
      28,   -66,   102,   -40,    -1,  -630,   128,   668,  -630,    29,
     248,    21,    44,  -630,   -28,   143,  -630,  1914,  -630,  -630,
    -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,
    -630,   191,   191,   170,  -630,  -630,  -630,  -630,   191,  -630,
    -630,  -630,  -630,  -630,   191,   167,  -630,    -7,   190,   210,
     221,  -630,  -630,  -630,  -630,  -630,   114,  -630,  -630,  -630,
    -630,  -630,  -630,  -630,  -630,   289,   315,     3,    50,  -630,
    -630,  -630,   -32,  -630,   252,   252,   171,  -630,   280,   189,
     189,  -630,  -630,   324,  -630,  -630,  -630,  -630,  -630,  -630,
    -630,    57,  1662,  -630,   153,   165,   313,   114,  -630,   -32,
     -84,   204,  1662,   214,   280,   280,  -630,  -630,  1614,  -630,
    -630,  1955,   366,  -630,  -630,  -630,  -630,  -630,  -630,  1996,
    -630,   -18,   219,  2230,  2230,  -630,   354,  -630,  -630,   -32,
    -630,   234,   260,  2040,  2040,   256,   -80,  2040,  -630,   411,
     275,  -630,  1955,  2040,   114,   278,   -32,   222,  -630,   282,
     438,   439,   440,   441,   442,   158,   443,  1568,   398,  -630,
      86,  -630,  -630,  -630,  -630,  -630,  -630,   400,  2081,   105,
     447,   189,  -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,
    -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,   480,
     284,   480,   284,  2040,  2040,  2040,  2040,  -630,  -630,  -630,
    -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,  2040,
    2040,  2040,  2040,  2040,  2040,  2040,  2040,  2040,  2040,  2040,
    2040,  -630,   189,  -630,    55,  -630,  -630,  -630,  -630,   424,
    1703,  -630,   -19,   -36,  -630,   287,   -32,   297,  -630,   398,
      13,  1614,  -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,
    -630,  -630,  -630,   480,   284,   480,   284,   299,   300,   306,
     307,   310,   317,   320,  1744,  2122,   541,   453,   323,   326,
     328,  -630,  -630,  -630,   330,  -630,   114,   984,  -630,   312,
    1157,  1157,  -630,  1157,  1996,  -630,  -630,  -630,  -630,  -630,
    -630,  -630,  -630,  -630,  -630,  2040,  -630,  -630,  -630,  -630,
    -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,
    -630,  -630,  2040,  2040,  2040,   -17,    24,  -630,   984,   -39,
     336,   337,   341,   342,   343,   345,   984,   984,   984,   984,
     984,   457,  1996,  2040,  2040,   509,  -630,  -630,  -630,  -630,
    -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,   283,  -630,
    -630,  -630,  -630,   283,  -630,   214,   486,   364,   365,   367,
     368,  1955,  1955,  1955,  1955,  1955,  1955,  1955,  -630,  -630,
      63,   650,    54,  -630,   -70,  -630,  1955,  1955,  1955,   363,
    1788,  -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,
    -630,   470,  1829,  2166,   755,   510,  -630,  -630,  -630,  -630,
    2040,   369,  -630,   374,  1157,   984,   984,   984,   984,    42,
    -630,    43,  -630,  -630,  1157,   373,  2040,  2040,  2040,  2040,
    2040,   377,   378,   379,   385,   397,  2040,  1157,   984,   403,
    -630,  -630,  -630,  -630,  -630,  -630,  -630,  -630,   363,   363,
    2040,  1955,  1955,  1955,  1955,  -630,   406,   420,   423,   425,
     378,   427,  1955,  -630,   417,  1432,   -60,  -630,  -630,   428,
     429,   557,    17,  -630,  -630,   -32,   431,   445,  -630,   580,
    -630,   177,  1034,   129,  -630,   -24,  -630,  -630,   588,   592,
     448,   446,   449,   450,   451,  1157,   608,  1157,   452,   460,
    1157,   463,   -32,  -630,   469,   471,   609,   614,   472,  2040,
    1157,  1157,   -32,   476,   475,  2040,  -630,  -630,    -6,   477,
     479,   482,   483,   141,  1955,  1955,  1955,  1955,   202,  1955,
    -630,  -630,   473,  1955,  1955,  2040,   600,   637,  -630,   363,
     481,  1870,  -630,   484,  -630,   474,   -21,  -630,  -630,  1157,
    1157,  2207,  1157,  1157,  1157,  1157,   475,  -630,   475,  2040,
    1157,   485,  2040,  2040,  2040,  -630,  -630,   647,   984,  -630,
    -630,  2207,   598,  -630,   984,  -630,  1955,  1955,  1955,  1955,
    -630,   489,   492,   491,   506,  -630,   378,  -630,   512,   513,
      47,  -630,  -630,  -630,  -630,  -630,  -630,   -32,    -5,   655,
    -630,   507,   516,   514,     4,   -32,   212,  -630,  -630,  -630,
    -630,  -630,  -630,  -630,   515,  1157,  -630,  -630,  -630,  -630,
     378,   242,   475,   521,   526,   532,   533,  1955,  -630,  1955,
    1955,   243,  -630,  -630,  -630,   481,  -630,   643,  -630,  -630,
     684,     1,   827,   827,  -630,  2248,  -630,   534,   472,  -630,
    -630,  -630,  -630,  -630,  -630,   539,   540,   542,  -630,  -630,
     702,   554,  1157,  -630,  1296,     2,   548,   549,  -630,  -630,
      40,     4,   -32,  -630,   283,  -630,  -630,  -630,  -630,   691,
     562,  -630,   552,  -630,  1296,   424,   424,   701,   827,   827,
    -630,   559,  -630,   705,   558,  1157,  -630,  -630,   147,  1157,
     708,   645,   424,   424,   563,  -630,   266,  -630,  -630,  1157,
     709,   188,  -630,   147,  -630,  1157,  -630,  -630,  -630,  -630
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -630,   220,   223,   285,  -181,  -175,  -174,  -630,     0,    94,
    -142,   638,  -630,    11,  -630,  -630,  -630,  -630,    71,  -630,
    -630,  -630,  -144,  -630,  -226,  -630,  -227,  -630,    26,  -630,
    -630,  -630,  -308,   132,  -630,  -424,  -630,  -630,   -26,   494,
    -126,  -630,   628,   635,  -113,  -165,  -271,   318,   352,   493,
    -630,  -630,   725,  -630,  -630,  -630,  -630,  -630,  -630,  -630,
    -630,  -630,  -630,  -630,   656,  -630,  -630,  -630,  -630,  -630,
    -630,  -629,   -56,   183,  -190,  -630,  -630,   688,  -630,   613,
    -630,  -630,  -630,   178,   293,  -451,  -630,   616,  -630,  -630
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -223
static const yytype_int16 yytable[] =
{
      11,    82,   281,   170,   382,   384,   293,   105,   168,   528,
     171,    13,   292,   292,    91,   327,   111,    11,   422,   111,
     323,   363,    94,   111,   516,   517,   283,   324,    13,   536,
     331,   332,   333,   334,   335,   682,   111,   294,   111,   249,
     341,    29,    30,    31,    32,    33,    34,    35,    36,   111,
      37,   536,   109,   495,   497,   694,    24,    64,    65,  -154,
     107,    67,    68,    69,    70,    71,    72,    73,   111,     1,
       2,   537,     3,     4,     5,    46,   139,    47,   687,   111,
     109,   147,   367,    21,   369,   147,   139,    25,   342,   368,
     148,   370,   156,    11,   245,   462,   496,   496,    26,    22,
      74,   252,   111,   156,   468,   462,     2,   364,   365,     4,
     466,   282,    84,    85,   532,   593,   290,   242,   243,    89,
      27,   246,   291,   112,    28,    90,   112,   250,    44,   424,
     112,   481,   483,   485,   -67,   631,   360,   441,   442,   443,
      60,   462,   444,   112,   462,   112,   445,   446,   419,   -67,
     548,   359,   287,   601,   439,   172,   112,   575,    38,    39,
      40,   255,   256,   257,   258,    62,  -154,    95,   414,   648,
    -154,   663,   683,   343,   344,   112,   106,   325,   326,   287,
     328,   284,   441,   442,   443,    58,   112,   444,    91,   421,
     -67,   445,   446,   329,   287,   287,   287,   287,   287,   336,
     337,   338,   339,   340,   287,    86,   573,    87,    59,   112,
     634,   546,    98,   116,   139,   117,   437,    75,    76,   462,
     487,    77,   136,    78,   108,   156,   467,   137,   462,    64,
      65,   408,    99,   463,   408,   408,   501,   408,   503,   504,
     505,     1,     2,   100,     3,     4,     5,  -222,   611,   540,
     612,   122,   123,   124,   125,   126,   127,   128,   455,   456,
     457,   458,   459,   460,   461,   -69,     1,     2,   156,     3,
       4,     5,   408,   469,   470,   471,   102,     6,     7,   415,
     408,   408,   408,   408,   408,   -55,   -55,   -55,   -55,    87,
      50,    51,    52,   103,   462,    53,   416,   417,   418,     8,
     281,   547,   704,     9,   580,   705,   559,    10,   253,   254,
     306,   307,   119,   120,   650,   598,   156,   438,   287,   104,
      64,    65,   143,   107,    67,    68,    69,    70,    71,    72,
      73,   132,     1,     2,   144,     3,     4,     5,   519,   520,
     521,   522,   462,     1,   716,   717,     3,   544,     5,   530,
      38,    39,    40,   228,   228,   465,   229,   229,   408,   408,
     408,   408,   408,    74,   475,   585,   149,   567,   408,   613,
     169,   635,   616,   617,   618,   644,   238,   645,   642,   643,
     151,   408,   408,   174,   287,   308,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,   319,   320,   321,   240,
     287,   502,   287,   287,   287,   649,   658,   645,   567,   282,
     512,   581,   582,   583,   584,   247,   586,   670,   230,   230,
     588,   589,   674,   241,   518,   441,   442,   443,   244,   712,
     444,   713,   152,   153,   445,   446,   114,   115,   248,   408,
     251,   408,   685,   686,   408,   688,   689,   -56,   -57,   -60,
     -59,   -58,   259,   111,   408,   408,   465,   285,   292,   361,
     362,   371,   372,   623,   624,   625,   626,   664,   373,   374,
     702,   703,   375,   411,   412,   385,   413,   410,   345,   376,
      75,    76,   377,   568,    77,   386,    78,   145,   387,   574,
     388,   684,   390,   408,   408,   436,   408,   408,   408,   408,
       1,   425,   426,     3,   408,     5,   427,   428,   429,   590,
     430,   423,   408,   440,   655,   597,   656,   657,   408,   431,
     432,   433,   434,   435,   450,   605,   451,   452,   472,   453,
     454,   478,   486,   287,   488,   345,   287,   287,   287,   489,
     594,   500,   506,   507,   509,   605,   346,   347,    64,    65,
     510,   107,   159,   160,   161,   162,   163,   164,    73,   408,
       1,     2,   511,     3,     4,     5,   348,   349,   515,   350,
     351,   524,   352,   353,   354,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   525,   408,   408,   526,   531,
     527,    74,   529,   533,   534,   535,   541,   490,   491,   492,
     493,   494,   543,   346,   347,   549,   408,   499,   542,   550,
     551,   552,   557,   565,   553,   554,   555,   559,   566,   672,
     513,   514,   591,   348,   349,   560,   350,   351,   562,   352,
     353,   354,   408,   408,   563,   594,   564,   567,   571,   408,
     572,   592,   576,   408,   577,   587,   600,   578,   579,   599,
     615,   619,   496,   408,   627,   628,   629,    64,    65,   408,
     107,   159,   160,   161,   162,   163,   164,    73,  -221,     1,
       2,   630,     3,     4,     5,   632,   633,   638,   556,   639,
     558,   640,   641,   561,   651,   646,   -69,     1,     2,   652,
       3,     4,     5,   569,   570,   653,   654,   660,     6,     7,
      74,   662,   675,   676,   673,   677,   678,   679,    75,    76,
     -18,   -19,    77,   690,    78,   383,   691,   693,   695,   710,
       8,   698,   699,   700,     9,   709,   715,   711,    10,   659,
     636,   133,   602,   603,   357,   607,   608,   609,   610,   718,
     150,   146,    45,   614,   366,    97,   131,   236,   523,   621,
     237,   620,     0,     0,     0,     0,     0,   622,     0,     0,
       0,     0,    64,    65,     0,   107,   159,   160,   161,   162,
     163,   164,    73,     0,     1,     2,     0,     3,     4,     5,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   647,     0,
       0,     0,     0,     0,     0,    74,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    75,    76,     0,
       0,    77,     0,    78,   464,   668,   669,     0,     0,     0,
     391,   392,   393,   394,    64,    65,   395,     0,     0,     0,
       0,     0,     0,     0,     0,   681,     1,     2,     0,     3,
       4,     5,   396,   397,   398,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   399,   400,     0,
       0,   696,   697,     0,     0,     0,     0,     0,   701,     0,
       0,   345,   708,     0,     0,     0,   401,     0,     0,     0,
       0,     0,   714,     0,     0,     0,     0,     0,   719,     0,
       0,     0,     0,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   263,   264,
     265,   266,    75,    76,     0,     0,    77,     0,    78,   484,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   267,   207,   666,
     667,   210,   211,   212,   213,   214,   215,   216,   217,   218,
       0,   268,     0,   269,   270,   271,     0,   272,   273,   348,
     349,     0,   350,   351,     0,   352,   353,   354,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   391,   392,   393,
     394,    64,    65,   395,     0,   402,     0,     0,   403,     0,
     404,     0,   405,     1,     2,     0,     3,     4,     5,   396,
     397,   398,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   399,   400,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   111,
       0,    64,    65,   401,   107,   159,   160,   161,   162,   163,
     164,    73,     0,     1,     2,     0,     3,     4,     5,     0,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   263,   264,   265,   266,     0,
       0,     0,     0,     0,    74,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   267,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,     0,   268,     0,
     269,   270,   271,     0,   272,   273,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   112,     0,     0,     0,
       0,     0,   402,     0,     0,   403,     0,   404,     0,   405,
     391,   392,   393,   394,    64,    65,   395,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     1,     2,     0,     3,
       4,     5,   396,   397,   398,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   399,   400,     0,
       0,    75,    76,     0,     0,    77,     0,    78,   545,     0,
       0,     0,     0,     0,     0,     0,   401,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   263,   264,
     265,   266,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   267,   207,   208,
     209,   210,   211,   212,   213,   214,   215,   216,   217,   218,
       0,   268,     0,   269,   270,   271,     0,   272,   273,   391,
     392,   393,   394,     0,     0,   395,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   396,   397,   398,     0,   402,     0,     0,   403,     0,
     404,     0,   405,     0,     0,     0,   399,   400,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   401,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   263,   264,   265,
     266,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   267,   207,   208,   209,
     210,   211,   212,   213,   214,   215,   216,   217,   218,     0,
     268,     0,   269,   270,   271,     0,   272,   273,     0,    64,
      65,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     1,     2,     0,     3,     4,     5,   260,     0,     0,
       0,     0,     0,     0,   402,     0,     0,   403,     0,   404,
       0,   405,   261,   262,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   111,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   184,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   263,   264,   265,   266,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   267,   207,   208,   209,   210,   211,   212,   213,
     214,   215,   216,   217,   218,     0,   268,     0,   269,   270,
     271,     0,   272,   273,     0,    64,    65,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     1,     2,     0,
       3,     4,     5,   260,   112,     0,     0,     0,   -67,     0,
     274,     0,     0,   275,     0,   276,     0,   277,   261,   262,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    64,    65,   111,   154,    67,    68,    69,    70,    71,
      72,    73,     0,     1,     2,     0,     3,     4,     5,     0,
       0,     0,     0,     0,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   263,
     264,   265,   266,     0,    74,     0,     0,     0,     0,    64,
      65,     0,   107,    67,    68,    69,    70,    71,    72,    73,
       0,     1,     2,     0,     3,     4,     5,     0,   267,   207,
     208,   209,   210,   211,   212,   213,   214,   215,   216,   217,
     218,   138,   268,     0,   269,   270,   271,     0,   272,   273,
      64,    65,    74,   107,    67,    68,    69,    70,    71,    72,
      73,     0,     1,     2,     0,     3,     4,     5,     0,     0,
     112,     0,     0,     0,     0,     0,   274,     0,     0,   275,
     155,   276,   356,   277,     0,     0,     0,     0,     0,     0,
       0,    64,    65,    74,   107,   159,   160,   161,   162,   163,
     164,    73,     0,     1,     2,     0,     3,     4,     5,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    75,    76,     0,     0,    77,     0,    78,     0,     0,
       0,     0,     0,     0,    74,    64,    65,     0,   107,    67,
      68,    69,    70,    71,    72,    73,     0,     1,     2,     0,
       3,     4,     5,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   474,     0,    75,
      76,     0,     0,    77,     0,    78,    64,    65,    74,   107,
     159,   160,   161,   162,   163,   164,    73,     0,     1,     2,
       0,     3,     4,     5,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      75,    76,     0,     0,    77,     0,    78,    64,    65,    74,
     107,    67,    68,    69,    70,    71,    72,    73,     0,     1,
       2,     0,     3,     4,     5,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   596,
       0,    75,    76,     0,   378,    77,     0,    78,     0,     0,
      74,    64,    65,     0,    66,    67,    68,    69,    70,    71,
      72,    73,     0,     1,     2,     0,     3,     4,     5,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    75,    76,     0,     0,    77,
       0,    78,    64,    65,    74,   107,   159,   160,   161,   162,
     163,   164,    73,     0,     1,     2,     0,     3,     4,     5,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    75,    76,     0,   480,
      77,     0,    78,    64,    65,    74,   154,    67,    68,    69,
      70,    71,    72,    73,     0,     1,     2,     0,     3,     4,
       5,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    75,    76,     0,
       0,    77,     0,    78,     0,     0,    74,    64,    65,     0,
     107,    67,    68,    69,    70,    71,    72,    73,     0,     1,
       2,     0,     3,     4,     5,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    75,    76,     0,     0,    77,     0,    78,    64,    65,
      74,   286,    67,    68,    69,    70,    71,    72,    73,     0,
       1,     2,     0,     3,     4,     5,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    75,    76,     0,     0,    77,     0,    78,    64,
      65,    74,   107,   159,   160,   161,   162,   163,   164,    73,
       0,     1,     2,     0,     3,     4,     5,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    75,    76,     0,     0,    77,     0,    78,
       0,     0,    74,    64,    65,     0,   107,   159,   160,   161,
     162,   163,   164,    73,     0,     1,     2,     0,     3,     4,
       5,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    75,    76,     0,
       0,    77,     0,    78,    64,    65,    74,   107,    67,    68,
      69,    70,    71,    72,   604,     0,     1,     2,     0,     3,
       4,     5,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    75,    76,
       0,     0,    77,     0,    78,    64,    65,    74,   107,    67,
      68,    69,    70,    71,    72,   671,   175,     1,     2,     0,
       3,     4,     5,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   176,   177,    75,
      76,     0,     0,    77,     0,   381,     0,     0,    74,     0,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   202,     0,     0,     0,     0,     0,
       0,     0,     0,    75,    76,     0,     0,    77,     0,   482,
       0,     0,     0,     0,     0,   203,   204,   205,     0,     0,
     206,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   227,     0,     0,    75,    76,     0,     0,    77,     0,
      78,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    75,    76,     0,     0,    77,
       0,    78
};

static const yytype_int16 yycheck[] =
{
       0,    27,   167,   129,   275,   276,   180,     4,   121,   460,
      28,     0,    11,    11,    21,   205,    55,    17,   326,    55,
     201,   248,    29,    55,   448,   449,   168,   202,    17,    34,
     220,   221,   222,   223,   224,   664,    55,   181,    55,   152,
     230,    42,    43,    44,    45,    46,    47,    48,    49,    55,
      51,    34,    78,    11,    11,   684,   164,     7,     8,    55,
      10,    11,    12,    13,    14,    15,    16,    17,    55,    19,
      20,    54,    22,    23,    24,    46,   102,    48,    38,    55,
     106,   165,   263,    53,   265,   165,   112,    59,   232,   264,
     174,   266,   118,    93,   174,   165,    54,    54,   164,    69,
      50,   157,    55,   129,   174,   165,    20,   249,   250,    23,
     381,   167,    41,    42,   174,   539,    11,   143,   144,    48,
      18,   147,    17,   162,   164,    54,   162,   153,     0,   168,
     162,   402,   403,   404,   166,   586,   172,   142,   143,   144,
     168,   165,   147,   162,   165,   162,   151,   152,   165,   166,
     174,   170,   178,   174,   344,   173,   162,   163,   159,   160,
     161,     3,     4,     5,     6,    22,   162,   174,   294,   620,
     166,   170,   170,   118,   119,   162,   173,   203,   204,   205,
     206,   170,   142,   143,   144,   164,   162,   147,    21,   165,
     166,   151,   152,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,    35,   514,    37,   164,   162,
     163,   482,    22,    42,   240,    44,   342,   167,   168,   165,
     410,   171,   165,   173,   174,   251,   172,   170,   165,     7,
       8,   287,    22,   170,   290,   291,   426,   293,   428,   429,
     430,    19,    20,    22,    22,    23,    24,     0,   556,   475,
     558,    62,    63,    64,    65,    66,    67,    68,   371,   372,
     373,   374,   375,   376,   377,    18,    19,    20,   294,    22,
      23,    24,   328,   386,   387,   388,   162,    30,    31,   305,
     336,   337,   338,   339,   340,     3,     4,     5,     6,    37,
      42,    43,    44,     4,   165,    47,   322,   323,   324,    52,
     465,   172,   155,    56,   163,   158,   165,    60,    26,    27,
      26,    27,    32,    33,   622,   542,   342,   343,   344,     4,
       7,     8,   169,    10,    11,    12,    13,    14,    15,    16,
      17,     7,    19,    20,   169,    22,    23,    24,   451,   452,
     453,   454,   165,    19,   156,   157,    22,   170,    24,   462,
     159,   160,   161,   133,   134,   381,   133,   134,   414,   415,
     416,   417,   418,    50,   390,   163,   162,   165,   424,   559,
       4,   597,   562,   563,   564,   163,    22,   165,   604,   605,
     166,   437,   438,   164,   410,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   165,
     426,   427,   428,   429,   430,   163,   163,   165,   165,   465,
     436,   524,   525,   526,   527,     4,   529,   644,   133,   134,
     533,   534,   649,   163,   450,   142,   143,   144,   172,   163,
     147,   165,   114,   115,   151,   152,    84,    85,   163,   495,
     162,   497,   668,   669,   500,   671,   672,     9,     9,     9,
       9,     9,     9,    55,   510,   511,   482,    57,    11,   172,
     163,   162,   162,   576,   577,   578,   579,   641,   162,   162,
     696,   697,   162,   290,   291,    22,   293,   165,    54,   162,
     167,   168,   162,   509,   171,   162,   173,   174,   162,   515,
     162,   665,   162,   549,   550,    38,   552,   553,   554,   555,
      19,   165,   165,    22,   560,    24,   165,   165,   165,   535,
     165,   328,   568,     4,   627,   541,   629,   630,   574,   336,
     337,   338,   339,   340,    38,   551,   162,   162,   165,   162,
     162,    61,    22,   559,   165,    54,   562,   563,   564,   165,
     540,   168,   165,   165,   165,   571,   122,   123,     7,     8,
     165,    10,    11,    12,    13,    14,    15,    16,    17,   615,
      19,    20,   165,    22,    23,    24,   142,   143,   165,   145,
     146,   165,   148,   149,   150,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   165,   642,   643,   165,   172,
     165,    50,   165,   165,   165,    38,   165,   414,   415,   416,
     417,   418,    22,   122,   123,    17,   662,   424,   163,    17,
     162,   165,     4,     4,   165,   165,   165,   165,     4,   645,
     437,   438,    22,   142,   143,   165,   145,   146,   165,   148,
     149,   150,   688,   689,   165,   635,   165,   165,   162,   695,
     165,     4,   165,   699,   165,   172,   172,   165,   165,   165,
     165,     4,    54,   709,   165,   163,   165,     7,     8,   715,
      10,    11,    12,    13,    14,    15,    16,    17,     0,    19,
      20,   165,    22,    23,    24,   163,   163,    22,   495,   172,
     497,   165,   168,   500,   163,   170,    18,    19,    20,   163,
      22,    23,    24,   510,   511,   163,   163,    54,    30,    31,
      50,    17,   163,   163,   170,   163,     4,   153,   167,   168,
     162,   162,   171,    22,   173,   174,   154,   165,    17,    74,
      52,   162,    17,   165,    56,    17,    17,   164,    60,   635,
     598,    93,   549,   550,   240,   552,   553,   554,   555,   713,
     112,   106,    17,   560,   251,    57,    90,   134,   455,   571,
     134,   568,    -1,    -1,    -1,    -1,    -1,   574,    -1,    -1,
      -1,    -1,     7,     8,    -1,    10,    11,    12,    13,    14,
      15,    16,    17,    -1,    19,    20,    -1,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   615,    -1,
      -1,    -1,    -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,   168,    -1,
      -1,   171,    -1,   173,   174,   642,   643,    -1,    -1,    -1,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   662,    19,    20,    -1,    22,
      23,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    40,    41,    -1,
      -1,   688,   689,    -1,    -1,    -1,    -1,    -1,   695,    -1,
      -1,    54,   699,    -1,    -1,    -1,    59,    -1,    -1,    -1,
      -1,    -1,   709,    -1,    -1,    -1,    -1,    -1,   715,    -1,
      -1,    -1,    -1,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,   167,   168,    -1,    -1,   171,    -1,   173,   174,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,   121,   122,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
      -1,   134,    -1,   136,   137,   138,    -1,   140,   141,   142,
     143,    -1,   145,   146,    -1,   148,   149,   150,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,     7,     8,     9,    -1,   168,    -1,    -1,   171,    -1,
     173,    -1,   175,    19,    20,    -1,    22,    23,    24,    25,
      26,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    40,    41,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,
      -1,     7,     8,    59,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    19,    20,    -1,    22,    23,    24,    -1,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    -1,
      -1,    -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,    -1,   134,    -1,
     136,   137,   138,    -1,   140,   141,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   162,    -1,    -1,    -1,
      -1,    -1,   168,    -1,    -1,   171,    -1,   173,    -1,   175,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    -1,    22,
      23,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    40,    41,    -1,
      -1,   167,   168,    -1,    -1,   171,    -1,   173,   174,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    59,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,   121,   122,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
      -1,   134,    -1,   136,   137,   138,    -1,   140,   141,     3,
       4,     5,     6,    -1,    -1,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    25,    26,    27,    -1,   168,    -1,    -1,   171,    -1,
     173,    -1,   175,    -1,    -1,    -1,    40,    41,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    59,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,    -1,
     134,    -1,   136,   137,   138,    -1,   140,   141,    -1,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    19,    20,    -1,    22,    23,    24,    25,    -1,    -1,
      -1,    -1,    -1,    -1,   168,    -1,    -1,   171,    -1,   173,
      -1,   175,    40,    41,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,    -1,   134,    -1,   136,   137,
     138,    -1,   140,   141,    -1,     7,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    -1,
      22,    23,    24,    25,   162,    -1,    -1,    -1,   166,    -1,
     168,    -1,    -1,   171,    -1,   173,    -1,   175,    40,    41,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     7,     8,    55,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    19,    20,    -1,    22,    23,    24,    -1,
      -1,    -1,    -1,    -1,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    -1,    50,    -1,    -1,    -1,    -1,     7,
       8,    -1,    10,    11,    12,    13,    14,    15,    16,    17,
      -1,    19,    20,    -1,    22,    23,    24,    -1,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,    39,   134,    -1,   136,   137,   138,    -1,   140,   141,
       7,     8,    50,    10,    11,    12,    13,    14,    15,    16,
      17,    -1,    19,    20,    -1,    22,    23,    24,    -1,    -1,
     162,    -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,   171,
     126,   173,    39,   175,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     7,     8,    50,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    19,    20,    -1,    22,    23,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   167,   168,    -1,    -1,   171,    -1,   173,    -1,    -1,
      -1,    -1,    -1,    -1,    50,     7,     8,    -1,    10,    11,
      12,    13,    14,    15,    16,    17,    -1,    19,    20,    -1,
      22,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,   167,
     168,    -1,    -1,   171,    -1,   173,     7,     8,    50,    10,
      11,    12,    13,    14,    15,    16,    17,    -1,    19,    20,
      -1,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     167,   168,    -1,    -1,   171,    -1,   173,     7,     8,    50,
      10,    11,    12,    13,    14,    15,    16,    17,    -1,    19,
      20,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,   167,   168,    -1,   170,   171,    -1,   173,    -1,    -1,
      50,     7,     8,    -1,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    19,    20,    -1,    22,    23,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   167,   168,    -1,    -1,   171,
      -1,   173,     7,     8,    50,    10,    11,    12,    13,    14,
      15,    16,    17,    -1,    19,    20,    -1,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   167,   168,    -1,   170,
     171,    -1,   173,     7,     8,    50,    10,    11,    12,    13,
      14,    15,    16,    17,    -1,    19,    20,    -1,    22,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,   168,    -1,
      -1,   171,    -1,   173,    -1,    -1,    50,     7,     8,    -1,
      10,    11,    12,    13,    14,    15,    16,    17,    -1,    19,
      20,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   167,   168,    -1,    -1,   171,    -1,   173,     7,     8,
      50,    10,    11,    12,    13,    14,    15,    16,    17,    -1,
      19,    20,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   167,   168,    -1,    -1,   171,    -1,   173,     7,
       8,    50,    10,    11,    12,    13,    14,    15,    16,    17,
      -1,    19,    20,    -1,    22,    23,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   167,   168,    -1,    -1,   171,    -1,   173,
      -1,    -1,    50,     7,     8,    -1,    10,    11,    12,    13,
      14,    15,    16,    17,    -1,    19,    20,    -1,    22,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,   168,    -1,
      -1,   171,    -1,   173,     7,     8,    50,    10,    11,    12,
      13,    14,    15,    16,    17,    -1,    19,    20,    -1,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,   168,
      -1,    -1,   171,    -1,   173,     7,     8,    50,    10,    11,
      12,    13,    14,    15,    16,    17,    36,    19,    20,    -1,
      22,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,    58,   167,
     168,    -1,    -1,   171,    -1,   173,    -1,    -1,    50,    -1,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   167,   168,    -1,    -1,   171,    -1,   173,
      -1,    -1,    -1,    -1,    -1,   115,   116,   117,    -1,    -1,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,    -1,    -1,   167,   168,    -1,    -1,   171,    -1,
     173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   167,   168,    -1,    -1,   171,
      -1,   173
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,    19,    20,    22,    23,    24,    30,    31,    52,    56,
      60,   184,   187,   189,   190,   191,   226,   227,   228,   230,
     229,    53,    69,   235,   164,    59,   164,    18,   164,    42,
      43,    44,    45,    46,    47,    48,    49,    51,   159,   160,
     161,   192,   193,   194,     0,   228,    46,    48,   195,   245,
      42,    43,    44,    47,   196,   242,   244,   252,   164,   164,
     168,   236,    22,   234,     7,     8,    10,    11,    12,    13,
      14,    15,    16,    17,    50,   167,   168,   171,   173,   184,
     189,   213,   214,   248,   194,   194,    35,    37,   224,   194,
     194,    21,   253,   254,    29,   174,   243,   253,    22,    22,
      22,   237,   162,     4,     4,     4,   173,    10,   174,   214,
     219,    55,   162,   186,   224,   224,    42,    44,   197,    32,
      33,   223,    62,    63,    64,    65,    66,    67,    68,   198,
     240,   240,     7,   187,   188,   257,   165,   170,    39,   214,
     215,   217,   218,   169,   169,   174,   219,   165,   174,   162,
     218,   166,   223,   223,    10,   126,   214,   216,   225,    11,
      12,    13,    14,    15,    16,   182,   183,   214,   220,     4,
     216,    28,   173,   241,   164,    36,    57,    58,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,   115,   116,   117,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   177,   178,
     179,   255,   262,   263,   264,   265,   255,   263,    22,   200,
     165,   163,   214,   214,   172,   174,   214,     4,   163,   220,
     214,   162,   248,    26,    27,     3,     4,     5,     6,     9,
      25,    40,    41,    91,    92,    93,    94,   120,   134,   136,
     137,   138,   140,   141,   168,   171,   173,   175,   177,   178,
     179,   221,   248,   186,   189,    57,    10,   214,   250,   251,
      11,    17,    11,   182,   198,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   180,    26,    27,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   181,   180,   181,   214,   214,   250,   214,   214,
     258,   250,   250,   250,   250,   250,   214,   214,   214,   214,
     214,   250,   198,   118,   119,    54,   122,   123,   142,   143,
     145,   146,   148,   149,   150,   199,    39,   215,   202,   170,
     172,   172,   163,   202,   186,   186,   225,   180,   181,   180,
     181,   162,   162,   162,   162,   162,   162,   162,   170,   220,
     222,   173,   222,   174,   222,    22,   162,   162,   162,   231,
     162,     3,     4,     5,     6,     9,    25,    26,    27,    40,
      41,    59,   168,   171,   173,   175,   221,   247,   248,   249,
     165,   249,   249,   249,   216,   214,   214,   214,   214,   165,
     208,   165,   208,   249,   168,   165,   165,   165,   165,   165,
     165,   249,   249,   249,   249,   249,    38,   216,   214,   250,
       4,   142,   143,   144,   147,   151,   152,   201,   232,   233,
      38,   162,   162,   162,   162,   220,   220,   220,   220,   220,
     220,   220,   165,   170,   174,   214,   222,   172,   174,   220,
     220,   220,   165,   211,    39,   214,   238,   239,    61,   246,
     170,   222,   173,   222,   174,   222,    22,   250,   165,   165,
     249,   249,   249,   249,   249,    11,    54,    11,   260,   249,
     168,   250,   214,   250,   250,   250,   165,   165,   261,   165,
     165,   165,   214,   249,   249,   165,   211,   211,   214,   220,
     220,   220,   220,   260,   165,   165,   165,   165,   261,   165,
     220,   172,   174,   165,   165,    38,    34,    54,   209,   212,
     200,   165,   163,    22,   170,   174,   222,   172,   174,    17,
      17,   162,   165,   165,   165,   165,   249,     4,   249,   165,
     165,   249,   165,   165,   165,     4,     4,   165,   214,   249,
     249,   162,   165,   208,   214,   163,   165,   165,   165,   165,
     163,   220,   220,   220,   220,   163,   220,   172,   220,   220,
     214,    22,     4,   211,   184,   185,    39,   214,   202,   165,
     172,   174,   249,   249,    17,   214,   259,   249,   249,   249,
     249,   208,   208,   250,   249,   165,   250,   250,   250,     4,
     249,   259,   249,   220,   220,   220,   220,   165,   163,   165,
     165,   261,   163,   163,   163,   200,   209,   210,    22,   172,
     165,   168,   200,   200,   163,   165,   170,   249,   261,   163,
     208,   163,   163,   163,   163,   220,   220,   220,   163,   185,
      54,   207,    17,   170,   182,   256,   122,   123,   249,   249,
     202,    17,   214,   170,   202,   163,   163,   163,     4,   153,
     206,   249,   247,   170,   182,   200,   200,    38,   200,   200,
      22,   154,   205,   165,   247,    17,   249,   249,   162,    17,
     165,   249,   200,   200,   155,   158,   203,   204,   249,    17,
      74,   164,   163,   165,   249,    17,   156,   157,   204,   249
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
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



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 29:

/* Line 1455 of yacc.c  */
#line 1148 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.IPredicate) = ICmpInst::ICMP_EQ; ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 1148 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.IPredicate) = ICmpInst::ICMP_NE; ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 1149 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.IPredicate) = ICmpInst::ICMP_SLT; ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 1149 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.IPredicate) = ICmpInst::ICMP_SGT; ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 1150 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.IPredicate) = ICmpInst::ICMP_SLE; ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 1150 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.IPredicate) = ICmpInst::ICMP_SGE; ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 1151 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.IPredicate) = ICmpInst::ICMP_ULT; ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 1151 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.IPredicate) = ICmpInst::ICMP_UGT; ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 1152 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.IPredicate) = ICmpInst::ICMP_ULE; ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 1152 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.IPredicate) = ICmpInst::ICMP_UGE; ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 1156 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_OEQ; ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 1156 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_ONE; ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 1157 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_OLT; ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 1157 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_OGT; ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 1158 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_OLE; ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 1158 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_OGE; ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 1159 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_ORD; ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 1159 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_UNO; ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 1160 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_UEQ; ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 1160 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_UNE; ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 1161 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_ULT; ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 1161 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_UGT; ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 1162 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_ULE; ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 1162 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_UGE; ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 1163 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_TRUE; ;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 1164 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FPredicate) = FCmpInst::FCMP_FALSE; ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 1173 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.StrVal) = 0; ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 1175 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.UIntVal)=(yyvsp[(3) - (4)].UInt64Val); ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 1176 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.UIntVal)=0; ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 1180 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.StrVal) = (yyvsp[(1) - (2)].StrVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 1184 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.StrVal) = 0;
    CHECK_FOR_ERROR
  ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1189 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
  (yyval.UIntVal) = (yyvsp[(1) - (2)].UIntVal);
  CHECK_FOR_ERROR
;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1198 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.StrVal) = 0;
    CHECK_FOR_ERROR
  ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1203 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.StrVal) = (yyvsp[(1) - (2)].StrVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1209 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::InternalLinkage; ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1210 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::WeakLinkage; ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1211 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::LinkOnceLinkage; ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1212 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::AppendingLinkage; ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 1213 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::DLLExportLinkage; ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 1214 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::CommonLinkage; ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1218 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::DLLImportLinkage; ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1219 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::ExternalWeakLinkage; ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1220 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::ExternalLinkage; ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1224 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Visibility) = GlobalValue::DefaultVisibility;   ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 1225 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Visibility) = GlobalValue::DefaultVisibility;   ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 1226 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Visibility) = GlobalValue::HiddenVisibility;    ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 1227 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Visibility) = GlobalValue::ProtectedVisibility; ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 1231 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::ExternalLinkage; ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 1232 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::DLLImportLinkage; ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 1233 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::ExternalWeakLinkage; ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 1237 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::ExternalLinkage; ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 1238 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::InternalLinkage; ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1239 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::LinkOnceLinkage; ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1240 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::WeakLinkage; ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1241 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::DLLExportLinkage; ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1245 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::ExternalLinkage; ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1246 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::WeakLinkage; ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1247 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.Linkage) = GlobalValue::InternalLinkage; ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1250 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.UIntVal) = CallingConv::C; ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1251 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.UIntVal) = CallingConv::C; ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1252 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.UIntVal) = CallingConv::Fast; ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1253 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.UIntVal) = CallingConv::Cold; ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1254 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.UIntVal) = CallingConv::X86_StdCall; ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1255 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.UIntVal) = CallingConv::X86_FastCall; ;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 1256 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.UIntVal) = CallingConv::X86_SSECall; ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1257 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
                   if ((unsigned)(yyvsp[(2) - (2)].UInt64Val) != (yyvsp[(2) - (2)].UInt64Val))
                     GEN_ERROR("Calling conv too large");
                   (yyval.UIntVal) = (yyvsp[(2) - (2)].UInt64Val);
                  CHECK_FOR_ERROR
                 ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1264 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::ZExt;      ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1265 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::ZExt;      ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1266 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::SExt;      ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1267 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::SExt;      ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 1268 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::InReg;     ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 1269 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::StructRet; ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1270 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::NoAlias;   ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1271 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::ByVal;     ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1272 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::Nest;      ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1273 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = 
                          ParamAttr::constructAlignmentFromInt((yyvsp[(2) - (2)].UInt64Val));    ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1277 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::None; ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1278 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
                (yyval.ParamAttrs) = (yyvsp[(1) - (2)].ParamAttrs) | (yyvsp[(2) - (2)].ParamAttrs);
              ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1283 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::NoReturn; ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1284 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::NoUnwind; ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1285 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::ZExt;     ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1286 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::SExt;     ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1287 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::ReadNone; ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1288 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::ReadOnly; ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1291 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamAttrs) = ParamAttr::None; ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1292 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
                (yyval.ParamAttrs) = (yyvsp[(1) - (2)].ParamAttrs) | (yyvsp[(2) - (2)].ParamAttrs);
              ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1297 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FunctionNotes) = (yyvsp[(1) - (1)].FunctionNotes); ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1298 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { 
                FunctionNotes tmp = (yyvsp[(1) - (3)].FunctionNotes) | (yyvsp[(3) - (3)].FunctionNotes);
                if ((yyvsp[(3) - (3)].FunctionNotes) == FN_NOTE_NoInline && ((yyvsp[(1) - (3)].FunctionNotes) & FN_NOTE_AlwaysInline))
                  GEN_ERROR("Function Notes may include only one inline notes!")
                if ((yyvsp[(3) - (3)].FunctionNotes) == FN_NOTE_AlwaysInline && ((yyvsp[(1) - (3)].FunctionNotes) & FN_NOTE_NoInline))
                  GEN_ERROR("Function Notes may include only one inline notes!")
                (yyval.FunctionNotes) = tmp;
                CHECK_FOR_ERROR 
              ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1309 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FunctionNotes) = FN_NOTE_NoInline; ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1310 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FunctionNotes) = FN_NOTE_AlwaysInline; ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1311 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FunctionNotes) = FN_NOTE_OptimizeForSize; ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1314 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.FunctionNotes) = FN_NOTE_None; ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1315 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
                (yyval.FunctionNotes) =  (yyvsp[(3) - (4)].FunctionNotes);
              ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1320 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.StrVal) = 0; ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1321 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
                (yyval.StrVal) = (yyvsp[(2) - (2)].StrVal);
              ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1328 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.UIntVal) = 0; ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1329 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
  (yyval.UIntVal) = (yyvsp[(2) - (2)].UInt64Val);
  if ((yyval.UIntVal) != 0 && !isPowerOf2_32((yyval.UIntVal)))
    GEN_ERROR("Alignment must be a power of two");
  CHECK_FOR_ERROR
;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1335 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.UIntVal) = 0; ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1336 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
  (yyval.UIntVal) = (yyvsp[(3) - (3)].UInt64Val);
  if ((yyval.UIntVal) != 0 && !isPowerOf2_32((yyval.UIntVal)))
    GEN_ERROR("Alignment must be a power of two");
  CHECK_FOR_ERROR
;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1345 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
  for (unsigned i = 0, e = (yyvsp[(2) - (2)].StrVal)->length(); i != e; ++i)
    if ((*(yyvsp[(2) - (2)].StrVal))[i] == '"' || (*(yyvsp[(2) - (2)].StrVal))[i] == '\\')
      GEN_ERROR("Invalid character in section name");
  (yyval.StrVal) = (yyvsp[(2) - (2)].StrVal);
  CHECK_FOR_ERROR
;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1353 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.StrVal) = 0; ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1354 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.StrVal) = (yyvsp[(1) - (1)].StrVal); ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1359 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1360 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1361 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    CurGV->setSection(*(yyvsp[(1) - (1)].StrVal));
    delete (yyvsp[(1) - (1)].StrVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1366 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if ((yyvsp[(2) - (2)].UInt64Val) != 0 && !isPowerOf2_32((yyvsp[(2) - (2)].UInt64Val)))
      GEN_ERROR("Alignment must be a power of two");
    CurGV->setAlignment((yyvsp[(2) - (2)].UInt64Val));
    CHECK_FOR_ERROR
  ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1382 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.TypeVal) = new PATypeHolder(OpaqueType::get());
    CHECK_FOR_ERROR
  ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1386 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.TypeVal) = new PATypeHolder((yyvsp[(1) - (1)].PrimType));
    CHECK_FOR_ERROR
  ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1390 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                             // Pointer type?
    if (*(yyvsp[(1) - (3)].TypeVal) == Type::LabelTy)
      GEN_ERROR("Cannot form a pointer to a basic block");
    (yyval.TypeVal) = new PATypeHolder(HandleUpRefs(PointerType::get(*(yyvsp[(1) - (3)].TypeVal), (yyvsp[(2) - (3)].UIntVal))));
    delete (yyvsp[(1) - (3)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1397 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {            // Named types are also simple types...
    const Type* tmp = getTypeVal((yyvsp[(1) - (1)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.TypeVal) = new PATypeHolder(tmp);
  ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1402 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                   // Type UpReference
    if ((yyvsp[(2) - (2)].UInt64Val) > (uint64_t)~0U) GEN_ERROR("Value out of range");
    OpaqueType *OT = OpaqueType::get();        // Use temporary placeholder
    UpRefs.push_back(UpRefRecord((unsigned)(yyvsp[(2) - (2)].UInt64Val), OT));  // Add to vector...
    (yyval.TypeVal) = new PATypeHolder(OT);
    UR_OUT("New Upreference!\n");
    CHECK_FOR_ERROR
  ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1410 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    // Allow but ignore attributes on function types; this permits auto-upgrade.
    // FIXME: remove in LLVM 3.0.
    const Type *RetTy = *(yyvsp[(1) - (5)].TypeVal);
    if (!FunctionType::isValidReturnType(RetTy))
      GEN_ERROR("Invalid result type for LLVM function");
      
    std::vector<const Type*> Params;
    TypeWithAttrsList::iterator I = (yyvsp[(3) - (5)].TypeWithAttrsList)->begin(), E = (yyvsp[(3) - (5)].TypeWithAttrsList)->end();
    for (; I != E; ++I ) {
      const Type *Ty = I->Ty->get();
      Params.push_back(Ty);
    }

    bool isVarArg = Params.size() && Params.back() == Type::VoidTy;
    if (isVarArg) Params.pop_back();

    for (unsigned i = 0; i != Params.size(); ++i)
      if (!(Params[i]->isFirstClassType() || isa<OpaqueType>(Params[i])))
        GEN_ERROR("Function arguments must be value types!");

    CHECK_FOR_ERROR

    FunctionType *FT = FunctionType::get(RetTy, Params, isVarArg);
    delete (yyvsp[(3) - (5)].TypeWithAttrsList);   // Delete the argument list
    delete (yyvsp[(1) - (5)].TypeVal);   // Delete the return type handle
    (yyval.TypeVal) = new PATypeHolder(HandleUpRefs(FT)); 
    CHECK_FOR_ERROR
  ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1439 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    // Allow but ignore attributes on function types; this permits auto-upgrade.
    // FIXME: remove in LLVM 3.0.
    std::vector<const Type*> Params;
    TypeWithAttrsList::iterator I = (yyvsp[(3) - (5)].TypeWithAttrsList)->begin(), E = (yyvsp[(3) - (5)].TypeWithAttrsList)->end();
    for ( ; I != E; ++I ) {
      const Type* Ty = I->Ty->get();
      Params.push_back(Ty);
    }

    bool isVarArg = Params.size() && Params.back() == Type::VoidTy;
    if (isVarArg) Params.pop_back();

    for (unsigned i = 0; i != Params.size(); ++i)
      if (!(Params[i]->isFirstClassType() || isa<OpaqueType>(Params[i])))
        GEN_ERROR("Function arguments must be value types!");

    CHECK_FOR_ERROR

    FunctionType *FT = FunctionType::get((yyvsp[(1) - (5)].PrimType), Params, isVarArg);
    delete (yyvsp[(3) - (5)].TypeWithAttrsList);      // Delete the argument list
    (yyval.TypeVal) = new PATypeHolder(HandleUpRefs(FT)); 
    CHECK_FOR_ERROR
  ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1464 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {          // Sized array type?
    (yyval.TypeVal) = new PATypeHolder(HandleUpRefs(ArrayType::get(*(yyvsp[(4) - (5)].TypeVal), (yyvsp[(2) - (5)].UInt64Val))));
    delete (yyvsp[(4) - (5)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1469 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {          // Vector type?
     const llvm::Type* ElemTy = (yyvsp[(4) - (5)].TypeVal)->get();
     if ((unsigned)(yyvsp[(2) - (5)].UInt64Val) != (yyvsp[(2) - (5)].UInt64Val))
        GEN_ERROR("Unsigned result not equal to signed result");
     if (!ElemTy->isFloatingPoint() && !ElemTy->isInteger())
        GEN_ERROR("Element type of a VectorType must be primitive");
     (yyval.TypeVal) = new PATypeHolder(HandleUpRefs(VectorType::get(*(yyvsp[(4) - (5)].TypeVal), (unsigned)(yyvsp[(2) - (5)].UInt64Val))));
     delete (yyvsp[(4) - (5)].TypeVal);
     CHECK_FOR_ERROR
  ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1479 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                        // Structure type?
    std::vector<const Type*> Elements;
    for (std::list<llvm::PATypeHolder>::iterator I = (yyvsp[(2) - (3)].TypeList)->begin(),
           E = (yyvsp[(2) - (3)].TypeList)->end(); I != E; ++I)
      Elements.push_back(*I);

    (yyval.TypeVal) = new PATypeHolder(HandleUpRefs(StructType::get(Elements)));
    delete (yyvsp[(2) - (3)].TypeList);
    CHECK_FOR_ERROR
  ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1489 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                                  // Empty structure type?
    (yyval.TypeVal) = new PATypeHolder(StructType::get(std::vector<const Type*>()));
    CHECK_FOR_ERROR
  ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1493 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    std::vector<const Type*> Elements;
    for (std::list<llvm::PATypeHolder>::iterator I = (yyvsp[(3) - (5)].TypeList)->begin(),
           E = (yyvsp[(3) - (5)].TypeList)->end(); I != E; ++I)
      Elements.push_back(*I);

    (yyval.TypeVal) = new PATypeHolder(HandleUpRefs(StructType::get(Elements, true)));
    delete (yyvsp[(3) - (5)].TypeList);
    CHECK_FOR_ERROR
  ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1503 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                         // Empty structure type?
    (yyval.TypeVal) = new PATypeHolder(StructType::get(std::vector<const Type*>(), true));
    CHECK_FOR_ERROR
  ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1510 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    // Allow but ignore attributes on function types; this permits auto-upgrade.
    // FIXME: remove in LLVM 3.0.
    (yyval.TypeWithAttrs).Ty = (yyvsp[(1) - (2)].TypeVal); 
    (yyval.TypeWithAttrs).Attrs = ParamAttr::None;
  ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1519 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (1)].TypeVal))->getDescription());
    if (!(*(yyvsp[(1) - (1)].TypeVal))->isFirstClassType() && !isa<StructType>((yyvsp[(1) - (1)].TypeVal)->get()))
      GEN_ERROR("LLVM functions cannot return aggregate types");
    (yyval.TypeVal) = (yyvsp[(1) - (1)].TypeVal);
  ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1526 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.TypeVal) = new PATypeHolder(Type::VoidTy);
  ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1531 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.TypeWithAttrsList) = new TypeWithAttrsList();
    (yyval.TypeWithAttrsList)->push_back((yyvsp[(1) - (1)].TypeWithAttrs));
    CHECK_FOR_ERROR
  ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1536 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    ((yyval.TypeWithAttrsList)=(yyvsp[(1) - (3)].TypeWithAttrsList))->push_back((yyvsp[(3) - (3)].TypeWithAttrs));
    CHECK_FOR_ERROR
  ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1544 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.TypeWithAttrsList)=(yyvsp[(1) - (3)].TypeWithAttrsList);
    TypeWithAttrs TWA; TWA.Attrs = ParamAttr::None;
    TWA.Ty = new PATypeHolder(Type::VoidTy);
    (yyval.TypeWithAttrsList)->push_back(TWA);
    CHECK_FOR_ERROR
  ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1551 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.TypeWithAttrsList) = new TypeWithAttrsList;
    TypeWithAttrs TWA; TWA.Attrs = ParamAttr::None;
    TWA.Ty = new PATypeHolder(Type::VoidTy);
    (yyval.TypeWithAttrsList)->push_back(TWA);
    CHECK_FOR_ERROR
  ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1558 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.TypeWithAttrsList) = new TypeWithAttrsList();
    CHECK_FOR_ERROR
  ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1566 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.TypeList) = new std::list<PATypeHolder>();
    (yyval.TypeList)->push_back(*(yyvsp[(1) - (1)].TypeVal)); 
    delete (yyvsp[(1) - (1)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1572 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    ((yyval.TypeList)=(yyvsp[(1) - (3)].TypeList))->push_back(*(yyvsp[(3) - (3)].TypeVal)); 
    delete (yyvsp[(3) - (3)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1584 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { // Nonempty unsized arr
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (4)].TypeVal))->getDescription());
    const ArrayType *ATy = dyn_cast<ArrayType>((yyvsp[(1) - (4)].TypeVal)->get());
    if (ATy == 0)
      GEN_ERROR("Cannot make array constant with type: '" + 
                     (*(yyvsp[(1) - (4)].TypeVal))->getDescription() + "'");
    const Type *ETy = ATy->getElementType();
    uint64_t NumElements = ATy->getNumElements();

    // Verify that we have the correct size...
    if (NumElements != uint64_t(-1) && NumElements != (yyvsp[(3) - (4)].ConstVector)->size())
      GEN_ERROR("Type mismatch: constant sized array initialized with " +
                     utostr((yyvsp[(3) - (4)].ConstVector)->size()) +  " arguments, but has size of " + 
                     utostr(NumElements) + "");

    // Verify all elements are correct type!
    for (unsigned i = 0; i < (yyvsp[(3) - (4)].ConstVector)->size(); i++) {
      if (ETy != (*(yyvsp[(3) - (4)].ConstVector))[i]->getType())
        GEN_ERROR("Element #" + utostr(i) + " is not of type '" + 
                       ETy->getDescription() +"' as required!\nIt is of type '"+
                       (*(yyvsp[(3) - (4)].ConstVector))[i]->getType()->getDescription() + "'.");
    }

    (yyval.ConstVal) = ConstantArray::get(ATy, *(yyvsp[(3) - (4)].ConstVector));
    delete (yyvsp[(1) - (4)].TypeVal); delete (yyvsp[(3) - (4)].ConstVector);
    CHECK_FOR_ERROR
  ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1612 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (3)].TypeVal))->getDescription());
    const ArrayType *ATy = dyn_cast<ArrayType>((yyvsp[(1) - (3)].TypeVal)->get());
    if (ATy == 0)
      GEN_ERROR("Cannot make array constant with type: '" + 
                     (*(yyvsp[(1) - (3)].TypeVal))->getDescription() + "'");

    uint64_t NumElements = ATy->getNumElements();
    if (NumElements != uint64_t(-1) && NumElements != 0) 
      GEN_ERROR("Type mismatch: constant sized array initialized with 0"
                     " arguments, but has size of " + utostr(NumElements) +"");
    (yyval.ConstVal) = ConstantArray::get(ATy, std::vector<Constant*>());
    delete (yyvsp[(1) - (3)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1628 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (3)].TypeVal))->getDescription());
    const ArrayType *ATy = dyn_cast<ArrayType>((yyvsp[(1) - (3)].TypeVal)->get());
    if (ATy == 0)
      GEN_ERROR("Cannot make array constant with type: '" + 
                     (*(yyvsp[(1) - (3)].TypeVal))->getDescription() + "'");

    uint64_t NumElements = ATy->getNumElements();
    const Type *ETy = ATy->getElementType();
    if (NumElements != uint64_t(-1) && NumElements != (yyvsp[(3) - (3)].StrVal)->length())
      GEN_ERROR("Can't build string constant of size " + 
                     utostr((yyvsp[(3) - (3)].StrVal)->length()) +
                     " when array has size " + utostr(NumElements) + "");
    std::vector<Constant*> Vals;
    if (ETy == Type::Int8Ty) {
      for (uint64_t i = 0; i < (yyvsp[(3) - (3)].StrVal)->length(); ++i)
        Vals.push_back(ConstantInt::get(ETy, (*(yyvsp[(3) - (3)].StrVal))[i]));
    } else {
      delete (yyvsp[(3) - (3)].StrVal);
      GEN_ERROR("Cannot build string arrays of non byte sized elements");
    }
    delete (yyvsp[(3) - (3)].StrVal);
    (yyval.ConstVal) = ConstantArray::get(ATy, Vals);
    delete (yyvsp[(1) - (3)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1655 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { // Nonempty unsized arr
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (4)].TypeVal))->getDescription());
    const VectorType *PTy = dyn_cast<VectorType>((yyvsp[(1) - (4)].TypeVal)->get());
    if (PTy == 0)
      GEN_ERROR("Cannot make packed constant with type: '" + 
                     (*(yyvsp[(1) - (4)].TypeVal))->getDescription() + "'");
    const Type *ETy = PTy->getElementType();
    unsigned NumElements = PTy->getNumElements();

    // Verify that we have the correct size...
    if (NumElements != unsigned(-1) && NumElements != (unsigned)(yyvsp[(3) - (4)].ConstVector)->size())
      GEN_ERROR("Type mismatch: constant sized packed initialized with " +
                     utostr((yyvsp[(3) - (4)].ConstVector)->size()) +  " arguments, but has size of " + 
                     utostr(NumElements) + "");

    // Verify all elements are correct type!
    for (unsigned i = 0; i < (yyvsp[(3) - (4)].ConstVector)->size(); i++) {
      if (ETy != (*(yyvsp[(3) - (4)].ConstVector))[i]->getType())
        GEN_ERROR("Element #" + utostr(i) + " is not of type '" + 
           ETy->getDescription() +"' as required!\nIt is of type '"+
           (*(yyvsp[(3) - (4)].ConstVector))[i]->getType()->getDescription() + "'.");
    }

    (yyval.ConstVal) = ConstantVector::get(PTy, *(yyvsp[(3) - (4)].ConstVector));
    delete (yyvsp[(1) - (4)].TypeVal); delete (yyvsp[(3) - (4)].ConstVector);
    CHECK_FOR_ERROR
  ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1683 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    const StructType *STy = dyn_cast<StructType>((yyvsp[(1) - (4)].TypeVal)->get());
    if (STy == 0)
      GEN_ERROR("Cannot make struct constant with type: '" + 
                     (*(yyvsp[(1) - (4)].TypeVal))->getDescription() + "'");

    if ((yyvsp[(3) - (4)].ConstVector)->size() != STy->getNumContainedTypes())
      GEN_ERROR("Illegal number of initializers for structure type");

    // Check to ensure that constants are compatible with the type initializer!
    for (unsigned i = 0, e = (yyvsp[(3) - (4)].ConstVector)->size(); i != e; ++i)
      if ((*(yyvsp[(3) - (4)].ConstVector))[i]->getType() != STy->getElementType(i))
        GEN_ERROR("Expected type '" +
                       STy->getElementType(i)->getDescription() +
                       "' for element #" + utostr(i) +
                       " of structure initializer");

    // Check to ensure that Type is not packed
    if (STy->isPacked())
      GEN_ERROR("Unpacked Initializer to vector type '" +
                STy->getDescription() + "'");

    (yyval.ConstVal) = ConstantStruct::get(STy, *(yyvsp[(3) - (4)].ConstVector));
    delete (yyvsp[(1) - (4)].TypeVal); delete (yyvsp[(3) - (4)].ConstVector);
    CHECK_FOR_ERROR
  ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1709 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (3)].TypeVal))->getDescription());
    const StructType *STy = dyn_cast<StructType>((yyvsp[(1) - (3)].TypeVal)->get());
    if (STy == 0)
      GEN_ERROR("Cannot make struct constant with type: '" + 
                     (*(yyvsp[(1) - (3)].TypeVal))->getDescription() + "'");

    if (STy->getNumContainedTypes() != 0)
      GEN_ERROR("Illegal number of initializers for structure type");

    // Check to ensure that Type is not packed
    if (STy->isPacked())
      GEN_ERROR("Unpacked Initializer to vector type '" +
                STy->getDescription() + "'");

    (yyval.ConstVal) = ConstantStruct::get(STy, std::vector<Constant*>());
    delete (yyvsp[(1) - (3)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1729 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    const StructType *STy = dyn_cast<StructType>((yyvsp[(1) - (6)].TypeVal)->get());
    if (STy == 0)
      GEN_ERROR("Cannot make struct constant with type: '" + 
                     (*(yyvsp[(1) - (6)].TypeVal))->getDescription() + "'");

    if ((yyvsp[(4) - (6)].ConstVector)->size() != STy->getNumContainedTypes())
      GEN_ERROR("Illegal number of initializers for structure type");

    // Check to ensure that constants are compatible with the type initializer!
    for (unsigned i = 0, e = (yyvsp[(4) - (6)].ConstVector)->size(); i != e; ++i)
      if ((*(yyvsp[(4) - (6)].ConstVector))[i]->getType() != STy->getElementType(i))
        GEN_ERROR("Expected type '" +
                       STy->getElementType(i)->getDescription() +
                       "' for element #" + utostr(i) +
                       " of structure initializer");

    // Check to ensure that Type is packed
    if (!STy->isPacked())
      GEN_ERROR("Vector initializer to non-vector type '" + 
                STy->getDescription() + "'");

    (yyval.ConstVal) = ConstantStruct::get(STy, *(yyvsp[(4) - (6)].ConstVector));
    delete (yyvsp[(1) - (6)].TypeVal); delete (yyvsp[(4) - (6)].ConstVector);
    CHECK_FOR_ERROR
  ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1755 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (5)].TypeVal))->getDescription());
    const StructType *STy = dyn_cast<StructType>((yyvsp[(1) - (5)].TypeVal)->get());
    if (STy == 0)
      GEN_ERROR("Cannot make struct constant with type: '" + 
                     (*(yyvsp[(1) - (5)].TypeVal))->getDescription() + "'");

    if (STy->getNumContainedTypes() != 0)
      GEN_ERROR("Illegal number of initializers for structure type");

    // Check to ensure that Type is packed
    if (!STy->isPacked())
      GEN_ERROR("Vector initializer to non-vector type '" + 
                STy->getDescription() + "'");

    (yyval.ConstVal) = ConstantStruct::get(STy, std::vector<Constant*>());
    delete (yyvsp[(1) - (5)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1775 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (2)].TypeVal))->getDescription());
    const PointerType *PTy = dyn_cast<PointerType>((yyvsp[(1) - (2)].TypeVal)->get());
    if (PTy == 0)
      GEN_ERROR("Cannot make null pointer constant with type: '" + 
                     (*(yyvsp[(1) - (2)].TypeVal))->getDescription() + "'");

    (yyval.ConstVal) = ConstantPointerNull::get(PTy);
    delete (yyvsp[(1) - (2)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1787 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (2)].TypeVal))->getDescription());
    (yyval.ConstVal) = UndefValue::get((yyvsp[(1) - (2)].TypeVal)->get());
    delete (yyvsp[(1) - (2)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1794 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (2)].TypeVal))->getDescription());
    const PointerType *Ty = dyn_cast<PointerType>((yyvsp[(1) - (2)].TypeVal)->get());
    if (Ty == 0)
      GEN_ERROR("Global const reference must be a pointer type " + (*(yyvsp[(1) - (2)].TypeVal))->getDescription());

    // ConstExprs can exist in the body of a function, thus creating
    // GlobalValues whenever they refer to a variable.  Because we are in
    // the context of a function, getExistingVal will search the functions
    // symbol table instead of the module symbol table for the global symbol,
    // which throws things all off.  To get around this, we just tell
    // getExistingVal that we are at global scope here.
    //
    Function *SavedCurFn = CurFun.CurrentFunction;
    CurFun.CurrentFunction = 0;

    Value *V = getExistingVal(Ty, (yyvsp[(2) - (2)].ValIDVal));
    CHECK_FOR_ERROR

    CurFun.CurrentFunction = SavedCurFn;

    // If this is an initializer for a constant pointer, which is referencing a
    // (currently) undefined variable, create a stub now that shall be replaced
    // in the future with the right type of variable.
    //
    if (V == 0) {
      assert(isa<PointerType>(Ty) && "Globals may only be used as pointers!");
      const PointerType *PT = cast<PointerType>(Ty);

      // First check to see if the forward references value is already created!
      PerModuleInfo::GlobalRefsType::iterator I =
        CurModule.GlobalRefs.find(std::make_pair(PT, (yyvsp[(2) - (2)].ValIDVal)));
    
      if (I != CurModule.GlobalRefs.end()) {
        V = I->second;             // Placeholder already exists, use it...
        (yyvsp[(2) - (2)].ValIDVal).destroy();
      } else {
        std::string Name;
        if ((yyvsp[(2) - (2)].ValIDVal).Type == ValID::GlobalName)
          Name = (yyvsp[(2) - (2)].ValIDVal).getName();
        else if ((yyvsp[(2) - (2)].ValIDVal).Type != ValID::GlobalID)
          GEN_ERROR("Invalid reference to global");

        // Create the forward referenced global.
        GlobalValue *GV;
        if (const FunctionType *FTy = 
                 dyn_cast<FunctionType>(PT->getElementType())) {
          GV = Function::Create(FTy, GlobalValue::ExternalWeakLinkage, Name,
                                CurModule.CurrentModule);
        } else {
          GV = new GlobalVariable(PT->getElementType(), false,
                                  GlobalValue::ExternalWeakLinkage, 0,
                                  Name, CurModule.CurrentModule);
        }

        // Keep track of the fact that we have a forward ref to recycle it
        CurModule.GlobalRefs.insert(std::make_pair(std::make_pair(PT, (yyvsp[(2) - (2)].ValIDVal)), GV));
        V = GV;
      }
    }

    (yyval.ConstVal) = cast<GlobalValue>(V);
    delete (yyvsp[(1) - (2)].TypeVal);            // Free the type handle
    CHECK_FOR_ERROR
  ;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1860 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (2)].TypeVal))->getDescription());
    if ((yyvsp[(1) - (2)].TypeVal)->get() != (yyvsp[(2) - (2)].ConstVal)->getType())
      GEN_ERROR("Mismatched types for constant expression: " + 
        (*(yyvsp[(1) - (2)].TypeVal))->getDescription() + " and " + (yyvsp[(2) - (2)].ConstVal)->getType()->getDescription());
    (yyval.ConstVal) = (yyvsp[(2) - (2)].ConstVal);
    delete (yyvsp[(1) - (2)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1870 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (2)].TypeVal))->getDescription());
    const Type *Ty = (yyvsp[(1) - (2)].TypeVal)->get();
    if (isa<FunctionType>(Ty) || Ty == Type::LabelTy || isa<OpaqueType>(Ty))
      GEN_ERROR("Cannot create a null initialized value of this type");
    (yyval.ConstVal) = Constant::getNullValue(Ty);
    delete (yyvsp[(1) - (2)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1880 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {      // integral constants
    if (!ConstantInt::isValueValidForType((yyvsp[(1) - (2)].PrimType), (yyvsp[(2) - (2)].SInt64Val)))
      GEN_ERROR("Constant value doesn't fit in type");
    (yyval.ConstVal) = ConstantInt::get((yyvsp[(1) - (2)].PrimType), (yyvsp[(2) - (2)].SInt64Val), true);
    CHECK_FOR_ERROR
  ;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1886 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {      // arbitrary precision integer constants
    uint32_t BitWidth = cast<IntegerType>((yyvsp[(1) - (2)].PrimType))->getBitWidth();
    if ((yyvsp[(2) - (2)].APIntVal)->getBitWidth() > BitWidth) {
      GEN_ERROR("Constant value does not fit in type");
    }
    (yyvsp[(2) - (2)].APIntVal)->sextOrTrunc(BitWidth);
    (yyval.ConstVal) = ConstantInt::get(*(yyvsp[(2) - (2)].APIntVal));
    delete (yyvsp[(2) - (2)].APIntVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1896 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {      // integral constants
    if (!ConstantInt::isValueValidForType((yyvsp[(1) - (2)].PrimType), (yyvsp[(2) - (2)].UInt64Val)))
      GEN_ERROR("Constant value doesn't fit in type");
    (yyval.ConstVal) = ConstantInt::get((yyvsp[(1) - (2)].PrimType), (yyvsp[(2) - (2)].UInt64Val), false);
    CHECK_FOR_ERROR
  ;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1902 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {      // arbitrary precision integer constants
    uint32_t BitWidth = cast<IntegerType>((yyvsp[(1) - (2)].PrimType))->getBitWidth();
    if ((yyvsp[(2) - (2)].APIntVal)->getBitWidth() > BitWidth) {
      GEN_ERROR("Constant value does not fit in type");
    } 
    (yyvsp[(2) - (2)].APIntVal)->zextOrTrunc(BitWidth);
    (yyval.ConstVal) = ConstantInt::get(*(yyvsp[(2) - (2)].APIntVal));
    delete (yyvsp[(2) - (2)].APIntVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1912 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                      // Boolean constants
    if (cast<IntegerType>((yyvsp[(1) - (2)].PrimType))->getBitWidth() != 1)
      GEN_ERROR("Constant true must have type i1");
    (yyval.ConstVal) = ConstantInt::getTrue();
    CHECK_FOR_ERROR
  ;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1918 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                     // Boolean constants
    if (cast<IntegerType>((yyvsp[(1) - (2)].PrimType))->getBitWidth() != 1)
      GEN_ERROR("Constant false must have type i1");
    (yyval.ConstVal) = ConstantInt::getFalse();
    CHECK_FOR_ERROR
  ;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1924 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                   // Floating point constants
    if (!ConstantFP::isValueValidForType((yyvsp[(1) - (2)].PrimType), *(yyvsp[(2) - (2)].FPVal)))
      GEN_ERROR("Floating point constant invalid for type");
    // Lexer has no type info, so builds all float and double FP constants 
    // as double.  Fix this here.  Long double is done right.
    if (&(yyvsp[(2) - (2)].FPVal)->getSemantics()==&APFloat::IEEEdouble && (yyvsp[(1) - (2)].PrimType)==Type::FloatTy)
      (yyvsp[(2) - (2)].FPVal)->convert(APFloat::IEEEsingle, APFloat::rmNearestTiesToEven);
    (yyval.ConstVal) = ConstantFP::get(*(yyvsp[(2) - (2)].FPVal));
    delete (yyvsp[(2) - (2)].FPVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1937 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(5) - (6)].TypeVal))->getDescription());
    Constant *Val = (yyvsp[(3) - (6)].ConstVal);
    const Type *DestTy = (yyvsp[(5) - (6)].TypeVal)->get();
    if (!CastInst::castIsValid((yyvsp[(1) - (6)].CastOpVal), (yyvsp[(3) - (6)].ConstVal), DestTy))
      GEN_ERROR("invalid cast opcode for cast from '" +
                Val->getType()->getDescription() + "' to '" +
                DestTy->getDescription() + "'"); 
    (yyval.ConstVal) = ConstantExpr::getCast((yyvsp[(1) - (6)].CastOpVal), (yyvsp[(3) - (6)].ConstVal), DestTy);
    delete (yyvsp[(5) - (6)].TypeVal);
  ;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1949 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!isa<PointerType>((yyvsp[(3) - (5)].ConstVal)->getType()))
      GEN_ERROR("GetElementPtr requires a pointer operand");

    const Type *IdxTy =
      GetElementPtrInst::getIndexedType((yyvsp[(3) - (5)].ConstVal)->getType(), (yyvsp[(4) - (5)].ValueList)->begin(), (yyvsp[(4) - (5)].ValueList)->end());
    if (!IdxTy)
      GEN_ERROR("Index list invalid for constant getelementptr");

    SmallVector<Constant*, 8> IdxVec;
    for (unsigned i = 0, e = (yyvsp[(4) - (5)].ValueList)->size(); i != e; ++i)
      if (Constant *C = dyn_cast<Constant>((*(yyvsp[(4) - (5)].ValueList))[i]))
        IdxVec.push_back(C);
      else
        GEN_ERROR("Indices to constant getelementptr must be constants");

    delete (yyvsp[(4) - (5)].ValueList);

    (yyval.ConstVal) = ConstantExpr::getGetElementPtr((yyvsp[(3) - (5)].ConstVal), &IdxVec[0], IdxVec.size());
    CHECK_FOR_ERROR
  ;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1970 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if ((yyvsp[(3) - (8)].ConstVal)->getType() != Type::Int1Ty)
      GEN_ERROR("Select condition must be of boolean type");
    if ((yyvsp[(5) - (8)].ConstVal)->getType() != (yyvsp[(7) - (8)].ConstVal)->getType())
      GEN_ERROR("Select operand types must match");
    (yyval.ConstVal) = ConstantExpr::getSelect((yyvsp[(3) - (8)].ConstVal), (yyvsp[(5) - (8)].ConstVal), (yyvsp[(7) - (8)].ConstVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1978 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if ((yyvsp[(3) - (6)].ConstVal)->getType() != (yyvsp[(5) - (6)].ConstVal)->getType())
      GEN_ERROR("Binary operator types must match");
    CHECK_FOR_ERROR;
    (yyval.ConstVal) = ConstantExpr::get((yyvsp[(1) - (6)].BinaryOpVal), (yyvsp[(3) - (6)].ConstVal), (yyvsp[(5) - (6)].ConstVal));
  ;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1984 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if ((yyvsp[(3) - (6)].ConstVal)->getType() != (yyvsp[(5) - (6)].ConstVal)->getType())
      GEN_ERROR("Logical operator types must match");
    if (!(yyvsp[(3) - (6)].ConstVal)->getType()->isInteger()) {
      if (!isa<VectorType>((yyvsp[(3) - (6)].ConstVal)->getType()) || 
          !cast<VectorType>((yyvsp[(3) - (6)].ConstVal)->getType())->getElementType()->isInteger())
        GEN_ERROR("Logical operator requires integral operands");
    }
    (yyval.ConstVal) = ConstantExpr::get((yyvsp[(1) - (6)].BinaryOpVal), (yyvsp[(3) - (6)].ConstVal), (yyvsp[(5) - (6)].ConstVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 1995 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if ((yyvsp[(4) - (7)].ConstVal)->getType() != (yyvsp[(6) - (7)].ConstVal)->getType())
      GEN_ERROR("icmp operand types must match");
    (yyval.ConstVal) = ConstantExpr::getICmp((yyvsp[(2) - (7)].IPredicate), (yyvsp[(4) - (7)].ConstVal), (yyvsp[(6) - (7)].ConstVal));
  ;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 2000 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if ((yyvsp[(4) - (7)].ConstVal)->getType() != (yyvsp[(6) - (7)].ConstVal)->getType())
      GEN_ERROR("fcmp operand types must match");
    (yyval.ConstVal) = ConstantExpr::getFCmp((yyvsp[(2) - (7)].FPredicate), (yyvsp[(4) - (7)].ConstVal), (yyvsp[(6) - (7)].ConstVal));
  ;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 2005 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if ((yyvsp[(4) - (7)].ConstVal)->getType() != (yyvsp[(6) - (7)].ConstVal)->getType())
      GEN_ERROR("vicmp operand types must match");
    (yyval.ConstVal) = ConstantExpr::getVICmp((yyvsp[(2) - (7)].IPredicate), (yyvsp[(4) - (7)].ConstVal), (yyvsp[(6) - (7)].ConstVal));
  ;}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 2010 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if ((yyvsp[(4) - (7)].ConstVal)->getType() != (yyvsp[(6) - (7)].ConstVal)->getType())
      GEN_ERROR("vfcmp operand types must match");
    (yyval.ConstVal) = ConstantExpr::getVFCmp((yyvsp[(2) - (7)].FPredicate), (yyvsp[(4) - (7)].ConstVal), (yyvsp[(6) - (7)].ConstVal));
  ;}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 2015 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!ExtractElementInst::isValidOperands((yyvsp[(3) - (6)].ConstVal), (yyvsp[(5) - (6)].ConstVal)))
      GEN_ERROR("Invalid extractelement operands");
    (yyval.ConstVal) = ConstantExpr::getExtractElement((yyvsp[(3) - (6)].ConstVal), (yyvsp[(5) - (6)].ConstVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 2021 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!InsertElementInst::isValidOperands((yyvsp[(3) - (8)].ConstVal), (yyvsp[(5) - (8)].ConstVal), (yyvsp[(7) - (8)].ConstVal)))
      GEN_ERROR("Invalid insertelement operands");
    (yyval.ConstVal) = ConstantExpr::getInsertElement((yyvsp[(3) - (8)].ConstVal), (yyvsp[(5) - (8)].ConstVal), (yyvsp[(7) - (8)].ConstVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 2027 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!ShuffleVectorInst::isValidOperands((yyvsp[(3) - (8)].ConstVal), (yyvsp[(5) - (8)].ConstVal), (yyvsp[(7) - (8)].ConstVal)))
      GEN_ERROR("Invalid shufflevector operands");
    (yyval.ConstVal) = ConstantExpr::getShuffleVector((yyvsp[(3) - (8)].ConstVal), (yyvsp[(5) - (8)].ConstVal), (yyvsp[(7) - (8)].ConstVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 2033 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!isa<StructType>((yyvsp[(3) - (5)].ConstVal)->getType()) && !isa<ArrayType>((yyvsp[(3) - (5)].ConstVal)->getType()))
      GEN_ERROR("ExtractValue requires an aggregate operand");

    (yyval.ConstVal) = ConstantExpr::getExtractValue((yyvsp[(3) - (5)].ConstVal), &(*(yyvsp[(4) - (5)].ConstantList))[0], (yyvsp[(4) - (5)].ConstantList)->size());
    delete (yyvsp[(4) - (5)].ConstantList);
    CHECK_FOR_ERROR
  ;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 2041 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!isa<StructType>((yyvsp[(3) - (7)].ConstVal)->getType()) && !isa<ArrayType>((yyvsp[(3) - (7)].ConstVal)->getType()))
      GEN_ERROR("InsertValue requires an aggregate operand");

    (yyval.ConstVal) = ConstantExpr::getInsertValue((yyvsp[(3) - (7)].ConstVal), (yyvsp[(5) - (7)].ConstVal), &(*(yyvsp[(6) - (7)].ConstantList))[0], (yyvsp[(6) - (7)].ConstantList)->size());
    delete (yyvsp[(6) - (7)].ConstantList);
    CHECK_FOR_ERROR
  ;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 2052 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    ((yyval.ConstVector) = (yyvsp[(1) - (3)].ConstVector))->push_back((yyvsp[(3) - (3)].ConstVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 2056 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ConstVector) = new std::vector<Constant*>();
    (yyval.ConstVector)->push_back((yyvsp[(1) - (1)].ConstVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 2064 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.BoolVal) = false; ;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 2064 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.BoolVal) = true; ;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 2067 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.BoolVal) = true; ;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 2067 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.BoolVal) = false; ;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 2070 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    const Type* VTy = (yyvsp[(1) - (2)].TypeVal)->get();
    Value *V = getVal(VTy, (yyvsp[(2) - (2)].ValIDVal));
    CHECK_FOR_ERROR
    GlobalValue* Aliasee = dyn_cast<GlobalValue>(V);
    if (!Aliasee)
      GEN_ERROR("Aliases can be created only to global values");

    (yyval.ConstVal) = Aliasee;
    CHECK_FOR_ERROR
    delete (yyvsp[(1) - (2)].TypeVal);
   ;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 2082 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    Constant *Val = (yyvsp[(3) - (6)].ConstVal);
    const Type *DestTy = (yyvsp[(5) - (6)].TypeVal)->get();
    if (!CastInst::castIsValid((yyvsp[(1) - (6)].CastOpVal), (yyvsp[(3) - (6)].ConstVal), DestTy))
      GEN_ERROR("invalid cast opcode for cast from '" +
                Val->getType()->getDescription() + "' to '" +
                DestTy->getDescription() + "'");
    
    (yyval.ConstVal) = ConstantExpr::getCast((yyvsp[(1) - (6)].CastOpVal), (yyvsp[(3) - (6)].ConstVal), DestTy);
    CHECK_FOR_ERROR
    delete (yyvsp[(5) - (6)].TypeVal);
   ;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 2103 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ModuleVal) = ParserResult = CurModule.CurrentModule;
    CurModule.ModuleDone();
    CHECK_FOR_ERROR;
  ;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 2108 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ModuleVal) = ParserResult = CurModule.CurrentModule;
    CurModule.ModuleDone();
    CHECK_FOR_ERROR;
  ;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 2121 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { CurFun.isDeclare = false; ;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 2121 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    CurFun.FunctionDone();
    CHECK_FOR_ERROR
  ;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 2125 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { CurFun.isDeclare = true; ;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 2125 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    CHECK_FOR_ERROR
  ;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 2128 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    CHECK_FOR_ERROR
  ;}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 2131 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(3) - (3)].TypeVal))->getDescription());
    // Eagerly resolve types.  This is not an optimization, this is a
    // requirement that is due to the fact that we could have this:
    //
    // %list = type { %list * }
    // %list = type { %list * }    ; repeated type decl
    //
    // If types are not resolved eagerly, then the two types will not be
    // determined to be the same type!
    //
    ResolveTypeTo((yyvsp[(1) - (3)].StrVal), *(yyvsp[(3) - (3)].TypeVal));

    if (!setTypeName(*(yyvsp[(3) - (3)].TypeVal), (yyvsp[(1) - (3)].StrVal)) && !(yyvsp[(1) - (3)].StrVal)) {
      CHECK_FOR_ERROR
      // If this is a named type that is not a redefinition, add it to the slot
      // table.
      CurModule.Types.push_back(*(yyvsp[(3) - (3)].TypeVal));
    }

    delete (yyvsp[(3) - (3)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 2155 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    ResolveTypeTo((yyvsp[(1) - (3)].StrVal), (yyvsp[(3) - (3)].PrimType));

    if (!setTypeName((yyvsp[(3) - (3)].PrimType), (yyvsp[(1) - (3)].StrVal)) && !(yyvsp[(1) - (3)].StrVal)) {
      CHECK_FOR_ERROR
      // If this is a named type that is not a redefinition, add it to the slot
      // table.
      CurModule.Types.push_back((yyvsp[(3) - (3)].PrimType));
    }
    CHECK_FOR_ERROR
  ;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 2167 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { 
    /* "Externally Visible" Linkage */
    if ((yyvsp[(5) - (6)].ConstVal) == 0) 
      GEN_ERROR("Global value initializer is not a constant");
    CurGV = ParseGlobalVariable((yyvsp[(1) - (6)].StrVal), GlobalValue::ExternalLinkage,
                                (yyvsp[(2) - (6)].Visibility), (yyvsp[(4) - (6)].BoolVal), (yyvsp[(5) - (6)].ConstVal)->getType(), (yyvsp[(5) - (6)].ConstVal), (yyvsp[(3) - (6)].BoolVal), (yyvsp[(6) - (6)].UIntVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 2174 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    CurGV = 0;
  ;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 2178 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if ((yyvsp[(6) - (7)].ConstVal) == 0) 
      GEN_ERROR("Global value initializer is not a constant");
    CurGV = ParseGlobalVariable((yyvsp[(1) - (7)].StrVal), (yyvsp[(2) - (7)].Linkage), (yyvsp[(3) - (7)].Visibility), (yyvsp[(5) - (7)].BoolVal), (yyvsp[(6) - (7)].ConstVal)->getType(), (yyvsp[(6) - (7)].ConstVal), (yyvsp[(4) - (7)].BoolVal), (yyvsp[(7) - (7)].UIntVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 2183 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    CurGV = 0;
  ;}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 2187 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(6) - (7)].TypeVal))->getDescription());
    CurGV = ParseGlobalVariable((yyvsp[(1) - (7)].StrVal), (yyvsp[(2) - (7)].Linkage), (yyvsp[(3) - (7)].Visibility), (yyvsp[(5) - (7)].BoolVal), *(yyvsp[(6) - (7)].TypeVal), 0, (yyvsp[(4) - (7)].BoolVal), (yyvsp[(7) - (7)].UIntVal));
    CHECK_FOR_ERROR
    delete (yyvsp[(6) - (7)].TypeVal);
  ;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 2193 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    CurGV = 0;
    CHECK_FOR_ERROR
  ;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 2197 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    std::string Name;
    if ((yyvsp[(1) - (5)].StrVal)) {
      Name = *(yyvsp[(1) - (5)].StrVal);
      delete (yyvsp[(1) - (5)].StrVal);
    }
    if (Name.empty())
      GEN_ERROR("Alias name cannot be empty");
    
    Constant* Aliasee = (yyvsp[(5) - (5)].ConstVal);
    if (Aliasee == 0)
      GEN_ERROR(std::string("Invalid aliasee for alias: ") + Name);

    GlobalAlias* GA = new GlobalAlias(Aliasee->getType(), (yyvsp[(4) - (5)].Linkage), Name, Aliasee,
                                      CurModule.CurrentModule);
    GA->setVisibility((yyvsp[(2) - (5)].Visibility));
    InsertValue(GA, CurModule.Values);
    
    
    // If there was a forward reference of this alias, resolve it now.
    
    ValID ID;
    if (!Name.empty())
      ID = ValID::createGlobalName(Name);
    else
      ID = ValID::createGlobalID(CurModule.Values.size()-1);
    
    if (GlobalValue *FWGV =
          CurModule.GetForwardRefForGlobal(GA->getType(), ID)) {
      // Replace uses of the fwdref with the actual alias.
      FWGV->replaceAllUsesWith(GA);
      if (GlobalVariable *GV = dyn_cast<GlobalVariable>(FWGV))
        GV->eraseFromParent();
      else
        cast<Function>(FWGV)->eraseFromParent();
    }
    ID.destroy();
    
    CHECK_FOR_ERROR
  ;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 2237 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { 
    CHECK_FOR_ERROR
  ;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 2240 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    CHECK_FOR_ERROR
  ;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 2246 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
  const std::string &AsmSoFar = CurModule.CurrentModule->getModuleInlineAsm();
  if (AsmSoFar.empty())
    CurModule.CurrentModule->setModuleInlineAsm(*(yyvsp[(1) - (1)].StrVal));
  else
    CurModule.CurrentModule->setModuleInlineAsm(AsmSoFar+"\n"+*(yyvsp[(1) - (1)].StrVal));
  delete (yyvsp[(1) - (1)].StrVal);
  CHECK_FOR_ERROR
;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 2256 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    CurModule.CurrentModule->setTargetTriple(*(yyvsp[(3) - (3)].StrVal));
    delete (yyvsp[(3) - (3)].StrVal);
  ;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 2260 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    CurModule.CurrentModule->setDataLayout(*(yyvsp[(3) - (3)].StrVal));
    delete (yyvsp[(3) - (3)].StrVal);
  ;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 2267 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
          CurModule.CurrentModule->addLibrary(*(yyvsp[(3) - (3)].StrVal));
          delete (yyvsp[(3) - (3)].StrVal);
          CHECK_FOR_ERROR
        ;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 2272 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
          CurModule.CurrentModule->addLibrary(*(yyvsp[(1) - (1)].StrVal));
          delete (yyvsp[(1) - (1)].StrVal);
          CHECK_FOR_ERROR
        ;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 2277 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
          CHECK_FOR_ERROR
        ;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 2286 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(3) - (5)].TypeVal))->getDescription());
    if (!(*(yyvsp[(3) - (5)].TypeVal))->isFirstClassType())
      GEN_ERROR("Argument types must be first-class");
    ArgListEntry E; E.Attrs = (yyvsp[(4) - (5)].ParamAttrs); E.Ty = (yyvsp[(3) - (5)].TypeVal); E.Name = (yyvsp[(5) - (5)].StrVal);
    (yyval.ArgList) = (yyvsp[(1) - (5)].ArgList);
    (yyvsp[(1) - (5)].ArgList)->push_back(E);
    CHECK_FOR_ERROR
  ;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 2296 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (3)].TypeVal))->getDescription());
    if (!(*(yyvsp[(1) - (3)].TypeVal))->isFirstClassType())
      GEN_ERROR("Argument types must be first-class");
    ArgListEntry E; E.Attrs = (yyvsp[(2) - (3)].ParamAttrs); E.Ty = (yyvsp[(1) - (3)].TypeVal); E.Name = (yyvsp[(3) - (3)].StrVal);
    (yyval.ArgList) = new ArgListType;
    (yyval.ArgList)->push_back(E);
    CHECK_FOR_ERROR
  ;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 2307 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ArgList) = (yyvsp[(1) - (1)].ArgList);
    CHECK_FOR_ERROR
  ;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 2311 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ArgList) = (yyvsp[(1) - (3)].ArgList);
    struct ArgListEntry E;
    E.Ty = new PATypeHolder(Type::VoidTy);
    E.Name = 0;
    E.Attrs = ParamAttr::None;
    (yyval.ArgList)->push_back(E);
    CHECK_FOR_ERROR
  ;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 2320 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ArgList) = new ArgListType;
    struct ArgListEntry E;
    E.Ty = new PATypeHolder(Type::VoidTy);
    E.Name = 0;
    E.Attrs = ParamAttr::None;
    (yyval.ArgList)->push_back(E);
    CHECK_FOR_ERROR
  ;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 2329 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ArgList) = 0;
    CHECK_FOR_ERROR
  ;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 2335 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
  std::string FunctionName(*(yyvsp[(3) - (11)].StrVal));
  delete (yyvsp[(3) - (11)].StrVal);  // Free strdup'd memory!
  
  // Check the function result for abstractness if this is a define. We should
  // have no abstract types at this point
  if (!CurFun.isDeclare && CurModule.TypeIsUnresolved((yyvsp[(2) - (11)].TypeVal)))
    GEN_ERROR("Reference to abstract result: "+ (yyvsp[(2) - (11)].TypeVal)->get()->getDescription());

  if (!FunctionType::isValidReturnType(*(yyvsp[(2) - (11)].TypeVal)))
    GEN_ERROR("Invalid result type for LLVM function");
    
  std::vector<const Type*> ParamTypeList;
  SmallVector<ParamAttrsWithIndex, 8> Attrs;
  if ((yyvsp[(7) - (11)].ParamAttrs) != ParamAttr::None)
    Attrs.push_back(ParamAttrsWithIndex::get(0, (yyvsp[(7) - (11)].ParamAttrs)));
  if ((yyvsp[(5) - (11)].ArgList)) {   // If there are arguments...
    unsigned index = 1;
    for (ArgListType::iterator I = (yyvsp[(5) - (11)].ArgList)->begin(); I != (yyvsp[(5) - (11)].ArgList)->end(); ++I, ++index) {
      const Type* Ty = I->Ty->get();
      if (!CurFun.isDeclare && CurModule.TypeIsUnresolved(I->Ty))
        GEN_ERROR("Reference to abstract argument: " + Ty->getDescription());
      ParamTypeList.push_back(Ty);
      if (Ty != Type::VoidTy && I->Attrs != ParamAttr::None)
        Attrs.push_back(ParamAttrsWithIndex::get(index, I->Attrs));
    }
  }

  bool isVarArg = ParamTypeList.size() && ParamTypeList.back() == Type::VoidTy;
  if (isVarArg) ParamTypeList.pop_back();

  PAListPtr PAL;
  if (!Attrs.empty())
    PAL = PAListPtr::get(Attrs.begin(), Attrs.end());

  FunctionType *FT = FunctionType::get(*(yyvsp[(2) - (11)].TypeVal), ParamTypeList, isVarArg);
  const PointerType *PFT = PointerType::getUnqual(FT);
  delete (yyvsp[(2) - (11)].TypeVal);

  ValID ID;
  if (!FunctionName.empty()) {
    ID = ValID::createGlobalName((char*)FunctionName.c_str());
  } else {
    ID = ValID::createGlobalID(CurModule.Values.size());
  }

  Function *Fn = 0;
  // See if this function was forward referenced.  If so, recycle the object.
  if (GlobalValue *FWRef = CurModule.GetForwardRefForGlobal(PFT, ID)) {
    // Move the function to the end of the list, from whereever it was 
    // previously inserted.
    Fn = cast<Function>(FWRef);
    assert(Fn->getParamAttrs().isEmpty() &&
           "Forward reference has parameter attributes!");
    CurModule.CurrentModule->getFunctionList().remove(Fn);
    CurModule.CurrentModule->getFunctionList().push_back(Fn);
  } else if (!FunctionName.empty() &&     // Merge with an earlier prototype?
             (Fn = CurModule.CurrentModule->getFunction(FunctionName))) {
    if (Fn->getFunctionType() != FT ) {
      // The existing function doesn't have the same type. This is an overload
      // error.
      GEN_ERROR("Overload of function '" + FunctionName + "' not permitted.");
    } else if (Fn->getParamAttrs() != PAL) {
      // The existing function doesn't have the same parameter attributes.
      // This is an overload error.
      GEN_ERROR("Overload of function '" + FunctionName + "' not permitted.");
    } else if (!CurFun.isDeclare && !Fn->isDeclaration()) {
      // Neither the existing or the current function is a declaration and they
      // have the same name and same type. Clearly this is a redefinition.
      GEN_ERROR("Redefinition of function '" + FunctionName + "'");
    } else if (Fn->isDeclaration()) {
      // Make sure to strip off any argument names so we can't get conflicts.
      for (Function::arg_iterator AI = Fn->arg_begin(), AE = Fn->arg_end();
           AI != AE; ++AI)
        AI->setName("");
    }
  } else  {  // Not already defined?
    Fn = Function::Create(FT, GlobalValue::ExternalWeakLinkage, FunctionName,
                          CurModule.CurrentModule);
    InsertValue(Fn, CurModule.Values);
  }

  CurFun.FunctionStart(Fn);

  if (CurFun.isDeclare) {
    // If we have declaration, always overwrite linkage.  This will allow us to
    // correctly handle cases, when pointer to function is passed as argument to
    // another function.
    Fn->setLinkage(CurFun.Linkage);
    Fn->setVisibility(CurFun.Visibility);
  }
  Fn->setCallingConv((yyvsp[(1) - (11)].UIntVal));
  Fn->setParamAttrs(PAL);
  Fn->setAlignment((yyvsp[(9) - (11)].UIntVal));
  if ((yyvsp[(8) - (11)].StrVal)) {
    Fn->setSection(*(yyvsp[(8) - (11)].StrVal));
    delete (yyvsp[(8) - (11)].StrVal);
  }
  if ((yyvsp[(10) - (11)].StrVal)) {
    Fn->setGC((yyvsp[(10) - (11)].StrVal)->c_str());
    delete (yyvsp[(10) - (11)].StrVal);
  }
  if ((yyvsp[(11) - (11)].FunctionNotes)) {
    Fn->setNotes((yyvsp[(11) - (11)].FunctionNotes));
  }

  // Add all of the arguments we parsed to the function...
  if ((yyvsp[(5) - (11)].ArgList)) {                     // Is null if empty...
    if (isVarArg) {  // Nuke the last entry
      assert((yyvsp[(5) - (11)].ArgList)->back().Ty->get() == Type::VoidTy && (yyvsp[(5) - (11)].ArgList)->back().Name == 0 &&
             "Not a varargs marker!");
      delete (yyvsp[(5) - (11)].ArgList)->back().Ty;
      (yyvsp[(5) - (11)].ArgList)->pop_back();  // Delete the last entry
    }
    Function::arg_iterator ArgIt = Fn->arg_begin();
    Function::arg_iterator ArgEnd = Fn->arg_end();
    unsigned Idx = 1;
    for (ArgListType::iterator I = (yyvsp[(5) - (11)].ArgList)->begin(); 
         I != (yyvsp[(5) - (11)].ArgList)->end() && ArgIt != ArgEnd; ++I, ++ArgIt) {
      delete I->Ty;                          // Delete the typeholder...
      setValueName(ArgIt, I->Name);       // Insert arg into symtab...
      CHECK_FOR_ERROR
      InsertValue(ArgIt);
      Idx++;
    }

    delete (yyvsp[(5) - (11)].ArgList);                     // We're now done with the argument list
  }
  CHECK_FOR_ERROR
;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 2468 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
  (yyval.FunctionVal) = CurFun.CurrentFunction;

  // Make sure that we keep track of the linkage type even if there was a
  // previous "declare".
  (yyval.FunctionVal)->setLinkage((yyvsp[(1) - (4)].Linkage));
  (yyval.FunctionVal)->setVisibility((yyvsp[(2) - (4)].Visibility));
;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 2479 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
  (yyval.FunctionVal) = (yyvsp[(1) - (2)].FunctionVal);
  CHECK_FOR_ERROR
;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 2484 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    CurFun.CurrentFunction->setLinkage((yyvsp[(1) - (3)].Linkage));
    CurFun.CurrentFunction->setVisibility((yyvsp[(2) - (3)].Visibility));
    (yyval.FunctionVal) = CurFun.CurrentFunction;
    CurFun.FunctionDone();
    CHECK_FOR_ERROR
  ;}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 2496 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.BoolVal) = false;
    CHECK_FOR_ERROR
  ;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 2500 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.BoolVal) = true;
    CHECK_FOR_ERROR
  ;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 2505 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {    // A reference to a direct constant
    (yyval.ValIDVal) = ValID::create((yyvsp[(1) - (1)].SInt64Val));
    CHECK_FOR_ERROR
  ;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 2509 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ValIDVal) = ValID::create((yyvsp[(1) - (1)].UInt64Val));
    CHECK_FOR_ERROR
  ;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 2513 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {      // arbitrary precision integer constants
    (yyval.ValIDVal) = ValID::create(*(yyvsp[(1) - (1)].APIntVal), true);
    delete (yyvsp[(1) - (1)].APIntVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 2518 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {      // arbitrary precision integer constants
    (yyval.ValIDVal) = ValID::create(*(yyvsp[(1) - (1)].APIntVal), false);
    delete (yyvsp[(1) - (1)].APIntVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 2523 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                     // Perhaps it's an FP constant?
    (yyval.ValIDVal) = ValID::create((yyvsp[(1) - (1)].FPVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 2527 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ValIDVal) = ValID::create(ConstantInt::getTrue());
    CHECK_FOR_ERROR
  ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 2531 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ValIDVal) = ValID::create(ConstantInt::getFalse());
    CHECK_FOR_ERROR
  ;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 2535 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ValIDVal) = ValID::createNull();
    CHECK_FOR_ERROR
  ;}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 2539 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ValIDVal) = ValID::createUndef();
    CHECK_FOR_ERROR
  ;}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 2543 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {     // A vector zero constant.
    (yyval.ValIDVal) = ValID::createZeroInit();
    CHECK_FOR_ERROR
  ;}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 2547 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { // Nonempty unsized packed vector
    const Type *ETy = (*(yyvsp[(2) - (3)].ConstVector))[0]->getType();
    unsigned NumElements = (yyvsp[(2) - (3)].ConstVector)->size(); 

    if (!ETy->isInteger() && !ETy->isFloatingPoint())
      GEN_ERROR("Invalid vector element type: " + ETy->getDescription());
    
    VectorType* pt = VectorType::get(ETy, NumElements);
    PATypeHolder* PTy = new PATypeHolder(HandleUpRefs(pt));
    
    // Verify all elements are correct type!
    for (unsigned i = 0; i < (yyvsp[(2) - (3)].ConstVector)->size(); i++) {
      if (ETy != (*(yyvsp[(2) - (3)].ConstVector))[i]->getType())
        GEN_ERROR("Element #" + utostr(i) + " is not of type '" + 
                     ETy->getDescription() +"' as required!\nIt is of type '" +
                     (*(yyvsp[(2) - (3)].ConstVector))[i]->getType()->getDescription() + "'.");
    }

    (yyval.ValIDVal) = ValID::create(ConstantVector::get(pt, *(yyvsp[(2) - (3)].ConstVector)));
    delete PTy; delete (yyvsp[(2) - (3)].ConstVector);
    CHECK_FOR_ERROR
  ;}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 2569 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { // Nonempty unsized arr
    const Type *ETy = (*(yyvsp[(2) - (3)].ConstVector))[0]->getType();
    uint64_t NumElements = (yyvsp[(2) - (3)].ConstVector)->size(); 

    if (!ETy->isFirstClassType())
      GEN_ERROR("Invalid array element type: " + ETy->getDescription());

    ArrayType *ATy = ArrayType::get(ETy, NumElements);
    PATypeHolder* PTy = new PATypeHolder(HandleUpRefs(ATy));

    // Verify all elements are correct type!
    for (unsigned i = 0; i < (yyvsp[(2) - (3)].ConstVector)->size(); i++) {
      if (ETy != (*(yyvsp[(2) - (3)].ConstVector))[i]->getType())
        GEN_ERROR("Element #" + utostr(i) + " is not of type '" + 
                       ETy->getDescription() +"' as required!\nIt is of type '"+
                       (*(yyvsp[(2) - (3)].ConstVector))[i]->getType()->getDescription() + "'.");
    }

    (yyval.ValIDVal) = ValID::create(ConstantArray::get(ATy, *(yyvsp[(2) - (3)].ConstVector)));
    delete PTy; delete (yyvsp[(2) - (3)].ConstVector);
    CHECK_FOR_ERROR
  ;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 2591 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    // Use undef instead of an array because it's inconvenient to determine
    // the element type at this point, there being no elements to examine.
    (yyval.ValIDVal) = ValID::createUndef();
    CHECK_FOR_ERROR
  ;}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 2597 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    uint64_t NumElements = (yyvsp[(2) - (2)].StrVal)->length();
    const Type *ETy = Type::Int8Ty;

    ArrayType *ATy = ArrayType::get(ETy, NumElements);

    std::vector<Constant*> Vals;
    for (unsigned i = 0; i < (yyvsp[(2) - (2)].StrVal)->length(); ++i)
      Vals.push_back(ConstantInt::get(ETy, (*(yyvsp[(2) - (2)].StrVal))[i]));
    delete (yyvsp[(2) - (2)].StrVal);
    (yyval.ValIDVal) = ValID::create(ConstantArray::get(ATy, Vals));
    CHECK_FOR_ERROR
  ;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 2610 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    std::vector<const Type*> Elements((yyvsp[(2) - (3)].ConstVector)->size());
    for (unsigned i = 0, e = (yyvsp[(2) - (3)].ConstVector)->size(); i != e; ++i)
      Elements[i] = (*(yyvsp[(2) - (3)].ConstVector))[i]->getType();

    const StructType *STy = StructType::get(Elements);
    PATypeHolder* PTy = new PATypeHolder(HandleUpRefs(STy));

    (yyval.ValIDVal) = ValID::create(ConstantStruct::get(STy, *(yyvsp[(2) - (3)].ConstVector)));
    delete PTy; delete (yyvsp[(2) - (3)].ConstVector);
    CHECK_FOR_ERROR
  ;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 2622 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    const StructType *STy = StructType::get(std::vector<const Type*>());
    (yyval.ValIDVal) = ValID::create(ConstantStruct::get(STy, std::vector<Constant*>()));
    CHECK_FOR_ERROR
  ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2627 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    std::vector<const Type*> Elements((yyvsp[(3) - (5)].ConstVector)->size());
    for (unsigned i = 0, e = (yyvsp[(3) - (5)].ConstVector)->size(); i != e; ++i)
      Elements[i] = (*(yyvsp[(3) - (5)].ConstVector))[i]->getType();

    const StructType *STy = StructType::get(Elements, /*isPacked=*/true);
    PATypeHolder* PTy = new PATypeHolder(HandleUpRefs(STy));

    (yyval.ValIDVal) = ValID::create(ConstantStruct::get(STy, *(yyvsp[(3) - (5)].ConstVector)));
    delete PTy; delete (yyvsp[(3) - (5)].ConstVector);
    CHECK_FOR_ERROR
  ;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 2639 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    const StructType *STy = StructType::get(std::vector<const Type*>(),
                                            /*isPacked=*/true);
    (yyval.ValIDVal) = ValID::create(ConstantStruct::get(STy, std::vector<Constant*>()));
    CHECK_FOR_ERROR
  ;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 2645 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ValIDVal) = ValID::create((yyvsp[(1) - (1)].ConstVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 2649 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ValIDVal) = ValID::createInlineAsm(*(yyvsp[(3) - (5)].StrVal), *(yyvsp[(5) - (5)].StrVal), (yyvsp[(2) - (5)].BoolVal));
    delete (yyvsp[(3) - (5)].StrVal);
    delete (yyvsp[(5) - (5)].StrVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2659 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {  // Is it an integer reference...?
    (yyval.ValIDVal) = ValID::createLocalID((yyvsp[(1) - (1)].UIntVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 2663 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ValIDVal) = ValID::createGlobalID((yyvsp[(1) - (1)].UIntVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 2667 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                   // Is it a named reference...?
    (yyval.ValIDVal) = ValID::createLocalName(*(yyvsp[(1) - (1)].StrVal));
    delete (yyvsp[(1) - (1)].StrVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2672 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                   // Is it a named reference...?
    (yyval.ValIDVal) = ValID::createGlobalName(*(yyvsp[(1) - (1)].StrVal));
    delete (yyvsp[(1) - (1)].StrVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2685 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (2)].TypeVal))->getDescription());
    (yyval.ValueVal) = getVal(*(yyvsp[(1) - (2)].TypeVal), (yyvsp[(2) - (2)].ValIDVal)); 
    delete (yyvsp[(1) - (2)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 2694 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ValueList) = new std::vector<Value *>();
    (yyval.ValueList)->push_back((yyvsp[(1) - (1)].ValueVal)); 
    CHECK_FOR_ERROR
  ;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 2699 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    ((yyval.ValueList)=(yyvsp[(1) - (3)].ValueList))->push_back((yyvsp[(3) - (3)].ValueVal)); 
    CHECK_FOR_ERROR
  ;}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 2704 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.FunctionVal) = (yyvsp[(1) - (2)].FunctionVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 2708 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { // Do not allow functions with 0 basic blocks   
    (yyval.FunctionVal) = (yyvsp[(1) - (2)].FunctionVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 2717 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    setValueName((yyvsp[(3) - (3)].TermInstVal), (yyvsp[(2) - (3)].StrVal));
    CHECK_FOR_ERROR
    InsertValue((yyvsp[(3) - (3)].TermInstVal));
    (yyvsp[(1) - (3)].BasicBlockVal)->getInstList().push_back((yyvsp[(3) - (3)].TermInstVal));
    (yyval.BasicBlockVal) = (yyvsp[(1) - (3)].BasicBlockVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 2726 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
  CHECK_FOR_ERROR
  int ValNum = InsertValue((yyvsp[(3) - (3)].TermInstVal));
  if (ValNum != (int)(yyvsp[(2) - (3)].UIntVal))
    GEN_ERROR("Result value number %" + utostr((yyvsp[(2) - (3)].UIntVal)) +
              " is incorrect, expected %" + utostr((unsigned)ValNum));
  
  (yyvsp[(1) - (3)].BasicBlockVal)->getInstList().push_back((yyvsp[(3) - (3)].TermInstVal));
  (yyval.BasicBlockVal) = (yyvsp[(1) - (3)].BasicBlockVal);
  CHECK_FOR_ERROR
;}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 2739 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (CastInst *CI1 = dyn_cast<CastInst>((yyvsp[(2) - (2)].InstVal)))
      if (CastInst *CI2 = dyn_cast<CastInst>(CI1->getOperand(0)))
        if (CI2->getParent() == 0)
          (yyvsp[(1) - (2)].BasicBlockVal)->getInstList().push_back(CI2);
    (yyvsp[(1) - (2)].BasicBlockVal)->getInstList().push_back((yyvsp[(2) - (2)].InstVal));
    (yyval.BasicBlockVal) = (yyvsp[(1) - (2)].BasicBlockVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2748 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {          // Empty space between instruction lists
    (yyval.BasicBlockVal) = defineBBVal(ValID::createLocalID(CurFun.NextValNum));
    CHECK_FOR_ERROR
  ;}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 2752 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {             // Labelled (named) basic block
    (yyval.BasicBlockVal) = defineBBVal(ValID::createLocalName(*(yyvsp[(1) - (1)].StrVal)));
    delete (yyvsp[(1) - (1)].StrVal);
    CHECK_FOR_ERROR

  ;}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 2760 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { // Return with a result...
    ValueList &VL = *(yyvsp[(2) - (2)].ValueList);
    assert(!VL.empty() && "Invalid ret operands!");
    const Type *ReturnType = CurFun.CurrentFunction->getReturnType();
    if (VL.size() > 1 ||
        (isa<StructType>(ReturnType) &&
         (VL.empty() || VL[0]->getType() != ReturnType))) {
      Value *RV = UndefValue::get(ReturnType);
      for (unsigned i = 0, e = VL.size(); i != e; ++i) {
        Instruction *I = InsertValueInst::Create(RV, VL[i], i, "mrv");
        ((yyvsp[(-1) - (2)].BasicBlockVal))->getInstList().push_back(I);
        RV = I;
      }
      (yyval.TermInstVal) = ReturnInst::Create(RV);
    } else {
      (yyval.TermInstVal) = ReturnInst::Create(VL[0]);
    }
    delete (yyvsp[(2) - (2)].ValueList);
    CHECK_FOR_ERROR
  ;}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 2780 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                                    // Return with no result...
    (yyval.TermInstVal) = ReturnInst::Create();
    CHECK_FOR_ERROR
  ;}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 2784 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {                           // Unconditional Branch...
    BasicBlock* tmpBB = getBBVal((yyvsp[(3) - (3)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.TermInstVal) = BranchInst::Create(tmpBB);
  ;}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 2789 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {  
    if (cast<IntegerType>((yyvsp[(2) - (9)].PrimType))->getBitWidth() != 1)
      GEN_ERROR("Branch condition must have type i1");
    BasicBlock* tmpBBA = getBBVal((yyvsp[(6) - (9)].ValIDVal));
    CHECK_FOR_ERROR
    BasicBlock* tmpBBB = getBBVal((yyvsp[(9) - (9)].ValIDVal));
    CHECK_FOR_ERROR
    Value* tmpVal = getVal(Type::Int1Ty, (yyvsp[(3) - (9)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.TermInstVal) = BranchInst::Create(tmpBBA, tmpBBB, tmpVal);
  ;}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 2800 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    Value* tmpVal = getVal((yyvsp[(2) - (9)].PrimType), (yyvsp[(3) - (9)].ValIDVal));
    CHECK_FOR_ERROR
    BasicBlock* tmpBB = getBBVal((yyvsp[(6) - (9)].ValIDVal));
    CHECK_FOR_ERROR
    SwitchInst *S = SwitchInst::Create(tmpVal, tmpBB, (yyvsp[(8) - (9)].JumpTable)->size());
    (yyval.TermInstVal) = S;

    std::vector<std::pair<Constant*,BasicBlock*> >::iterator I = (yyvsp[(8) - (9)].JumpTable)->begin(),
      E = (yyvsp[(8) - (9)].JumpTable)->end();
    for (; I != E; ++I) {
      if (ConstantInt *CI = dyn_cast<ConstantInt>(I->first))
          S->addCase(CI, I->second);
      else
        GEN_ERROR("Switch case is constant, but not a simple integer");
    }
    delete (yyvsp[(8) - (9)].JumpTable);
    CHECK_FOR_ERROR
  ;}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 2819 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    Value* tmpVal = getVal((yyvsp[(2) - (8)].PrimType), (yyvsp[(3) - (8)].ValIDVal));
    CHECK_FOR_ERROR
    BasicBlock* tmpBB = getBBVal((yyvsp[(6) - (8)].ValIDVal));
    CHECK_FOR_ERROR
    SwitchInst *S = SwitchInst::Create(tmpVal, tmpBB, 0);
    (yyval.TermInstVal) = S;
    CHECK_FOR_ERROR
  ;}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2829 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {

    // Handle the short syntax
    const PointerType *PFTy = 0;
    const FunctionType *Ty = 0;
    if (!(PFTy = dyn_cast<PointerType>((yyvsp[(3) - (14)].TypeVal)->get())) ||
        !(Ty = dyn_cast<FunctionType>(PFTy->getElementType()))) {
      // Pull out the types of all of the arguments...
      std::vector<const Type*> ParamTypes;
      ParamList::iterator I = (yyvsp[(6) - (14)].ParamList)->begin(), E = (yyvsp[(6) - (14)].ParamList)->end();
      for (; I != E; ++I) {
        const Type *Ty = I->Val->getType();
        if (Ty == Type::VoidTy)
          GEN_ERROR("Short call syntax cannot be used with varargs");
        ParamTypes.push_back(Ty);
      }
      
      if (!FunctionType::isValidReturnType(*(yyvsp[(3) - (14)].TypeVal)))
        GEN_ERROR("Invalid result type for LLVM function");

      Ty = FunctionType::get((yyvsp[(3) - (14)].TypeVal)->get(), ParamTypes, false);
      PFTy = PointerType::getUnqual(Ty);
    }

    delete (yyvsp[(3) - (14)].TypeVal);

    Value *V = getVal(PFTy, (yyvsp[(4) - (14)].ValIDVal));   // Get the function we're calling...
    CHECK_FOR_ERROR
    BasicBlock *Normal = getBBVal((yyvsp[(11) - (14)].ValIDVal));
    CHECK_FOR_ERROR
    BasicBlock *Except = getBBVal((yyvsp[(14) - (14)].ValIDVal));
    CHECK_FOR_ERROR

    SmallVector<ParamAttrsWithIndex, 8> Attrs;
    if ((yyvsp[(8) - (14)].ParamAttrs) != ParamAttr::None)
      Attrs.push_back(ParamAttrsWithIndex::get(0, (yyvsp[(8) - (14)].ParamAttrs)));

    // Check the arguments
    ValueList Args;
    if ((yyvsp[(6) - (14)].ParamList)->empty()) {                                   // Has no arguments?
      // Make sure no arguments is a good thing!
      if (Ty->getNumParams() != 0)
        GEN_ERROR("No arguments passed to a function that "
                       "expects arguments");
    } else {                                     // Has arguments?
      // Loop through FunctionType's arguments and ensure they are specified
      // correctly!
      FunctionType::param_iterator I = Ty->param_begin();
      FunctionType::param_iterator E = Ty->param_end();
      ParamList::iterator ArgI = (yyvsp[(6) - (14)].ParamList)->begin(), ArgE = (yyvsp[(6) - (14)].ParamList)->end();
      unsigned index = 1;

      for (; ArgI != ArgE && I != E; ++ArgI, ++I, ++index) {
        if (ArgI->Val->getType() != *I)
          GEN_ERROR("Parameter " + ArgI->Val->getName()+ " is not of type '" +
                         (*I)->getDescription() + "'");
        Args.push_back(ArgI->Val);
        if (ArgI->Attrs != ParamAttr::None)
          Attrs.push_back(ParamAttrsWithIndex::get(index, ArgI->Attrs));
      }

      if (Ty->isVarArg()) {
        if (I == E)
          for (; ArgI != ArgE; ++ArgI, ++index) {
            Args.push_back(ArgI->Val); // push the remaining varargs
            if (ArgI->Attrs != ParamAttr::None)
              Attrs.push_back(ParamAttrsWithIndex::get(index, ArgI->Attrs));
          }
      } else if (I != E || ArgI != ArgE)
        GEN_ERROR("Invalid number of parameters detected");
    }

    PAListPtr PAL;
    if (!Attrs.empty())
      PAL = PAListPtr::get(Attrs.begin(), Attrs.end());

    // Create the InvokeInst
    InvokeInst *II = InvokeInst::Create(V, Normal, Except,
                                        Args.begin(), Args.end());
    II->setCallingConv((yyvsp[(2) - (14)].UIntVal));
    II->setParamAttrs(PAL);
    (yyval.TermInstVal) = II;
    delete (yyvsp[(6) - (14)].ParamList);
    CHECK_FOR_ERROR
  ;}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 2914 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.TermInstVal) = new UnwindInst();
    CHECK_FOR_ERROR
  ;}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 2918 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.TermInstVal) = new UnreachableInst();
    CHECK_FOR_ERROR
  ;}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 2925 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.JumpTable) = (yyvsp[(1) - (6)].JumpTable);
    Constant *V = cast<Constant>(getExistingVal((yyvsp[(2) - (6)].PrimType), (yyvsp[(3) - (6)].ValIDVal)));
    CHECK_FOR_ERROR
    if (V == 0)
      GEN_ERROR("May only switch on a constant pool value");

    BasicBlock* tmpBB = getBBVal((yyvsp[(6) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.JumpTable)->push_back(std::make_pair(V, tmpBB));
  ;}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 2936 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.JumpTable) = new std::vector<std::pair<Constant*, BasicBlock*> >();
    Constant *V = cast<Constant>(getExistingVal((yyvsp[(1) - (5)].PrimType), (yyvsp[(2) - (5)].ValIDVal)));
    CHECK_FOR_ERROR

    if (V == 0)
      GEN_ERROR("May only switch on a constant pool value");

    BasicBlock* tmpBB = getBBVal((yyvsp[(5) - (5)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.JumpTable)->push_back(std::make_pair(V, tmpBB)); 
  ;}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 2949 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    // Is this definition named?? if so, assign the name...
    setValueName((yyvsp[(2) - (2)].InstVal), (yyvsp[(1) - (2)].StrVal));
    CHECK_FOR_ERROR
    InsertValue((yyvsp[(2) - (2)].InstVal));
    (yyval.InstVal) = (yyvsp[(2) - (2)].InstVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 2958 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    CHECK_FOR_ERROR
    int ValNum = InsertValue((yyvsp[(2) - (2)].InstVal));
  
    if (ValNum != (int)(yyvsp[(1) - (2)].UIntVal))
      GEN_ERROR("Result value number %" + utostr((yyvsp[(1) - (2)].UIntVal)) +
                " is incorrect, expected %" + utostr((unsigned)ValNum));

    (yyval.InstVal) = (yyvsp[(2) - (2)].InstVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 2971 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {    // Used for PHI nodes
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (6)].TypeVal))->getDescription());
    (yyval.PHIList) = new std::list<std::pair<Value*, BasicBlock*> >();
    Value* tmpVal = getVal(*(yyvsp[(1) - (6)].TypeVal), (yyvsp[(3) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    BasicBlock* tmpBB = getBBVal((yyvsp[(5) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.PHIList)->push_back(std::make_pair(tmpVal, tmpBB));
    delete (yyvsp[(1) - (6)].TypeVal);
  ;}
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 2982 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.PHIList) = (yyvsp[(1) - (7)].PHIList);
    Value* tmpVal = getVal((yyvsp[(1) - (7)].PHIList)->front().first->getType(), (yyvsp[(4) - (7)].ValIDVal));
    CHECK_FOR_ERROR
    BasicBlock* tmpBB = getBBVal((yyvsp[(6) - (7)].ValIDVal));
    CHECK_FOR_ERROR
    (yyvsp[(1) - (7)].PHIList)->push_back(std::make_pair(tmpVal, tmpBB));
  ;}
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 2992 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    // FIXME: Remove trailing OptParamAttrs in LLVM 3.0, it was a mistake in 2.0
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(1) - (4)].TypeVal))->getDescription());
    // Used for call and invoke instructions
    (yyval.ParamList) = new ParamList();
    ParamListEntry E; E.Attrs = (yyvsp[(2) - (4)].ParamAttrs) | (yyvsp[(4) - (4)].ParamAttrs); E.Val = getVal((yyvsp[(1) - (4)].TypeVal)->get(), (yyvsp[(3) - (4)].ValIDVal));
    (yyval.ParamList)->push_back(E);
    delete (yyvsp[(1) - (4)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 3003 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    // FIXME: Remove trailing OptParamAttrs in LLVM 3.0, it was a mistake in 2.0
    // Labels are only valid in ASMs
    (yyval.ParamList) = new ParamList();
    ParamListEntry E; E.Attrs = (yyvsp[(2) - (4)].ParamAttrs) | (yyvsp[(4) - (4)].ParamAttrs); E.Val = getBBVal((yyvsp[(3) - (4)].ValIDVal));
    (yyval.ParamList)->push_back(E);
    CHECK_FOR_ERROR
  ;}
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 3011 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    // FIXME: Remove trailing OptParamAttrs in LLVM 3.0, it was a mistake in 2.0
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(3) - (6)].TypeVal))->getDescription());
    (yyval.ParamList) = (yyvsp[(1) - (6)].ParamList);
    ParamListEntry E; E.Attrs = (yyvsp[(4) - (6)].ParamAttrs) | (yyvsp[(6) - (6)].ParamAttrs); E.Val = getVal((yyvsp[(3) - (6)].TypeVal)->get(), (yyvsp[(5) - (6)].ValIDVal));
    (yyval.ParamList)->push_back(E);
    delete (yyvsp[(3) - (6)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 3021 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    // FIXME: Remove trailing OptParamAttrs in LLVM 3.0, it was a mistake in 2.0
    (yyval.ParamList) = (yyvsp[(1) - (6)].ParamList);
    ParamListEntry E; E.Attrs = (yyvsp[(4) - (6)].ParamAttrs) | (yyvsp[(6) - (6)].ParamAttrs); E.Val = getBBVal((yyvsp[(5) - (6)].ValIDVal));
    (yyval.ParamList)->push_back(E);
    CHECK_FOR_ERROR
  ;}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 3028 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ParamList) = new ParamList(); ;}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 3031 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    { (yyval.ValueList) = new std::vector<Value*>(); ;}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 3032 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ValueList) = (yyvsp[(1) - (3)].ValueList);
    (yyval.ValueList)->push_back((yyvsp[(3) - (3)].ValueVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 3040 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ConstantList) = new std::vector<unsigned>();
    if ((unsigned)(yyvsp[(2) - (2)].UInt64Val) != (yyvsp[(2) - (2)].UInt64Val))
      GEN_ERROR("Index " + utostr((yyvsp[(2) - (2)].UInt64Val)) + " is not valid for insertvalue or extractvalue.");
    (yyval.ConstantList)->push_back((yyvsp[(2) - (2)].UInt64Val));
  ;}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 3046 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.ConstantList) = (yyvsp[(1) - (3)].ConstantList);
    if ((unsigned)(yyvsp[(3) - (3)].UInt64Val) != (yyvsp[(3) - (3)].UInt64Val))
      GEN_ERROR("Index " + utostr((yyvsp[(3) - (3)].UInt64Val)) + " is not valid for insertvalue or extractvalue.");
    (yyval.ConstantList)->push_back((yyvsp[(3) - (3)].UInt64Val));
    CHECK_FOR_ERROR
  ;}
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 3055 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.BoolVal) = true;
    CHECK_FOR_ERROR
  ;}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 3059 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.BoolVal) = false;
    CHECK_FOR_ERROR
  ;}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 3064 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(2) - (5)].TypeVal))->getDescription());
    if (!(*(yyvsp[(2) - (5)].TypeVal))->isInteger() && !(*(yyvsp[(2) - (5)].TypeVal))->isFloatingPoint() && 
        !isa<VectorType>((*(yyvsp[(2) - (5)].TypeVal)).get()))
      GEN_ERROR(
        "Arithmetic operator requires integer, FP, or packed operands");
    Value* val1 = getVal(*(yyvsp[(2) - (5)].TypeVal), (yyvsp[(3) - (5)].ValIDVal)); 
    CHECK_FOR_ERROR
    Value* val2 = getVal(*(yyvsp[(2) - (5)].TypeVal), (yyvsp[(5) - (5)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = BinaryOperator::Create((yyvsp[(1) - (5)].BinaryOpVal), val1, val2);
    if ((yyval.InstVal) == 0)
      GEN_ERROR("binary operator returned null");
    delete (yyvsp[(2) - (5)].TypeVal);
  ;}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 3080 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(2) - (5)].TypeVal))->getDescription());
    if (!(*(yyvsp[(2) - (5)].TypeVal))->isInteger()) {
      if (!isa<VectorType>((yyvsp[(2) - (5)].TypeVal)->get()) ||
          !cast<VectorType>((yyvsp[(2) - (5)].TypeVal)->get())->getElementType()->isInteger())
        GEN_ERROR("Logical operator requires integral operands");
    }
    Value* tmpVal1 = getVal(*(yyvsp[(2) - (5)].TypeVal), (yyvsp[(3) - (5)].ValIDVal));
    CHECK_FOR_ERROR
    Value* tmpVal2 = getVal(*(yyvsp[(2) - (5)].TypeVal), (yyvsp[(5) - (5)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = BinaryOperator::Create((yyvsp[(1) - (5)].BinaryOpVal), tmpVal1, tmpVal2);
    if ((yyval.InstVal) == 0)
      GEN_ERROR("binary operator returned null");
    delete (yyvsp[(2) - (5)].TypeVal);
  ;}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 3097 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(3) - (6)].TypeVal))->getDescription());
    if (isa<VectorType>((*(yyvsp[(3) - (6)].TypeVal)).get()))
      GEN_ERROR("Vector types not supported by icmp instruction");
    Value* tmpVal1 = getVal(*(yyvsp[(3) - (6)].TypeVal), (yyvsp[(4) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    Value* tmpVal2 = getVal(*(yyvsp[(3) - (6)].TypeVal), (yyvsp[(6) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = CmpInst::Create((yyvsp[(1) - (6)].OtherOpVal), (yyvsp[(2) - (6)].IPredicate), tmpVal1, tmpVal2);
    if ((yyval.InstVal) == 0)
      GEN_ERROR("icmp operator returned null");
    delete (yyvsp[(3) - (6)].TypeVal);
  ;}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 3111 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(3) - (6)].TypeVal))->getDescription());
    if (isa<VectorType>((*(yyvsp[(3) - (6)].TypeVal)).get()))
      GEN_ERROR("Vector types not supported by fcmp instruction");
    Value* tmpVal1 = getVal(*(yyvsp[(3) - (6)].TypeVal), (yyvsp[(4) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    Value* tmpVal2 = getVal(*(yyvsp[(3) - (6)].TypeVal), (yyvsp[(6) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = CmpInst::Create((yyvsp[(1) - (6)].OtherOpVal), (yyvsp[(2) - (6)].FPredicate), tmpVal1, tmpVal2);
    if ((yyval.InstVal) == 0)
      GEN_ERROR("fcmp operator returned null");
    delete (yyvsp[(3) - (6)].TypeVal);
  ;}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 3125 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(3) - (6)].TypeVal))->getDescription());
    if (!isa<VectorType>((*(yyvsp[(3) - (6)].TypeVal)).get()))
      GEN_ERROR("Scalar types not supported by vicmp instruction");
    Value* tmpVal1 = getVal(*(yyvsp[(3) - (6)].TypeVal), (yyvsp[(4) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    Value* tmpVal2 = getVal(*(yyvsp[(3) - (6)].TypeVal), (yyvsp[(6) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = CmpInst::Create((yyvsp[(1) - (6)].OtherOpVal), (yyvsp[(2) - (6)].IPredicate), tmpVal1, tmpVal2);
    if ((yyval.InstVal) == 0)
      GEN_ERROR("icmp operator returned null");
    delete (yyvsp[(3) - (6)].TypeVal);
  ;}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 3139 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(3) - (6)].TypeVal))->getDescription());
    if (!isa<VectorType>((*(yyvsp[(3) - (6)].TypeVal)).get()))
      GEN_ERROR("Scalar types not supported by vfcmp instruction");
    Value* tmpVal1 = getVal(*(yyvsp[(3) - (6)].TypeVal), (yyvsp[(4) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    Value* tmpVal2 = getVal(*(yyvsp[(3) - (6)].TypeVal), (yyvsp[(6) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = CmpInst::Create((yyvsp[(1) - (6)].OtherOpVal), (yyvsp[(2) - (6)].FPredicate), tmpVal1, tmpVal2);
    if ((yyval.InstVal) == 0)
      GEN_ERROR("fcmp operator returned null");
    delete (yyvsp[(3) - (6)].TypeVal);
  ;}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 3153 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(4) - (4)].TypeVal))->getDescription());
    Value* Val = (yyvsp[(2) - (4)].ValueVal);
    const Type* DestTy = (yyvsp[(4) - (4)].TypeVal)->get();
    if (!CastInst::castIsValid((yyvsp[(1) - (4)].CastOpVal), Val, DestTy))
      GEN_ERROR("invalid cast opcode for cast from '" +
                Val->getType()->getDescription() + "' to '" +
                DestTy->getDescription() + "'"); 
    (yyval.InstVal) = CastInst::Create((yyvsp[(1) - (4)].CastOpVal), Val, DestTy);
    delete (yyvsp[(4) - (4)].TypeVal);
  ;}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 3165 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if ((yyvsp[(2) - (6)].ValueVal)->getType() != Type::Int1Ty)
      GEN_ERROR("select condition must be boolean");
    if ((yyvsp[(4) - (6)].ValueVal)->getType() != (yyvsp[(6) - (6)].ValueVal)->getType())
      GEN_ERROR("select value types should match");
    (yyval.InstVal) = SelectInst::Create((yyvsp[(2) - (6)].ValueVal), (yyvsp[(4) - (6)].ValueVal), (yyvsp[(6) - (6)].ValueVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 3173 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(4) - (4)].TypeVal))->getDescription());
    (yyval.InstVal) = new VAArgInst((yyvsp[(2) - (4)].ValueVal), *(yyvsp[(4) - (4)].TypeVal));
    delete (yyvsp[(4) - (4)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 3180 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!ExtractElementInst::isValidOperands((yyvsp[(2) - (4)].ValueVal), (yyvsp[(4) - (4)].ValueVal)))
      GEN_ERROR("Invalid extractelement operands");
    (yyval.InstVal) = new ExtractElementInst((yyvsp[(2) - (4)].ValueVal), (yyvsp[(4) - (4)].ValueVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 3186 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!InsertElementInst::isValidOperands((yyvsp[(2) - (6)].ValueVal), (yyvsp[(4) - (6)].ValueVal), (yyvsp[(6) - (6)].ValueVal)))
      GEN_ERROR("Invalid insertelement operands");
    (yyval.InstVal) = InsertElementInst::Create((yyvsp[(2) - (6)].ValueVal), (yyvsp[(4) - (6)].ValueVal), (yyvsp[(6) - (6)].ValueVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 3192 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!ShuffleVectorInst::isValidOperands((yyvsp[(2) - (6)].ValueVal), (yyvsp[(4) - (6)].ValueVal), (yyvsp[(6) - (6)].ValueVal)))
      GEN_ERROR("Invalid shufflevector operands");
    (yyval.InstVal) = new ShuffleVectorInst((yyvsp[(2) - (6)].ValueVal), (yyvsp[(4) - (6)].ValueVal), (yyvsp[(6) - (6)].ValueVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 3198 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    const Type *Ty = (yyvsp[(2) - (2)].PHIList)->front().first->getType();
    if (!Ty->isFirstClassType())
      GEN_ERROR("PHI node operands must be of first class type");
    (yyval.InstVal) = PHINode::Create(Ty);
    ((PHINode*)(yyval.InstVal))->reserveOperandSpace((yyvsp[(2) - (2)].PHIList)->size());
    while ((yyvsp[(2) - (2)].PHIList)->begin() != (yyvsp[(2) - (2)].PHIList)->end()) {
      if ((yyvsp[(2) - (2)].PHIList)->front().first->getType() != Ty) 
        GEN_ERROR("All elements of a PHI node must be of the same type");
      cast<PHINode>((yyval.InstVal))->addIncoming((yyvsp[(2) - (2)].PHIList)->front().first, (yyvsp[(2) - (2)].PHIList)->front().second);
      (yyvsp[(2) - (2)].PHIList)->pop_front();
    }
    delete (yyvsp[(2) - (2)].PHIList);  // Free the list...
    CHECK_FOR_ERROR
  ;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 3214 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {

    // Handle the short syntax
    const PointerType *PFTy = 0;
    const FunctionType *Ty = 0;
    if (!(PFTy = dyn_cast<PointerType>((yyvsp[(3) - (8)].TypeVal)->get())) ||
        !(Ty = dyn_cast<FunctionType>(PFTy->getElementType()))) {
      // Pull out the types of all of the arguments...
      std::vector<const Type*> ParamTypes;
      ParamList::iterator I = (yyvsp[(6) - (8)].ParamList)->begin(), E = (yyvsp[(6) - (8)].ParamList)->end();
      for (; I != E; ++I) {
        const Type *Ty = I->Val->getType();
        if (Ty == Type::VoidTy)
          GEN_ERROR("Short call syntax cannot be used with varargs");
        ParamTypes.push_back(Ty);
      }

      if (!FunctionType::isValidReturnType(*(yyvsp[(3) - (8)].TypeVal)))
        GEN_ERROR("Invalid result type for LLVM function");

      Ty = FunctionType::get((yyvsp[(3) - (8)].TypeVal)->get(), ParamTypes, false);
      PFTy = PointerType::getUnqual(Ty);
    }

    Value *V = getVal(PFTy, (yyvsp[(4) - (8)].ValIDVal));   // Get the function we're calling...
    CHECK_FOR_ERROR

    // Check for call to invalid intrinsic to avoid crashing later.
    if (Function *theF = dyn_cast<Function>(V)) {
      if (theF->hasName() && (theF->getValueName()->getKeyLength() >= 5) &&
          (0 == strncmp(theF->getValueName()->getKeyData(), "llvm.", 5)) &&
          !theF->getIntrinsicID(true))
        GEN_ERROR("Call to invalid LLVM intrinsic function '" +
                  theF->getName() + "'");
    }

    // Set up the ParamAttrs for the function
    SmallVector<ParamAttrsWithIndex, 8> Attrs;
    if ((yyvsp[(8) - (8)].ParamAttrs) != ParamAttr::None)
      Attrs.push_back(ParamAttrsWithIndex::get(0, (yyvsp[(8) - (8)].ParamAttrs)));
    // Check the arguments 
    ValueList Args;
    if ((yyvsp[(6) - (8)].ParamList)->empty()) {                                   // Has no arguments?
      // Make sure no arguments is a good thing!
      if (Ty->getNumParams() != 0)
        GEN_ERROR("No arguments passed to a function that "
                       "expects arguments");
    } else {                                     // Has arguments?
      // Loop through FunctionType's arguments and ensure they are specified
      // correctly.  Also, gather any parameter attributes.
      FunctionType::param_iterator I = Ty->param_begin();
      FunctionType::param_iterator E = Ty->param_end();
      ParamList::iterator ArgI = (yyvsp[(6) - (8)].ParamList)->begin(), ArgE = (yyvsp[(6) - (8)].ParamList)->end();
      unsigned index = 1;

      for (; ArgI != ArgE && I != E; ++ArgI, ++I, ++index) {
        if (ArgI->Val->getType() != *I)
          GEN_ERROR("Parameter " + ArgI->Val->getName()+ " is not of type '" +
                         (*I)->getDescription() + "'");
        Args.push_back(ArgI->Val);
        if (ArgI->Attrs != ParamAttr::None)
          Attrs.push_back(ParamAttrsWithIndex::get(index, ArgI->Attrs));
      }
      if (Ty->isVarArg()) {
        if (I == E)
          for (; ArgI != ArgE; ++ArgI, ++index) {
            Args.push_back(ArgI->Val); // push the remaining varargs
            if (ArgI->Attrs != ParamAttr::None)
              Attrs.push_back(ParamAttrsWithIndex::get(index, ArgI->Attrs));
          }
      } else if (I != E || ArgI != ArgE)
        GEN_ERROR("Invalid number of parameters detected");
    }

    // Finish off the ParamAttrs and check them
    PAListPtr PAL;
    if (!Attrs.empty())
      PAL = PAListPtr::get(Attrs.begin(), Attrs.end());

    // Create the call node
    CallInst *CI = CallInst::Create(V, Args.begin(), Args.end());
    CI->setTailCall((yyvsp[(1) - (8)].BoolVal));
    CI->setCallingConv((yyvsp[(2) - (8)].UIntVal));
    CI->setParamAttrs(PAL);
    (yyval.InstVal) = CI;
    delete (yyvsp[(6) - (8)].ParamList);
    delete (yyvsp[(3) - (8)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 3303 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.InstVal) = (yyvsp[(1) - (1)].InstVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 3308 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.BoolVal) = true;
    CHECK_FOR_ERROR
  ;}
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 3312 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    (yyval.BoolVal) = false;
    CHECK_FOR_ERROR
  ;}
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 3319 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(2) - (3)].TypeVal))->getDescription());
    (yyval.InstVal) = new MallocInst(*(yyvsp[(2) - (3)].TypeVal), 0, (yyvsp[(3) - (3)].UIntVal));
    delete (yyvsp[(2) - (3)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 3326 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(2) - (6)].TypeVal))->getDescription());
    if ((yyvsp[(4) - (6)].PrimType) != Type::Int32Ty)
      GEN_ERROR("Malloc array size is not a 32-bit integer!");
    Value* tmpVal = getVal((yyvsp[(4) - (6)].PrimType), (yyvsp[(5) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = new MallocInst(*(yyvsp[(2) - (6)].TypeVal), tmpVal, (yyvsp[(6) - (6)].UIntVal));
    delete (yyvsp[(2) - (6)].TypeVal);
  ;}
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 3336 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(2) - (3)].TypeVal))->getDescription());
    (yyval.InstVal) = new AllocaInst(*(yyvsp[(2) - (3)].TypeVal), 0, (yyvsp[(3) - (3)].UIntVal));
    delete (yyvsp[(2) - (3)].TypeVal);
    CHECK_FOR_ERROR
  ;}
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 3343 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(2) - (6)].TypeVal))->getDescription());
    if ((yyvsp[(4) - (6)].PrimType) != Type::Int32Ty)
      GEN_ERROR("Alloca array size is not a 32-bit integer!");
    Value* tmpVal = getVal((yyvsp[(4) - (6)].PrimType), (yyvsp[(5) - (6)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = new AllocaInst(*(yyvsp[(2) - (6)].TypeVal), tmpVal, (yyvsp[(6) - (6)].UIntVal));
    delete (yyvsp[(2) - (6)].TypeVal);
  ;}
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 3353 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!isa<PointerType>((yyvsp[(2) - (2)].ValueVal)->getType()))
      GEN_ERROR("Trying to free nonpointer type " + 
                     (yyvsp[(2) - (2)].ValueVal)->getType()->getDescription() + "");
    (yyval.InstVal) = new FreeInst((yyvsp[(2) - (2)].ValueVal));
    CHECK_FOR_ERROR
  ;}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 3361 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(3) - (5)].TypeVal))->getDescription());
    if (!isa<PointerType>((yyvsp[(3) - (5)].TypeVal)->get()))
      GEN_ERROR("Can't load from nonpointer type: " +
                     (*(yyvsp[(3) - (5)].TypeVal))->getDescription());
    if (!cast<PointerType>((yyvsp[(3) - (5)].TypeVal)->get())->getElementType()->isFirstClassType())
      GEN_ERROR("Can't load from pointer of non-first-class type: " +
                     (*(yyvsp[(3) - (5)].TypeVal))->getDescription());
    Value* tmpVal = getVal(*(yyvsp[(3) - (5)].TypeVal), (yyvsp[(4) - (5)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = new LoadInst(tmpVal, "", (yyvsp[(1) - (5)].BoolVal), (yyvsp[(5) - (5)].UIntVal));
    delete (yyvsp[(3) - (5)].TypeVal);
  ;}
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 3375 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(5) - (7)].TypeVal))->getDescription());
    const PointerType *PT = dyn_cast<PointerType>((yyvsp[(5) - (7)].TypeVal)->get());
    if (!PT)
      GEN_ERROR("Can't store to a nonpointer type: " +
                     (*(yyvsp[(5) - (7)].TypeVal))->getDescription());
    const Type *ElTy = PT->getElementType();
    if (ElTy != (yyvsp[(3) - (7)].ValueVal)->getType())
      GEN_ERROR("Can't store '" + (yyvsp[(3) - (7)].ValueVal)->getType()->getDescription() +
                     "' into space of type '" + ElTy->getDescription() + "'");

    Value* tmpVal = getVal(*(yyvsp[(5) - (7)].TypeVal), (yyvsp[(6) - (7)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = new StoreInst((yyvsp[(3) - (7)].ValueVal), tmpVal, (yyvsp[(1) - (7)].BoolVal), (yyvsp[(7) - (7)].UIntVal));
    delete (yyvsp[(5) - (7)].TypeVal);
  ;}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 3392 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(2) - (5)].TypeVal))->getDescription());
    if (!isa<StructType>((yyvsp[(2) - (5)].TypeVal)->get()) && !isa<ArrayType>((yyvsp[(2) - (5)].TypeVal)->get()))
      GEN_ERROR("getresult insn requires an aggregate operand");
    if (!ExtractValueInst::getIndexedType(*(yyvsp[(2) - (5)].TypeVal), (yyvsp[(5) - (5)].UInt64Val)))
      GEN_ERROR("Invalid getresult index for type '" +
                     (*(yyvsp[(2) - (5)].TypeVal))->getDescription()+ "'");

    Value *tmpVal = getVal(*(yyvsp[(2) - (5)].TypeVal), (yyvsp[(3) - (5)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = ExtractValueInst::Create(tmpVal, (yyvsp[(5) - (5)].UInt64Val));
    delete (yyvsp[(2) - (5)].TypeVal);
  ;}
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 3406 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(2) - (4)].TypeVal))->getDescription());
    if (!isa<PointerType>((yyvsp[(2) - (4)].TypeVal)->get()))
      GEN_ERROR("getelementptr insn requires pointer operand");

    if (!GetElementPtrInst::getIndexedType(*(yyvsp[(2) - (4)].TypeVal), (yyvsp[(4) - (4)].ValueList)->begin(), (yyvsp[(4) - (4)].ValueList)->end()))
      GEN_ERROR("Invalid getelementptr indices for type '" +
                     (*(yyvsp[(2) - (4)].TypeVal))->getDescription()+ "'");
    Value* tmpVal = getVal(*(yyvsp[(2) - (4)].TypeVal), (yyvsp[(3) - (4)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = GetElementPtrInst::Create(tmpVal, (yyvsp[(4) - (4)].ValueList)->begin(), (yyvsp[(4) - (4)].ValueList)->end());
    delete (yyvsp[(2) - (4)].TypeVal); 
    delete (yyvsp[(4) - (4)].ValueList);
  ;}
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 3421 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(2) - (4)].TypeVal))->getDescription());
    if (!isa<StructType>((yyvsp[(2) - (4)].TypeVal)->get()) && !isa<ArrayType>((yyvsp[(2) - (4)].TypeVal)->get()))
      GEN_ERROR("extractvalue insn requires an aggregate operand");

    if (!ExtractValueInst::getIndexedType(*(yyvsp[(2) - (4)].TypeVal), (yyvsp[(4) - (4)].ConstantList)->begin(), (yyvsp[(4) - (4)].ConstantList)->end()))
      GEN_ERROR("Invalid extractvalue indices for type '" +
                     (*(yyvsp[(2) - (4)].TypeVal))->getDescription()+ "'");
    Value* tmpVal = getVal(*(yyvsp[(2) - (4)].TypeVal), (yyvsp[(3) - (4)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = ExtractValueInst::Create(tmpVal, (yyvsp[(4) - (4)].ConstantList)->begin(), (yyvsp[(4) - (4)].ConstantList)->end());
    delete (yyvsp[(2) - (4)].TypeVal); 
    delete (yyvsp[(4) - (4)].ConstantList);
  ;}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 3436 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"
    {
    if (!UpRefs.empty())
      GEN_ERROR("Invalid upreference in type: " + (*(yyvsp[(2) - (7)].TypeVal))->getDescription());
    if (!isa<StructType>((yyvsp[(2) - (7)].TypeVal)->get()) && !isa<ArrayType>((yyvsp[(2) - (7)].TypeVal)->get()))
      GEN_ERROR("extractvalue insn requires an aggregate operand");

    if (ExtractValueInst::getIndexedType(*(yyvsp[(2) - (7)].TypeVal), (yyvsp[(7) - (7)].ConstantList)->begin(), (yyvsp[(7) - (7)].ConstantList)->end()) != (yyvsp[(5) - (7)].TypeVal)->get())
      GEN_ERROR("Invalid insertvalue indices for type '" +
                     (*(yyvsp[(2) - (7)].TypeVal))->getDescription()+ "'");
    Value* aggVal = getVal(*(yyvsp[(2) - (7)].TypeVal), (yyvsp[(3) - (7)].ValIDVal));
    Value* tmpVal = getVal(*(yyvsp[(5) - (7)].TypeVal), (yyvsp[(6) - (7)].ValIDVal));
    CHECK_FOR_ERROR
    (yyval.InstVal) = InsertValueInst::Create(aggVal, tmpVal, (yyvsp[(7) - (7)].ConstantList)->begin(), (yyvsp[(7) - (7)].ConstantList)->end());
    delete (yyvsp[(2) - (7)].TypeVal); 
    delete (yyvsp[(5) - (7)].TypeVal);
    delete (yyvsp[(7) - (7)].ConstantList);
  ;}
    break;



/* Line 1455 of yacc.c  */
#line 7401 "llvmAsmParser.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
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
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 3455 "/home/hamayun/workspace/NaSiK/sw/llvm/lib/AsmParser/llvmAsmParser.y"


// common code from the two 'RunVMAsmParser' functions
static Module* RunParser(Module * M) {
  CurModule.CurrentModule = M;
  // Check to make sure the parser succeeded
  if (yyparse()) {
    if (ParserResult)
      delete ParserResult;
    return 0;
  }

  // Emit an error if there are any unresolved types left.
  if (!CurModule.LateResolveTypes.empty()) {
    const ValID &DID = CurModule.LateResolveTypes.begin()->first;
    if (DID.Type == ValID::LocalName) {
      GenerateError("Undefined type remains at eof: '"+DID.getName() + "'");
    } else {
      GenerateError("Undefined type remains at eof: #" + itostr(DID.Num));
    }
    if (ParserResult)
      delete ParserResult;
    return 0;
  }

  // Emit an error if there are any unresolved values left.
  if (!CurModule.LateResolveValues.empty()) {
    Value *V = CurModule.LateResolveValues.back();
    std::map<Value*, std::pair<ValID, int> >::iterator I =
      CurModule.PlaceHolderInfo.find(V);

    if (I != CurModule.PlaceHolderInfo.end()) {
      ValID &DID = I->second.first;
      if (DID.Type == ValID::LocalName) {
        GenerateError("Undefined value remains at eof: "+DID.getName() + "'");
      } else {
        GenerateError("Undefined value remains at eof: #" + itostr(DID.Num));
      }
      if (ParserResult)
        delete ParserResult;
      return 0;
    }
  }

  // Check to make sure that parsing produced a result
  if (!ParserResult)
    return 0;

  // Reset ParserResult variable while saving its value for the result.
  Module *Result = ParserResult;
  ParserResult = 0;

  return Result;
}

void llvm::GenerateError(const std::string &message, int LineNo) {
  if (LineNo == -1) LineNo = LLLgetLineNo();
  // TODO: column number in exception
  if (TheParseError)
    TheParseError->setError(LLLgetFilename(), message, LineNo);
  TriggerError = 1;
}

int yyerror(const char *ErrorMsg) {
  std::string where = LLLgetFilename() + ":" + utostr(LLLgetLineNo()) + ": ";
  std::string errMsg = where + "error: " + std::string(ErrorMsg);
  if (yychar != YYEMPTY && yychar != 0) {
    errMsg += " while reading token: '";
    errMsg += std::string(LLLgetTokenStart(), 
                          LLLgetTokenStart()+LLLgetTokenLength()) + "'";
  }
  GenerateError(errMsg);
  return 0;
}

