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
// TODO: Remove the following include and make it Target Independent
#include "C62xDecodedInstruction.h"

namespace native
{
    LLVMGenerator :: LLVMGenerator(string input_bcfile, string output_bcfile, uint32_t addr_tbl_size) :
        p_module(NULL), m_context(getGlobalContext()), m_irbuilder(m_context), m_curr_function(NULL),
        p_pass_manager(NULL), p_func_pass_manager(NULL),
        i1(IntegerType::get(m_context, 1)), i8(IntegerType::get(m_context, 8)), i16(IntegerType::get(m_context, 16)),
        i32(IntegerType::get(m_context, 32)), iptr(IntegerType::get(m_context, 8 * sizeof(intptr_t))),
        p_outs_gen_mod(GetOutputStream(output_bcfile.c_str())),
        p_outs_addr_mod(GetOutputStream("gen_addr.bc"))
    {
        string error;

        // Load in the bitcode file containing this module.
        if(MemoryBuffer* buffer = MemoryBuffer::getFile(input_bcfile, &error))
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

        /* Modify Function Linkage to be internal;
         * So as to make the library functions invisible at link time.
         * Only explicitly externalized functions should be available for linker.
         */
        for (llvm::Module::FunctionListType::iterator I = p_module->getFunctionList().begin(),
             E = p_module->getFunctionList().end(); I != E; ++I)
        {
            Function *F = cast<Function>(I);
            if (!I->isDeclaration())
            {
                F->setLinkage(GlobalValue::InternalLinkage);
                F->addFnAttr(llvm::Attribute::AlwaysInline);
            }
        }

        // Create a clone of the input module
        p_gen_mod   = CloneModule(p_module);

        //p_addr_mod  = CloneModule(p_module);
        p_addr_mod = new Module("gen_addresses.bc", GetContext());
        p_addr_mod->setDataLayout(p_module->getDataLayout());
        p_addr_mod->setTargetTriple(p_module->getTargetTriple());

        // Create the Addressing Table object
        p_addr_table = new native::AddressingTable(addr_tbl_size);

        // Create Module and Function Pass Managers.
        p_pass_manager = new llvm::PassManager();
        p_func_pass_manager = new llvm::FunctionPassManager(p_gen_mod);

        p_pass_manager->add(new TargetData(p_module));
        p_func_pass_manager->add(new TargetData(p_module));

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

#if 0
    int32_t LLVMGenerator :: GenerateLLVM_BBLevel(llvm::Function * function,
        ExecutePacket * exec_packet, llvm::BasicBlock * next_bb, llvm::BasicBlock * return_bb)
    {
        Instruction        * instr         = NULL;
        DecodedInstruction * dec_instr     = NULL;
        llvm::Value        * updated_pc    = NULL;
        llvm::IRBuilder<>  & irbuilder     = GetIRBuilder();
        uint32_t             instr_index   = 0;
        string               main_bbname   = "BB_Core_" + exec_packet->GetName();
        string               update_bbname = "BB_Update_" + exec_packet->GetName();

        llvm::BasicBlock         * main_bb = llvm::BasicBlock::Create(GetContext(), main_bbname, function);
        llvm::BasicBlock       * update_bb = llvm::BasicBlock::Create(GetContext(), update_bbname, function);
        ConstantInt     * const_int32_zero = ConstantInt::get(GetContext(), APInt(32, StringRef("0"), 10));
        llvm::Value         * func_ret_val = new llvm::Value[exec_packet->GetSize()];
        llvm::Value     * should_exit_early, * should_exit_func;

        irbuilder.SetInsertPoint(main_bb);

        // Create the "C62x_Result_t" typed result nodes before the beginning of this exec_packet code.
        llvm::AllocaInst * instr_results = irbuilder.CreateAlloca(p_result_type, Geti32Value(exec_packet->GetSize()), "instr_results");
        instr_results->setAlignment(8);
        //instr_results->dump();

        exec_packet->ResetInstrIterator();
        m_earlyexit_bb_flag = 0;

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

            func_ret_val[instr_index] = dec_instr->CreateLLVMFunctionCall(this, p_gen_mod, return_bb, result);
            ASSERT(func_ret_val[instr_index], "Error: In Creating Function Call");

            instr_index++;
        }

        // Now decide what to do with the results. Update Now or Put them in Buffer.
        // Caution No Updates for Store Instructions.
        exec_packet->ResetInstrIterator();
        instr_index = 0;
        while((instr = exec_packet->GetNextInstruction()))
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


            // Check if this instruction updated the PC
            should_exit_early = irbuilder.CreateOr(func_ret_val[instr_index], should_exit_early);
            instr_index++;
        }

        llvm::BranchInst::Create(return_bb, update_bb, should_exit_early, main_bb);

