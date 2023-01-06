#ifndef OBFUSCATION_FLATTENING_H
#define OBFUSCATION_FLATTENING_H

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/PassManager.h"

namespace flattening {
    struct FlatteningPass : public llvm::PassInfoMixin<FlatteningPass> {
        static llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
    };
} // namespace flattening

#endif //OBFUSCATION_FLATTENING_H
