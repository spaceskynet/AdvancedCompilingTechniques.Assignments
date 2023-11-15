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
bool StackFrame::findDecl(Decl *decl)
{
    return mVars.find(decl) != mVars.end();
}
int64_t StackFrame::getDeclVal(Decl *decl)
{
    assert(this->findDecl(decl));
    return mVars.find(decl)->second;
}
void StackFrame::bindStmt(Stmt *stmt, int64_t val)
{
    mExprs[stmt] = val;
}
bool StackFrame::findStmt(Stmt *stmt)
{
    return mExprs.find(stmt) != mExprs.end();
}
int64_t StackFrame::getStmtVal(Stmt *stmt)
{
    assert(this->findStmt(stmt));
    return mExprs[stmt];
}

void StackFrame::bindPtr(Stmt *stmt, int64_t val)
{
    mPtrs[stmt] = val;
}
bool StackFrame::findPtr(Stmt *stmt)
{
    return mPtrs.find(stmt) != mPtrs.end();
}
int64_t StackFrame::getPtrVal(Stmt *stmt)
{
    assert(this->findStmt(stmt));
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

void GlobalVars::bindDecl(Decl *decl, int64_t val)
{
    mVars[decl] = val;
}
int64_t GlobalVars::getDeclVal(Decl *decl)
{
    assert(mVars.find(decl) != mVars.end());
    return mVars.find(decl)->second;
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
        if (VarDecl *vardecl = dyn_cast<VarDecl>(SubDecl)) {
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
    // 全局里不应该存在 Stmt，指令都需要进函数
    val = mStack.back().getStmtVal(expr);
    return val;
}

int64_t Environment::getDeclVal(Decl *decl)
{ 
    int64_t val;
    // 如果当前栈帧中没有此变量，则应该在全局中
    if(mStack.back().findDecl(decl)) 
        val = mStack.back().getDeclVal(decl);
    else
        val = mGlobal.getDeclVal(decl);
    return val;
}

int64_t Environment::getPtrVal(Expr *expr)
{
    int64_t val;
    val = mStack.back().getPtrVal(expr);
    return val;
}

void Environment::bindStmt(Expr *expr, int64_t val)
{
    mStack.back().bindStmt(expr, val);
}

void Environment::bindPtr(Expr *expr, int64_t val)
{
    mStack.back().bindPtr(expr, val);
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
        int64_t addr = getPtrVal(arraysub);
        QualType type = arraysub->getType();
        if(type->isCharType()) {
            *((char *)addr) = (char)val;
        } else if(type->isIntegerType()) {
            *((int *)addr) = (int)val;
        } else if(type->isPointerType()) {
            *((int64_t *)addr) = val;
        }
    }
    else if (UnaryOperator *uop = dyn_cast<UnaryOperator>(expr))
    {
        assert(uop->getOpcode() == UO_Deref);
        int64_t addr = getPtrVal(uop);
        QualType type = uop->getType();
        if(type->isCharType()) {
            *((char *)addr) = (char)val;
        } else if(type->isIntegerType()) {
            *((int *)addr) = (int)val;
        } else if(type->isPointerType()) {
            *((int64_t *)addr) = val;
        }
    }
}

int64_t Environment::cond(Expr *cond)
{
    int64_t val = getStmtVal(cond);
    return val;
}

void Environment::intliteral(Expr *intl) {
    int64_t val;
    llvm::APSInt intResult;

    // 判断是否是 ASTContext 中合法的整数常量
    if (intl->isIntegerConstantExpr(intResult, this->context)) {
        val = intResult.getExtValue();
        // 将（全局/局部）常量存入栈帧
        bindStmt(intl, val);
    }
}
void Environment::charliteral(Expr *charl) {
    int64_t val;
    llvm::APSInt charResult;

    // 判断是否是 ASTContext 中合法的整数常量（Clang AST中会将字符视作整数常量）
    if (charl->isIntegerConstantExpr(charResult, this->context)) {
        val = charResult.getExtValue();
        // 将（全局/局部）常量存入栈帧
        bindStmt(charl, val);
    }
}

void Environment::paren(Expr *expr)
{
    ParenExpr * paren = dyn_cast<ParenExpr>(expr);
    if(paren == nullptr) return;

    // 将子节点的val绑定到paren
    bindStmt(
        paren,
        getStmtVal(paren->getSubExpr())
    );
}

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
    
    if(leftType->isPointerType() && (rightType->isCharType() || rightType->isIntegerType()))
    {
        QualType peType = leftType->getPointeeType();
        if(peType->isCharType()) {
            rightVal *= sizeof(char);
        } else if (peType->isIntegerType()) {
            rightVal *= sizeof(int);
        } else if (peType->isPointerType()) {
            rightVal *= sizeof(void *);
        }
    }
    else if ((leftType->isCharType() || leftType->isIntegerType()) && rightType->isPointerType())
    {
        QualType peType = rightType->getPointeeType();
        if(peType->isCharType()) {
            leftVal *= sizeof(char);
        } else if (peType->isIntegerType()) {
            leftVal *= sizeof(int);
        } else if (peType->isPointerType()) {
            leftVal *= sizeof(void *);
        }        
    }

    // 赋值运算符 =, *=, /=, %=, +=, -=, <<=, >>=, &=, ^=, |=
    // from clang/AST/OperationKinds.def
    if (bop->isAssignmentOp())
    {
        // 左值为字符/整数，右值为指针非法
        assert(!((leftType->isCharType() || leftType->isIntegerType()) && rightType->isPointerType()));
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
                bop->dump(self::errs());
                break;
        }
    }
    bindStmt(bop, val);
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
            if(peType->isCharType()) {
                unit = sizeof(char);
            } else if (peType->isIntegerType()) {
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
                if(type->isCharType()) {
                    val = *((char *)exprVal);
                } else if (type->isIntegerType()) {
                    val = *((int *)exprVal);
                } else if (type->isPointerType()) {
                    val = *((int64_t *)exprVal);
                }
                bindPtr(uop, exprVal);
                break;
            }
            case UO_AddrOf: {
                // Extra TODO: Support UO_AddrOf like `int *p = &a;`
                break;
            }

            default:
                self::errs() << "[Error] Unsupported UnaryOperator.\n";
                uop->dump(self::errs());
                break;
        }
    }
    bindStmt(uop, val);
}

