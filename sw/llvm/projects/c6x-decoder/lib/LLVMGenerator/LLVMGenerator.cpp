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

#include "LLVMGenerator.h"
#include "BasicBlock.h"
#include "llvm/Analysis/CFGPrinter.h"

// TODO: Remove the following include and make it Target Independent
#include "C62xDecodedInstruction.h"

namespace native
{
    LLVMGenerator :: LLVMGenerator(string input_isa, LLVMCodeGenLevel_t code_gen_lvl,
        LLVMCodeGenOptions_t code_gen_opt, bool enable_exec_stats) :
        p_module(NULL), m_context(getGlobalContext()), m_irbuilder(m_context), m_curr_function(NULL),
        p_pm(NULL), p_fpm(NULL), m_code_gen_lvl(code_gen_lvl), m_code_gen_opt(code_gen_opt), m_enable_exec_stats(enable_exec_stats),
        m_bb_local_maps(true),
        i1(IntegerType::get(m_context, 1)), i8(IntegerType::get(m_context, 8)), i16(IntegerType::get(m_context, 16)),
        i32(IntegerType::get(m_context, 32)), iptr(IntegerType::get(m_context, 8 * sizeof(intptr_t))),
        p_outs_gen_mod(GetOutputStream("gen_code.bc")),
        p_outs_addr_mod(GetOutputStream("gen_addr.bc"))
    {
        string error;

        // Load in the bitcode file containing this module.
        if(MemoryBuffer* buffer = MemoryBuffer::getFile(input_isa, &error))
        {
            if (isBitcode((const unsigned char *)buffer->getBufferStart(),
               (const unsigned char *)buffer->getBufferEnd()))
            {
                SMDiagnostic Err;
                p_module = ParseBitcodeFile(buffer, m_context, &error);
                if (p_module == 0)
                    Err = SMDiagnostic(buffer->getBufferIdentifier(), error);
                // ParseBitcodeFile does not take ownership of the Buffer.
                delete buffer;
            }
            else
            {
                ASSERT(false, "The given file is not a valid bitcode file");
            }
        }
        else
        {
            ASSERT(false, "Opening Bitcode File");
        }

        // Setup Frequently Used Functions List
        SetupFUFList();

        /* Modify Function Linkage to be internal;
         * So as to make the library functions invisible at link time.
         * Only explicitly externalized functions should be available for linker.
         */
        for (llvm::Module::FunctionListType::iterator I = p_module->getFunctionList().begin(),
             E = p_module->getFunctionList().end(); I != E; ++I)
        {
            llvm::Function *F = cast<Function>(I);
            if (!I->isDeclaration())
            {
                F->setLinkage(GlobalValue::InternalLinkage);
                SetInliningAttribute(F);
            }
        }

        // Create a clone of the input module
        p_gen_mod   = CloneModule(p_module);

        p_addr_mod = new Module("gen_addr.bc", GetContext());
        p_addr_mod->setDataLayout(p_module->getDataLayout());
        p_addr_mod->setTargetTriple(p_module->getTargetTriple());

        // Create Module and Function Pass Managers.
        p_pm = new llvm::PassManager();
        p_fpm = new llvm::FunctionPassManager(p_gen_mod);

        p_pm->add(new TargetData(p_module));
        p_fpm->add(new TargetData(p_module));

        /* The Set of Passes that we Run on Each Module / Function */
        AddOptimizationPasses(3);

        // TODO: Remove the following Hard-Coded Processor State Type
        const Type * proc_state_type = p_gen_mod->getTypeByName("struct.C62x_DSPState_t");
        ASSERT(proc_state_type, "Could Not Get Processor State Type from Module");

        p_proc_state_type = llvm::PointerType::getUnqual(proc_state_type);
        ASSERT(proc_state_type, "Could Not Get Processor State Type Pointer");

        // Some common Processor State Manipulation Functions
        p_update_pc    = p_gen_mod->getFunction("Update_PC");           ASSERT(p_update_pc, "Could Not Get Update_PC");
        p_get_pc       = p_gen_mod->getFunction("Get_DSP_PC");          ASSERT(p_get_pc, "Could Not Get Get_DSP_PC");
        p_set_pc       = p_gen_mod->getFunction("Set_DSP_PC");          ASSERT(p_set_pc, "Could Not Get Set_DSP_PC");
        p_inc_cycles   = p_gen_mod->getFunction("Inc_DSP_Cycles");      ASSERT(p_inc_cycles, "Could Not Get Inc_DSP_Cycles");
        p_get_cycles   = p_gen_mod->getFunction("Get_DSP_Cycles");      ASSERT(p_get_cycles, "Could Not Get Get_DSP_Cycles");

        // Get the "C62x_Result_t" from our Module.
        p_result_type = p_gen_mod->getTypeByName("struct.C62x_Result_t");
        ASSERT(p_result_type, "Could Not Get Result Type from Module");
        //p_result_type->dump();

        p_result_type_ptr = llvm::PointerType::getUnqual(p_result_type);
        ASSERT(p_result_type_ptr, "Could Not Get Result Type Pointer");
        //p_result_type_ptr->dump();

        p_enq_result   = p_gen_mod->getFunction("EnQ_Delay_Result");    ASSERT(p_enq_result, "Could Not Get EnQ_Delay_Result");
        p_update_immed = p_gen_mod->getFunction("Update_Immediate");    ASSERT(p_update_immed, "Could Not Get Update_Immediate");
    }

