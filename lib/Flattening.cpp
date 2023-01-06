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
        auto PA = PreservedAnalyses::all();

//        // save original bbs
//        std::vector<BasicBlock *> orgBB;
//        for (Function::iterator I = F.begin(); I != F.end(); I++)
//            orgBB.push_back(&*I);
//
//        if (orgBB.empty())
//            return PA;
//
//        BasicBlock *insert = &*F.begin(); // get ptr to the first basic block
//
//        // 1. Create dispatcher
//        int dispatcherVal = 0;
//        AllocaInst *dispatcher = new AllocaInst(Type::getInt32Ty(F.getContext()), 4, "dispatcher", insert);
//        StoreInst *store = new StoreInst(
//                ConstantInt::get(Type::getInt32Ty(F.getContext()), dispatcherVal),
//                dispatcher, insert);
//
//        // 2. create while loop
//        BasicBlock *loopEntry =  BasicBlock::Create(F.getContext(), "entry", &F, insert);
//        BasicBlock *loopExit =  BasicBlock::Create(F.getContext(), "exit", &F, insert);
//        LoadInst *load = new LoadInst(
//                Type::getInt32Ty(F.getContext()),
//                ConstantInt::get(Type::getInt32Ty(F.getContext()), dispatcherVal),
//                "dispatcher",
//                loopEntry);
//
//        BranchInst::Create(loopEntry, insert);
//        BranchInst::Create(loopEntry, loopExit);
//
//        insert->moveBefore(loopEntry);
//        BranchInst::Create(loopEntry, insert);
//
//        BasicBlock *swDefault = BasicBlock::Create(F.getContext());
//        BranchInst::Create(loopEntry, swDefault);

        std::vector<BasicBlock *> origBB;
        BasicBlock *loopEntry;
        BasicBlock *loopEnd;
        LoadInst *load;
        SwitchInst *switchI;
        AllocaInst *dispatcher;

        // Save all original BB
        for (Function::iterator i = F.begin(); i != F.end(); ++i) {
            BasicBlock *tmp = &*i;
            origBB.push_back(tmp);

//            BasicBlock *bb = &*i;
//            if (isa<InvokeInst>(bb->getTerminator())) {
//                return false;
//            }
        }

        // Nothing to flatten
        if (origBB.empty())
            return PA;

        // Remove first BB
        origBB.erase(origBB.begin());

        // Get a pointer on the first BB
        Function::iterator tmp = F.begin();  //++tmp;
        BasicBlock *insert = &*tmp;

        // If main begin with an if
        BranchInst *br = NULL;
        if (isa<BranchInst>(insert->getTerminator())) {
            br = cast<BranchInst>(insert->getTerminator());
        }

        if ((br != NULL && br->isConditional()) ||
            insert->getTerminator()->getNumSuccessors() > 1) {
            BasicBlock::iterator i = insert->end();
            --i;

            if (insert->size() > 1) {
                --i;
            }

            BasicBlock *tmpBB = insert->splitBasicBlock(i, "first");
            origBB.insert(origBB.begin(), tmpBB);
        }

        // Remove jump
        insert->getTerminator()->eraseFromParent();

        // Create switch variable and set as it
        dispatcher = new AllocaInst(Type::getInt32Ty(F.getContext()), 0, "dispatcher", insert);
        new StoreInst(ConstantInt::get(Type::getInt32Ty(F.getContext()), 0), dispatcher, insert);

        // Create main loop
        loopEntry = BasicBlock::Create(F.getContext(), "loopEntry", &F, insert);
        loopEnd = BasicBlock::Create(F.getContext(), "loopEnd", &F, insert);

        load = new LoadInst(
                Type::getInt32Ty(F.getContext()),
                ConstantInt::get(Type::getInt32Ty(F.getContext()), 0),
                "dispatcher",
                loopEntry);

        // Move first BB on top
        insert->moveBefore(loopEntry);
        BranchInst::Create(loopEntry, insert);

        // loopEnd jump to loopEntry
        BranchInst::Create(loopEntry, loopEnd);

        BasicBlock *swDefault =
                BasicBlock::Create(f->getContext(), "switchDefault", f, loopEnd);
        BranchInst::Create(loopEnd, swDefault);

        // Create switch instruction itself and set condition
        switchI = SwitchInst::Create(&*f->begin(), swDefault, 0, loopEntry);
        switchI->setCondition(load);

        // Remove branch jump from 1st BB and make a jump to the while
        f->begin()->getTerminator()->eraseFromParent();

        BranchInst::Create(loopEntry, &*F.begin());

        // Put all BB in the switch
        for (std::vector<BasicBlock *>::iterator b = origBB.begin(); b != origBB.end(); ++b) {
            BasicBlock *i = *b;
            ConstantInt *numCase = NULL;

            // Move the BB inside the switch (only visual, no code logic)
            i->moveBefore(loopEnd);

            // Add case to switch
            numCase = cast<ConstantInt>(
                    ConstantInt::get(switchI->getCondition()->getType(),switchI->getNumCases()));
            switchI->addCase(numCase, i);
        }

        // Recalculate switchVar
        for (std::vector<BasicBlock *>::iterator b = origBB.begin(); b != origBB.end(); ++b) {
            BasicBlock *i = *b;
            ConstantInt *numCase = NULL;

            // Ret BB
            if (i->getTerminator()->getNumSuccessors() == 0) {
                continue;
            }

            // If it's a non-conditional jump
            if (i->getTerminator()->getNumSuccessors() == 1) {
                // Get successor and delete terminator
                BasicBlock *succ = i->getTerminator()->getSuccessor(0);
                i->getTerminator()->eraseFromParent();

                // Get next case
                numCase = switchI->findCaseDest(succ);

                // If next case == default case (switchDefault)
                if (numCase == NULL)
                    numCase = cast<ConstantInt>(ConstantInt::get(switchI->getCondition()->getType(), switchI->getNumCases() - 1));

                // Update switchVar and jump to the end of loop
                new StoreInst(numCase, load->getPointerOperand(), i);
                BranchInst::Create(loopEnd, i);
                continue;
            }

            // If it's a conditional jump
            if (i->getTerminator()->getNumSuccessors() == 2) {
                // Get next cases
                ConstantInt *numCaseTrue =
                        switchI->findCaseDest(i->getTerminator()->getSuccessor(0));
                ConstantInt *numCaseFalse =
                        switchI->findCaseDest(i->getTerminator()->getSuccessor(1));

                // Check if next case == default case (switchDefault)
                if (numCaseTrue == NULL) {
                    numCaseTrue = cast<ConstantInt>(
                            ConstantInt::get(switchI->getCondition()->getType(), switchI->getNumCases() - 1));
                }

                if (numCaseFalse == NULL) {
                    numCaseFalse = cast<ConstantInt>(
                            ConstantInt::get(switchI->getCondition()->getType(), switchI->getNumCases() - 1));
                }

                // Create a SelectInst
                BranchInst *br = cast<BranchInst>(i->getTerminator());
                SelectInst *sel =
                        SelectInst::Create(br->getCondition(), numCaseTrue, numCaseFalse, "",
                                           i->getTerminator());

                // Erase terminator
                i->getTerminator()->eraseFromParent();

                // Update switchVar and jump to the end of loop
                new StoreInst(sel, load->getPointerOperand(), i);
                BranchInst::Create(loopEnd, i);
                continue;
            }
        }

        // fixStack(F);

        return PA;
    }
} // End of FlatteningPass implementation