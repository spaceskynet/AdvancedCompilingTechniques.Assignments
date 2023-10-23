//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

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
        mEnv->call(call);
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
            // thenStmt 为空仍然是有效的 If 语句，故不提前 return
            if(thenStmt)
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
            // bodyStmt 为空仍然是有效的 while 语句，故也不提前 return
            if(bodyStmt)
                Visit(bodyStmt);
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
            Visit(bodyStmt); // unless NullStmt
            if(incExpr) Visit(incExpr);
        }
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
        VisitStmt(entry->getBody());
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

char* readFileContent(const char *, int &);

int main(int argc, char *argv[])
{
    int option;
    const char* requiredArg = nullptr;
    bool useFile = false;

    // 定义长选项结构体
    const struct option longOptions[] = {
        {"file", no_argument, nullptr, 'f'},
        {nullptr, 0, nullptr, 0}
    };

    while ((option = getopt_long(argc, argv, "f", longOptions, nullptr)) != -1)
    {
        switch (option) {
            case 'f':
                useFile = true;
                break;
            case '?': // 处理未知选项或错误
                break; 
            default: // 处理其他选项
                break;
        }
    }

    // 检查是否存在必选参数
    if (optind < argc)
    {
        requiredArg = argv[optind];
    }
    else
    {
        llvm::errs() << "[Error] Missing required argument.\n";
        return 1;
    }

    // 判断直接传入参数字符数组还是从文件读取
    if (!useFile)
    {
        clang::tooling::runToolOnCode(
            std::unique_ptr<clang::FrontendAction>(new InterpreterClassAction), requiredArg);
    }
    else
    {
        int fileSize = 0;
        char* fileData = readFileContent(requiredArg, fileSize);
        if (fileData) {
            clang::tooling::runToolOnCode(
                std::unique_ptr<clang::FrontendAction>(new InterpreterClassAction), fileData);
            delete[] fileData;
        } else {
            return 1;
        }
    }
    return 0;
}

char* readFileContent(const char* filePath, int &fileSize) 
{
    FILE* file = fopen(filePath, "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        char* fileData = new char[fileSize];

        if (fileData) {
            size_t bytesRead = fread(fileData, 1, fileSize, file);
            fclose(file);
            if (bytesRead == fileSize) {
                return fileData;
            } else {
                llvm::errs() << "[Error] Read error.\n";
                delete[] fileData;
            }
        } else {
            llvm::errs() << "[Error] Memory allocation failed.\n";
        }
    }
    else 
    {
        llvm::errs() << "[Error] Could not open the file: " << filePath << ".\n" ;
    }
    return nullptr;
}