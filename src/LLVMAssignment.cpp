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
    std::map<int, std::set<std::string> > lineFuncList;
    std::map<Function *, CallInst *> funcUser;
    std::set<BasicBlock *> selectedBlock;

    bool runOnModule(Module &M) override 
    {
        // self::errs() << "Hello: ";
        // self::errs().write_escaped(M.getName()) << '\n';
        // M.print(self::errs(), nullptr);
        // self::errs() << "------------------------------\n";
        std::stack<CallInst *> callinstStack;
        for (Function &func : M) {
            if (func.isDeclaration()) continue;
            for (BasicBlock &block : func) {
                for (Instruction &inst : block) {
                    if (isa<llvm::DbgInfoIntrinsic>(inst)) continue;
                    if (CallInst *callinst = dyn_cast<CallInst>(&inst)) {
                        handleCallInst(callinst, false, callinstStack);
                    }
                }
            }
        }
        printResult();
        return false;
    }

    void handleCallInst(CallInst *callinst, bool isInnerCall, std::stack<CallInst *> callinstStack)
    {
        callinstStack.push(callinst);

        Value *operand = callinst->getCalledOperand();
        handleValue(operand, isInnerCall, callinstStack);

        callinstStack.pop();
    }

    void handleFunctionReturn(Function *func, std::stack<CallInst *> callinstStack)
    {
        funcUser[func] = callinstStack.top(), callinstStack.pop();

        // 在调用的 func 函数中遍历寻找 return 语句
        for (BasicBlock &block : *func) {
            for (Instruction &inst : block) {
                if (isa<llvm::DbgInfoIntrinsic>(inst)) continue;
                if (ReturnInst *returninst = dyn_cast<ReturnInst>(&inst)) {
                    Value *retval = returninst->getReturnValue();
                    handleValue(retval, false, callinstStack);
                }
            }
        }

        funcUser.erase(func);
    }

    void handleFunction(Function *func, std::stack<CallInst *> callinstStack)
    {
        // 跳过名称由 "llvm." 开头的函数
        // if (func->isIntrinsic()) return;
        funcUser[func] = callinstStack.top(), callinstStack.pop();
        saveResult(func, funcUser[func]);

        // 遍历 func 内部调用的函数
        for (BasicBlock &block : *func) {
            for (Instruction &inst : block) {
                if (isa<llvm::DbgInfoIntrinsic>(inst)) continue;
                if (CallInst *callinst = dyn_cast<CallInst>(&inst)) {
                    handleCallInst(callinst, false, callinstStack);
                }
            }
        }

        funcUser.erase(func);
    }

    void handlePHINode(PHINode *phinode, bool isInnerCall, std::stack<CallInst *> callinstStack)
    {
        // 保证同一基本块内的 phi 结点的前驱基本块一致（保证了test14的精确结果：32 : plus）
        for (BasicBlock *block : selectedBlock) {
            int i = phinode->getBasicBlockIndex(block);
            if (i != -1) { // 如果在当前 phi 结点的前驱基本块中存在已经被选定的基本块，直接选定对应分支处理后 return
                Value *incomingvalue = phinode->getIncomingValue(i);
                handleValue(incomingvalue, isInnerCall, callinstStack);
                return;
            }
        }
        for (unsigned int i = 0; i < phinode->getNumIncomingValues(); ++i)
        {
            Value *incomingvalue = phinode->getIncomingValue(i);
            BasicBlock *incomingblock = phinode->getIncomingBlock(i);

            selectedBlock.insert(incomingblock);
            handleValue(incomingvalue, isInnerCall, callinstStack);
            selectedBlock.erase(incomingblock);
        }
    }

    void handleValue(Value *value, bool isInnerCall, std::stack<CallInst *> callinstStack)
    {
        if (CallInst *callinst = dyn_cast<CallInst>(value)) {
            handleCallInst(callinst, true, callinstStack);
        }
        else if (PHINode *phinode = dyn_cast<PHINode>(value)) {
            handlePHINode(phinode, isInnerCall, callinstStack);
        }
        else if (Function *func = dyn_cast<Function>(value)) {
            if (isInnerCall)
                handleFunctionReturn(func, callinstStack);
            else
                handleFunction(func, callinstStack);
        }
        else if (Argument *argument = dyn_cast<Argument>(value)) {
            handleArgument(argument, isInnerCall, callinstStack);
        }
        else {
            self::errs() << "Unsupported Value: " << *value << ".\n";
        }
    }

    void handleArgument(Argument *argument, bool isInnerCall, std::stack<CallInst *> callinstStack)
    {
        unsigned int argindex = argument->getArgNo();
        Function *parentfunc = argument->getParent();

        // 嵌套调用时，回溯寻找参数
        if (funcUser.find(parentfunc) != funcUser.end()) {
            CallInst *callinst = funcUser[parentfunc];
            Value *operand = callinst->getArgOperand(argindex);
            handleValue(operand, isInnerCall, callinstStack);
        }
        else {
            self::errs() << "Unexpected Parent Function: " << *parentfunc << ".\n";
        }
    }

    void saveResult(Function *func, CallInst *callinst)
    {
        std::string funcName = func->getName().str();
        int lineno = callinst->getDebugLoc().getLine();

        lineFuncList[lineno].insert(funcName);
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
        argc, argv, "FuncPtrPass \n LLVM Pass to print function call in IR.\n");

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