    void LLVMGenerator :: SetInliningAttribute(llvm::Function * func)
    {
        bool should_inline = false;
        bool is_freq_func  = IsFUF(func);

        if(m_code_gen_opt & LLVM_CGO_INLINE)
            should_inline = true;

        if(is_freq_func && !(m_code_gen_opt & LLVM_CGO_FUF_INLINE))
            should_inline = false;

        if(should_inline)
            func->addFnAttr(llvm::Attribute::AlwaysInline);

        return;
    }

    void LLVMGenerator :: SetupFUFList()
    {
        m_fuf_list.push_back("Update_Registers");
        //m_fuf_list.push_back("Update_PC");
        //m_fuf_list.push_back("Inc_DSP_Cycles");
        return;
    }

    bool LLVMGenerator :: IsFUF(llvm::Function * func)
    {
        for(FrequentFuncList_Iterator_t FFLI = m_fuf_list.begin(),
            FFLE = m_fuf_list.end(); FFLI != FFLE; ++FFLI)
        {
            if(func->getNameStr().compare(*FFLI) == 0)
            {
                return (true);
            }
        }
        return(false);
    }

    tool_output_file * LLVMGenerator :: GetOutputStream(const char *FileName)
    {
      std::string error;

      tool_output_file *FDOut = new tool_output_file(FileName, error, raw_fd_ostream::F_Binary);
      if (!error.empty())
      {
        errs() << error << '\n';
        delete FDOut;
        return 0;
      }

      return FDOut;
    }

    llvm::Value * LLVMGenerator :: CreateCallByName(string func_name)
    {
        llvm::Function    * func_ptr     = NULL;
        llvm::Value       * func_value   = NULL;
        std::string         func_rname   = (func_name.size() > 4 ? func_name.substr(0, 4) : func_name);
        std::vector<llvm::Value*> args;

        args.push_back(p_proc_state);

        func_ptr = p_gen_mod->getFunction(StringRef(func_name));
        if(!func_ptr)
        {
          COUT << "Could not Get Function Call for: "  << func_name << endl;
          ASSERT(func_ptr != NULL, "Failed to Get Function Call");
        }

        INFO << "    Call to: " << func_name << "(...)" << endl;
        func_value = GetIRBuilder().CreateCall(func_ptr, args.begin(), args.end(), func_rname);

        return (func_value);
    }

    void LLVMGenerator :: CreateCallByNameNoParams(string func_name)
    {
        llvm::Function    * func_ptr     = NULL;
        std::vector<llvm::Value*> args;

        func_ptr = p_gen_mod->getFunction(StringRef(func_name));
        if(!func_ptr)
        {
          COUT << "Could not Get Function Call for: "  << func_name << endl;
          ASSERT(func_ptr != NULL, "Failed to Get Function Call");
        }

        INFO << "    Call to: " << func_name << "(...)" << endl;
        GetIRBuilder().CreateCall(func_ptr, args.begin(), args.end());

        return;
    }

