#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"

#include "Substitution.h"

using namespace llvm;

// SubstitutionAnalysis
namespace substitution {
    // Initialize the analysis ID
    AnalysisKey SubstitutionAnalysis::Key;

    SubstitutionAnalysis::Result SubstitutionAnalysis::run(Function &F, FunctionAnalysisManager &FAM) {
        SmallVector<BinaryOperator*, 0> Insts;
        auto attrs = F.getAttributes();

        errs() << "=============================" << "\n";
        errs() << "F_Name: " << F.getName() << "\n";

        for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++) {
            if (!I->isBinaryOp())
                continue;

            auto opcode = I->getOpcode();
            switch (opcode) {
                case Instruction::Add:
                    errs() << "Type: " << "Add" << "\n";
                    break;
                case Instruction::Sub:
                    errs() << "Type: " << "Sub" << "\n";
                    break;
            }

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
         *
         *  substitute addition, so that:
         *      add i32, %a, %b
         *
         *      -> a + b
         *  becomes:
         *      %neg = sub i32 0, %b
         *      sub i32 %a, %neg
         *
         *      -> a - (-b)
        */
        void ReplaceAddInst(BinaryOperator *BO) {
            auto op = BinaryOperator::CreateNeg(
                    BO->getOperand(1),
                    "",
                    BO);

            auto sub = BinaryOperator::Create(
                    Instruction::Sub,
                    BO->getOperand(0),
                    op,
                    "",
                    BO);

            BO->replaceAllUsesWith(sub);
            BO->eraseFromParent();
        }

        /*
         *  substitute subtraction, so that:
         *      sub i32, %a, %b
         *  becomes:
         *      %neg = sub i32 0, %b
         *      add i32 %a, %neg
        */
        void ReplaceSubInst(BinaryOperator *BO) {
            auto op = BinaryOperator::CreateNeg(
                    BO->getOperand(1),
                    "",
                    BO);

            op = BinaryOperator::Create(
                    Instruction::Add,
                    BO->getOperand(0),
                    op,
                    "",
                    BO);

            BO->replaceAllUsesWith(op);
            BO->eraseFromParent();
        }
    }

    PreservedAnalyses SubstitutionPass::run(Function &F, FunctionAnalysisManager &FAM) {
        auto &AddInsts = FAM.getResult<SubstitutionAnalysis>(F);

        for (auto BO: AddInsts) {
            switch (BO->getOpcode()) {
                case Instruction::Add:
                    ReplaceAddInst(BO);
                    break;
                case Instruction::Sub:
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
