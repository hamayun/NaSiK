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
#include "ExecutePacketList.h"
#include "BasicBlockList.h"

using namespace llvm;

//#define JUST_ADD_ISA

namespace native
{
    typedef std::map<uint32_t, llvm::Function *>           AddressTable_t;
    typedef std::map<uint32_t, llvm::Function *>::iterator AddressTable_Iterator_t;

    typedef std::map<uint32_t, llvm::AllocaInst *>           GenFuncInstrMap_t;
    typedef std::map<uint32_t, llvm::AllocaInst *>::iterator GenFuncInstrMap_Iterator_t;

    typedef std::list<string>                   FrequentFuncList_t;
    typedef std::list<string>::iterator         FrequentFuncList_Iterator_t;

    typedef enum LLVMCodeGenLevel
    {
        LLVM_CG_EP_LVL = 0,
        LLVM_CG_BB_LVL = 1
    } LLVMCodeGenLevel_t;

    /* Specifies the Bit Positions for Command-line Parsing */
    typedef enum LLVMCGOBitVector
    {
        LLVM_CGO_INLINE_BIT      = 0,
        LLVM_CGO_FUF_INLINE_BIT  = 1,
        LLVM_CGO_OPT_MOD_BIT     = 2,
        LLVM_CGO_OPT_FUN_BIT     = 3,
        LLVM_CGO_OPT_SPE_BIT     = 4
    } LLVMCGOBitVector_t;

    /* Specifies the Actuals Bits for Code Generation Options */
    typedef enum LLVMCodeGenOptions
    {
        LLVM_CGO_NONE        = 0x0,            // No Optimization
        LLVM_CGO_INLINE      = 0x1,            // Enable Inlining
        LLVM_CGO_FUF_INLINE  = 0x2,            // Enable Inlining of Frequently Used Functions
        LLVM_CGO_OPT_MOD     = 0x4,            // Enable Module Level Optimizations
        LLVM_CGO_OPT_FUN     = 0x8,            // Enable Function Level Optimizations
        LLVM_CGO_OPT_SPE     = 0x10            // Enable Special Optimization (If Any)
    } LLVMCodeGenOptions_t;

    // The class responsible for generating LLVM Code
    class LLVMGenerator
    {
    private:
        llvm::Module                   * p_module;           // The input IR module; contains the ISA Behavior
        llvm::LLVMContext              & m_context;
        llvm::IRBuilder<>                m_irbuilder;        // LLVM IR Builder Object
        llvm::Function *                 m_curr_function;    // The current function that we are building

        llvm::PassManager              * p_pm;
        llvm::FunctionPassManager      * p_fpm;              /* Would be suitable for Functions Generated at Runtime; if any */

        LLVMCodeGenLevel_t               m_code_gen_lvl;
        LLVMCodeGenOptions_t             m_code_gen_opt;
        // Add Function Calls for Execution Statistics Generation
        bool                             m_enable_exec_stats;
        // Enable the Basic Block Level Target Address to Host Function Pointer Mapping.
        bool                             m_bb_local_maps;
        GenFuncInstrMap_t                m_ureg_rval_map;
        GenFuncInstrMap_t                m_nfp_instr_map;

        FrequentFuncList_t   m_fuf_list;                      // List of Frequently Used Functions;

        tool_output_file *GetOutputStream(const char *FileName);

    public:
        const llvm::IntegerType * const i1;
        const llvm::IntegerType * const i8;
        const llvm::IntegerType * const i16;
        const llvm::IntegerType * const i32;
        const llvm::IntegerType * const iptr;

        llvm::Module            * p_gen_mod;         // The output module that contains the generated code
        llvm::Module            * p_addr_mod;        // The output module that contains the Addressing Table

        /* IMPORTANT:
         * We _MUST_ Delete the following object to Correctly Write the Bitstream File.
         * Or Use the llvm::OwningPtr<tool_output_file> * m_output_stream;
         */
        llvm::tool_output_file  * p_outs_gen_mod;      // The output bitcode file.
        llvm::tool_output_file  * p_outs_addr_mod;      // The output bitcode file.

        AddressTable_t            m_addr_table;

        llvm::PointerType         * p_proc_state_type;    // Pointer type to the Processor State. i.e. "Proc_State_t *"
        llvm::Value               * p_proc_state;         // Pointer to Processor State argument that is passed to the current function
        llvm::PointerType         * p_void_ptr_type;      // Void Pointer Type (Actually i8 *)
        llvm::FunctionType        * p_gen_func_type;      // Generated Function Type
        llvm::PointerType         * p_gen_func_ptr_type;  // Pointer to the Generated Function Type
        llvm::ConstantPointerNull * p_const_null_fptr;    // Const Null Function Pointer.

