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

namespace native
{
    LLVMGenerator :: LLVMGenerator(string input_isa, LLVMCodeGenLevel_t code_gen_lvl,
        LLVMCodeGenOptions_t code_gen_opt, bool enable_exec_stats, bool enable_locmaps) :
        p_module(NULL), m_context(getGlobalContext()), m_irbuilder(m_context), m_curr_function(NULL),
        p_pm(NULL), p_fpm(NULL), m_code_gen_lvl(code_gen_lvl), m_code_gen_opt(code_gen_opt), m_enable_exec_stats(enable_exec_stats),
        m_bb_local_maps(enable_locmaps),
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

        p_void_ptr_type = llvm::PointerType::get(Geti8Type(), 0 /* Address Space */);
        ASSERT(p_void_ptr_type != NULL, "Error Creating Void Pointer Type");

        // Types for the Generated Simulation Functions
        if(m_bb_local_maps)
        {
            const Type * return_type = p_void_ptr_type;
            std::vector<const Type*> params;
            params.push_back(p_proc_state_type);

            p_gen_func_type = llvm::FunctionType::get(return_type, params, /*not vararg*/false);
            ASSERT(p_gen_func_type != NULL, "Error Creating Gen Function Type");
        }
        else
        {
            const Type * return_type = Geti32Type();
            std::vector<const Type*> params;
            params.push_back(p_proc_state_type);

            p_gen_func_type = llvm::FunctionType::get(return_type, params, /*not vararg*/false);
            ASSERT(p_gen_func_type != NULL, "Error Creating Gen Function Type");
        }

        p_gen_func_ptr_type = llvm::PointerType::get(p_gen_func_type, 0 /*Address Space*/);
        ASSERT(p_gen_func_ptr_type != NULL, "Error Creating Pointer to Gen Function Type");

        p_const_null_fptr = llvm::ConstantPointerNull::get(p_gen_func_ptr_type);
        ASSERT(p_const_null_fptr != NULL, "Error Creating Const Null Function Pointer");

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
            dec_instr = instr->GetDecodedInstruction();
            ASSERT(dec_instr != NULL, "Instructions need to be Decoded First");

            // Get Pointer to the "C62x_Result_t *" type node in the generated function.
            Value * result = irbuilder.CreateGEP(instr_results, Geti32Value(instr_index));

