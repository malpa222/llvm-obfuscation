#ifndef OBFUSCATION_SUBSTITUTION_H
#define OBFUSCATION_SUBSTITUTION_H

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/PassManager.h"

namespace substitution {
    struct SubstitutionPass : public llvm::PassInfoMixin<SubstitutionPass> {
        static llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
    };

    struct SubstitutionAnalysis : public llvm::AnalysisInfoMixin<SubstitutionAnalysis> {
        using Result = llvm::SmallVector<llvm::BinaryOperator *, 0>;
        Result run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);

        static llvm::AnalysisKey Key;
    };
} // namespace substitution

#endif //OBFUSCATION_SUBSTITUTION_H