    uint32_t LLVMGenerator :: Gen_LLVM_Immed_or_Buff_Updates(ExecutePacket * exec_pkt,
            llvm::AllocaInst * instr_results, llvm::BasicBlock * llvm_bb)
    {
        uint32_t             instr_index      = 0;
        Instruction        * instr            = NULL;
        DecodedInstruction * dec_instr        = NULL;
        llvm::IRBuilder<>  & irbuilder        = GetIRBuilder();

        irbuilder.SetInsertPoint(llvm_bb);

        // Now decide what to do with the results. Update Now or Put them in Buffer.
        // Caution No Updates for Store Instructions.
        exec_pkt->ResetInstrIterator();
        instr_index = 0;
        while((instr = exec_pkt->GetNextInstruction()))
        {
            uint32_t skip_enq_call = 0;

            dec_instr = instr->GetDecodedInstruction();
            ASSERT(dec_instr != NULL, "Instructions need to be Decoded First");

            // Get Pointer to the "C62x_Result_t *" type node in the generated function.
            Value * result = irbuilder.CreateGEP(instr_results, Geti32Value(instr_index));

            if(dec_instr->GetDelaySlots())
            {
                if(dec_instr->IsLoadStoreInstruction())
                {
                    // TODO: Make It independent of Target Arch.
                    C62xLDSTInstr * ldst_instr = (C62xLDSTInstr *) dec_instr;
                    if(ldst_instr->GetLoadStoreType() == STORE_INSTR)
                    {
                        skip_enq_call = 1;
                    }
                }

                if(!skip_enq_call)
                {
                    // Add to Delay Buffer; Will be Updated Later
                    irbuilder.CreateCall3(p_enq_result, p_proc_state, result, Geti8Value(dec_instr->GetDelaySlots()));
                }
            }
            else
            {
                // Immediate Update;
                irbuilder.CreateCall2(p_update_immed, p_proc_state, result);
            }
            instr_index++;
        }

        return (0);
    }

    llvm::Value * LLVMGenerator :: GenerateLLVM_Exec_Packet(llvm::Function * function, ExecutePacket * exec_packet,
                        llvm::BasicBlock * llvm_bb_core, llvm::BasicBlock * llvm_bb_update, llvm::BasicBlock * llvm_bb_return)
    {
        uint32_t             instr_index      = 0;
        llvm::Value        * instr_status     = NULL;
        Instruction        * instr            = NULL;
        DecodedInstruction * dec_instr        = NULL;
        llvm::IRBuilder<>  & irbuilder        = GetIRBuilder();
        ConstantInt     * const_int32_zero    = ConstantInt::get(GetContext(), APInt(32, StringRef("0"), 10));

        irbuilder.SetInsertPoint(llvm_bb_core);

        // Create a Variable for checking Early Exit Condition.
        llvm::AllocaInst * early_exit_cond = irbuilder.CreateAlloca(llvm::IntegerType::get(GetContext(), 32));
        early_exit_cond->setAlignment(8);
        irbuilder.CreateStore(const_int32_zero, early_exit_cond, false);        // Initialize with Zero

        // Create a Temporary Variable for Storing Function Return Value.
        llvm::AllocaInst * isa_func_rval = irbuilder.CreateAlloca(llvm::IntegerType::get(GetContext(), 32));
        isa_func_rval->setAlignment(8);

        // Create the "C62x_Result_t" typed result nodes before the beginning of this exec_packet code.
        llvm::AllocaInst * instr_results = irbuilder.CreateAlloca(p_result_type, Geti32Value(exec_packet->GetSize()), "instr_results");
        instr_results->setAlignment(8);
        //instr_results->dump();

        exec_packet->ResetInstrIterator();
        while((instr = exec_packet->GetNextInstruction()))
        {
            dec_instr = instr->GetDecodedInstruction();
            ASSERT(dec_instr != NULL, "Instructions need to be Decoded First");

            // Get Pointer to the corresponding "C62x_Result_t *" type node in the generated function.
            llvm::Value * result = irbuilder.CreateGEP(instr_results, Geti32Value(instr_index));
            //result->dump();

            // Get Pointer to the m_type in the above C62x_Result_t
            std::vector<Value*> index_vector;
            index_vector.push_back(const_int32_zero);
            index_vector.push_back(const_int32_zero);
            llvm::Value * result_type = irbuilder.CreateGEP(result, index_vector.begin(), index_vector.end());
            //result_type->dump();

            // Create a Store Instruction to m_type; So we Put Zero in Result Type before Calling the ISA
            irbuilder.CreateStore(const_int32_zero, result_type, false);

            instr_status = dec_instr->CreateLLVMFunctionCall(this, p_gen_mod, result);
            ASSERT(instr_status, "Error: In Creating Function Call");

            // Store the ISA Return Value in Temporary Stack Space
            irbuilder.CreateStore(instr_status, isa_func_rval, false);

            llvm::Value * instr_status_val = irbuilder.CreateLoad(isa_func_rval, false);
            llvm::Value * early_exit_val = irbuilder.CreateLoad(early_exit_cond, false);

            llvm::Value * early_exit_result = irbuilder.CreateOr(early_exit_val, instr_status_val);
            irbuilder.CreateStore(early_exit_result, early_exit_cond, false);

            instr_index++;
        }

        Gen_LLVM_Immed_or_Buff_Updates(exec_packet, instr_results, llvm_bb_core);

        // Create a Branch for Early Exit or Update Basic Blocks
        llvm::Value * early_exit_result = irbuilder.CreateLoad(early_exit_cond, false);
        llvm::Value * should_exit_early = irbuilder.CreateICmp(CmpInst::ICMP_NE, early_exit_result, Geti32Value(0));
        llvm::BranchInst::Create(llvm_bb_return, llvm_bb_update, should_exit_early, llvm_bb_core);

        // Now we Fill the Update Basic Block
        irbuilder.SetInsertPoint(llvm_bb_update);
        INFO << "    Call to: " << "Update_PC" << "(...)" << endl;
        irbuilder.CreateCall2(p_update_pc, p_proc_state, Geti32Value(exec_packet->GetSize() * 4));
        INFO << "    Call to: " << "Inc_DSP_Cycles" << "(...)" << endl;
        irbuilder.CreateCall(p_inc_cycles, p_proc_state);

        //CreateCallByName("Do_Memory_Writebacks");
        llvm::Value * update_regs_rval = CreateCallByName("Update_Registers"); INFO << endl;
        return (update_regs_rval);
    }

