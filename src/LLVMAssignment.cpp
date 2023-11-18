//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ToolOutputFile.h>

#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/InstrTypes.h>

#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>

namespace self {
    bool useErrs = true, useErrsInsteadOuts = false;
    llvm::raw_ostream &errs() {
        return useErrs && !useErrsInsteadOuts ? llvm::errs() : llvm::nulls();
    }
    llvm::raw_ostream &outs() {
        return useErrsInsteadOuts ? llvm::errs() : llvm::outs();
    }
}

using namespace llvm;

static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext() { return *GlobalContext; }
/* In LLVM 5.0, when  -O0 passed to clang , the functions generated with clang
 * will have optnone attribute which would lead to some transform passes
 * disabled, like mem2reg.
 */
struct EnableFunctionOptPass : public FunctionPass
{
    static char ID;
    EnableFunctionOptPass() : FunctionPass(ID) {}
    bool runOnFunction(Function &F) override {
        if (F.hasFnAttribute(Attribute::OptimizeNone)) {
            F.removeFnAttr(Attribute::OptimizeNone);
        }
        return true;
    }
};

char EnableFunctionOptPass::ID = 0;

///!TODO TO BE COMPLETED BY YOU FOR ASSIGNMENT 2
/// Updated 11/10/2017 by fargo: make all functions
/// processed by mem2reg before this pass.
struct FuncPtrPass : public ModulePass
{
    static char ID; // Pass identification, replacement for typeid
    FuncPtrPass() : ModulePass(ID) {}
    std::set<std::string> funcList;
    std::map<int, std::set<std::string> > lineFuncList;

    std::map<Function *, CallInst *> funcUser;
    std::stack<CallInst *> callinstStack;

    std::map<PHINode *, Value *> phiNodeIncoming;
    std::set<BasicBlock *> selectedPhiNodeBasicBlock;

    bool runOnModule(Module &M) override 
    {
        self::errs() << "Hello: ";
        self::errs().write_escaped(M.getName()) << '\n';
        M.print(self::errs(), nullptr);
        self::errs() << "------------------------------\n";
        for (Function &func : M) {
            if (func.isDeclaration()) continue;
            for (BasicBlock &block : func) {
                for (Instruction &inst : block) {
                    if (isa<llvm::DbgInfoIntrinsic>(inst)) continue;
                    if (CallInst *callinst = dyn_cast<CallInst>(&inst)) {
                        // callinst->print(self::outs()), self::outs() << ":\n";
                        handleCallInst(callinst);
                        // self::outs() << "\n";
                    }
                }
            }
        }
        printResult();
        return false;
    }

    void saveCallInst(CallInst *callinst) { callinstStack.push(callinst); }

    void delCallInst() { callinstStack.pop(); }

    void bindFunctionUser(Function *func) { funcUser[func] = callinstStack.top(); }

    void unBindFunctionUser(Function *func) { funcUser.erase(func); }

    // runOnModule 遍历到的 Call 才是需要输出的
    void handleCallInst(CallInst *callinst)
    {
        saveCallInst(callinst);

        int lineno = callinst->getDebugLoc().getLine();
        Value *operand = callinst->getCalledOperand();
        handleValue(operand, false);

        delCallInst();

        saveResult(lineno);
    }

    // 在查找真正 Call 的 func 过程中，内部调用的函数都只是需要其返回值
    void handleInnerCallInst(CallInst *callinst)
    {
        saveCallInst(callinst);

        Value *operand = callinst->getCalledOperand();
        handleValue(operand, true);

        delCallInst();
    }

    void handleFunctionReturn(Function *func)
    {
        bindFunctionUser(func);
        // 在调用函数中遍历寻找 return 语句
        for (BasicBlock &block : *func) {
            for (Instruction &inst : block) {
                if (ReturnInst *returninst = dyn_cast<ReturnInst>(&inst)) {
                    Value *retval = returninst->getReturnValue();
                    handleValue(retval, false);
                }
            }
        }
    }

    void handleFunction(Function *func)
    {
        bindFunctionUser(func);
        // Returns true if the function's name starts with "llvm.".
        if (func->isIntrinsic()) return;
        std::string funcName = func->getName().str();
        funcList.insert(funcName);
    }

    void bindPHINodeIncome(PHINode *phinode, Value *incomingvalue) {
        phiNodeIncoming[phinode] = incomingvalue;
    }

    void unBindPHINodeIncome(PHINode *phinode) {
        phiNodeIncoming.erase(phinode);
    }

