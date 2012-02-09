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

namespace native
{
    LLVMGenerator :: LLVMGenerator(string input_bcfile, string output_bcfile) :
        p_module(NULL), m_context(getGlobalContext()), m_irbuilder(m_context), m_curr_function(NULL),
        p_pass_manager(NULL), p_func_pass_manager(NULL),
        i1(IntegerType::get(m_context, 1)), i8(IntegerType::get(m_context, 8)), i16(IntegerType::get(m_context, 16)),
        i32(IntegerType::get(m_context, 32)), iptr(IntegerType::get(m_context, 8 * sizeof(intptr_t))),
        p_output_stream(GetOutputStream(output_bcfile.c_str()))
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
                //F->setLinkage(GlobalValue::InternalLinkage);
                F->addFnAttr(llvm::Attribute::AlwaysInline);
            }
        }

        // Create a clone of the input module
        p_out_module   = CloneModule(p_module);

        p_pass_manager = new llvm::PassManager();
        p_func_pass_manager = new llvm::FunctionPassManager(p_out_module);

        p_pass_manager->add(new TargetData(p_module));
        p_func_pass_manager->add(new TargetData(p_module));

        /* The Set of Passes that we Run on Each Module / Function */
        AddOptimizationPasses(3);

        // TODO: Remove the following Hard-Coded Processor State Type
        const Type * proc_state_type = p_out_module->getTypeByName("struct.C62x_Proc_State_t");
        ASSERT(proc_state_type, "Could Not Get Processor State Type from Module");

        p_proc_state_type = llvm::PointerType::getUnqual(proc_state_type);
        ASSERT(proc_state_type, "Could Not Get Processor State Type Pointer");

        // Some common Processor State Manipulation Functions
        p_increment_pc = p_out_module->getFunction("IncrementPC"); ASSERT(p_increment_pc, "Could Not Get IncrementPC");
        p_get_pc       = p_out_module->getFunction("GetPC"); ASSERT(p_get_pc, "Could Not Get GetPC");
        p_set_pc       = p_out_module->getFunction("SetPC"); ASSERT(p_set_pc, "Could Not Get SetPC");
        p_inc_cycles   = p_out_module->getFunction("IncrementCycles"); ASSERT(p_inc_cycles, "Could Not Get IncrementCycles");
        p_get_cycles   = p_out_module->getFunction("GetCycles"); ASSERT(p_get_cycles, "Could Not Get GetCycles");
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

        func_ptr = p_out_module->getFunction(StringRef(func_name));
        if(!func_ptr)
        {
          COUT << "Could not Get Function Call for: "  << func_name << endl;
          return NULL;
        }

        INFO << "    Call to: " << func_name << "(...)" << endl;
        func_value = GetIRBuilder().CreateCall(func_ptr, args.begin(), args.end(), func_rname);

        return (func_value);
    }

    int32_t LLVMGenerator :: GenerateLLVMBBLevel(ExecutePacket * exec_packet)
    {
        Instruction        * instr      = NULL;
        DecodedInstruction * dec_instr  = NULL;
        llvm::Value        * pc_updated = NULL;

        // Add an instruction that returns if the PC update is indicated by the above function call;
        //GetIRBuilder().CreateICmpEQ(pc_updated, Geti1Value(1), "");
        //CreateCallByName("ShowProcessorState");
        exec_packet->ResetInstrIterator();
        while((instr = exec_packet->GetNextInstruction()))
        {
            dec_instr = instr->GetDecodedInstruction();
            ASSERT(dec_instr != NULL, "Instructions need to be Decoded First");

            Value * func_value = dec_instr->CreateLLVMFunctionCall(this, p_out_module);
#ifdef FUNC_CALL_ERROR_CHECK
            ASSERT(func_value, "Error: In Creating Function Call");
#endif
            //CreateCallByName("ShowProcessorState");
        }

        CreateCallByName("ShowProcessorState");
        // TODO: Verify the Program Counter Update Method;
        // This is the immediate update; Branch Updates will be done through delay buffer method.
        GetIRBuilder().CreateCall2(p_increment_pc, p_proc_state, Geti32Value(exec_packet->GetSize() * 4));
        GetIRBuilder().CreateCall(p_inc_cycles, p_proc_state);
        pc_updated = CreateCallByName("UpdateRegisters");
        return (0);
    }

    int32_t LLVMGenerator :: GenerateLLVMBBLevel(native::BasicBlock * input_bb)
    {
        llvm::FunctionType * func_type = llvm::FunctionType::get(Type::getInt32Ty(GetContext()), /*not vararg*/false);
        //llvm::Function   * function  = llvm::Function::Create(func_type, Function::ExternalLinkage, "GenFunc" + input_bb->GetName(), p_out_module);
        llvm::Function   * function  = llvm::Function::Create(func_type, Function::ExternalLinkage, "simulated_bb", p_out_module);
        llvm::BasicBlock * gen_block = llvm::BasicBlock::Create(GetContext(), "EntryBB", function);

        SetCurrentFunction(function);
        GetIRBuilder().SetInsertPoint(gen_block);
        CreateCallByName("InitProcessorState");
        //GetIRBuilder().CreateCall2(p_set_pc, p_proc_state, Geti32Value(0x100));

        for (uint32_t index = 0; index < input_bb->GetSize(); index++)
        {
            ExecutePacket * exec_packet = input_bb->GetExecutePacketByIndex(index);
            if(exec_packet->GetPacketType() == NORMAL_EXEC_PACKET)
            {
                exec_packet->ResetInstrIterator();
                if(GenerateLLVMBBLevel(exec_packet))
                {
                    COUT << "Error: In Generating LLVM" << endl;
                    return (-1);
                }
            }
            else if (exec_packet->GetPacketType() == SPECIAL_EXEC_PACKET)
            {
                switch(exec_packet->GetSpecialFlags())
                {
                    case BRANCH_TAKEN:
                        CreateCallByName("ShowProcessorState");
                        GetIRBuilder().CreateRet(Geti32Value(0));
                        break;
                    case POSSIBLE_BRANCH:
                        /* Create instructions to check whether the PC has been
                         * updated in the previous cycle or not?
                         * And Create a Conditional Return Instruction */
                        break;
                }
            }
        }

        llvm::verifyModule(*p_out_module, PrintMessageAction);
        return (0);
    }

    int32_t LLVMGenerator :: GenerateLLVMEPLevel(ExecutePacket * exec_packet)
    {
        Instruction        * instr         = NULL;
        DecodedInstruction * dec_instr     = NULL;
        llvm::Value        * pc_updated    = NULL;
        string               function_name = "Simulate_" + exec_packet->GetName();

        const Type * return_type = Type::getInt32Ty(GetContext());
        std::vector<const Type*> params;
        params.push_back(p_proc_state_type);
        llvm::FunctionType * func_type = llvm::FunctionType::get(return_type, params, /*not vararg*/false);
        llvm::Function     * function  = llvm::Function::Create(func_type, Function::ExternalLinkage, function_name, p_out_module);
        llvm::BasicBlock   * gen_block = llvm::BasicBlock::Create(GetContext(), "EntryBB", function);

        INFO << "Function ... " << function_name << "(proc_state_t *)" << endl;

        SetCurrentFunction(function);
        // Here we get the Processor State Pointer from the Currently Generating Function.
        // The Processor State is passed to the Generated Function as a pointer.
        Function::arg_iterator curr_gen_func_args = function->arg_begin();
        p_proc_state = curr_gen_func_args++;          // We know that its the only single parameter; but increment anyways
        p_proc_state->setName("proc_state");

        GetIRBuilder().SetInsertPoint(gen_block);

        exec_packet->ResetInstrIterator();
        while((instr = exec_packet->GetNextInstruction()))
        {
            dec_instr = instr->GetDecodedInstruction();
            ASSERT(dec_instr != NULL, "Instructions need to be Decoded First");

            Value * func_value = dec_instr->CreateLLVMFunctionCall(this, p_out_module);
#ifdef FUNC_CALL_ERROR_CHECK
            ASSERT(func_value, "Error: In Creating Function Call");
#endif
        }

        //CreateCallByName("ShowProcessorState");

        // TODO: Verify the Program Counter Update Method;
        // This is the immediate update; Branch Updates will be done through delay buffer method.
        INFO << "    Call to: " << "IncrementPC" << "(...)" << endl;
        GetIRBuilder().CreateCall2(p_increment_pc, p_proc_state, Geti32Value(exec_packet->GetSize() * 4));

        INFO << "    Call to: " << "IncrementCycles" << "(...)" << endl;
        GetIRBuilder().CreateCall(p_inc_cycles, p_proc_state);

        pc_updated = CreateCallByName("UpdateRegisters");
        INFO << endl;

        GetIRBuilder().CreateRet(Geti32Value(0));

        llvm::verifyModule(*p_out_module, PrintMessageAction);

        return (0);
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
        p_pass_manager->run(*p_out_module);
        return (0);
    }

    int32_t LLVMGenerator :: OptimizeFunction(llvm::Function * func)
    {
        p_func_pass_manager->run(* func);
        return (0);
    }

    int32_t LLVMGenerator :: WriteBitcodeFile()
    {
        WriteBitcodeToFile(p_out_module, p_output_stream->os());
        p_output_stream->keep();
        delete p_out_module;
        delete p_output_stream;
        return (0);
    }
}