    int32_t LLVMGenerator :: GenerateLLVM_BBLevel(native::BasicBlock * basic_block)
    {
        string           function_name      = "Sim" + basic_block->GetName();
        llvm::IRBuilder<>  & irbuilder      = GetIRBuilder();

        const Type * return_type            = Type::getInt32Ty(GetContext());
        std::vector<const Type*> params;
        params.push_back(p_proc_state_type);

        llvm::FunctionType * func_type      = llvm::FunctionType::get(return_type, params, /*not vararg*/false);
        llvm::Function     * function       = llvm::Function::Create(func_type, Function::ExternalLinkage, function_name, p_gen_mod);
        ConstantInt      * const_int32_zero = ConstantInt::get(GetContext(), APInt(32, StringRef("0"), 10));

        INFO << "Function ... " << function_name << "(C62x_DSPState_t * p_state, ...)" << endl;
        SetCurrentFunction(function);

        // Here we get the Processor State Pointer from the Currently Generating Function.
        // The Processor State is passed to the Generated Function as a pointer.
        Function::arg_iterator curr_gen_func_args = function->arg_begin();
        p_proc_state = curr_gen_func_args++;          // We know that its the only single parameter; but increment anyways
        p_proc_state->setName("p_state");

        // Add Addressing Table Entry
        m_addr_table[basic_block->GetTargetAddress()] = function;

        uint32_t exec_pkt_count           = basic_block->GetSize();
        llvm::BasicBlock * llvm_bb_entry  = llvm::BasicBlock::Create(GetContext(), "BB_Entry", function);
        llvm::BasicBlock * llvm_bb_return = llvm::BasicBlock::Create(GetContext(), "BB_Return", function);

        if(m_enable_exec_stats)
        {
            GetIRBuilder().SetInsertPoint(llvm_bb_entry);
            CreateCallByNameNoParams("Inc_BB_Count");
        }

        if(m_bb_local_maps)     // If we have enabled the Local Target to Host Function Mapping Inside Basic Blocks
        {
            llvm::AllocaInst * next_func_ptr = new llvm::AllocaInst(Geti32Type(), "InitNFP");
            next_func_ptr->setAlignment(8);
            llvm_bb_entry->getInstList().push_back(next_func_ptr);
            // Initialize with NULL
            llvm_bb_entry->getInstList().push_back(new llvm::StoreInst(const_int32_zero, next_func_ptr, false));

            // In the Return BB; Load the Value from InitNFP and Return to Caller.
            llvm::GetElementPtrInst * ret_nfp_ptr = llvm::GetElementPtrInst::Create(next_func_ptr, Geti32Value(0));
            llvm_bb_return->getInstList().push_back(ret_nfp_ptr);
            llvm::LoadInst * ret_nfp = new llvm::LoadInst(ret_nfp_ptr, "RetNFP", false);
            llvm_bb_return->getInstList().push_back(ret_nfp);
            llvm::ReturnInst * ret_instr = llvm::ReturnInst::Create(GetContext(), ret_nfp);
            llvm_bb_return->getInstList().push_back(ret_instr);
        }
        else
        {
            llvm_bb_return->getInstList().push_back(llvm::ReturnInst::Create(GetContext(), const_int32_zero));
        }

        // Here We Create a Skeleton of our Simulation Function; Except for Entry/Return that were create above
        for(uint32_t index = 0; index < exec_pkt_count; index++)
        {
            native::ExecutePacket  * exec_pkt = basic_block->GetExecutePacketByIndex(index);
            string llvm_bb_core_name          = "BB_" + exec_pkt->GetName() + "_Core";
            string llvm_bb_update_name        = "BB_" + exec_pkt->GetName() + "_Update";

            llvm::BasicBlock::Create(GetContext(), llvm_bb_core_name, function);
            llvm::BasicBlock::Create(GetContext(), llvm_bb_update_name, function);
        }

        llvm::Function::iterator BBI = function->getBasicBlockList().begin();
        llvm::Function::iterator BBE = function->getBasicBlockList().end();

        // We skip the Entry and Return LLVM Basic Blocks.
        ++BBI; ++BBI;

        // Unconditional Branch from Entry to First EPs BB
        llvm_bb_entry->getInstList().push_back(llvm::BranchInst::Create(BBI));

        llvm::BasicBlock * prev_llvm_bb = NULL;         // The last llvm_bb for chaining
        llvm::Value        * pc_updated = NULL;
        for(uint32_t index = 0; ((BBI != BBE) && (index < exec_pkt_count)); index++)
        {
            llvm::BasicBlock * llvm_bb_core   = &(*BBI); ++BBI;
            llvm::BasicBlock * llvm_bb_update = &(*BBI); ++BBI;
            native::ExecutePacket  * exec_pkt = basic_block->GetExecutePacketByIndex(index);

            if(prev_llvm_bb)
            {
                irbuilder.SetInsertPoint(prev_llvm_bb);
                llvm::Value * pc_updated_flag = irbuilder.CreateICmp(CmpInst::ICMP_NE, pc_updated, Geti32Value(0));
                // A branch instruction in the previous update bb for either return bb or the current core bb
                llvm::BranchInst::Create(llvm_bb_return, llvm_bb_core, pc_updated_flag, prev_llvm_bb);
            }

            pc_updated = GenerateLLVM_Exec_Packet(function, exec_pkt, llvm_bb_core, llvm_bb_update, llvm_bb_return);
            if(!pc_updated)
            {
                COUT << "Error: Generating LLVM for ... " << exec_pkt->GetName() << endl;
                return (-1);
            }

            prev_llvm_bb = llvm_bb_update; // Save for Next Iteration Basic Block Chaining
        }

        // We create an unconditional branch instruction in the last update bb to return bb.
        prev_llvm_bb->getInstList().push_back(llvm::BranchInst::Create(llvm_bb_return));
        ASSERT(BBI == BBE, "Did not process all the LLVM Basic Blocks");

        //function->dump();
#if 0
        for (uint32_t index = 0; index < basic_block->GetSize(); index++)
        {
            ExecutePacket * exec_packet = basic_block->GetExecutePacketByIndex(index);
            if(exec_packet->GetPacketType() == NORMAL_EXEC_PACKET)
            {
                prev_bb = GenerateLLVM_BBLevel(function, exec_packet, prev_bb, return_bb);
                if(!prev_bb)
                {
                    COUT << "Error: Generating LLVM for ... " << exec_packet->GetName() << endl;
                    return (-1);
                }
            }
            else if (exec_packet->GetPacketType() == SPECIAL_EXEC_PACKET)
            {
                switch(exec_packet->GetSpecialFlags())
                {
                    case BRANCH_TAKEN:
                        //CreateCallByName("Print_DSP_State");
                        irbuilder.CreateRet(Geti32Value(0));
                        break;
                    case POSSIBLE_BRANCH:
                        /* Create instructions to check whether the PC has been
                         * updated in the previous cycle or not?
                         * And Create a Conditional Return Instruction */
                        break;
                }
            }
        }
#endif
        return (0);
    }

