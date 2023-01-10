#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "Flattening.h"

using namespace llvm;

// FlatteningPass
namespace flattening {
    PreservedAnalyses FlatteningPass::run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM) {
        std::vector<BasicBlock *> blocks;

        // Nothing to flatten
        if (F.empty())
            return PreservedAnalyses::all();

        // Save all original BB
        for (BasicBlock &b : F)
            blocks.push_back(&b);

        // Remove terminator from entry block and remove it from the list of blocks
        BasicBlock *first = &*F.begin();
        first->getTerminator()->eraseFromParent();
        blocks.erase(blocks.begin());

        auto *dispatcherPtr = new AllocaInst(Type::getInt32Ty(F.getContext()), 0, "dispatcherPtr", first);
        auto store = new StoreInst(ConstantInt::get(Type::getInt32Ty(F.getContext()), 0),dispatcherPtr,first);

        // Create the loop
        BasicBlock *loopStart = BasicBlock::Create(F.getContext(), "loopStart", &F, first);
        BasicBlock *loopEnd = BasicBlock::Create(F.getContext(), "loopEnd", &F, first);
        BranchInst::Create(loopStart, loopEnd);

        // Terminate the first block
        BranchInst::Create(loopStart, first);
        first->moveBefore(loopStart);

        // Create switch block
        BasicBlock *switchBlock = BasicBlock::Create(F.getContext(), "switch", &F);
        switchBlock->moveAfter(loopStart);

        // get the pointer to the dispatcher variable
        auto *dispatcher = new LoadInst(
                Type::getInt32Ty(F.getContext()),
                dispatcherPtr,
                "dispatcher",
                switchBlock);

        BranchInst::Create(switchBlock, loopStart);

        // Create a default statement
        BasicBlock *swDefault = BasicBlock::Create(F.getContext(), "switchDefault", &F, loopEnd);
        ReturnInst::Create(F.getContext(), ConstantInt::get(Type::getInt32Ty(F.getContext()), 0), swDefault);

        // Add the switch instruction to the switch block
        SwitchInst *switchInst = SwitchInst::Create(&*F.begin(), swDefault, blocks.size(), switchBlock);
        switchInst->setCondition(dispatcher);

        // Create cases
        for (BasicBlock *block : blocks) {
            switchInst->addCase(ConstantInt::get(Type::getInt32Ty(F.getContext()),switchInst->getNumCases()), block);
            block->moveAfter(switchBlock);
        }

        // Apply the control flow to cases
        for (BasicBlock *block : blocks) {
            if (block->getTerminator()->getNumSuccessors() == 1) { // regular block
                ConstantInt *nextCase = switchInst->findCaseDest(block->getTerminator()->getSuccessor(0));
                if (nextCase == nullptr)
                    nextCase = ConstantInt::get(Type::getInt32Ty(F.getContext()), switchInst->getNumCases() - 1);

                block->getTerminator()->eraseFromParent();
                new StoreInst(nextCase, store->getPointerOperand(), block);
                BranchInst::Create(loopEnd, block);
            } else if (block->getTerminator()->getNumSuccessors() == 2) { // conditional jump
                ConstantInt *nextTrue = switchInst->findCaseDest(block->getTerminator()->getSuccessor(0));
                ConstantInt *nextFalse = switchInst->findCaseDest(block->getTerminator()->getSuccessor(1));

                if (nextTrue == nullptr)
                    nextTrue = ConstantInt::get(Type::getInt32Ty(F.getContext()), switchInst->getNumCases() - 1);
                if (nextFalse == nullptr)
                    nextFalse = ConstantInt::get(Type::getInt32Ty(F.getContext()), switchInst->getNumCases() - 1);

                // Create a SelectInst
                BranchInst *br = cast<BranchInst>(block->getTerminator());
                SelectInst *sel = SelectInst::Create(
                        br->getCondition(),
                        nextTrue,
                        nextFalse,
                        "",
                        block->getTerminator());

                block->getTerminator()->eraseFromParent();
                new StoreInst(sel, store->getPointerOperand(), block);
                BranchInst::Create(loopEnd, block);
            }
        }

        return PreservedAnalyses::none();
    }
} // End of FlatteningPass implementation