            if(dec_instr->GetDelaySlots())
            {
                if(!dec_instr->IsStoreInstr()){
                    // Add to Delay Buffer; Will be Updated Later
                    irbuilder.CreateCall3(p_enq_result, p_proc_state, result, Geti8Value(dec_instr->GetDelaySlots()));
                }

                if(dec_instr->IsLoadStoreInstruction()){
                    // We can have a possible side effect in case of load store instructions
                    irbuilder.CreateCall2(p_update_immed, p_proc_state, result);
                }
            }
            else {
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

        llvm::Function     * function       = llvm::Function::Create(p_gen_func_type, Function::ExternalLinkage, function_name, p_gen_mod);
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

        if(m_bb_local_maps)
        {
            // Allocate Space for Next Function Pointer & Initialize with NULL
            llvm::AllocaInst * next_func_ptr = new llvm::AllocaInst(p_gen_func_ptr_type, "NFP", llvm_bb_entry);
            next_func_ptr->setAlignment(8);
            new llvm::StoreInst(p_const_null_fptr, next_func_ptr, false, llvm_bb_entry);  // Initialize with NULL

            llvm::LoadInst * ret_nfp = new llvm::LoadInst(next_func_ptr, "RetNFP", false, llvm_bb_return);
            llvm::CastInst * ret_nfp_void_ptr = new llvm::BitCastInst(ret_nfp, p_void_ptr_type, "", llvm_bb_return);
            llvm::ReturnInst::Create(GetContext(), ret_nfp_void_ptr, llvm_bb_return);

            // Here We Create A Variable to Store the Return Value of Update Registers Function Calls
            llvm::AllocaInst * update_rval = new llvm::AllocaInst(Geti32Type(), "RetValUpReg", llvm_bb_entry);
            update_rval->setAlignment(8);
            new llvm::StoreInst(const_int32_zero, update_rval, false, llvm_bb_entry);  // Initialize with 0
            m_ureg_rval_map[basic_block->GetTargetAddress()] = update_rval;
            m_nfp_instr_map[basic_block->GetTargetAddress()] = next_func_ptr;
        }
        else
        {
            llvm::ReturnInst::Create(GetContext(), const_int32_zero, llvm_bb_return);
        }

        if(m_enable_exec_stats)
        {
            GetIRBuilder().SetInsertPoint(llvm_bb_entry);
            CreateCallByNameNoParams("Inc_BB_Count");
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

            if(m_bb_local_maps)
            {
                new llvm::StoreInst(pc_updated, m_ureg_rval_map[basic_block->GetTargetAddress()], false, llvm_bb_update);
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

    int32_t LLVMGenerator :: GenerateLLVM_LocalMapping(const native::BasicBlockList_t * bb_list)
    {
        if(!m_bb_local_maps || m_code_gen_lvl == LLVM_CG_EP_LVL)
        {
            WARN << "Local Mapping Generation is Disabled" << endl;
            return 0;
        }

        for(BasicBlockList_ConstIterator_t BBLCI = bb_list->begin(), BBLCE = bb_list->end(); BBLCI != BBLCE; ++BBLCI)
        {
            native::BasicBlock * basic_block = (*BBLCI);
            uint32_t exec_pkt_count          = basic_block->GetSize();
            llvm::Function        * gen_func = m_addr_table[basic_block->GetTargetAddress()];
            ConstantInt  * const_int32_zero  = ConstantInt::get(GetContext(), APInt(32, StringRef("0"), 10));
            ConstantInt  * const_int32_one   = ConstantInt::get(GetContext(), APInt(32, StringRef("1"), 10));

            llvm::Function::iterator BBI = gen_func->getBasicBlockList().begin();
            llvm::Function::iterator BBE = gen_func->getBasicBlockList().end();

            ++BBI;                                                      // The First BB is Entry; Skipped Here
            llvm::BasicBlock * llvm_bb_return = & (*BBI); ++BBI;        // The Second BB is Return

            // Here we Create the Local Mapping Table in the Entry Basic Block
            ConstOffsetsSet_t * br_offsets = basic_block->GetConstOffsetsSet();
            uint32_t          lmap_tblsize = br_offsets->size();

            std::vector<const Type*> lmap_struct_elems;
            lmap_struct_elems.push_back(Geti32Type());
            lmap_struct_elems.push_back(gen_func->getType());

            llvm::StructType * lmap_struct_type = llvm::StructType::get(GetContext(), lmap_struct_elems, false);
            ASSERT(lmap_struct_type, "Failed to Create Local Map Structure Type");  //lmap_struct_type->dump();

            llvm::ArrayType * lmap_array_type = llvm::ArrayType::get(lmap_struct_type, lmap_tblsize);
            ASSERT(lmap_array_type, "Failed to Create Local Map Array Type");       //lmap_array_type->dump();

            GlobalVariable* lmap_table = new GlobalVariable(* p_gen_mod,
                /*Type=*/lmap_array_type,
                /*isConstant=*/true,
                /*Linkage=*/GlobalValue::InternalLinkage,
                /*Initializer=*/0, // has initializer, specified below
                /*Name=*/ gen_func->getNameStr() + "_lMap");
            lmap_table->setAlignment(32);

            std::vector<Constant*> const_lmap_entries;
            for (ConstOffsetsSet_Iterator_t COSI = br_offsets->begin(), COSE = br_offsets->end(); COSI != COSE; ++COSI)
            {
                std::vector<Constant *> lmap_entry_vals;
                ConstantInt* const_target_addr = ConstantInt::get(GetContext(), llvm::APInt(32, *COSI));

                lmap_entry_vals.push_back(const_target_addr);
                lmap_entry_vals.push_back(m_addr_table[*COSI]);

                Constant * lmap_entry = ConstantStruct::get(lmap_struct_type, lmap_entry_vals);
                const_lmap_entries.push_back(lmap_entry);
            }
            // Now Actually Initialize the Local Mapping Table
            Constant* const_array = ConstantArray::get(lmap_array_type, const_lmap_entries);
            lmap_table->setInitializer(const_array);

            // Write the Size of Local Mapping Table; As global variables in this module ... :)
            // So that we don't have to create this table every time we execute this function.
            Constant * const_lmap_size = llvm::ConstantInt::get(Geti32Type(), lmap_tblsize);
            GlobalVariable* global_lmap_size = new GlobalVariable(* p_gen_mod,
                /*Type=*/ Geti32Type(),
                /*isConstant=*/ true,
                /*Linkage=*/ GlobalValue::InternalLinkage,
                /*Initializer=*/ const_lmap_size,
                /*Name=*/ gen_func->getNameStr() + "_lMap_Size");
            global_lmap_size->setAlignment(32);

            // We add the Local Mapping Address Checking Basic Blocks
            llvm::BasicBlock  * llvm_bb_msize = llvm::BasicBlock::Create(GetContext(), "BB_lMapSize", gen_func);
            llvm::BasicBlock  * llvm_bb_mtest = llvm::BasicBlock::Create(GetContext(), "BB_lMapTest", gen_func);
            llvm::BasicBlock  * llvm_bb_mload = llvm::BasicBlock::Create(GetContext(), "BB_lMapLoad", gen_func);
            llvm::BasicBlock * llvm_bb_incidx = llvm::BasicBlock::Create(GetContext(), "BB_InclMapIdx", gen_func);
            llvm::BasicBlock * llvm_bb_updnfp = llvm::BasicBlock::Create(GetContext(), "BB_UpdateNFP", gen_func);

            // Initialize the Map Size and Index
            llvm::LoadInst        * lmap_size = new llvm::LoadInst(global_lmap_size, "lMapSize", false, llvm_bb_msize);
            llvm::AllocaInst     * lmap_index = new llvm::AllocaInst(Geti32Type(), "lMapIndex", llvm_bb_msize);
            lmap_index->setAlignment(8);
            new llvm::StoreInst(const_int32_zero, lmap_index, false, llvm_bb_msize);  // Initialize with 0
            if(m_enable_exec_stats)
            {
                GetIRBuilder().SetInsertPoint(llvm_bb_msize);
                CreateCallByNameNoParams("Inc_LMAP_Search");
            }
            llvm::BranchInst::Create(llvm_bb_mtest, llvm_bb_msize);

            // Check the Current Index vs Local Mapping Size
            llvm::LoadInst * lmap_curr_idx = new llvm::LoadInst(lmap_index, "lmap_curr_idx", false, llvm_bb_mtest);
            llvm::ICmpInst * lmap_check = new llvm::ICmpInst(*llvm_bb_mtest, llvm::ICmpInst::ICMP_EQ, lmap_curr_idx, lmap_size, "idx_eq_size");
            llvm_bb_mtest->getInstList().push_back(llvm::BranchInst::Create(llvm_bb_return, llvm_bb_mload, lmap_check));

            // We load the Return Value of UpdateRegisters Call and the one from local mapping table
            std::vector<Value*> index_vector;
            index_vector.push_back(const_int32_zero);           // First Index for mapping table pointer
            index_vector.push_back(lmap_curr_idx);              // Second Index for the table entry
            index_vector.push_back(const_int32_zero);           // Third Index for the target address in table entry
            llvm::GetElementPtrInst * gep_lmap_index = llvm::GetElementPtrInst::Create(lmap_table, index_vector.begin(), index_vector.end(),
                                                        "lmap_target_addr", llvm_bb_mload);
            llvm::LoadInst * target_addr_val = new llvm::LoadInst(gep_lmap_index, "target_addr_val", false, llvm_bb_mload);
            llvm::LoadInst * upRegs_rval     = new llvm::LoadInst(m_ureg_rval_map[basic_block->GetTargetAddress()],
                                                                  "rvalURegs", false, llvm_bb_mload);
            llvm::ICmpInst * tgt_addr_check  = new llvm::ICmpInst(*llvm_bb_mload, llvm::ICmpInst::ICMP_EQ,
                                                                  target_addr_val, upRegs_rval, "tgt_eq_uregs");
            llvm::BranchInst::Create(llvm_bb_updnfp, llvm_bb_incidx, tgt_addr_check, llvm_bb_mload);

            // Increment Index Basic Block
            llvm::BinaryOperator* inc_index = BinaryOperator::Create(llvm::Instruction::Add, lmap_curr_idx, const_int32_one, "", llvm_bb_incidx);
            new llvm::StoreInst(inc_index, lmap_index, false, llvm_bb_incidx);
            if(m_enable_exec_stats)
            {
                GetIRBuilder().SetInsertPoint(llvm_bb_incidx);
                CreateCallByNameNoParams("Inc_LMAP_Extrloops");
            }
            llvm_bb_incidx->getInstList().push_back(llvm::BranchInst::Create(llvm_bb_mtest));

            // Update the Next Function Pointer
            index_vector.clear();
            index_vector.push_back(const_int32_zero);           // First Index for mapping table pointer
            index_vector.push_back(lmap_curr_idx);              // Second Index for the table entry
            index_vector.push_back(const_int32_one);            // Third Index for the native function pointer
            llvm::GetElementPtrInst * gep_lmap_funptr = llvm::GetElementPtrInst::Create(lmap_table, index_vector.begin(), index_vector.end(),
                                                                                        "lmap_target_addr", llvm_bb_updnfp);
            llvm::LoadInst            * native_funptr = new llvm::LoadInst(gep_lmap_funptr, "native_funptr", false, llvm_bb_updnfp);
            llvm::GetElementPtrInst   * nfp_ptr = llvm::GetElementPtrInst::Create(m_nfp_instr_map[basic_block->GetTargetAddress()],
                                                                                  Geti32Value(0), "", llvm_bb_updnfp);
            new llvm::StoreInst(native_funptr, nfp_ptr, false, llvm_bb_updnfp);
            if(m_enable_exec_stats)
            {
                GetIRBuilder().SetInsertPoint(llvm_bb_updnfp);
                CreateCallByNameNoParams("Inc_LMAP_Found");
            }
            llvm_bb_updnfp->getInstList().push_back(llvm::BranchInst::Create(llvm_bb_return));

            // Now we loop through the rest of Core and Update Basic Blocks.
            for(uint32_t index = 0; ((BBI != BBE) && (index < exec_pkt_count)); index++)
            {
                ++BBI;  // We Skip the Core Basic Block
                llvm::BasicBlock * llvm_bb_update = &(*BBI); ++BBI;

                llvm::TerminatorInst * t_instr = llvm_bb_update->getTerminator();
                for(llvm::TerminatorInst::op_iterator OI = t_instr->op_begin(), OE = t_instr->op_end(); OI != OE; ++OI)
                {
                    llvm::Value * opr_val =  (*OI).get();
                    if(opr_val == llvm_bb_return)
                    {
                        (*OI).set(llvm_bb_msize);
                    }
                }
            }
        }
        return 0;
    }

    int32_t LLVMGenerator :: GenerateLLVM_EPLevel(native::ExecutePacket * exec_pkt)
    {
        string           function_name      = "Sim" + exec_pkt->GetName();
        llvm::Function     * function       = llvm::Function::Create(p_gen_func_type, Function::ExternalLinkage, function_name, p_gen_mod);

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

        if(m_bb_local_maps)
        {
            // Allocate Space for Next Function Pointer & Initialize with NULL
            llvm::AllocaInst * next_func_ptr = new llvm::AllocaInst(p_gen_func_ptr_type, "NFP", llvm_bb_entry);
            next_func_ptr->setAlignment(8);
            new llvm::StoreInst(p_const_null_fptr, next_func_ptr, false, llvm_bb_entry);  // Initialize with NULL

            llvm::LoadInst * ret_nfp = new llvm::LoadInst(next_func_ptr, "RetNFP", false, llvm_bb_return);
            llvm::CastInst * ret_nfp_void_ptr = new llvm::BitCastInst(ret_nfp, p_void_ptr_type, "", llvm_bb_return);
            llvm::ReturnInst::Create(GetContext(), ret_nfp_void_ptr, llvm_bb_return);
        }
        else
        {
            ConstantInt * const_int32_zero = ConstantInt::get(GetContext(), APInt(32, StringRef("0"), 10));
            llvm::ReturnInst::Create(GetContext(), const_int32_zero, llvm_bb_return);
        }

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

        //function->dump();
        return (0);
    }

    int32_t LLVMGenerator :: VerifyGeneratedModule()
    {
        llvm::verifyModule(*p_gen_mod, PrintMessageAction);
        return (0);
    }

    int32_t LLVMGenerator :: WriteAddressingTable(bool use_hash_maps, bool gen_text_table)
    {
        uint32_t         tablesize     = m_addr_table.size();
        llvm::IRBuilder<>  & irbuilder = GetIRBuilder();
        ofstream        * p_text_table = NULL;
        uint32_t          null_entries = 0;

        if(use_hash_maps)
        {
            AddressTable_Iterator_t ATI = m_addr_table.end(); --ATI;
            cout << "Last Target Address: 0x" << hex << setfill('0') << setw(8) << (*ATI).first << endl;
            tablesize = ((*ATI).first >> 2) + 1;
        }

        if(gen_text_table)
        {
            p_text_table = new ofstream("GlobalAddrMap.txt", ios::out);
            if(!p_text_table->is_open())
            {
                DOUT << "Error: Creating Text Table File" << endl;
                return (EXIT_FAILURE);
            }

            (*p_text_table) << "Global Address Table Size: " << tablesize << endl;
        }

        // Type Definitions
        std::vector<const Type*> gen_struct_elems;
        gen_struct_elems.push_back(irbuilder.getInt32Ty());
        gen_struct_elems.push_back(p_gen_func_ptr_type);

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

        AddressTable_Iterator_t ATI = m_addr_table.begin();
        for(uint32_t index = 0; index < tablesize; index++)
        {
            if(use_hash_maps)
            {
                uint32_t target_addr = (*ATI).first;
                ConstantInt * const_target_addr = NULL;
                llvm::Function * func_decl = (llvm::Function *) p_const_null_fptr;

                if((target_addr >> 2) == index)
                {
                    const_target_addr = ConstantInt::get(GetContext(), llvm::APInt(32, (*ATI).first));

                    // Add External Function Declarations
                    llvm::Function* func_ptr = (*ATI).second;
                    func_decl = Function::Create(/*Type=*/func_ptr->getFunctionType(),
                        /*Linkage=*/GlobalValue::ExternalLinkage,/*Name=*/func_ptr->getNameStr(), p_addr_mod); // (external, no body)
                    func_decl->setCallingConv(CallingConv::C);
                    AttrListPtr func_decl_PAL;
                    func_decl->setAttributes(func_decl_PAL);
                    // Move to the Next Entry in Address Map
                    ATI++;

                    if(gen_text_table)
                    {
                        (*p_text_table) << "[" << dec << index << "]: 0x" << hex << setfill('0') << setw(8) << target_addr
                                        << " --> @" << func_ptr->getNameStr() << endl;
                    }
                }
                else
                {
                    const_target_addr = ConstantInt::get(GetContext(), llvm::APInt(32, 0));
                    if(gen_text_table)
                    {
                        (*p_text_table) << "[" << dec << index << "]: 0x0 --> NULL" << endl;
                    }
                    null_entries++;
                }

                // Create Addressing Table Entry (Target Address, Native Function Pointer)
                std::vector<Constant*> const_struct_fields;
                const_struct_fields.push_back(const_target_addr);
                const_struct_fields.push_back(func_decl);

                Constant* const_gen_entry = ConstantStruct::get(gen_addr_entry_ty, const_struct_fields);
                const_array_elems.push_back(const_gen_entry);
            }
            else
            {
                ConstantInt* const_target_addr = ConstantInt::get(GetContext(), llvm::APInt(32, (*ATI).first));

                // Add External Function Declarations
                llvm::Function* func_ptr = (*ATI).second;
                llvm::Function* func_decl = Function::Create(/*Type=*/func_ptr->getFunctionType(),
                    /*Linkage=*/GlobalValue::ExternalLinkage,/*Name=*/func_ptr->getNameStr(), p_addr_mod); // (external, no body)
                func_decl->setCallingConv(CallingConv::C);
                AttrListPtr func_decl_PAL;
                func_decl->setAttributes(func_decl_PAL);

                // Create Addressing Table Entry (Target Address, Native Function Pointer)
                std::vector<Constant*> const_struct_fields;
                const_struct_fields.push_back(const_target_addr);
                const_struct_fields.push_back(func_decl);

                Constant* const_gen_entry = ConstantStruct::get(gen_addr_entry_ty, const_struct_fields);
                const_array_elems.push_back(const_gen_entry);

                // Move to the Next Entry in Address Map
                ATI++;
                if(gen_text_table)
                {
                    (*p_text_table) << "[" << dec << index << "]: 0x" << hex << setfill('0') << setw(8) << (*ATI).first
                                    << " --> @" << func_ptr->getNameStr() << endl;
                }
            }
        }

        // Now Actually Initialize the Addressing Table
        Constant* const_array = ConstantArray::get(gen_addr_table_ty, const_array_elems);
        gen_addr_table->setInitializer(const_array);

        if(gen_text_table)
        {
            p_text_table->close();
            p_text_table = NULL;
        }

        if(use_hash_maps)
        {
            cout << "NULL Hash Entries  : " << dec << null_entries << ", Table Size: " << tablesize << " ["
                 << ((float)null_entries/tablesize) * 100 << "%]" << endl;
        }

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

        p_fpm->add(createCFGOnlyPrinterPass("_1"));
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