    int32_t LLVMGenerator :: GenerateLLVM_EPLevel(native::ExecutePacket * exec_pkt)
    {
        string           function_name      = "Sim" + exec_pkt->GetName();

        const Type * return_type            = Type::getInt32Ty(GetContext());
        std::vector<const Type*> params;
        params.push_back(p_proc_state_type);

        llvm::FunctionType * func_type      = llvm::FunctionType::get(return_type, params, /*not vararg*/false);
        llvm::Function     * function       = llvm::Function::Create(func_type, Function::ExternalLinkage, function_name, p_gen_mod);
        ConstantInt     * const_int32_zero  = ConstantInt::get(GetContext(), APInt(32, StringRef("0"), 10));

        INFO << "Function ... " << function_name << "(C62x_DSPState_t * p_state, ...)" << endl;
        SetCurrentFunction(function);

        // Here we get the Processor State Pointer from the Currently Generating Function.
        // The Processor State is passed to the Generated Function as a pointer.
        Function::arg_iterator curr_gen_func_args = function->arg_begin();
        p_proc_state = curr_gen_func_args++;          // We know that its the only single parameter; but increment anyways
        p_proc_state->setName("p_state");

        // Add Addressing Table Entry
        m_addr_table[exec_pkt->GetTargetAddress()] = function;

        llvm::BasicBlock * llvm_bb_entry  = llvm::BasicBlock::Create(GetContext(), "BB_Entry", function);
        llvm::BasicBlock * llvm_bb_return = llvm::BasicBlock::Create(GetContext(), "BB_Return", function);

        if(m_enable_exec_stats)
        {
            GetIRBuilder().SetInsertPoint(llvm_bb_entry);
            CreateCallByNameNoParams("Inc_EP_Count");
        }

        string llvm_bb_core_name          = "BB_" + exec_pkt->GetName() + "_Core";
        string llvm_bb_update_name        = "BB_" + exec_pkt->GetName() + "_Update";

        llvm::BasicBlock * llvm_bb_core   = llvm::BasicBlock::Create(GetContext(), llvm_bb_core_name, function);
        llvm::BasicBlock * llvm_bb_update = llvm::BasicBlock::Create(GetContext(), llvm_bb_update_name, function);

        // Unconditional Branch from Entry to First EPs BB
        llvm_bb_entry->getInstList().push_back(llvm::BranchInst::Create(llvm_bb_core));

        llvm::Value * pc_updated = GenerateLLVM_Exec_Packet(function, exec_pkt, llvm_bb_core, llvm_bb_update, llvm_bb_return);
        if(!pc_updated)
        {
            COUT << "Error: Generating LLVM for ... " << exec_pkt->GetName() << endl;
            return (-1);
        }

        // We create an unconditional branch instruction in the update bb to return bb.
        llvm_bb_update->getInstList().push_back(llvm::BranchInst::Create(llvm_bb_return));

        llvm_bb_return->getInstList().push_back(llvm::ReturnInst::Create(GetContext(), const_int32_zero));
        // TODO: We can return the reason to Dispatcher; If it needs to use it.
        //llvm_bb_return->getInstList().push_back(llvm::ReturnInst::Create(GetContext(), pc_updated));

        //function->dump();
        return (0);
    }

