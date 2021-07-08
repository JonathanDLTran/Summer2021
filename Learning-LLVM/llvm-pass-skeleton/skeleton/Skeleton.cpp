#include <list>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
using namespace llvm;

namespace {
struct SkeletonPass : public FunctionPass {
    static char ID;
    SkeletonPass() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
        for (auto &B : F) {
            errs() << "Function body:\n" << F << "\n";

            std::map<Value *, bool> reg_use_map;
            for (auto &I : B) {
                if (auto *op = dyn_cast<StoreInst>(&I)) {
                    Value *use = op->getOperand(1);
                    auto loc = reg_use_map.find(use);
                    if (loc != reg_use_map.end()) {
                        return false;
                    }
                    reg_use_map.insert({use, true});
                }
            }

            std::list<std::tuple<int, int>> index_list;
            int curr_idx = 0;
            int start_idx = 0;
            int end_idx = 0;
            bool run_started = false;
            for (auto &I : B) {
                auto *op = dyn_cast<CallInst>(&I);
                if (run_started && op) {
                    index_list.push_back(std::make_tuple(start_idx, end_idx));
                    start_idx = 0;
                    end_idx = 0;
                    run_started = false;
                } else if (!run_started && op) {
                } else if (run_started && !op) {
                    end_idx++;
                } else {
                    start_idx = curr_idx;
                    end_idx = curr_idx;
                    run_started = true;
                }
                curr_idx++;
            }
            // print out list
            for (auto tup : index_list) {
                int start;
                int end;
                std::tie(start, end) = tup;
                errs() << start << " " << end << "\n";

                // Instruction *instr;
                int num_adds = 0;
                int counter = 0;
                for (auto &I : B) {
                    if (start <= counter && counter <= end) {
                        if (auto *op = dyn_cast<BinaryOperator>(&I)) {
                            if (I.getOpcode() == Instruction::Add) {
                                Value *lhs = op->getOperand(0);
                                Value *rhs = op->getOperand(1);
                                // if (num_adds == 0) {
                                //     instr = dyn_cast<Instruction>(&I);
                                // }
                                ++num_adds;
                            }
                        }
                    }
                    counter++;
                }
                if (num_adds == 0) {
                    continue;
                    // return false;
                }

                Instruction *instr;
                int idx = 0;
                counter = 0;
                for (auto &I : B) {
                    if (start <= counter && counter <= end) {
                        if (auto *op = dyn_cast<BinaryOperator>(&I)) {
                            if (I.getOpcode() == Instruction::Add) {
                                Value *lhs = op->getOperand(0);
                                Value *rhs = op->getOperand(1);
                                ++idx;
                                if (idx == num_adds) {
                                    instr = dyn_cast<Instruction>(&I);
                                    errs() << *instr << "\n";
                                }
                            }
                        }
                    }
                    counter++;
                }

                std::vector<BinaryOperator *> vec;

                Value *leftVector;
                Value *rightVector;

                IRBuilder<> builder(instr);
                // https://github.com/rolph-recto/cs6120-autovec/blob/master/skeleton/skeleton/Skeleton.cpp
                leftVector = builder.CreateVectorSplat(
                    num_adds, ConstantInt::get(
                                  Type::getInt32Ty(builder.getContext()), 0));
                rightVector = builder.CreateVectorSplat(
                    num_adds, ConstantInt::get(
                                  Type::getInt32Ty(builder.getContext()), 0));
                // https://github.com/rolph-recto/cs6120-autovec/blob/master/skeleton/skeleton/Skeleton.cpp

                int64_t count = 0;
                counter = 0;
                for (auto &I : B) {
                    if (start <= counter && counter <= end) {
                        if (auto *op = dyn_cast<BinaryOperator>(&I)) {
                            if (I.getOpcode() == Instruction::Add) {
                                Value *lhs = op->getOperand(0);
                                Value *rhs = op->getOperand(1);
                                leftVector = builder.CreateInsertElement(
                                    leftVector, lhs, count);
                                rightVector = builder.CreateInsertElement(
                                    rightVector, rhs, count);
                                vec.push_back(op);
                                ++count;
                            }
                        }
                    }
                    counter++;
                }

                Value *sumVector;
                sumVector = builder.CreateAdd(leftVector, rightVector);

                for (int64_t i = 0; i < count; i++) {
                    BinaryOperator *old_instr =
                        vec[i];  // old instruction would  be an add
                    // builder.SetInsertPoint(old_instr);
                    Value *extracted_sum = builder.CreateExtractElement(
                        sumVector, i);  // index into vector add
                    for (auto &U : old_instr->uses()) {
                        User *user = U.getUser();  // user of the add; could be
                                                   // a store, for example
                        user->setOperand(U.getOperandNo(), extracted_sum);
                        // if user is a store, move to end
                        if (auto *op = dyn_cast<StoreInst>(user)) {
                            Value *val = op->getOperand(0);
                            Value *ptr = op->getOperand(1);
                            builder.CreateStore(val, ptr);
                        }
                    }
                    // delete old instruction
                    old_instr->eraseFromParent();
                }
            }
        }

        errs() << "Function body:\n" << F << "\n";
        return true;
    }
};
}  // namespace

char SkeletonPass::ID = 0;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerSkeletonPass(const PassManagerBuilder &,
                                 legacy::PassManagerBase &PM) {
    PM.add(new SkeletonPass());
}
static RegisterStandardPasses RegisterMyPass(
    PassManagerBuilder::EP_EarlyAsPossible, registerSkeletonPass);