    void handlePHINode(PHINode *phinode, bool isInnerCall)
    {
        // phinode->print(self::outs()), self::outs() << "\n";
        // 如果之前已经选择了这个 phi 结点的某个 Income，则不再遍历 phi 结点的 incomes
        if (phiNodeIncoming.find(phinode) != phiNodeIncoming.end()) {
            handleValue(phiNodeIncoming[phinode], isInnerCall);
            return;
        }
        for (BasicBlock *block : selectedPhiNodeBasicBlock) {
            int i = phinode->getBasicBlockIndex(block);
            if(i != -1) {
                Value *incomingvalue = phinode->getIncomingValue(i);
                bindPHINodeIncome(phinode, incomingvalue);
                handleValue(incomingvalue, isInnerCall);
                unBindPHINodeIncome(phinode);
                return;
            }
        }
        for (unsigned int i = 0; i < phinode->getNumIncomingValues(); ++i)
        {
            Value *incomingvalue = phinode->getIncomingValue(i);
            BasicBlock *incomingblock = phinode->getIncomingBlock(i);

            bindPHINodeIncome(phinode, incomingvalue);
            selectedPhiNodeBasicBlock.insert(incomingblock);
            // self::outs() << incomingvalue->getName() << *incomingblock << "\n";
            handleValue(incomingvalue, isInnerCall);
            selectedPhiNodeBasicBlock.erase(incomingblock);
            unBindPHINodeIncome(phinode);
        }
    }

    void handleValue(Value *value, bool isInnerCall)
    {
        if (CallInst *callinst = dyn_cast<CallInst>(value)) {
            handleInnerCallInst(callinst);
        }
        else if (PHINode *phinode = dyn_cast<PHINode>(value)) {
            handlePHINode(phinode, isInnerCall);
        }
        else if (Function *func = dyn_cast<Function>(value)) {
            if (isInnerCall)
                handleFunctionReturn(func);
            else
                handleFunction(func);
        }
        else if (Argument *argument = dyn_cast<Argument>(value)) {
            handleArgument(argument, isInnerCall);
        }
        else {
            self::errs() << "Unsupported Value: " << *value << ".\n";
        }
    }

    void handleArgument(Argument *argument, bool isInnerCall)
    {
        // argument->print(self::outs());
        unsigned int argindex = argument->getArgNo();
        Function *parentfunc = argument->getParent();
        // parentfunc->print(self::outs()), self::outs() << ":\n";

        // 嵌套调用时，在处理返回值时，回溯寻找参数
        if (funcUser.find(parentfunc) != funcUser.end()) {
            CallInst *callinst = funcUser[parentfunc];
            unBindFunctionUser(parentfunc);
            Value *operand = callinst->getArgOperand(argindex);
            handleValue(operand, isInnerCall);
            return;
        }

        // 
        for (User *user : parentfunc->users())
        {
            if (CallInst *callinst = dyn_cast<CallInst>(user)) {
                Function *calledfunc = callinst->getCalledFunction();
                if (calledfunc != parentfunc) continue;
                Value * operand = callinst->getArgOperand(argindex);
                handleValue(operand, isInnerCall);
            }
            else {
                self::errs() << "Unsupported user of parentfunc for argument: " << *user << ".\n";
            }
        }
    }

    void printResult()
    {
        if (lineFuncList.empty()) return;
        
        for (auto item : lineFuncList) 
        {
            auto &lineno = item.first;
            auto &funcList = item.second;
            self::outs() << lineno << " : ";

            for (auto iter = funcList.begin(); iter != funcList.end(); ++iter) {
                if (iter != funcList.begin()) self::outs() << ", ";
                self::outs() << *iter;
            }
            self::outs() << "\n";
        }
        lineFuncList.clear();
    }

    void saveResult(int lineno)
    {
        if (funcList.empty()) return;
        lineFuncList[lineno].insert(funcList.begin(), funcList.end());
        funcList.clear();
    }
};

char FuncPtrPass::ID = 0;
static RegisterPass<FuncPtrPass> X("funcptrpass",
                                   "Print function call instruction");

cl::opt<std::string>
    InputFilename(cl::Positional, cl::desc("<filename>.bc"), cl::init(""));
cl::opt<bool> StdErrOption("stderr", llvm::cl::desc("Enable stderr output"));
cl::alias StdErrOptionShort("e", llvm::cl::aliasopt(StdErrOption));

int main(int argc, char **argv) 
{
    LLVMContext &Context = getGlobalContext();
    SMDiagnostic Err;
    // Parse the command line to read the Inputfilename
    cl::ParseCommandLineOptions(
        argc, argv, "FuncPtrPass \n My first LLVM too which does not do much.\n");

    // 是否开启错误流输出
    bool enableStdErrOutput = StdErrOption;
    self::useErrs = enableStdErrOutput;

    // Load the input module
    std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
    if (!M) {
        Err.print(argv[0], self::errs());
        return 1;
    }

    llvm::legacy::PassManager Passes;

    /// Remove functions' optnone attribute in LLVM5.0
    Passes.add(new EnableFunctionOptPass());
    /// Transform it to SSA
    Passes.add(llvm::createPromoteMemoryToRegisterPass());

    /// Your pass to print Function and Call Instructions
    Passes.add(new FuncPtrPass());
    Passes.run(*M.get());
}