    int32_t LLVMGenerator :: VerifyGeneratedModule()
    {
        llvm::verifyModule(*p_gen_mod, PrintMessageAction);
        return (0);
    }

#if 0 // The following function creates something like this ...
    %struct.address_entry_t = type { i32, i32 (%struct.C62x_DSPState_t*)* }

    @AddressingTable = global [8 x %struct.address_entry_t] [
            %struct.address_entry_t { i32 0, i32 (%struct.C62x_DSPState_t*)* @SimEP_00000000 },
            %struct.address_entry_t { i32 4, i32 (%struct.C62x_DSPState_t*)* @SimEP_00000004 },
            %struct.address_entry_t { i32 8, i32 (%struct.C62x_DSPState_t*)* @SimEP_00000008 },
            %struct.address_entry_t { i32 12, i32 (%struct.C62x_DSPState_t*)* @SimEP_0000000c },
            %struct.address_entry_t { i32 16, i32 (%struct.C62x_DSPState_t*)* @SimEP_00000010 },
            %struct.address_entry_t { i32 20, i32 (%struct.C62x_DSPState_t*)* @SimEP_00000014 },
            %struct.address_entry_t { i32 24, i32 (%struct.C62x_DSPState_t*)* @SimEP_00000018 },
            %struct.address_entry_t { i32 28, i32 (%struct.C62x_DSPState_t*)* @SimEP_0000001c }], align 32
#endif

