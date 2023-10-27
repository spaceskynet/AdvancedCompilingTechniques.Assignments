//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

using namespace clang;

#include "Environment.h"
#include <getopt.h>

class InterpreterVisitor : public EvaluatedExprVisitor<InterpreterVisitor>
{
  public:
    explicit InterpreterVisitor(const ASTContext &context, Environment *env)
        : EvaluatedExprVisitor(context), mEnv(env){}
    virtual ~InterpreterVisitor(){}

    virtual void VisitIntegerLiteral(IntegerLiteral *intl)
    {
        mEnv->literal(intl);
    }

    virtual void VisitParenExpr(ParenExpr *paren)
    {
        VisitStmt(paren);
        mEnv->paren(paren);
    }

    virtual void VisitBinaryOperator(BinaryOperator *bop)
    {
        VisitStmt(bop);
        mEnv->binop(bop);
    }
    virtual void VisitUnaryOperator(UnaryOperator *uop)
    {
        VisitStmt(uop);
        mEnv->unaryop(uop);
    }
    virtual void VisitDeclRefExpr(DeclRefExpr *expr)
    {
        VisitStmt(expr);
        mEnv->declref(expr);
    }
    virtual void VisitCastExpr(CastExpr *expr)
    {
        VisitStmt(expr);
        mEnv->cast(expr);
    }
    virtual void VisitCallExpr(CallExpr *call)
    {
        VisitStmt(call);
        
        if(mEnv->isBuildIn(call)) {
            mEnv->callbuildin(call);
            return;
        }

        mEnv->call(call);
        FunctionDecl *callee = call->getDirectCallee();
        if(callee->isDefined()) {
            callee = callee->getDefinition();
        }
        try {
            VisitStmt(callee->getBody());
        } catch (self::ReturnException &e) {
            self::errs() << e.what() << "\n";
        }
        
        mEnv->exit(call);
    }

    virtual void VisitReturnStmt(ReturnStmt *returnstmt)
    {
        VisitStmt(returnstmt);
        mEnv->returnstmt(returnstmt);
        throw self::ReturnException();
    }

    virtual void VisitArraySubscriptExpr(ArraySubscriptExpr *arraysub)
    {
        VisitStmt(arraysub);
        mEnv->arraysub(arraysub);
    }

    virtual void VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *ueott)
    {
        VisitStmt(ueott);
        mEnv->ueott(ueott);
    }

    virtual void VisitDeclStmt(DeclStmt *declstmt)
    {
        for (auto *SubDecl : declstmt->decls())
        {
            if(SubDecl == nullptr) continue;
            if(VarDecl *vardecl = dyn_cast<VarDecl>(SubDecl)) {
                VisitVarDecl(vardecl);
            }
            // more decl
        }
    }

    virtual void VisitIfStmt(IfStmt *ifstmt)
    {
        Expr *condExpr = ifstmt->getCond();
        Stmt *thenStmt = ifstmt->getThen();
        Stmt *elseStmt = ifstmt->getElse();
        
        if(condExpr == nullptr) return;
        Visit(condExpr);
        if(mEnv->cond(condExpr)) {
            Visit(thenStmt);
        } else if (elseStmt) {
            Visit(elseStmt);
        }
    }

    // 处理三目运算符
    virtual void VisitConditionalOperator(ConditionalOperator *condop)
    {
        Expr *condExpr = condop->getCond();
        Expr *trueExpr = condop->getTrueExpr();
        Expr *falseExpr = condop->getFalseExpr();
        if (condExpr == nullptr || trueExpr == nullptr || falseExpr == nullptr)
          return;

        Visit(condExpr);
        if(mEnv->cond(condExpr)) {
            Visit(trueExpr);
            mEnv->condop(condop, trueExpr);
        } else {
            Visit(falseExpr);
            mEnv->condop(condop, falseExpr);
        }        
    }

    virtual void VisitWhileStmt(WhileStmt * whstmt)
    {
        Expr *condExpr = whstmt->getCond();
        Stmt *bodyStmt = whstmt->getBody();
        if(condExpr == nullptr) return;

        Visit(condExpr);
        while(mEnv->cond(condExpr)) {
            try {
                Visit(bodyStmt); // At least: NullStmt
            } catch (self::BreakException &e) {
                self::errs() << e.what() << "\n";
                break;
            } catch (self::ContinueException &e) {
                self::errs() << e.what() << "\n";
            }
            Visit(condExpr);
        }
    }

    virtual void VisitForStmt(ForStmt *forstmt)
    {
        Stmt *initStmt = forstmt->getInit();
        Expr *condExpr = forstmt->getCond();
        Expr *incExpr = forstmt->getInc();
        Stmt *bodyStmt = forstmt->getBody();
            
        if(initStmt) Visit(initStmt);
        
        while(true) 
        {
            if(condExpr) {
                Visit(condExpr);
                // for(;;); -> condExpr is nullptr
                if(!mEnv->cond(condExpr)) break;
            }
            try {
                Visit(bodyStmt); // At least: NullStmt
            } catch (self::BreakException &e) {
                self::errs() << e.what() << "\n";
                break;
            } catch (self::ContinueException &e) {
                self::errs() << e.what() << "\n";
            }
            if(incExpr) Visit(incExpr);
        }
    }

    virtual void VisitBreakStmt(BreakStmt *breakstmt)
    {
        throw self::BreakException();
    }

    virtual void VisitBreakStmt(ContinueStmt *constmt)
    {
        throw self::ContinueException();
    }  

    virtual void VisitVarDecl(VarDecl *vardecl)
    {
        if(vardecl == nullptr) return;
        if(vardecl->hasInit()) {
            Expr * init = vardecl->getInit();
            Visit(init); // 从init子节点本身开始Visit，而不是VisitStmt
        }
        mEnv->vardecl(vardecl);
    }

    virtual void VisitFunctionDecl(FunctionDecl *fdecl)
    {
        mEnv->fdecl(fdecl);
    }

    void Init(TranslationUnitDecl *unit)
    {
        for (auto *SubDecl : unit->decls())
        {
            if(SubDecl == nullptr) continue;
            if(FunctionDecl *fdecl = dyn_cast<FunctionDecl>(SubDecl)) {
                VisitFunctionDecl(fdecl);
            } else if(VarDecl *vardecl = dyn_cast<VarDecl>(SubDecl)) {
                VisitVarDecl(vardecl);
            }
            // more decl
        }
        mEnv->init(unit);
        FunctionDecl *entry = mEnv->getEntry();
        try {
            VisitStmt(entry->getBody());
        } catch (self::ReturnException &e) {
            self::errs() << e.what() << "\n";
        }
    }

  private:
    Environment *mEnv;
};

