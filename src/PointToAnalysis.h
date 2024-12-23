#pragma once

#include <llvm/IR/IntrinsicInst.h>

#include "Dataflow.h"

typedef std::map<int, std::set<std::string> > LineFunc;
typedef Value* ValuePtr;

class ValueSet : public std::set<ValuePtr>
{
  public:
    ValueSet() {this->clear();}
    ValueSet& operator = (const ValueSet &other) {
        if (this != &other) {
            this->clear();
            this->operator+=(other);
        }
        return *this;
    }
    ValueSet& operator = (const ValuePtr &val) {
        this->clear();
        this->operator+=(val);
        return *this;
    }
    ValueSet& operator += (const ValueSet &other) {
        this->insert(other.begin(), other.end());
        return *this;
    }
    ValueSet& operator += (const ValuePtr &val) {
        this->insert(val);
        return *this;
    }
};

class ValueMapSets : public std::map<ValuePtr, ValueSet>
{
  public:
    ValueMapSets() {this->clear();}
    ValueMapSets& operator = (const ValueMapSets &other) {
        if (this != &other) {
            this->clear();
            this->operator+=(other);
        }
        return *this;
    }
    ValueMapSets& operator += (const ValueMapSets &other) {
        for (const auto &item : other) {
            this->operator[](item.first) += item.second;
        }
        return *this;
    }
};

struct PointToInfo
{
  public:
    ValueMapSets pointToSet;
    ValueMapSets aliasSet;
    std::pair<ValueSet, ValueMapSets> *retSetsPtr;

    PointToInfo() : retSetsPtr(nullptr) {}
    PointToInfo(ValueMapSets &pointToSet, ValueMapSets &aliasSet, std::pair<ValueSet, ValueMapSets> &retSets) {
        this->pointToSet = pointToSet;
        this->aliasSet = aliasSet;
        this->retSetsPtr = &retSets;
    }

    PointToInfo& operator = (const PointToInfo &other) {
        if (this != &other) {
            this->pointToSet = other.pointToSet;
            this->aliasSet = other.aliasSet;
            this->retSetsPtr = other.retSetsPtr;
        }
        return *this;
    }

    bool operator == (const PointToInfo &info) const {
        return this->pointToSet == info.pointToSet && 
               this->aliasSet == info.aliasSet;
    }

    ValueSet pointTo(ValuePtr key) 
    {
        ValueSet ret;
        if (key == nullptr) return ret;
        if (Function *func = dyn_cast<Function>(key)) {
            this->pointToSet[key] += func;
        }
        ret = this->pointToSet[key];
        return ret;
    }

    ValueSet alias(const ValueSet &keys) 
    {
        ValueSet ret;
        if (keys.empty()) return ret;
        for (ValuePtr key : keys) {
            ret += this->aliasSet[key];
        }
        return ret;
    }

    void extend(const PointToInfo &info)
    {
        this->pointToSet += info.pointToSet;
        this->aliasSet += info.aliasSet;
    }

    void pointToExtend(const ValueSet &keys, const ValueSet &valueset)
    {
        for (ValuePtr key : keys) {
            this->pointToSet[key] += valueset;
        }
    }

    void aliasExtend(const ValueSet &keys, const ValueSet &valueset)
    {
        aliasClear(keys);
        for (ValuePtr key : keys) {
            this->aliasSet[key] += valueset;
        }
    }

    void aliasClear(const ValueSet &keys)
    {
        if (keys.size() != 1) return;
        for (ValuePtr key : keys) {
            if (isa<Function>(key)) continue;
            this->aliasSet[key].clear();
        }
    }
/*
    void aliasClear(const ValueMapSets &valuemapsets)
    {
        if (valuemapsets.size() != 1) return;
        for (const auto &item : valuemapsets) {
            auto &key = item.first;
            if (isa<Function>(key)) continue;
            this->aliasSet[key].clear();
        }
    }
*/
};

class PointToVisitor: public DataflowVisitor<struct PointToInfo>
{
  public:
    PointToVisitor() {}
    void merge(PointToInfo *dest, const PointToInfo &src) override 
    {
        dest->extend(src);
    }

