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
        return useErrs ? llvm::errs() : llvm::nulls();
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

    bool runOnModule(Module &M) override 
    {
        self::errs() << "Hello: ";
        self::errs().write_escaped(M.getName()) << '\n';
        M.print(self::errs(), nullptr);
        self::errs() << "------------------------------\n";
        for (Function &func : M) {
            if (func.isDeclaration()) continue;
            for (BasicBlock &block : func) {
                // if (isa<DbgInfoIntrinsic>(block)) continue;
                for (Instruction &inst : block) {
                    if (isa<llvm::DbgInfoIntrinsic>(inst)) continue;
                    if (CallInst *callinst = dyn_cast<CallInst>(&inst)) {
                        handleCallInst(callinst);
                    }
                }
            }
        }
        return false;
    }

    void handleCallInst(CallInst *callinst)
    {
        int lineno = callinst->getDebugLoc().getLine();
        if (Function *func = callinst->getCalledFunction())
        {
            handleFunction(func);
        }
        else
        {
            Value *operand = callinst->getCalledOperand();

            if (CallInst *innercall = dyn_cast<CallInst>(operand)) {

            }
            else {
                handleValue(operand);
            }
        }
        printResult(lineno);
    }

    void handleFunction(Function *func)
    {
        // Returns true if the function's name starts with "llvm.".
        if (func->isIntrinsic()) return;
        std::string funcName = func->getName().str();
        funcList.insert(funcName);
    }

    void handlePHINode(PHINode *phinode)
    {
        for (Value *incomevalue : phinode->incoming_values())
        {
            handleValue(incomevalue);
        }
    }

    void handleValue(Value *value)
    {
        if (CallInst *callinst = dyn_cast<CallInst>(value)) {
            handleCallInst(callinst);
        }
        else if (PHINode *phinode = dyn_cast<PHINode>(value)) {
            handlePHINode(phinode);
        }
        else if (Function *func = dyn_cast<Function>(value)) {
            handleFunction(func);
        }
        else if (Argument *argument = dyn_cast<Argument>(value)) {
            handleArgument(argument);
        }
        else {
            self::errs() << "Unsupported Value: " << *value << ".\n";
        }
    }

    void handleArgument(Argument *argument)
    {
        // argument->print(self::outs());
        unsigned int argindex = argument->getArgNo();
        Function *parentfunc = argument->getParent();
        for (User *user : parentfunc->users()) {
            if (CallInst *callinst = dyn_cast<CallInst>(user)) {
                // callinst->print(self::outs());
                Value * operand = callinst->getArgOperand(argindex);
                handleValue(operand);
            } else {
                self::errs() << "Unsupported user of parentfunc for argument: " << *user << ".\n";
            }
        }
    }

    void printResult(int lineno)
    {
        if (funcList.empty()) return;
        
        self::outs() << lineno << " : ";
        for (auto iter = funcList.begin(); iter != funcList.end(); ++iter) {
            if (iter != funcList.begin()) self::outs() << ", ";
            self::outs() << *iter;
        }
        self::outs() << "\n";
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
