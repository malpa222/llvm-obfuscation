#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "Flattening.h"

using namespace llvm;

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
            LLVM_PLUGIN_API_VERSION,
            "Control flow flattening",
            LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                // plugin transformation pass
                PB.registerPipelineParsingCallback(
                        [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                            if (Name == "flattening") {
                                FPM.addPass(flattening::FlatteningPass());
                                return true;
                            }

                            return false;
                        });

                // register the pass to run at any level
                PB.registerVectorizerStartEPCallback([](FunctionPassManager &FPM, OptimizationLevel OL) {
                    FPM.addPass(flattening::FlatteningPass());
                });
            }};
}