    void compDFVal(Instruction *inst, PointToInfo *dfval) override 
    {
        if (isa<DbgInfoIntrinsic>(inst)) return;
        self::errs() << "Instruction (" << inst->getOpcodeName() << "): " << *inst << ".\n";
/*
        if (isa<BranchInst>(inst)) return; // br
        if (isa<ICmpInst>(inst)) return; // icmp
        if (isa<SExtInst>(inst)) return; // sext
        if (inst->isBinaryOp()) return; // add, sub
*/
        if (AllocaInst *allocainst = dyn_cast<AllocaInst>(inst)) {
            handleAllocaInst(allocainst, dfval);
        }
        else if (LoadInst *loadinst = dyn_cast<LoadInst>(inst)) {
            handleLoadInst(loadinst, dfval);
        }
        else if (StoreInst *storeinst = dyn_cast<StoreInst>(inst)) {
            handleStoreInst(storeinst, dfval);
        }
        else if (MemCpyInst *memcpyinst = dyn_cast<MemCpyInst>(inst)) {
            handleMemCpyInst(memcpyinst, dfval);
        }
        else if (MemSetInst *memsetinst = dyn_cast<MemSetInst>(inst)) {
            handleMemSetInst(memsetinst, dfval);
        }
        else if (PHINode *phinode = dyn_cast<PHINode>(inst)) {
            handlePHINode(phinode, dfval);
        }
        else if (GetElementPtrInst *getelemptrinst = dyn_cast<GetElementPtrInst>(inst)) {
            handleGetElementPtrInst(getelemptrinst, dfval);
        }
        else if (CallInst *callinst = dyn_cast<CallInst>(inst)) {
            handleCallInst(callinst, dfval);
        }
        else if (ReturnInst *returninst = dyn_cast<ReturnInst>(inst)) {
            handleReturnInst(returninst, dfval);
        }
        else if (BitCastInst *bitcastinst = dyn_cast<BitCastInst>(inst)) {
            handleBitCastInst(bitcastinst, dfval);
        }
/*
        else {
            self::errs() << "Unsupported Instruction (" << inst->getOpcodeName() << "): " << *inst << ".\n";
        }
*/
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

  private:
    LineFunc lineFuncList;

    void saveResult(Function *func, CallInst *callinst)
    {
        std::string funcName = func->getName().str();
        int lineno = callinst->getDebugLoc().getLine();

        lineFuncList[lineno].insert(funcName);
    }

    void handleAllocaInst(AllocaInst *allocainst, PointToInfo *dfval)
    {
        dfval->pointToSet[allocainst] = allocainst;
    }

    void handleLoadInst(LoadInst *loadinst, PointToInfo *dfval)
    {
        ValueSet ptrs = dfval->pointTo(loadinst->getPointerOperand());
        ValueSet vals = dfval->alias(ptrs);

        // Load 时需先清空指向集后再绑定
        dfval->pointToSet[loadinst] = vals;
    }

    void handleStoreInst(StoreInst *storeinst, PointToInfo *dfval)
    {
        ValueSet ptrs = dfval->pointTo(storeinst->getPointerOperand());
        ValueSet vals = dfval->pointTo(storeinst->getValueOperand());

        // Store 时需先清空别名后再重新绑定
        // dfval->aliasClear(ptrs);
        dfval->aliasExtend(ptrs, vals);
    }

    void handleMemCpyInst(MemCpyInst *memcpyinst, PointToInfo *dfval)
    {
        // 不能使用 getDest 和 getSrc，因为会去掉 CastInst 导致 dyn_cast 失败
        BitCastInst *destinst = dyn_cast<BitCastInst>(memcpyinst->getArgOperand(0));
        BitCastInst *srcinst = dyn_cast<BitCastInst>(memcpyinst->getArgOperand(1));
        if (!(destinst && srcinst)) return;

        ValueSet dests = dfval->pointTo(destinst->getOperand(0));
        ValueSet srcs = dfval->pointTo(srcinst->getOperand(0));

        ValueSet vals = dfval->alias(srcs);
        // MemCpy 时需先清空别名后再重新绑定
        // dfval->aliasClear(dests);
        dfval->aliasExtend(dests, vals);
    }

    void handleMemSetInst(MemSetInst *memsetinst, PointToInfo *dfval)
    {
        BitCastInst *destinst = dyn_cast<BitCastInst>(memsetinst->getArgOperand(0));
        if (destinst == nullptr) return;

        // 默认认为 memset 对应清空
        ValueSet dests = dfval->pointTo(destinst->getOperand(0));
        dfval->aliasClear(dests);
    }

    void handlePHINode(PHINode *phinode, PointToInfo *dfval)
    {
        dfval->pointToSet[phinode].clear();
        for (ValuePtr val : phinode->incoming_values()) {
            ValueSet vals = dfval->pointTo(val);
            dfval->pointToSet[phinode] += vals;
        }
    }

    void handleGetElementPtrInst(GetElementPtrInst *getelemptrinst, PointToInfo *dfval)
    {
        ValueSet ptrs = dfval->pointTo(getelemptrinst->getPointerOperand());
        dfval->pointToSet[getelemptrinst] = ptrs;
    }

    void handleCallInst(CallInst *callinst, PointToInfo *dfval)
    {
        ValuePtr val = callinst->getCalledOperand();
        ValueSet callees = dfval->pointTo(val);
        self::errs() << "CallInst Called Value name: " << val->getName() << ".\n";

        for (ValuePtr callee : callees)
        {
            if (Function *func = dyn_cast<Function>(callee)) {
                if (func->isIntrinsic()) continue;
                self::errs() << "#1 Callee function name: " << func->getName() << ".\n";

                if (func->isDeclaration()) {
                    dfval->pointToSet[callinst] = callinst;
                }
                saveResult(func, callinst);
            }
            else {
                self::errs() << "#1 Callee is not a function (" << callee->getValueName() << "): " << *callee << ".\n";
            }
        }

        std::vector<ValueSet> calleeArgsVec;
        ValueMapSets calleeAliasSet;

        int argNum = callinst->getNumArgOperands();
        for (int i = 0; i < argNum; ++i) {
            // 不能直接将 argi 作为 key
            Value *argi = callinst->getArgOperand(i);
            calleeArgsVec.push_back(dfval->pointTo(argi));
            self::errs() << "\tCallInst Arg[" << i << "]: " << *argi << ".\n";
        }

        bool flag = false;
        for (ValuePtr callee : callees)
        {
            if (Function *func = dyn_cast<Function>(callee)) {
                if (func->isIntrinsic()) continue;
                self::errs() << "#2 Callee function name: " << func->getName() << ".\n";
                if (func->isDeclaration()) continue;
                
                ValueMapSets calleeArgs;
                for (int i = 0; i < argNum; ++i) {
                    Argument *arg = func->getArg(i);
                    calleeArgs[arg] = calleeArgsVec[i];
                    self::errs() << "\t#2 Callee Arg[" << i << "]: " << *arg << ".\n";
                }

                DataflowResult<PointToInfo>::Type result;
                std::pair<ValueSet, ValueMapSets> retSets;
                PointToInfo initval(calleeArgs, dfval->aliasSet, retSets);
                
                compForwardDataflow(func, this, &result, initval);
                // printDataflowResult<PointToInfo>(self::errs(), result);

                dfval->pointToSet[callinst] += retSets.first;
                calleeAliasSet += retSets.second;
                flag = true;
            }
            else {
                self::errs() << "#2 Callee is not a function (" << callee->getValueName() << "): " << *callee << ".\n";
            }
        }

        if (flag) {
            // dfval->aliasClear(calleeAliasSet);
            dfval->aliasSet = calleeAliasSet;
        }
    }

    void handleReturnInst(ReturnInst *returninst, PointToInfo *dfval)
    {
        if (dfval->retSetsPtr == nullptr) return;
        ValueSet vals = dfval->pointTo(returninst->getReturnValue());
        dfval->retSetsPtr->first += vals;
        dfval->retSetsPtr->second += dfval->aliasSet;
    }

    void handleBitCastInst(BitCastInst *bitcastinst, PointToInfo *dfval)
    {
        ValueSet ptrs = dfval->pointTo(bitcastinst->getOperand(0));
        dfval->pointToSet[bitcastinst] = ptrs;
    }
};