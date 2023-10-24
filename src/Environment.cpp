#include "Environment.h"

namespace self {
    bool useErrs = true;
    llvm::raw_ostream &errs() {
        return useErrs ? llvm::errs() : llvm::nulls();
    }
}

void StackFrame::bindDecl(Decl *decl, int64_t val)
{
    mVars[decl] = val;
}
bool StackFrame::findDeclVal(Decl *decl)
{
    return mVars.find(decl) != mVars.end();
}
int64_t StackFrame::getDeclVal(Decl *decl) {
  assert(this->findDeclVal(decl));
  return mVars.find(decl)->second;
}
void StackFrame::bindStmt(Stmt *stmt, int64_t val)
{
    mExprs[stmt] = val;
}
bool StackFrame::findStmtVal(Stmt *stmt)
{
    return mExprs.find(stmt) != mExprs.end();
}
int64_t StackFrame::getStmtVal(Stmt *stmt)
{
    assert(this->findStmtVal(stmt));
    return mExprs[stmt];
}

void StackFrame::bindPtr(Stmt *stmt, int64_t val)
{
    mPtrs[stmt] = val;
}
bool StackFrame::findPtrVal(Stmt *stmt)
{
    return mPtrs.find(stmt) != mPtrs.end();
}
int64_t StackFrame::getPtrVal(Stmt *stmt)
{
    assert(this->findStmtVal(stmt));
    return mPtrs[stmt];
}

void StackFrame::setReturnValue(int64_t val)
{
    returnValue = val;
}
int64_t StackFrame::getReturnValue()
{
    return returnValue;
}

void StackFrame::setPC(Stmt *stmt)
{
    mPC = stmt;
}
Stmt *StackFrame::getPC()
{
    return mPC;
}


void GlobalValue::bindDecl(Decl *decl, int64_t val)
{
    mVars[decl] = val;
}
int64_t GlobalValue::getDeclVal(Decl *decl)
{
    assert(mVars.find(decl) != mVars.end());
    return mVars.find(decl)->second;
}
void GlobalValue::bindStmt(Stmt *stmt, int64_t val)
{
    mExprs[stmt] = val;
}
int64_t GlobalValue::getStmtVal(Stmt *stmt)
{
    assert(mExprs.find(stmt) != mExprs.end());
    return mExprs[stmt];
}

void *Heap::Malloc(int size)
{
    void *ptr = malloc(size);
    mSpace[ptr] = size;
    return ptr;
}
void Heap::Free(void *ptr)
{
    assert(mSpace.find(ptr) != mSpace.end());
    mSpace.erase(ptr);
    free(ptr);
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

void Environment::bindDecl(Expr *expr, int64_t val)
{
    if (DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(expr))
    {
        Decl *decl = declexpr->getFoundDecl();
        mStack.back().bindDecl(decl, val);
    }
    else if (ArraySubscriptExpr *arraysub = dyn_cast<ArraySubscriptExpr>(expr))
    {
        int64_t addr = mStack.back().getPtrVal(arraysub);
        QualType type = arraysub->getType();
        if(type->isIntegerType()) {
            *((int *)addr) = (int)val;
        } else if(type->isPointerType()) {
            *((int64_t *)addr) = val;
        }
    }
    else if (UnaryOperator *uop = dyn_cast<UnaryOperator>(expr))
    {
        assert(uop->getOpcode() == UO_Deref);
        int64_t addr = mStack.back().getPtrVal(uop);
        QualType type = uop->getType();
        if(type->isIntegerType()) {
            *((int *)addr) = (int)val;
        } else if(type->isPointerType()) {
            *((int64_t *)addr) = val;
        }
    }
}

/// !TODO Support comparison operation
void Environment::binop(BinaryOperator *bop)
{
    Expr *left = bop->getLHS();
    Expr *right = bop->getRHS();

    BinaryOperator::Opcode op = bop->getOpcode();
    int64_t leftVal = getStmtVal(left);
    int64_t rightVal = getStmtVal(right);
    int64_t val = 0;

    QualType leftType = left->getType();
    QualType rightType = right->getType();

    if(leftType->isPointerType() && rightType->isIntegerType())
    {
        QualType peType = leftType->getPointeeType();
        if (peType->isIntegerType()) {
            rightVal *= sizeof(int);
        } else if (peType->isPointerType()) {
            rightVal *= sizeof(void *);
        }
    }
    else if (leftType->isIntegerType() && rightType->isPointerType())
    {
        QualType peType = rightType->getPointeeType();
        if (peType->isIntegerType()) {
            leftVal *= sizeof(int);
        } else if (peType->isPointerType()) {
            leftVal *= sizeof(void *);
        }        
    }

    // 赋值运算符 =, *=, /=, %=, +=, -=, <<=, >>=, &=, ^=, |=
    // from clang/AST/OperationKinds.def
    if (bop->isAssignmentOp())
    {
        // 左值为整数，右值为指针非法
        assert(!(leftType->isIntegerType() && rightType->isPointerType()));
        switch (op) 
        {
            case BO_Assign:
                val = rightVal; break;
            case BO_AddAssign:
                val = leftVal + rightVal; break;
            case BO_SubAssign:
                val = leftVal - rightVal; break;
            case BO_MulAssign:
                val = leftVal * rightVal; break;
            case BO_DivAssign:
                assert(rightVal != 0);
                val = leftVal / rightVal; break;
            case BO_RemAssign:
                assert(rightVal != 0);
                val = leftVal % rightVal; break;
            case BO_ShlAssign:
                val = leftVal << rightVal; break;
            case BO_ShrAssign:
                val = leftVal >> rightVal; break;
            case BO_AndAssign:
                val = leftVal & rightVal; break;
            case BO_XorAssign:
                val = leftVal ^ rightVal; break;
            case BO_OrAssign:
                val = leftVal | rightVal; break;
            default:
                break;
        }
        bindDecl(left, val);
        // mStack.back().bindStmt(left, rightVal);
    }
    else
    {
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
                self::errs() << "[Error] Unsupported BinaryOperator.\n";
                bop->dump();
                break;
        }
    }
    mStack.back().bindStmt(bop, val);
}

