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

        // Create dispatcher variable
        auto *dispatcher = new AllocaInst(Type::getInt32Ty(F.getContext()), 0, "dispatcher", first);
        new StoreInst(ConstantInt::get(Type::getInt32Ty(F.getContext()), 0), dispatcher, first);

        // Create while(true) loop that increments dispatcher with each iteration
        BasicBlock *loopStart = BasicBlock::Create(F.getContext(), "loopStart", &F, first);
        BasicBlock *loopEnd = BasicBlock::Create(F.getContext(), "loopEnd", &F, first);
        auto dispatcherTemp = new LoadInst(
                Type::getInt32Ty(F.getContext()),
                dispatcher,
                "dispatcherTemp",
                loopEnd);
        auto dispatcherTempIncremented = BinaryOperator::Create(
                Instruction::Add,
                dispatcherTemp,
                ConstantInt::get(Type::getInt32Ty(F.getContext()), 1),
                "dispatcherTempIncremented",
                loopEnd);
        new StoreInst(dispatcherTempIncremented, dispatcher, loopEnd);
        BranchInst::Create(loopStart, loopEnd);

        // Move first BB on top
        first->moveBefore(loopStart);
        BranchInst::Create(loopStart, first);

        // Create switch block
        BasicBlock *switchBlock = BasicBlock::Create(F.getContext(), "switch", &F);
        switchBlock->moveAfter(loopStart);
        BranchInst::Create(switchBlock, loopStart);
        auto *switchVar = new LoadInst(
                Type::getInt32Ty(F.getContext()),
                dispatcher,
                "switchVar",
                switchBlock);

        // Create a default statement
        BasicBlock *swDefault = BasicBlock::Create(F.getContext(), "switchDefault", &F, loopEnd);
        ReturnInst::Create(F.getContext(), ConstantInt::get(Type::getInt32Ty(F.getContext()), 0), swDefault);

        // Add the switch instruction to the switch block
        SwitchInst *switchInst = SwitchInst::Create(&*F.begin(), swDefault, blocks.size(), switchBlock);
        switchInst->setCondition(switchVar);

        // Create cases
        for (BasicBlock *block : blocks) {
            // Add case to switch
            switchInst->addCase(
                    ConstantInt::get(Type::getInt32Ty(F.getContext()),
                    switchInst->getNumCases()), block);

            block->moveAfter(switchBlock);
        }

        return PreservedAnalyses::none();
    }
} // End of FlatteningPass implementation