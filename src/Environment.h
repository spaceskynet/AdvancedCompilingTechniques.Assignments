//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#pragma once
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

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
    std::map<Decl *, int64_t> mVars;
    std::map<Stmt *, int64_t> mExprs;
    std::map<Stmt *, int64_t> mPtrs;
    /// The return value
    int64_t returnValue;

  public:
    StackFrame() : mVars(), mExprs(), mPtrs(), returnValue(0){}

    void bindDecl(Decl *, int64_t);
    bool findDecl(Decl *);
    int64_t getDeclVal(Decl *);
    void bindStmt(Stmt *, int64_t);
    bool findStmt(Stmt *);
    int64_t getStmtVal(Stmt *);
    void bindPtr(Stmt *, int64_t);
    bool findPtr(Stmt *);
    int64_t getPtrVal(Stmt *);
    void setReturnValue(int64_t);
    int64_t getReturnValue();
};

class GlobalVars
{
  private:
    std::map<Decl *, int64_t> mVars;
  public:
    GlobalVars() : mVars(){}

    void bindDecl(Decl *, int64_t);
    int64_t getDeclVal(Decl *);
};

/// Heap maps address to a value
class Heap
{
  private:
    std::map<void *, int64_t> mSpace;
  public:
    Heap() : mSpace(){}
    ~Heap() {
        // 析构 Heap 时 Free 掉局部变量中申请的空间（当然也有testcase源码中忘 Free 的情况）
        if(mSpace.empty()) return;
        for(auto& pair : mSpace) {
            void *ptr = pair.first;
            free(ptr);
        }
        mSpace.clear();
    }

    void *Malloc(int);
    void Free(void *);
};

class Environment
{
    std::vector<StackFrame> mStack;
    Heap mHeap; // 用于 Malloc / Free，管理堆上空间
    GlobalVars mGlobal; // 存储全局变量/常量

    const ASTContext &context;

    /// Declarations to the built-in functions
    FunctionDecl *mFree; 
    FunctionDecl *mMalloc;
    FunctionDecl *mInput;
    FunctionDecl *mOutput;

    FunctionDecl *mEntry;

  public:
    /// Get the declarations to the built-in functions
    Environment(const ASTContext &Context) : mStack(), mHeap(), mGlobal(), context(Context),mFree(nullptr), mMalloc(nullptr), mInput(nullptr), mOutput(nullptr), mEntry(nullptr) {
        mStack.push_back(StackFrame());
    }

    /// Initialize the Environment
    void init(TranslationUnitDecl *);

    FunctionDecl *getEntry();
    int64_t getStmtVal(Expr *);
    int64_t getDeclVal(Decl *);
    int64_t getPtrVal(Expr *);
    void bindDecl(Expr *, int64_t);
    void bindStmt(Expr *, int64_t);
    void bindPtr(Expr *, int64_t);
    bool isBuildIn(CallExpr *);

    int64_t cond(Expr *);
    void intliteral(Expr *);
    void charliteral(Expr *);
    void paren(Expr *);

    void binop(BinaryOperator *);
    void unaryop(UnaryOperator *);
    void condop(ConditionalOperator *, Expr *);
    void ueott(UnaryExprOrTypeTraitExpr *);
    void vardecl(Decl *);
    void fdecl(Decl *);
    void declref(DeclRefExpr *);
    void cast(CastExpr *);
    void arraysub(ArraySubscriptExpr *);

    void callbuildin(CallExpr *);
    void call(CallExpr *);
    void exit(CallExpr *);
    void returnstmt(ReturnStmt *);
};

namespace self {
    // 继承 std::exception 实现 return, break, continue 三个子类，其中 what() 函数以提供异常描述信息
    class ReturnException : public std::exception {
        public:
            ReturnException() : errorMessage("Function returns.") {}
            const char* what() const noexcept override {
                return errorMessage.c_str();
            }
        private:
            std::string errorMessage;
    };

    class BreakException : public std::exception {
        public:
            BreakException() : errorMessage("Loop Break.") {}
            const char* what() const noexcept override {
                return errorMessage.c_str();
            }
        private:
            std::string errorMessage;
    };

    class ContinueException : public std::exception {
        public:
            ContinueException() : errorMessage("Loop Continue.") {}
            const char* what() const noexcept override {
                return errorMessage.c_str();
            }
        private:
            std::string errorMessage;
    };

    // 根据命令行参数 --debug 判断是否开启标准错误流
    extern bool useErrs;
    llvm::raw_ostream &errs();
}
