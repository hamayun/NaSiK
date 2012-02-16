/*************************************************************************************
 * Copyright (C)    2011 TIMA Laboratory
 * Author(s) :      Mian Muhammad, HAMAYUN mian-muhammad.hamayun@imag.fr
 * Bug Fixer(s) :
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *************************************************************************************/

#ifndef LLVM_GENERATOR_H
#define LLVM_GENERATOR_H

#include "llvm/Pass.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/CallingConv.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/IRReader.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/StandardPasses.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetSelect.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "C62xCommon.h"
#include "ExecutePacket.h"

using namespace llvm;

#define FUNC_CALL_ERROR_CHECK
//#define INLINE_FUNCTIONS
#define OPTIMIZE_MODULE

namespace native
{
    // The class responsible for generating LLVM Code
    class LLVMGenerator
    {
    private:
        llvm::Module        *p_module;           // The input IR module; contains the ISA Behaviour
        llvm::LLVMContext   &m_context;
        llvm::IRBuilder<>    m_irbuilder;        // LLVM IR Builder Object
        llvm::Function *     m_curr_function;    // The current function that we are building

        llvm::PassManager          * p_pass_manager;
        llvm::FunctionPassManager  * p_func_pass_manager;       /* Would be suitable for Functions Generated at Runtime */

        tool_output_file *GetOutputStream(const char *FileName);
    public:
        const llvm::IntegerType * const i1;
        const llvm::IntegerType * const i8;
        const llvm::IntegerType * const i16;
        const llvm::IntegerType * const i32;
        const llvm::IntegerType * const iptr;

        llvm::Module                      * p_out_module;         // The output module that contains the generated code

        /* IMPORTANT:
         * We _MUST_ Delete the following object to Correctly Write the Bitstream File.
         * Or Use the llvm::OwningPtr<tool_output_file> * m_output_stream;
         */
        llvm::tool_output_file  * p_output_stream;      // The output bitcode file.

        llvm::PointerType       * p_proc_state_type;    // Pointer type to the Processor State. i.e. "Proc_State_t *"
        llvm::Value             * p_proc_state;         // Pointer to Processor State argument that is passed to the current function
        llvm::Function          * p_update_pc;
        llvm::Function          * p_get_pc;
        llvm::Function          * p_set_pc;
        llvm::Function          * p_inc_cycles;
        llvm::Function          * p_get_cycles;

        LLVMGenerator(string input_bcfile, string output_bcfile);

        virtual llvm::Module * GetModule() { return (p_module); }
        virtual llvm::LLVMContext & GetContext() { return (m_context); }
        virtual llvm::IRBuilder<> & GetIRBuilder() { return (m_irbuilder); }

        virtual void SetCurrentFunction(llvm::Function * curr_function) { m_curr_function = curr_function; }
        virtual llvm::Function * GetCurrentFunction() { return (m_curr_function); }

        virtual llvm::Value * Geti1Value (bool    value) { return(llvm::ConstantInt::get(i1,  value)); }
        virtual llvm::Value * Geti8Value (int8_t  value) { return(llvm::ConstantInt::get(i8,  value)); }
        virtual llvm::Value * Geti16Value(int16_t value) { return(llvm::ConstantInt::get(i16, value)); }
        virtual llvm::Value * Geti32Value(int32_t value) { return(llvm::ConstantInt::get(i32, value)); }

        virtual llvm::Value * CreateCallByName(string func_name);

        virtual int32_t GenerateLLVMEPLevel(ExecutePacket * exec_packet);       // Using Execute Packet Level Granularity

        virtual int32_t GenerateLLVMBBLevel(native::BasicBlock * input_bb);     // Using Basic Block Level Granularity
        virtual int32_t GenerateLLVMBBLevel(ExecutePacket * exec_packet);

        virtual void AddOptimizationPasses(unsigned OptLevel);
        virtual int32_t OptimizeModule();
        virtual int32_t OptimizeFunction(llvm::Function * func);
        virtual int32_t WriteBitcodeFile();

        virtual ~LLVMGenerator() {}
    };
}
#endif // LLVM_GENERATOR_H
