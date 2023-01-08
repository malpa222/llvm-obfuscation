#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "Flattening.h"

using namespace llvm;

// FlatteningPass
/*
 * 1. Count basic blocks -> N
 * 2. Create dispatcher variable = 0
 * 3. Create while loop (dispatcher < N)
 * 4. Create switch condition on dispatcher
 * 5. Put each basic block into a switch case
 * 6. Increment dispatcher in default (this will break the loop)
 */
namespace flattening {
    PreservedAnalyses FlatteningPass::run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM) {
        PreservedAnalyses PA = PreservedAnalyses::none();
        std::vector<BasicBlock *> orgBB;

        // Nothing to flatten
        if (F.empty())
            return PA;

        // Save all original BBs
        for (BasicBlock &BB : F)
            orgBB.push_back(&BB);

        // Remove first BB
        orgBB.erase(orgBB.begin()); // Remove first BB
        BasicBlock *first = &*F.begin(); // Get a pointer to the first BB

        // Remove jump
        first->getTerminator()->eraseFromParent();

        // Create dispatcher variable
        auto *dispatcher = new AllocaInst(Type::getInt32Ty(F.getContext()), 0, "dispatcher", first);
        new StoreInst(ConstantInt::get(Type::getInt32Ty(F.getContext()), 0), dispatcher, first);

        // Create main loop
        BasicBlock *loopEntry = BasicBlock::Create(F.getContext(), "loopEntry", &F, first);
        BasicBlock *loopEnd = BasicBlock::Create(F.getContext(), "loopEnd", &F, first);

        // load dispatcher to switchVar
        auto *switchVar = new LoadInst(
                Type::getInt32Ty(F.getContext()),
                dispatcher,
                "switchVar",
                loopEntry);

        // Move first BB on top
        first->moveBefore(loopEntry);
        BranchInst::Create(loopEntry, first);

        // loopEnd jump to loopEntry
        BranchInst::Create(loopEntry, loopEnd);

        BasicBlock *swDefault =
                BasicBlock::Create(F.getContext(), "switchDefault", &F, loopEnd);
        BranchInst::Create(loopEnd, swDefault);

        // Create switch instruction itself and set condition
        SwitchInst *switchI = SwitchInst::Create(&*F.begin(), swDefault, 0, loopEntry);
        switchI->setCondition(switchVar);

        // Remove branch jump from 1st BB and make a jump to the while
        F.begin()->getTerminator()->eraseFromParent();

        BranchInst::Create(loopEntry, &*F.begin());

        for (BasicBlock *BB : orgBB) {
            ConstantInt *numCase;

            // Move the BB inside the switch (only visual, no code logic)
            BB->moveBefore(loopEnd);

            // Add case to switch
            numCase = cast<ConstantInt>(ConstantInt::get(switchI->getCondition()->getType(),switchI->getNumCases()));
            switchI->addCase(numCase, BB);
        }

        // Recalculate switchVar
        for (BasicBlock *BB : orgBB) {
            ConstantInt *numCase;

            // Ret BB
            if (BB->getTerminator()->getNumSuccessors() == 0)
                continue;

            // If it's a non-conditional jump
            if (BB->getTerminator()->getNumSuccessors() == 1) {
                // Get successor and delete terminator
                BasicBlock *succ = BB->getTerminator()->getSuccessor(0);
                BB->getTerminator()->eraseFromParent();

                // Get next case
                numCase = switchI->findCaseDest(succ);

                // If next case == default case (switchDefault)
                if (numCase == nullptr)
                    numCase = cast<ConstantInt>(ConstantInt::get(switchI->getCondition()->getType(), switchI->getNumCases() - 1));

                // Update switchVar and jump to the end of loop
                new StoreInst(numCase, switchVar->getPointerOperand(), BB);
                BranchInst::Create(loopEnd, BB);
                continue;
            }

            // If it's a conditional jump
            if (BB->getTerminator()->getNumSuccessors() == 2) {
                // Get next cases
                ConstantInt *numCaseTrue =
                        switchI->findCaseDest(BB->getTerminator()->getSuccessor(0));
                ConstantInt *numCaseFalse =
                        switchI->findCaseDest(BB->getTerminator()->getSuccessor(1));

                // Check if next case == default case (switchDefault)
                if (numCaseTrue == nullptr) {
                    numCaseTrue = cast<ConstantInt>(
                            ConstantInt::get(switchI->getCondition()->getType(), switchI->getNumCases() - 1));
                }

                if (numCaseFalse == nullptr) {
                    numCaseFalse = cast<ConstantInt>(
                            ConstantInt::get(switchI->getCondition()->getType(), switchI->getNumCases() - 1));
                }

                // Create a SelectInst
                auto *branch = cast<BranchInst>(BB->getTerminator());
                auto *sel = SelectInst::Create(
                        branch->getCondition(),
                        numCaseTrue,
                        numCaseFalse,
                        "",
                        BB->getTerminator());

                // Erase terminator
                BB->getTerminator()->eraseFromParent();

                // Update switchVar and jump to the end of loop
                new StoreInst(sel, switchVar->getPointerOperand(), BB);
                BranchInst::Create(loopEnd, BB);
                continue;
            }
        }

        return PA;
    }
} // End of FlatteningPass implementation