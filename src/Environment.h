//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#include <stdio.h>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

class StackFrame
{
  private:
    /// StackFrame maps Variable Declaration to Value
    /// Which are either integer or addresses (also represented using an Integer value)
    std::map<Decl *, int> mVars;
    std::map<Stmt *, int> mExprs;
    /// The current stmt
    Stmt *mPC;

  public:
    StackFrame() : mVars(), mExprs(), mPC(){}

    void bindDecl(Decl *, int);
    bool findDeclVal(Decl *);
    int getDeclVal(Decl *);
    void bindStmt(Stmt *, int);
    bool findStmtVal(Stmt *);
    int getStmtVal(Stmt *);
    void setPC(Stmt *);
    Stmt *getPC();
};

class GlobalValue
{
  private:
    std::map<Decl *, int> mVars;
    std::map<Stmt *, int> mExprs;
  public:
    GlobalValue() : mVars(), mExprs(){}

    void bindDecl(Decl *, int);
    int getDeclVal(Decl *);
    void bindStmt(Stmt *, int);
    int getStmtVal(Stmt *);
};

/// Heap maps address to a value
class Heap {
  private:
    std::map<void *, int64_t> mSpace;
  public:
   void *Malloc(int);
   void Free(void *);
};

class Environment
{
    std::vector<StackFrame> mStack;
    Heap mHeap;
    GlobalValue mGlobal; // 存储全局变量/常量

    const ASTContext &context;

    FunctionDecl *mFree; /// Declarations to the built-in functions
    FunctionDecl *mMalloc;
    FunctionDecl *mInput;
    FunctionDecl *mOutput;

    FunctionDecl *mEntry;

  public:
    /// Get the declarations to the built-in functions
    Environment(const ASTContext &Context) : mStack(), mHeap(), mGlobal(), context(Context),mFree(nullptr), mMalloc(nullptr), mInput(nullptr), mOutput(nullptr), mEntry(nullptr)
    {
        mStack.push_back(StackFrame());
    }

    /// Initialize the Environment
    void init(TranslationUnitDecl *);

    FunctionDecl *getEntry();
    int64_t getStmtVal(Expr *);
    int64_t getDeclVal(Decl *);

    int64_t cond(Expr *);
    void literal(Expr *);
    void paren(Expr *);
    /// !TODO Support comparison operation
    void binop(BinaryOperator *);
    void unaryop(UnaryOperator *);
    void condop(ConditionalOperator *, Expr *);
    void vardecl(Decl *);
    void fdecl(Decl *);
    void decl(DeclStmt *);
    void declref(DeclRefExpr *);
    void cast(CastExpr *);

    /// !TODO Support Function Call
    void call(CallExpr *);
};
