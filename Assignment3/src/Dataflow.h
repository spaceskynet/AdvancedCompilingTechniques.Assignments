/************************************************************************
 *
 * @file Dataflow.h
 *
 * General dataflow framework
 *
 ***********************************************************************/

#pragma once

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>

#include <llvm/Support/raw_ostream.h>

#include <map>
#include <set>

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

///Base dataflow visitor class, defines the dataflow function

template <class T>
class DataflowVisitor
{
  public:
    virtual ~DataflowVisitor() { }

    /// Dataflow Function invoked for each basic block 
    /// 
    /// @block the Basic Block
    /// @dfval the input dataflow value
    /// @isforward true to compute dfval forward, otherwise backward
    virtual void compDFVal(BasicBlock *block, T *dfval, bool isforward) {
        if (isforward) {
           for (BasicBlock::iterator ii = block->begin(), ie = block->end(); 
                ii != ie; ++ii) {
                Instruction *inst = &*ii;
                compDFVal(inst, dfval);
           }
        } else {
           for (BasicBlock::reverse_iterator ii = block->rbegin(), ie = block->rend();
                ii != ie; ++ii) {
                Instruction *inst = &*ii;
                compDFVal(inst, dfval);
           }
        }
    }

    ///
    /// Dataflow Function invoked for each instruction
    ///
    /// @inst the Instruction
    /// @dfval the input dataflow value
    /// @return true if dfval changed
    virtual void compDFVal(Instruction *inst, T *dfval ) = 0;

    ///
    /// Merge of two dfvals, dest will be ther merged result
    /// @return true if dest changed
    ///
    virtual void merge(T *dest, const T &src ) = 0;
};

///
/// Dummy class to provide a typedef for the detailed result set
/// For each basicblock, we compute its input dataflow val and its output dataflow val
///
template<class T>
struct DataflowResult
{
    typedef typename std::map<BasicBlock *, std::pair<T, T> > Type;
};

/// 
/// Compute a forward iterated fixedpoint dataflow function, using a user-supplied
/// visitor function. Note that the caller must ensure that the function is
/// in fact a monotone function, as otherwise the fixedpoint may not terminate.
/// 
/// @param fn The function
/// @param visitor A function to compute dataflow vals
/// @param result The results of the dataflow 
/// @initval the Initial dataflow value
template<class T>
void compForwardDataflow(Function *fn,
    DataflowVisitor<T> *visitor,
    typename DataflowResult<T>::Type *result,
    const T & initval)
{
    std::set<BasicBlock *> worklist;

    for (Function::iterator bi = fn->begin(); bi != fn->end(); ++bi) {
        BasicBlock* bb = &*bi;
        result->insert(std::make_pair(bb, std::make_pair(initval, initval)));
        worklist.insert(bb);
    }

    while(!worklist.empty()) {
        BasicBlock* bb = *worklist.begin();
        worklist.erase(worklist.begin());

        T bbentryval = (*result)[bb].first;
        for (pred_iterator pi = pred_begin(bb), pe = pred_end(bb); pi != pe; ++pi) {
            BasicBlock *pred = *pi;
            visitor->merge(&bbentryval, (*result)[pred].second);
        }

        (*result)[bb].first = bbentryval;
        visitor->compDFVal(bb, &bbentryval, true);

        if (bbentryval == (*result)[bb].second) continue;
        (*result)[bb].second = bbentryval;

        for (succ_iterator si = succ_begin(bb), se = succ_end(bb); si != se; ++si) {
            worklist.insert(*si);
        }
    }

    return;
}
/// 
/// Compute a backward iterated fixedpoint dataflow function, using a user-supplied
/// visitor function. Note that the caller must ensure that the function is
/// in fact a monotone function, as otherwise the fixedpoint may not terminate.
/// 
/// @param fn The function
/// @param visitor A function to compute dataflow vals
/// @param result The results of the dataflow 
/// @initval The initial dataflow value
template<class T>
void compBackwardDataflow(Function *fn,
    DataflowVisitor<T> *visitor,
    typename DataflowResult<T>::Type *result,
    const T &initval)
{

    std::set<BasicBlock *> worklist;

    // Initialize the worklist with all exit blocks
    for (Function::iterator bi = fn->begin(); bi != fn->end(); ++bi) {
        BasicBlock * bb = &*bi;
        result->insert(std::make_pair(bb, std::make_pair(initval, initval)));
        worklist.insert(bb);
    }

    // Iteratively compute the dataflow result
    while (!worklist.empty()) {
        BasicBlock *bb = *worklist.begin();
        worklist.erase(worklist.begin());

        // Merge all incoming value
        T bbexitval = (*result)[bb].second;
        for (auto si = succ_begin(bb), se = succ_end(bb); si != se; si++) {
            BasicBlock *succ = *si;
            visitor->merge(&bbexitval, (*result)[succ].first);
        }

        (*result)[bb].second = bbexitval;
        visitor->compDFVal(bb, &bbexitval, false);

        // If outgoing value changed, propagate it along the CFG
        if (bbexitval == (*result)[bb].first) continue;
        (*result)[bb].first = bbexitval;

        for (pred_iterator pi = pred_begin(bb), pe = pred_end(bb); pi != pe; pi++) {
            worklist.insert(*pi);
        }
    }

    return;
}

template<class T>
void printDataflowResult(raw_ostream &out,
                         const typename DataflowResult<T>::Type &dfresult)
{
    for (typename DataflowResult<T>::Type::const_iterator it = dfresult.begin(), ie = dfresult.end();
         it != ie; ++it) 
    {
        if (it->first == nullptr) 
            out << "*";
        else 
            it->first->print(self::errs());
        out << "\n\tin : " 
            << it->second.first
            << "\n\tout :  " 
            << it->second.second << "\n";
    }
}