    int32_t LLVMGenerator :: WriteAddressingTable()
    {
        uint32_t         tablesize     = m_addr_table.size();
        llvm::Function * nativefptr    = m_addr_table.begin()->second;
        llvm::IRBuilder<>  & irbuilder = GetIRBuilder();

        // Type Definitions
        std::vector<const Type*> gen_struct_elems;
        gen_struct_elems.push_back(irbuilder.getInt32Ty());
        gen_struct_elems.push_back(nativefptr->getType());

        llvm::StructType * gen_addr_entry_ty = llvm::StructType::get(GetContext(), gen_struct_elems, false);
        ASSERT(gen_addr_entry_ty, "Failed to Create Address Entry Structure Type");
        //gen_addr_entry_ty->dump();

        llvm::ArrayType * gen_addr_table_ty = llvm::ArrayType::get(gen_addr_entry_ty, tablesize);
        ASSERT(gen_addr_table_ty, "Failed to Create Address Table Array Type");
        //gen_addr_table_ty->dump();

        GlobalVariable* gen_addr_table = new GlobalVariable(*p_addr_mod,
            /*Type=*/gen_addr_table_ty,
            /*isConstant=*/false,
            /*Linkage=*/GlobalValue::ExternalLinkage,
            /*Initializer=*/0, // has initializer, specified below
            /*Name=*/"AddressTable");
        gen_addr_table->setAlignment(32);

        Constant *const_addr_table_size = irbuilder.getInt32(tablesize);
        GlobalVariable* gen_addr_table_sz = new GlobalVariable(*p_addr_mod,
            /*Type=*/irbuilder.getInt32Ty(),
            /*isConstant=*/false,
            /*Linkage=*/GlobalValue::ExternalLinkage,
            /*Initializer=*/const_addr_table_size,
            /*Name=*/"AddressTableSize");
        gen_addr_table_sz->setAlignment(32);

        std::vector<Constant*> const_array_elems;

        for(AddressTable_Iterator_t ATI = m_addr_table.begin(), ATE = m_addr_table.end(); ATI != ATE; ++ATI)
        {
            ConstantInt* const_target_addr = ConstantInt::get(GetContext(), llvm::APInt(32, (*ATI).first));

            // Add External Function Declarations
            llvm::Function* func_ptr = (*ATI).second;
            llvm::Function* func_decl = Function::Create(
                /*Type=*/func_ptr->getFunctionType(),
                /*Linkage=*/GlobalValue::ExternalLinkage,
                /*Name=*/func_ptr->getNameStr(), p_addr_mod); // (external, no body)
            func_decl->setCallingConv(CallingConv::C);
            AttrListPtr func_decl_PAL;
            func_decl->setAttributes(func_decl_PAL);

            // Create Addressing Table Entry (Target Address, Native Function Pointer)
            std::vector<Constant*> const_struct_fields;
            const_struct_fields.push_back(const_target_addr);
            const_struct_fields.push_back(func_decl);

            Constant* const_gen_entry = ConstantStruct::get(gen_addr_entry_ty, const_struct_fields);
            const_array_elems.push_back(const_gen_entry);
        }

        // Now Actually Initialize the Addressing Table
        Constant* const_array = ConstantArray::get(gen_addr_table_ty, const_array_elems);
        gen_addr_table->setInitializer(const_array);

        return (0);
    }

