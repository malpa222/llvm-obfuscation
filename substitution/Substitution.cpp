// USAGE:
//      opt -load-pass-plugin=substitution.so -passses=substitution `\`
//        -disable-output <input-llvm-file>
//

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
    struct Substitution : PassInfoMixin<Substitution> {
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
            errs() << "FName: " << F.getName() << "\n";

            return PreservedAnalyses::all();
        }

        static bool isRequired() { return true; }
    }; // end of struct Substitution
} // namespace

// register pass as a plugin
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "Substitution", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "substitution") {
                        FPM.addPass(Substitution());
                        return true;
                    }

                    return false;
                });
        }
    };
}