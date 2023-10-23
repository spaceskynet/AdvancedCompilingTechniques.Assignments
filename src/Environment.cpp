#include "Environment.h"

void StackFrame::bindDecl(Decl *decl, int val)
{
    mVars[decl] = val;
}
bool StackFrame::findDeclVal(Decl *decl)
{
    return mVars.find(decl) != mVars.end();
}
int StackFrame::getDeclVal(Decl *decl) {
  assert(this->findDeclVal(decl));
  return mVars.find(decl)->second;
}
void StackFrame::bindStmt(Stmt *stmt, int val)
{
    mExprs[stmt] = val;
}
bool StackFrame::findStmtVal(Stmt *stmt)
{
    return mExprs.find(stmt) != mExprs.end();
}
int StackFrame::getStmtVal(Stmt *stmt)
{
    assert(this->findStmtVal(stmt)); // 保证 stmt 一定在栈帧中存在
    return mExprs[stmt];
}
void StackFrame::setPC(Stmt *stmt)
{
    mPC = stmt;
}
Stmt *StackFrame::getPC()
{
    return mPC;
}


void GlobalValue::bindDecl(Decl *decl, int val)
{
    mVars[decl] = val;
}
int GlobalValue::getDeclVal(Decl *decl)
{
    assert(mVars.find(decl) != mVars.end());
    return mVars.find(decl)->second;
}
void GlobalValue::bindStmt(Stmt *stmt, int val)
{
    mExprs[stmt] = val;
}
int GlobalValue::getStmtVal(Stmt *stmt)
{
    assert(mExprs.find(stmt) != mExprs.end());
    return mExprs[stmt];
}

void *Heap::Malloc(int size)
{
    
    return nullptr;
}
void Heap::Free(void *addr)
{

}

/// Initialize the Environment
void Environment::init(TranslationUnitDecl *unit)
{
    for (auto *SubDecl : unit->decls())
    {
        if (VarDecl *vardecl = dyn_cast<VarDecl>(SubDecl))
        {
            int64_t val = getDeclVal(vardecl);
            mGlobal.bindDecl(vardecl, val);
        }
        // more decl
    }
    // 清除用于全局变量的栈帧
    mStack.pop_back();
    // 添加 main 函数的栈帧
    mStack.push_back(StackFrame());
}

FunctionDecl *Environment::getEntry()
{
    return mEntry;
}

int64_t Environment::getStmtVal(Expr *expr)
{
    int64_t val;
    // 如果当前栈帧中没有此（局部）变量，则在全局中
    if(mStack.back().findStmtVal(expr)) 
        val = mStack.back().getStmtVal(expr);
    else
        val = mGlobal.getStmtVal(expr);
    return val;
}

int64_t Environment::getDeclVal(Decl *decl)
{ 
    int64_t val;
    // 如果当前栈帧中没有此（局部）变量，则在全局中
    if(mStack.back().findDeclVal(decl)) 
        val = mStack.back().getDeclVal(decl);
    else
        val = mGlobal.getDeclVal(decl);
    return val;
}

int64_t Environment::cond(Expr *cond)
{
    int64_t val = getStmtVal(cond);
    return val;
}

void Environment::literal(Expr *intl) {
  int64_t val;
  llvm::APSInt intResult;

  // 判断是否是 ASTContext 中合法的整数常量
  if (intl->isIntegerConstantExpr(intResult, this->context)) {
    val = intResult.getExtValue();
    // 将（全局/局部）常量存入栈帧
    mStack.back().bindStmt(intl, val);
  }
}

void Environment::paren(Expr *expr)
{
    ParenExpr * paren = dyn_cast<ParenExpr>(expr);
    if(paren == nullptr) return;

    // 将子节点的val绑定到paren
    mStack.back().bindStmt(
        paren,
        getStmtVal(paren->getSubExpr())
    );
}

/// !TODO Support comparison operation
void Environment::binop(BinaryOperator *bop)
{
    Expr *left = bop->getLHS();
    Expr *right = bop->getRHS();

    BinaryOperator::Opcode op = bop->getOpcode();
    int64_t rightVal = getStmtVal(right);
    int64_t val = 0;

    // 赋值运算符 =, *=, /=, %=, +=, -=, <<=, >>=, &=, ^=, |=
    // from clang/AST/OperationKinds.def
    if (bop->isAssignmentOp())
    {
        mStack.back().bindStmt(left, rightVal);
        if (DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(left))
        {
            Decl *decl = declexpr->getFoundDecl();
            mStack.back().bindDecl(decl, rightVal);
        }
    }
    else
    {
        int64_t leftVal = getStmtVal(left);
        switch (op)
        {
            case BO_Add:
                val = leftVal + rightVal; break;
            case BO_Sub:
                val = leftVal - rightVal; break;
            case BO_Mul:
                val = leftVal * rightVal; break;
            case BO_Div:
                assert(rightVal != 0);
                val = leftVal / rightVal; break;
            case BO_Rem:
                assert(rightVal != 0);
                val = leftVal % rightVal; break;
            case BO_Shl:
                val = leftVal << rightVal; break;
            case BO_Shr:
                val = leftVal >> rightVal; break;
            case BO_LT:
                val = leftVal < rightVal; break;
            case BO_GT:
                val = leftVal > rightVal; break;
            case BO_LE:
                val = leftVal <= rightVal; break;
            case BO_GE:
                val = leftVal >= rightVal; break;                       
            case BO_EQ:
                val = leftVal == rightVal; break;
            case BO_NE:
                val = leftVal != rightVal; break;
            case BO_And:
                val = leftVal & rightVal; break;
            case BO_Xor:
                val = leftVal ^ rightVal; break;
            case BO_Or:
                val = leftVal | rightVal; break;
            case BO_LAnd:
                val = leftVal && rightVal; break;
            case BO_LOr:
                val = leftVal || rightVal; break;
            default:
                llvm::errs() << "[Error] Unsupport BinaryOperator.\n";
                break;
        }
        mStack.back().bindStmt(bop, val);
    }
}

