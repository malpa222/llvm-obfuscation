#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "Substitution.h"

using namespace llvm;

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
            LLVM_PLUGIN_API_VERSION,
            "Substitution",
            LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                // plugin the analysis pass
                PB.registerAnalysisRegistrationCallback([](FunctionAnalysisManager &FAM) {
                    FAM.registerPass([] {
                        return substitution::SubstitutionAnalysis(); });
                });

                // plugin transformation pass
                PB.registerPipelineParsingCallback(
                        [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                            if (Name == "sub") {
                                FPM.addPass(substitution::SubstitutionPass());
                                return true;
                            }

                            return false;
                        });

                // register the pass to run at any level
                PB.registerVectorizerStartEPCallback([](FunctionPassManager &FPM, OptimizationLevel OL) {
                    FPM.addPass(substitution::SubstitutionPass());
                });
            }};
}