#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"

#include "Substitution.h"

#define DEBUG_TYPE "substitution"
STATISTIC(NumAdd, "add substituted");
STATISTIC(NumSub, "sub substituted");

using namespace llvm;

// SubstitutionAnalysis
namespace substitution {
    using InstOp = Instruction::BinaryOps;

    // Initialize the analysis ID
    AnalysisKey SubstitutionAnalysis::Key;

    SubstitutionAnalysis::Result SubstitutionAnalysis::run(Function &F, FunctionAnalysisManager &FAM) {
        SmallVector<BinaryOperator*, 0> Insts;

        for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++) {
            if (!I->isBinaryOp())
                continue;

            // check if the instruction is either add or sub
            auto opcode = I->getOpcode();
            if (opcode == InstOp::Add || opcode == InstOp::Sub)
                Insts.push_back(cast<BinaryOperator>(&*I));
        }

        return Insts;
    }
} // End of SubstitutionAnalysis implementation

// SubstitutionPass
namespace substitution {
    // SubstitutionPass helper methods
    namespace {
        /*
            negate the second operator of the instruction, so that:
                add i32, %a, %b
            becomes:
                %neg = sub i32 0, %b
                sub i32 %a, %neg
        */
        void ReplaceAddInst(BinaryOperator *BO) {
            ++NumAdd;

            auto op = BinaryOperator::CreateNeg(
                    BO->getOperand(1),
                    "",
                    BO);

            op = BinaryOperator::Create(
                    Instruction::Sub,
                    BO->getOperand(0),
                    op,
                    "",
                    BO);

            BO->replaceAllUsesWith(op);
            // BO->eraseFromParent();
        }

        void ReplaceSubInst(BinaryOperator *BO) {
            ++NumSub;
        }
    }

    PreservedAnalyses SubstitutionPass::run(Function &F, FunctionAnalysisManager &FAM) {
        auto &AddInsts = FAM.getResult<SubstitutionAnalysis>(F);

        for (auto BO: AddInsts) {
            switch (BO->getOpcode()) {
                case InstOp::Add:
                    ReplaceAddInst(BO);
                    break;
                case InstOp::Sub:
                    ReplaceSubInst(BO);
                    break;
                default:
                    continue;
            }
        }

        auto PA = PreservedAnalyses::all();
        PA.abandon<SubstitutionAnalysis>();

        return PA;
    }
} // End of SubstitutionPass implementation