class InterpreterConsumer : public ASTConsumer
{
  public:
    explicit InterpreterConsumer(const ASTContext &context) : mEnv(context), mVisitor(context, &mEnv){}
    virtual ~InterpreterConsumer(){}

    virtual void HandleTranslationUnit(clang::ASTContext &Context)
    {
        TranslationUnitDecl *decl = Context.getTranslationUnitDecl();
        mVisitor.Init(decl);
    }

  private:
    Environment mEnv;
    InterpreterVisitor mVisitor;
};

class InterpreterClassAction : public ASTFrontendAction
{
  public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler,
                                                                  llvm::StringRef InFile)
    {
        return std::unique_ptr<clang::ASTConsumer>(
            new InterpreterConsumer(Compiler.getASTContext()));
    }
};

llvm::cl::opt<std::string> InputFilename(llvm::cl::Positional, llvm::cl::desc("<source>.c"), llvm::cl::Required);
llvm::cl::opt<bool> FileOption("file", llvm::cl::desc("Enable read from file"));
llvm::cl::alias FileOptionShort("f", llvm::cl::aliasopt(FileOption));
llvm::cl::opt<bool> DebugOption("debug", llvm::cl::desc("Enable debugging output"));
llvm::cl::alias DebugOptionShort("d", llvm::cl::aliasopt(DebugOption));
std::string readFileContent(std::string);

int main(int argc, char *argv[])
{
    llvm::cl::ParseCommandLineOptions(argc, argv, "Clang AST Interpreter for tiny C.\n");

    if (InputFilename.empty()) {
        self::errs() << "[Error] Missing required C source file parameter.\n";
        llvm::cl::PrintHelpMessage(false, true);
        return 1;
    }

    // 获取必需参数的值
    std::string inputFile = InputFilename;
    // 获取可选参数的值
    bool useFile = FileOption;
    bool enableDebuggingOutput = DebugOption;

    std::string sourceCode;
    // 判断直接传入源代码字符串还是从源代码文件读取
    if (!useFile) sourceCode = inputFile;
    else {
        sourceCode = readFileContent(inputFile);
        if(sourceCode.empty()) return 1;
    }

    self::useErrs = enableDebuggingOutput;

    clang::tooling::runToolOnCode(
        std::unique_ptr<clang::FrontendAction>(new InterpreterClassAction),
        sourceCode
    );
    return 0;
}

std::string readFileContent(std::string filePath) 
{
    llvm::StringRef InputFilename(filePath);
    std::string FileContent;
    // 打开输入文件
    auto FileOrErr = llvm::MemoryBuffer::getFile(InputFilename);
    if (FileOrErr) {
        auto File = std::move(FileOrErr.get());

        // 将文件内容保存到 std::string
        FileContent = File->getBuffer().str();
    } else {
        self::errs() << "[Error] Fail to read file: " << InputFilename << ".\n";
    }
    return FileContent;
}