        // Now we Fill the Update BB
        irbuilder.SetInsertPoint(update_bb);

        INFO << "    Call to: " << "Update_PC" << "(...)" << endl;
        irbuilder.CreateCall2(p_update_pc, p_proc_state, Geti32Value(exec_packet->GetSize() * 4));
        INFO << "    Call to: " << "Inc_DSP_Cycles" << "(...)" << endl;
        irbuilder.CreateCall(p_inc_cycles, p_proc_state);
        //CreateCallByName("Do_Memory_Writebacks");
        updated_pc = CreateCallByName("Update_Registers"); INFO << endl;
        should_exit_func = irbuilder.CreateOr(updated_pc, const_int32_zero);
        llvm::BranchInst::Create(return_bb, )
        
        return (updated_pc);
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
        llvm::BasicBlock   * entry_bb       = llvm::BasicBlock::Create(GetContext(), "EntryBB", function);
        llvm::BasicBlock   * return_bb      = llvm::BasicBlock::Create(GetContext(), "ReturnBB", function);
        llvm::BasicBlock   * next_bb        = NULL;

        INFO << "Function ... " << function_name << "(C62x_DSPState_t * p_state, ...)" << endl;

        // Add Addressing Table Entry
        p_addr_table->AddAddressEntry(basic_block->GetTargetAddress(), function);

        SetCurrentFunction(function);

        // Here we get the Processor State Pointer from the Currently Generating Function.
        // The Processor State is passed to the Generated Function as a pointer.
        Function::arg_iterator curr_gen_func_args = function->arg_begin();
        p_proc_state = curr_gen_func_args++;          // We know that its the only single parameter; but increment anyways
        p_proc_state->setName("p_state");

        prev_bb = return_bb;
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

        return (0);
    }