void Environment::condop(ConditionalOperator *condop, Expr *expr)
{
    bindStmt(
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
        ueott->dump(self::errs());
    }
    bindStmt(ueott, size);
}


void Environment::vardecl(Decl *decl)
{
    VarDecl *vardecl = dyn_cast<VarDecl>(decl);
    if(vardecl == nullptr) return;
    QualType type = vardecl->getType();
    if(type->isCharType() || type->isIntegerType() || type->isPointerType())
    {
        int64_t val = 0;
        if(vardecl->hasInit()) {
            // 使用 isIntegerConstantExpr，会直接计算 a = 10 + 13 * 2 右侧这种常量表达式，跳过自定义的 binop 函数
            // llvm::APSInt intResult;
            // if(init->isIntegerConstantExpr(intResult, this->context)) val = intResult.getExtValue();
            val = getStmtVal(vardecl->getInit());
        }
        mStack.back().bindDecl(vardecl, val);
    }
    else if(type->isArrayType())
    {
        auto array = dyn_cast<ConstantArrayType>(type.getTypePtr());
        int size = array->getSize().getSExtValue();
        QualType elemType = array->getElementType();
        void *addr = nullptr;
        if(elemType->isCharType()) {
            addr = mHeap.Malloc(size * sizeof(char));
            for(int i = 0; i < size; ++i) *((char *)addr + i) = 0;
        } else if(elemType->isIntegerType()) {
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
    QualType type = declref->getType();
    
    // declref 时，获取当前 Decl 的值绑定到当前 Stmt，但是不对函数进行处理（Call函数单独处理）
    if (type->isCharType() || type->isIntegerType() || type->isArrayType() || type->isPointerType())
    {
        Decl *decl = declref->getFoundDecl();

        int64_t val = getDeclVal(decl);
        bindStmt(declref, val);
    }
    else if(!type->isFunctionType())
    {
        self::errs() << "[Error] Unsupported DeclRef.\n";
        declref->dump(self::errs());
    }
}

void Environment::cast(CastExpr *castexpr)
{
    QualType type = castexpr->getType();

    // cast 时，不能包括 FunctionPointer，因为栈帧中可能并没有其信息，如果是指定的那几个extern函数
    if (type->isCharType() || type->isIntegerType() ||
        (type->isPointerType() && !type->isFunctionPointerType()))
    {
        Expr *expr = castexpr->getSubExpr();

        int64_t val = getStmtVal(expr);
        bindStmt(castexpr, val);
    }
    else if(!type->isFunctionPointerType())
    {
        self::errs() << "[Error] Unsupported CastExpr.\n";
        castexpr->dump(self::errs());
    }
}

void Environment::arraysub(ArraySubscriptExpr *arraysub)
{
    Expr *base = arraysub->getBase();
    Expr *index = arraysub->getIdx();

    QualType type = arraysub->getType();

    int64_t val = 0, addr = 0;
    if(type->isCharType()) {
        addr = getStmtVal(base) + getStmtVal(index) * sizeof(char);
        val = *((char *)addr);
    } else if(type->isIntegerType()) {
        addr = getStmtVal(base) + getStmtVal(index) * sizeof(int);
        val = *((int *)addr);
    } else if(type->isPointerType()) {
        addr = getStmtVal(base) + getStmtVal(index) * sizeof(int64_t);
        val = *((int64_t *)addr);
    }
    bindStmt(arraysub, val);
    bindPtr(arraysub, addr);
}


bool Environment::isBuildIn(CallExpr *callexpr)
{
    FunctionDecl *callee = callexpr->getDirectCallee();
    if (callee == mInput  || callee == mOutput ||
        callee == mMalloc || callee == mFree ) 
        return true;
    else
        return false;
}

void Environment::callbuildin(CallExpr *callexpr)
{
    int64_t val = 0;
    FunctionDecl *callee = callexpr->getDirectCallee();
    if (callee == mInput)
    {
        self::errs() << "Please Input an Integer Value : ";
        scanf("%ld", &val);

        bindStmt(callexpr, val);
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
        bindStmt(callexpr, (int64_t)ptr);
    }
    else if (callee == mFree)
    {
        Expr *decl = callexpr->getArg(0);
        val = getStmtVal(decl);
        mHeap.Free((void *)val);
    }
    else
    {
        self::errs() << "[Error] Unsupported BuildIn Function.\n";
        callexpr->dump(self::errs());
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
        bindStmt(callexpr, returnValue);
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