    int32_t LLVMGenerator :: WriteBinaryConfigs(BinaryReader * reader)
    {
        llvm::IRBuilder<>  & irbuilder = GetIRBuilder();

        // Write the Startup Program Counter Address
        Constant * entry_point = irbuilder.getInt32(reader->GetEntryPoint());
        GlobalVariable* gen_entry_point = new GlobalVariable(*p_addr_mod,
            /*Type=*/irbuilder.getInt32Ty(),
            /*isConstant=*/true,
            /*Linkage=*/GlobalValue::ExternalLinkage,
            /*Initializer=*/entry_point,
            /*Name=*/"ENTRY_POINT_PC");
        gen_entry_point->setAlignment(32);

        Constant * exit_point = irbuilder.getInt32(reader->GetExitPoint());
        GlobalVariable* gen_exit_point = new GlobalVariable(*p_addr_mod,
            /*Type=*/irbuilder.getInt32Ty(),
            /*isConstant=*/true,
            /*Linkage=*/GlobalValue::ExternalLinkage,
            /*Initializer=*/exit_point,
            /*Name=*/"EXIT_POINT_PC");
        gen_exit_point->setAlignment(32);

        Constant * cioflush_point = irbuilder.getInt32(reader->GetCIOFlushPoint());
        GlobalVariable* gen_cioflush_point = new GlobalVariable(*p_addr_mod,
            /*Type=*/irbuilder.getInt32Ty(),
            /*isConstant=*/true,
            /*Linkage=*/GlobalValue::ExternalLinkage,
            /*Initializer=*/cioflush_point,
            /*Name=*/"CIOFLUSH_POINT_PC");
        gen_cioflush_point->setAlignment(32);

        Constant * ciobuff_addr = irbuilder.getInt32(reader->GetCIOBufferAddr());
        GlobalVariable* gen_ciobuff_addr = new GlobalVariable(*p_addr_mod,
            /*Type=*/irbuilder.getInt32Ty(),
            /*isConstant=*/true,
            /*Linkage=*/GlobalValue::ExternalLinkage,
            /*Initializer=*/ciobuff_addr,
            /*Name=*/"CIOBUFF_ADDR");
        gen_ciobuff_addr->setAlignment(32);

        return 0;
    }

    /// AddOptimizationPasses - This routine adds optimization passes
    /// based on selected optimization level, OptLevel. This routine
    /// duplicates llvm-gcc behaviour.
    ///
    /// OptLevel - Optimization Level
    void LLVMGenerator :: AddOptimizationPasses(unsigned OptLevel)
    {
        llvm::Pass *AlwaysInliningPass = createAlwaysInlinerPass();

        if(m_code_gen_opt & LLVM_CGO_OPT_MOD)
        {
            createStandardModulePasses(p_pm, OptLevel,
               /*OptimizeSize=*/ false, /*UnitAtATime,*/true,
               /*UnrollLoops=*/true, /*!DisableSimplifyLibCalls,*/true,
               /*HaveExceptions=*/ true, AlwaysInliningPass);
        }
        
        p_fpm->add(createCFGOnlyPrinterPass("_0"));

        p_fpm->add(createGVNPass());
        p_fpm->add(createInstructionCombiningPass());
        p_fpm->add(createCFGSimplificationPass());
        p_fpm->add(createDeadStoreEliminationPass());
    }

    int32_t LLVMGenerator :: OptimizeModule()
    {
        p_pm->run(*p_gen_mod);
        p_pm->run(*p_addr_mod);

        if(m_code_gen_opt & LLVM_CGO_OPT_FUN)
        {
            for(llvm::Module::iterator FI = p_gen_mod->getFunctionList().begin(),
                FIE = p_gen_mod->getFunctionList().end(); FI != FIE; FI++)
            {
                if(strncmp("Sim", FI->getNameStr().c_str(), 3) == 0)
                {
                    cout << "Optimizing Function ... " << FI->getNameStr() << endl;
                    OptimizeFunction(&(*FI));
                }
            }
        }

#if 0
            if(strncmp("SimEP_00000000", FI->getNameStr().c_str(), 14) == 0)
            {
                //FI->dump();
                for(llvm::Function::iterator BBI = FI->getBasicBlockList().begin(),
                    BBE = FI->getBasicBlockList().end(); BBI != BBE; BBI++)
                {
                    cout << "Basic Block:" << BBI->getNameStr() << endl;

                    for(llvm::BasicBlock::iterator II = BBI->getInstList().begin(),
                        IE = BBI->getInstList().end(); II != IE; II++)
                    {
                        //cout << "[Used " << II->getNumUses() << " Times]: ";
                        if(II->mayReadFromMemory() || II->mayWriteToMemory())
                        {
                            II->dump();
                        }
                    }
                }
            }
        }
#endif
        return (0);
    }

    int32_t LLVMGenerator :: OptimizeFunction(llvm::Function * func)
    {
        p_fpm->run(*func);
        return (0);
    }

    int32_t LLVMGenerator :: WriteBitcodeFile()
    {
        WriteBitcodeToFile(p_gen_mod, p_outs_gen_mod->os());
        p_outs_gen_mod->keep();
        delete p_gen_mod;
        delete p_outs_gen_mod;

        WriteBitcodeToFile(p_addr_mod, p_outs_addr_mod->os());
        p_outs_addr_mod->keep();
        delete p_addr_mod;
        delete p_outs_addr_mod;

        return (0);
    }
}