#endif

    int32_t LLVMGenerator :: GenerateLLVM_EPLevel(ExecutePacket * exec_packet)
    {
        Instruction        * instr         = NULL;
        DecodedInstruction * dec_instr     = NULL;
        llvm::Value        * updated_pc    = NULL;
        string               function_name = "Sim" + exec_packet->GetName();
        llvm::IRBuilder<>  & irbuilder     = GetIRBuilder();
        uint32_t             instr_index   = 0;

        const Type * return_type           = Type::getInt32Ty(GetContext());
        std::vector<const Type*> params;
        params.push_back(p_proc_state_type);

        llvm::FunctionType * func_type      = llvm::FunctionType::get(return_type, params, /*not vararg*/false);
        llvm::Function     * function       = llvm::Function::Create(func_type, Function::ExternalLinkage, function_name, p_gen_mod);
        llvm::BasicBlock   * entry_bb       = llvm::BasicBlock::Create(GetContext(), "EntryBB", function);
        llvm::BasicBlock   * update_exit_bb = llvm::BasicBlock::Create(GetContext(), "UpdateExitBB", function);
        ConstantInt        * const_int32_zero = ConstantInt::get(GetContext(), APInt(32, StringRef("0"), 10));

        INFO << "Function ... " << function_name << "(C62x_DSPState_t * p_state, ...)" << endl;

        // Add Addressing Table Entry
        p_addr_table->AddAddressEntry(exec_packet->GetTargetAddress(), function);

        SetCurrentFunction(function);
        // Here we get the Processor State Pointer from the Currently Generating Function.
        // The Processor State is passed to the Generated Function as a pointer.
        Function::arg_iterator curr_gen_func_args = function->arg_begin();
        p_proc_state = curr_gen_func_args++;          // We know that its the only single parameter; but increment anyways
        p_proc_state->setName("p_state");

        irbuilder.SetInsertPoint(entry_bb);

        // Create the "C62x_Result_t" typed result nodes in this function.
        llvm::AllocaInst * instr_results = irbuilder.CreateAlloca(p_result_type, Geti32Value(exec_packet->GetSize()), "instr_results");
        instr_results->setAlignment(8);
        //instr_results->dump();

        exec_packet->ResetInstrIterator();
        m_earlyexit_bb_flag = 0;

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

            Value * func_value = dec_instr->CreateLLVMFunctionCall(this, p_gen_mod, update_exit_bb, result);
            ASSERT(func_value, "Error: In Creating Function Call");

            if(m_earlyexit_bb_flag)
            {
                irbuilder.SetInsertPoint(update_exit_bb);
            }

            instr_index++;
        }

        // Now decide what to do with the results. Update Now or Put them in Buffer.
        // Caution No Updates for Store Instructions.
        exec_packet->ResetInstrIterator();
        instr_index = 0;
        while((instr = exec_packet->GetNextInstruction()))
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

        if(!m_earlyexit_bb_flag)
        {
            irbuilder.SetInsertPoint(entry_bb);
            irbuilder.CreateBr(update_exit_bb);
        }

        irbuilder.SetInsertPoint(update_exit_bb);

        // TODO: Verify the Program Counter Update Method;
        // This is the immediate update; Branch Updates will be done through delay buffer method.
        INFO << "    Call to: " << "Update_PC" << "(...)" << endl;
        irbuilder.CreateCall2(p_update_pc, p_proc_state, Geti32Value(exec_packet->GetSize() * 4));
        INFO << "    Call to: " << "Inc_DSP_Cycles" << "(...)" << endl;
        irbuilder.CreateCall(p_inc_cycles, p_proc_state);

        //CreateCallByName("Do_Memory_Writebacks");
        updated_pc = CreateCallByName("Update_Registers"); INFO << endl;
        irbuilder.CreateRet(updated_pc);
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
        AddrTableEntry_t * p_entry     = p_addr_table->GetAddressEntry(0);
        uint32_t         tablesize     = p_addr_table->GetCurrSize();
        llvm::Function * nativefptr    = p_entry->p_native_func;
        llvm::IRBuilder<>  & irbuilder = GetIRBuilder();

        // Type Definitions
        std::vector<const Type*> gen_struct_elems;
        gen_struct_elems.push_back(irbuilder.getInt32Ty());
        gen_struct_elems.push_back(nativefptr->getType());

        llvm::StructType * gen_addr_entry_ty = llvm::StructType::get(GetContext(), gen_struct_elems, false);
        ASSERT(gen_addr_entry_ty, "Failed to Create Address Entry Structure Type");
        //gen_addr_entry_ty->dump();

        llvm::ArrayType * gen_addr_table_ty = llvm::ArrayType::get(gen_addr_entry_ty, p_addr_table->GetCurrSize());
        ASSERT(gen_addr_table_ty, "Failed to Create Address Table Array Type");
        //gen_addr_table_ty->dump();

        GlobalVariable* gen_addr_table = new GlobalVariable(*p_addr_mod,
            /*Type=*/gen_addr_table_ty,
            /*isConstant=*/false,
            /*Linkage=*/GlobalValue::ExternalLinkage,
            /*Initializer=*/0, // has initializer, specified below
            /*Name=*/"AddressingTable");
        gen_addr_table->setAlignment(32);

        Constant *const_addr_table_size = irbuilder.getInt32(p_addr_table->GetCurrSize());
        GlobalVariable* gen_addr_table_sz = new GlobalVariable(*p_addr_mod,
            /*Type=*/irbuilder.getInt32Ty(),
            /*isConstant=*/false,
            /*Linkage=*/GlobalValue::ExternalLinkage,
            /*Initializer=*/const_addr_table_size,
            /*Name=*/"AddressingTableSize");
        gen_addr_table_sz->setAlignment(32);

        std::vector<Constant*> const_array_elems;
        for(uint32_t index = 0 ; index < tablesize ; index++)
        {
            p_entry = p_addr_table->GetAddressEntry(index);
            ConstantInt* const_target_addr = ConstantInt::get(GetContext(), llvm::APInt(32, p_entry->m_target_addr));

            // Add External Function Declarations
            llvm::Function* func_ptr = p_entry->p_native_func;
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
        //unsigned Threshold = 250;
        //llvm::Pass *InliningPass = createFunctionInliningPass(Threshold);
#ifdef INLINE_FUNCTIONS
        llvm::Pass *AlwaysInliningPass = createAlwaysInlinerPass();
#else
        llvm::Pass *AlwaysInliningPass = NULL;
#endif

#ifdef OPTIMIZE_MODULE
        createStandardModulePasses(p_pass_manager, OptLevel,
                                   /*OptimizeSize=*/ false, /*UnitAtATime,*/true,
                                   /*UnrollLoops=*/true, /*!DisableSimplifyLibCalls,*/true,
                                   /*HaveExceptions=*/ true, AlwaysInliningPass);
#else
        if(AlwaysInliningPass)
        {
            p_pass_manager->add(AlwaysInliningPass);
        }
#endif
        p_func_pass_manager->add(createGVNPass());
        p_func_pass_manager->add(createInstructionCombiningPass());
        p_func_pass_manager->add(createCFGSimplificationPass());
        p_func_pass_manager->add(createDeadStoreEliminationPass());
    }

    int32_t LLVMGenerator :: OptimizeModule()
    {
        p_pass_manager->run(*p_gen_mod);
        p_pass_manager->run(*p_addr_mod);

#ifdef OPTIMIZE_FUNCTIONS
        for(llvm::Module::iterator FI = p_gen_mod->getFunctionList().begin(),
            FIE = p_gen_mod->getFunctionList().end(); FI != FIE; FI++)
        {
            COUT << "Optimizing Function ..." << FI->getNameStr() << endl;
            OptimizeFunction(&(*FI));

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
        p_func_pass_manager->run(*func);
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
