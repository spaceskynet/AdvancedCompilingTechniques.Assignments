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

#include <llvm/Pass.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IRReader/IRReader.h>

#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>

#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Scalar.h>

// #include "Liveness.h"
#include "PointToAnalysis.h"

using namespace llvm;
static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext() { return *GlobalContext; }

struct EnableFunctionOptPass : public FunctionPass
{
    static char ID;
    EnableFunctionOptPass() :FunctionPass(ID) {}
    bool runOnFunction(Function & F) override {
        if (F.hasFnAttribute(Attribute::OptimizeNone))
        {
            F.removeFnAttr(Attribute::OptimizeNone);
        }
        return true;
    }
};

char EnableFunctionOptPass::ID = 0;

///!TODO TO BE COMPLETED BY YOU FOR ASSIGNMENT 3
struct FuncPtrPass : public ModulePass
{
    static char ID; // Pass identification, replacement for typeid
    FuncPtrPass() : ModulePass(ID) {}

    bool runOnModule(Module &M) override
    {
        // self::errs() << "Hello: ";
        // self::errs().write_escaped(M.getName()) << '\n';
        // M.print(self::errs(), nullptr);
        // self::errs() << "------------------------------\n";
        PointToVisitor visitor;
        PointToInfo inital;
        
        for (Function &func : M) {
            // 跳过名称由 "llvm." 开头的函数
            if (func.isIntrinsic()) continue;
            self::errs() << "Function name: " << func.getName() << ".\n";
            // func.print(self::errs());
            DataflowResult<PointToInfo>::Type result;
            compForwardDataflow(&func, &visitor, &result, inital);
            // printDataflowResult<PointToInfo>(self::errs(), result);
        }
        visitor.printResult();
        return false;
    }
};


char FuncPtrPass::ID = 0;
static RegisterPass<FuncPtrPass> X("funcptrpass", "Print function call instruction");

// char Liveness::ID = 0;
// static RegisterPass<Liveness> Y("liveness", "Liveness Dataflow Analysis");

cl::opt<std::string> InputFilename(cl::Positional,
                                   cl::desc("<filename>.bc"),
                                   cl::init(""));
cl::opt<bool> StdErrOption("stderr", llvm::cl::desc("Enable stderr output"));
cl::alias StdErrOptionShort("e", llvm::cl::aliasopt(StdErrOption));

int main(int argc, char **argv)
{
    LLVMContext &Context = getGlobalContext();
    SMDiagnostic Err;
    // Parse the command line to read the Inputfilename
    cl::ParseCommandLineOptions(argc, argv,
                                "FuncPtrPass \n LLVM Pass which uses point-to analysis.\n");

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
#if LLVM_VERSION_MAJOR >= 5
    Passes.add(new EnableFunctionOptPass());
#endif
    /// Transform it to SSA
    Passes.add(llvm::createPromoteMemoryToRegisterPass());

    /// Your pass to print Function and Call Instructions
    // Passes.add(new Liveness());
    Passes.add(new FuncPtrPass());
    Passes.run(*M.get());
}