        llvm::Function          * p_update_pc;
        llvm::Function          * p_get_pc;
        llvm::Function          * p_set_pc;
        llvm::Function          * p_inc_cycles;
        llvm::Function          * p_get_cycles;

        const llvm::Type        * p_result_type;        // Type of the Instruction Result. i.e. "C62x_Result_t"
        llvm::PointerType       * p_result_type_ptr;    // Pointer type to the Instruction Result. i.e. "C62x_Result_t *"
        llvm::Function          * p_enq_result;
        llvm::Function          * p_update_immed;

        LLVMGenerator(string input_isa, LLVMCodeGenLevel_t code_gen_lvl,
                      LLVMCodeGenOptions_t code_gen_opt, bool enable_exec_stats, bool enable_locmaps);

        virtual llvm::Module *      GetModule()         { return (p_module); }
        virtual llvm::LLVMContext & GetContext() const  { return (m_context); }
        virtual llvm::IRBuilder<> & GetIRBuilder()      { return (m_irbuilder); }

        virtual void SetCurrentFunction(llvm::Function * curr_function) { m_curr_function = curr_function; }
        virtual llvm::Function * GetCurrentFunction() { return (m_curr_function); }

        virtual void SetLLVMCodeGenLevel(LLVMCodeGenLevel_t code_gen_lvl) { m_code_gen_lvl = code_gen_lvl; }
        virtual LLVMCodeGenLevel_t GetLLVMCodeGenLevel() { return (m_code_gen_lvl); }

        virtual void SetInliningAttribute(llvm::Function * func);
        virtual void SetupFUFList();
        virtual bool IsFUF(llvm::Function * func);

        virtual llvm::Value * Geti1Value (bool    value) { return(llvm::ConstantInt::get(i1,  value)); }
        virtual llvm::Value * Geti8Value (int8_t  value) { return(llvm::ConstantInt::get(i8,  value)); }
        virtual llvm::Value * Geti16Value(int16_t value) { return(llvm::ConstantInt::get(i16, value)); }
        virtual llvm::Value * Geti32Value(int32_t value) { return(llvm::ConstantInt::get(i32, value)); }

        virtual const llvm::Type * Geti1Type()  const { return (llvm::IntegerType::get(GetContext(),  1)); }
        virtual const llvm::Type * Geti8Type()  const { return (llvm::IntegerType::get(GetContext(),  8)); }
        virtual const llvm::Type * Geti16Type() const { return (llvm::IntegerType::get(GetContext(), 16)); }
        virtual const llvm::Type * Geti32Type() const { return (llvm::IntegerType::get(GetContext(), 32)); }

        virtual llvm::Value * CreateCallByName(string func_name);
        virtual void CreateCallByNameNoParams(string func_name);

        virtual uint32_t Gen_LLVM_Immed_or_Buff_Updates(ExecutePacket * exec_pkt,
                        llvm::AllocaInst * instr_results, llvm::BasicBlock * llvm_bb);
        virtual llvm::Value * GenerateLLVM_Exec_Packet(llvm::Function * function, ExecutePacket * exec_packet,
                        llvm::BasicBlock * llvm_bb_core, llvm::BasicBlock * llvm_bb_update, llvm::BasicBlock * llvm_bb_return);
        virtual int32_t GenerateLLVM_LocalMapping(const native::BasicBlockList_t * bb_list);

        virtual int32_t GenerateLLVM_EPLevel(ExecutePacket * exec_packet);   // Using Execute Packet Level Granularity
        virtual int32_t GenerateLLVM_BBLevel(native::BasicBlock * input_bb);         // Using Basic Block Level Granularity

        virtual int32_t VerifyGeneratedModule();

        virtual void    AddOptimizationPasses(unsigned OptLevel);
        virtual int32_t OptimizeModule();
        virtual int32_t OptimizeFunction(llvm::Function * func);

        virtual int32_t WriteAddressingTable(bool use_hash_maps, bool gen_text_table = false);
        virtual int32_t WriteBinaryConfigs(BinaryReader * reader);
        virtual int32_t WriteBitcodeFile();

        virtual ~LLVMGenerator() {}
    };
}
#endif // LLVM_GENERATOR_H