void Environment::unaryop(UnaryOperator *uop)
{
    Expr *expr = uop->getSubExpr();

	int64_t val = 0;
    int64_t exprVal = getStmtVal(expr);
    UnaryOperator::Opcode op = uop->getOpcode();
    
    if (uop->isIncrementDecrementOp())
    {
        QualType type = expr->getType();
        int unit = 1;
        if (type->isPointerType()) {
            QualType peType = type->getPointeeType();
            if (peType->isIntegerType()) {
                unit = sizeof(int);
            } else if (peType->isPointerType()) {
                unit = sizeof(void *);
            }
        }

        switch (op)
        {
            case UO_PreInc:
                exprVal += unit;
                val = exprVal;
                break;
            case UO_PreDec:
                exprVal -= unit;
                val = exprVal;
                break;
            case UO_PostInc:
                val = exprVal;
                exprVal += unit;
                break;
            case UO_PostDec:
                val = exprVal;
                exprVal -= unit;
                break;
            default:
                break;
        }
        bindDecl(expr, exprVal);
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

            case UO_Deref: {
                QualType type = uop->getType();
                if (type->isIntegerType()) {
                    val = *((int *)exprVal);
                } else if (type->isPointerType()) {
                    val = *((int64_t *)exprVal);
                }
                mStack.back().bindPtr(uop, exprVal);
                break;
            }

            default:
                self::errs() << "[Error] Unsupported UnaryOperator.\n";
                uop->dump();
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

void Environment::ueott(UnaryExprOrTypeTraitExpr *ueott)
{
    UnaryExprOrTypeTrait uett = ueott->getKind();
    int64_t size = 0;
    if (uett == UETT_SizeOf)
    {
        QualType argType = ueott->getTypeOfArgument();
        CharUnits sizeInChars = context.getTypeSizeInChars(argType);
        size = sizeInChars.getQuantity();
    }
    else
    {
        self::errs() << "[Error] Unsupported UnaryExprOrTypeTraitExpr.\n";
        ueott->dump();
    }
    mStack.back().bindStmt(ueott, size);
}


void Environment::vardecl(Decl *decl)
{
    VarDecl *vardecl = dyn_cast<VarDecl>(decl);
    if(vardecl == nullptr) return;
    QualType type = vardecl->getType();
    if(type->isIntegerType() || type->isPointerType())
    {
        int64_t val = 0;
        if(vardecl->hasInit()) {
            // 使用 isIntegerConstantExpr，会直接计算 a = 10 + 13 * 2 右侧这种常量表达式，跳过自定义的 binop 函数
            // llvm::APSInt intResult;
            // if(init->isIntegerConstantExpr(intResult, this->context)) val = intResult.getExtValue();
            val = this->getStmtVal(vardecl->getInit());
        }
        mStack.back().bindDecl(vardecl, val);
    }
    else if(type->isArrayType())
    {
        auto array = dyn_cast<ConstantArrayType>(type.getTypePtr());
        int size = array->getSize().getSExtValue();
        QualType elemType = array->getElementType();
        void *addr = nullptr;
        if(elemType->isIntegerType()) {
            addr = mHeap.Malloc(size * sizeof(int));
            for(int i = 0; i < size; ++i) *((int *)addr + i) = 0;
        } else if(elemType->isPointerType()) {
            addr =  mHeap.Malloc(size * sizeof(void *));
            for(int i = 0; i < size; ++i) *((int64_t *)addr + i) = 0;          
        }
        mStack.back().bindDecl(vardecl, (int64_t)addr);
    }
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
    QualType type = declref->getType();
    
    // declref 时，获取当前 Decl 的值绑定到当前 Stmt，但是不对函数进行处理（Call函数单独处理）
    if (type->isIntegerType() || type->isArrayType() || type->isPointerType())
    {
        Decl *decl = declref->getFoundDecl();

        int val = this->getDeclVal(decl);
        mStack.back().bindStmt(declref, val);
    }
    else if(!type->isFunctionType())
    {
        self::errs() << "[Error] Unsupported DeclRef.\n";
        declref->dump();
    }
}

void Environment::cast(CastExpr *castexpr)
{
    mStack.back().setPC(castexpr);
    QualType type = castexpr->getType();

    // cast 时，不能包括 FunctionPointer，因为栈帧中可能并没有其信息，而是单独的几个extern函数
    if (type->isIntegerType() || 
        (type->isPointerType() && !type->isFunctionPointerType()))
    {
        Expr *expr = castexpr->getSubExpr();

        int val = this->getStmtVal(expr);
        mStack.back().bindStmt(castexpr, val);
    }
    else if(!type->isFunctionPointerType())
    {
        self::errs() << "[Error] Unsupported CastExpr.\n";
        castexpr->dump();
    }
}

void Environment::arraysub(ArraySubscriptExpr *arraysub)
{
    Expr *base = arraysub->getBase();
    Expr *index = arraysub->getIdx();

    QualType type = arraysub->getType();

    int64_t val = 0, addr = 0;
    if(type->isIntegerType()) {
        addr = getStmtVal(base) + getStmtVal(index) * sizeof(int);
        val = *((int *)addr);
    } else if(type->isPointerType()) {
        addr = getStmtVal(base) + getStmtVal(index) * sizeof(int64_t);
        val = *((int64_t *)addr);
    }
    mStack.back().bindStmt(arraysub, val);
    mStack.back().bindPtr(arraysub, addr);
}


bool Environment::isBuildIn(CallExpr *callexpr)
{
    FunctionDecl *callee = callexpr->getDirectCallee();
    if (callee == mInput || callee == mOutput ||
        callee == mMalloc || callee == mFree ) return true;
    else return false;
}

/// !TODO Support Function Call
void Environment::callbuildin(CallExpr *callexpr)
{
    mStack.back().setPC(callexpr);
    int64_t val = 0;
    FunctionDecl *callee = callexpr->getDirectCallee();
    if (callee == mInput)
    {
        self::errs() << "Please Input an Integer Value : ";
        scanf("%ld", &val);

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
        Expr *decl = callexpr->getArg(0);
        val = getStmtVal(decl);
        void *ptr = mHeap.Malloc(val);
        mStack.back().bindStmt(callexpr, (int64_t)ptr);
    }
    else if (callee == mFree)
    {
        Expr *decl = callexpr->getArg(0);
        val = getStmtVal(decl);
        mHeap.Free((void *)val);
    }
    else
    {
        self::errs() << "[Error] Unsupported BuildIn.\n";
        callexpr->dump();
    }
}

void Environment::call(CallExpr *callexpr)
{
    FunctionDecl *callee = callexpr->getDirectCallee();
    if(callee->isDefined()) {
        callee = callee->getDefinition();
    }
    int argsNum = callexpr->getNumArgs();
    assert(argsNum == callee->getNumParams());
    StackFrame calleeFrame = StackFrame();
    for(int i = 0; i < argsNum; ++i)
    {
        calleeFrame.bindDecl(
            callee->getParamDecl(i),
            getStmtVal(callexpr->getArg(i))
        );
    }
    mStack.push_back(calleeFrame);
}

void Environment::exit(CallExpr *callexpr)
{
    FunctionDecl *callee = callexpr->getDirectCallee();
    QualType type = callee->getReturnType();
    if (!type->isVoidType()) {
        int64_t returnValue = mStack.back().getReturnValue();
        mStack.pop_back();
        mStack.back().bindStmt(callexpr, returnValue);
    }
    else {
        mStack.pop_back();
    }
}

void Environment::returnstmt(ReturnStmt *returnstmt)
{
    Expr *retVal = returnstmt->getRetValue();
    QualType type = retVal->getType();
    if (type->isVoidType()) return;

    mStack.back().setReturnValue(
        getStmtVal(retVal)
    );
}
