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
        PreservedAnalyses PA = PreservedAnalyses::none();
        std::vector<BasicBlock *> blocks;

        // Nothing to flatten
        if (F.empty())
            return PA;

        // Save all original BB
        for (BasicBlock &BB : F)
            blocks.push_back(&BB);

        // Set up the structure for flattening
        BasicBlock *first = &*F.begin(); // Get a pointer to the first BB
        BasicBlock *loopStart = BasicBlock::Create(F.getContext(), "loopStart", &F, first);
        BasicBlock *loopEnd = BasicBlock::Create(F.getContext(), "loopEnd", &F, first);
        BasicBlock *swDefault = BasicBlock::Create(F.getContext(), "switchDefault", &F, loopEnd);
        BasicBlock *switchBlock = BasicBlock::Create(F.getContext(), "switch", &F);
        BasicBlock *last = BasicBlock::Create(F.getContext(), "return", &F);

        auto bbIterator = first->begin();
        BasicBlock *case0 = first->splitBasicBlock(bbIterator, "case0");

        blocks.erase(blocks.begin()); // remove first bb
        first->getTerminator()->eraseFromParent(); // remove jump

        // Create dispatcher variable
        auto *dispatcher = new AllocaInst(Type::getInt32Ty(F.getContext()), 0, "dispatcher", first);
        new StoreInst(ConstantInt::get(Type::getInt32Ty(F.getContext()), 0), dispatcher, first);

        /*
         * Create do...while loop with condition dispatcher < numBlocks:
         * %isBigger = icmp ult ptr %dispatcher, %numBlocks
         * br i1 %isBigger, label %loopStart, label %return
         */
        auto *numBlocks = new AllocaInst(Type::getInt32Ty(F.getContext()), 0, "numBlocks", first);
        new StoreInst(ConstantInt::get(Type::getInt32Ty(F.getContext()), blocks.size()), numBlocks, first);
        auto icmp = new ICmpInst(
                *loopEnd,
                CmpInst::Predicate::ICMP_ULT,
                dispatcher,
                numBlocks,
                "isBigger");
        BranchInst::Create(loopStart, last, icmp, loopEnd);

        // Move first BB on top
        first->moveBefore(loopStart);
        BranchInst::Create(loopStart, first);

        /*
         * Create default switch case condition
         *
         * %dispatcher = load i32, ptr %dispatcher, align 4
         * %dispatcherIncremented = add nsw i32 %dispatcher, 1
         * store i32 %dispatcherIncremented, ptr %dispatcher, align 4
         * br label %loopEnd
         */
        auto tmp = new LoadInst(
                Type::getInt32Ty(F.getContext()),
                dispatcher,
                "tmp",
                swDefault);
        auto tmpIncremented = BinaryOperator::Create(
                Instruction::Add,
                tmp,
                ConstantInt::get(Type::getInt32Ty(F.getContext()), 1),
                "tmpIncremented",
                swDefault);
        new StoreInst(tmpIncremented, dispatcher, swDefault);
        BranchInst::Create(loopEnd, swDefault); // go to loopEnd

        // Create switch instruction itself and set condition
        switchBlock->moveAfter(loopStart);
        BranchInst::Create(switchBlock, loopStart);
        auto *switchVar = new LoadInst(
                Type::getInt32Ty(F.getContext()),
                dispatcher,
                "switchVar",
                switchBlock);
        SwitchInst *switchInst = SwitchInst::Create(&*F.begin(), swDefault, blocks.size(), switchBlock);
        switchInst->setCondition(switchVar);

        // Create cases
        for (BasicBlock *block : blocks) {
            ConstantInt *numCase;

            // Add case to switch
            numCase = cast<ConstantInt>(ConstantInt::get(
                    switchInst->getCondition()->getType(),
                    switchInst->getNumCases()));
            switchInst->addCase(numCase, block);

            block->moveAfter(switchBlock);
        }
        case0->moveAfter(switchBlock);

        // Add return statement to the last block
        ReturnInst::Create(F.getContext(), ConstantInt::get(Type::getInt32Ty(F.getContext()), 0), last);

        // Remove branch jump from 1st BB and make a jump to the while
//        F.begin()->getTerminator()->eraseFromParent();
//        BranchInst::Create(loopStart, &*F.begin());


        // Recalculate switchVar
//        for (BasicBlock *BB : blocks) {
//            ConstantInt *numCase;
//
//            // Ret BB
//            if (BB->getTerminator()->getNumSuccessors() == 0)
//                continue;
//
//            // If it's a non-conditional jump
//            if (BB->getTerminator()->getNumSuccessors() == 1) {
//                // Get successor and delete terminator
//                BasicBlock *succ = BB->getTerminator()->getSuccessor(0);
//                BB->getTerminator()->eraseFromParent();
//
//                // Get next case
//                numCase = switchInst->findCaseDest(succ);
//
//                // If next case == default case (switchDefault)
//                if (numCase == nullptr)
//                    numCase = cast<ConstantInt>(ConstantInt::get(switchInst->getCondition()->getType(), switchInst->getNumCases() - 1));
//
//                // Update switchVar and jump to the end of loop
//                new StoreInst(numCase, switchVar->getPointerOperand(), BB);
//                BranchInst::Create(loopEnd, BB);
//                continue;
//            }
//
//            // If it's a conditional jump
//            if (BB->getTerminator()->getNumSuccessors() == 2) {
//                // Get next cases
//                ConstantInt *numCaseTrue =
//                        switchInst->findCaseDest(BB->getTerminator()->getSuccessor(0));
//                ConstantInt *numCaseFalse =
//                        switchInst->findCaseDest(BB->getTerminator()->getSuccessor(1));
//
//                // Check if next case == default case (switchDefault)
//                if (numCaseTrue == nullptr) {
//                    numCaseTrue = cast<ConstantInt>(
//                            ConstantInt::get(switchInst->getCondition()->getType(), switchInst->getNumCases() - 1));
//                }
//
//                if (numCaseFalse == nullptr) {
//                    numCaseFalse = cast<ConstantInt>(
//                            ConstantInt::get(switchInst->getCondition()->getType(), switchInst->getNumCases() - 1));
//                }
//
//                // Create a SelectInst
//                auto *branch = cast<BranchInst>(BB->getTerminator());
//                auto *sel = SelectInst::Create(
//                        branch->getCondition(),
//                        numCaseTrue,
//                        numCaseFalse,
//                        "",
//                        BB->getTerminator());
//
//                // Erase terminator
//                BB->getTerminator()->eraseFromParent();
//
//                // Update switchVar and jump to the end of loop
//                new StoreInst(sel, switchVar->getPointerOperand(), BB);
//                BranchInst::Create(loopEnd, BB);
//                continue;
//            }
//        }

        return PA;
    }
} // End of FlatteningPass implementation