void Environment::unaryop(UnaryOperator *uop)
{
    Expr *expr = uop->getSubExpr();

	int64_t val = 0;
    int64_t exprVal = getStmtVal(expr);
    UnaryOperator::Opcode op = uop->getOpcode();
    
    if (uop->isIncrementDecrementOp())
    {
        switch (op)
        {
            case UO_PreInc:
                val = ++exprVal;
                break;
            case UO_PreDec:
                val = --exprVal;
                break;
            case UO_PostInc:
                val = exprVal++;
                break;
            case UO_PostDec:
                val = exprVal--;
                break;
            default:
                break;
        }
        if (DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(expr))
        {
            Decl *decl = declexpr->getFoundDecl();
            mStack.back().bindDecl(decl, exprVal);
        }
    }
    else
    {
        switch (op)
        {
            case UO_Minus:
                val = -exprVal;
                break;
            case UO_Plus:
                val = +exprVal;
                break;
            case UO_Not:
                val = ~exprVal;
                break;
            case UO_LNot:
                val = !exprVal;
                break;

            default:
                llvm::errs() << "[Error] Unsupport UnaryOperator.\n";
                break;
        }
    }
    mStack.back().bindStmt(uop, val);
}

void Environment::condop(ConditionalOperator *condop, Expr *expr)
{
    mStack.back().bindStmt(
        condop,
        getStmtVal(expr)
    );
}

void Environment::decl(DeclStmt *declstmt)
{
    for (DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end();
            it != ie; ++it)
    {
        Decl *decl = *it;
        if (VarDecl *vardecl = dyn_cast<VarDecl>(decl))
        {
            mStack.back().bindDecl(vardecl, 0);
        }
    }
}

void Environment::vardecl(Decl *decl)
{
    int64_t val = 0;
    VarDecl *vardecl = dyn_cast<VarDecl>(decl);
    if(vardecl == nullptr) return;
    if(vardecl->hasInit()) {
        // 使用 isIntegerConstantExpr，会直接计算 a = 10 + 13 * 2 右侧这种常量表达式，跳过自定义的 binop 函数
        // llvm::APSInt intResult;
        // if(init->isIntegerConstantExpr(intResult, this->context)) val = intResult.getExtValue();
        val = this->getStmtVal(vardecl->getInit());
    }
    mStack.back().bindDecl(vardecl, val);
}

void Environment::fdecl(Decl *decl)
{
    FunctionDecl *fdecl = dyn_cast<FunctionDecl>(decl);

    if(fdecl == nullptr) return;
    if (fdecl->getName().equals("FREE"))
        mFree = fdecl;
    else if (fdecl->getName().equals("MALLOC"))
        mMalloc = fdecl;
    else if (fdecl->getName().equals("GET"))
        mInput = fdecl;
    else if (fdecl->getName().equals("PRINT"))
        mOutput = fdecl;
    else if (fdecl->getName().equals("main"))
        mEntry = fdecl;
}

void Environment::declref(DeclRefExpr *declref)
{
    mStack.back().setPC(declref);
    if (declref->getType()->isIntegerType())
    {
        Decl *decl = declref->getFoundDecl();

        int val = this->getDeclVal(decl);
        mStack.back().bindStmt(declref, val);
    }
}

void Environment::cast(CastExpr *castexpr)
{
    mStack.back().setPC(castexpr);
    if (castexpr->getType()->isIntegerType())
    {
        Expr *expr = castexpr->getSubExpr();
        int val = mStack.back().getStmtVal(expr);
        mStack.back().bindStmt(castexpr, val);
    }
}

/// !TODO Support Function Call
void Environment::call(CallExpr *callexpr)
{
    mStack.back().setPC(callexpr);
    int val = 0;
    FunctionDecl *callee = callexpr->getDirectCallee();
    if (callee == mInput)
    {
        llvm::outs() << "Please Input an Integer Value : ";
        scanf("%d", &val);

        mStack.back().bindStmt(callexpr, val);
    }
    else if (callee == mOutput)
    {
        Expr *decl = callexpr->getArg(0);
        val = getStmtVal(decl);
        llvm::outs() << val;
    }
    else if (callee == mMalloc)
    {

    }
    else if (callee == mFree)
    {
        
    }
    else
    {
        
    }
}
