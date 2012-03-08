
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Assembly/AssemblyAnnotationWriter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/System/Signals.h"

#include "RawBinaryReader.h"
#include "COFFBinaryReader.h"
#include "FetchPacketList.h"
#include "ExecutePacketList.h"
#include "BasicBlockList.h"
#include "C62xInstructionDecoder.h"
#include "DecodedInstruction.h"
#include "LLVMGenerator.h"

using namespace native;

#define COFF_INPUT_FILE
#define GENERATE_CODE
//#define BASIC_BLOCK_LEVEL_CODE

void print_usage(char **argv)
{
    cout << argv[0] << " <input filename>" << " <output filename>" << endl;
    return;
}

int main (int argc, char ** argv)
{
    if(argc != 3)
    {
        print_usage(argv);
        return (EXIT_FAILURE);
    }

#ifdef COFF_INPUT_FILE
    BinaryReader * reader = new COFFBinaryReader(argv[1]);
#else
    BinaryReader * reader = new RawBinaryReader(argv[1]);
#endif

    uint32_t *section_handle = reader->GetSectionHandle(".text");
    uint32_t  instr_address  = reader->GetSectionStartAddress(section_handle);

    ofstream  *p_output = new ofstream(argv[2], ios::out);
    if(!p_output->is_open()){
        DOUT << "Error: Opening Output File: " << argv[2] << endl;
        return (EXIT_FAILURE);
    }

    COUT << "Reading Input Binary ..." << endl;

    // Here we construction the instruction list.
    InstructionList instruction_list;
    native :: Instruction * instruction = NULL;
    while ((instruction = reader->Read(section_handle, instr_address)))
    {
        instruction_list.PushBack(instruction);
        instr_address += 4;
    }

    string sections_to_dump[] = {".text", ".data", ".cinit", ".const", ""};

    // Dump the different sections of input binary to files for loading to KVM Memory
    for(int i = 0; strcmp(sections_to_dump[i].c_str(), "") != 0; i++)
    {
        reader->DumpSection(argv[1], argv[1], sections_to_dump[i]);
    }

    FetchPacketList fetch_packet_list(&instruction_list);
    ExecutePacketList execute_packet_list(&instruction_list);

    // Create a New Decoder Object.
    InstructionDecoder * decoder = new C62xInstructionDecoder();

    COUT << "Decoding Binary Instructions ..." << endl;
    // Decode all instructions present in the list
    for(InstructionList_Iterator_t ILI = instruction_list.GetInstructionList()->begin(),
        ILE = instruction_list.GetInstructionList()->end(); ILI != ILE; ++ILI)
    {
        DecodedInstruction * dec_instr = decoder->DecodeInstruction(*ILI);
        ASSERT(dec_instr != NULL, "Instruction Decoding Failed !!!")
        //dec_instr->Print(& cout);
    }

#ifdef COFF_INPUT_FILE
    //instruction_list.MarkBranchTargets();
#endif
    BasicBlockList basic_block_list (& execute_packet_list);

    //instruction_list.Print(p_output);
    //fetch_packet_list.Print(p_output);
    execute_packet_list.Print(p_output);
    //basic_block_list.Print(p_output);

#ifdef GENERATE_CODE

#ifdef BASIC_BLOCK_LEVEL_CODE
    LLVMGenerator * llvm_gen = new LLVMGenerator("../lib/ISABehavior/C62xISABehavior.bc", "gen_code.bc", basic_block_list.GetSize());
    const BasicBlockList_t * bb_list = basic_block_list.GetBasicBlockList();

    COUT << "Generating LLVM Instructions (Basic Block Level) ..." << endl;
    for(BasicBlockList_ConstIterator_t BBLCI = bb_list->begin(), BBLCE = bb_list->end();
        BBLCI != BBLCE; ++BBLCI)
    {
        if(llvm_gen->GenerateLLVMBBLevel(*BBLCI))
        {
            DOUT << "Error: Generating LLVM Code" << endl;
            return (-1);
        }
    }
#else
#ifdef C62x_ISA_VER2
    LLVMGenerator * llvm_gen = new LLVMGenerator("../lib/ISABehavior/C62xISABehavior_v2.bc", "gen_code.bc", execute_packet_list.GetSize());
#else
    LLVMGenerator * llvm_gen = new LLVMGenerator("../lib/ISABehavior/C62xISABehavior.bc", "gen_code.bc", execute_packet_list.GetSize());
#endif
    ExecutePacketList_t * exec_list = execute_packet_list.GetExecutePacketList();
    uint32_t total_pkts = exec_list->size();
    uint32_t curr_pkt = 0;
    uint32_t progress = 0;

    COUT << "Generating LLVM (EP Level) ... " << total_pkts << " Packets" << endl;
    for(ExecutePacketList_ConstIterator_t EPLI = exec_list->begin(), EPLE = exec_list->end();
        EPLI != EPLE; ++EPLI)
    {
        if((*EPLI)->GetPacketType() == NORMAL_EXEC_PACKET)
        {
            if(llvm_gen->GenerateLLVMEPLevel(*EPLI))
            {
                DOUT << "Error: Generating LLVM Code" << endl;
                return (-1);
            }
        }

        progress = ++curr_pkt / total_pkts * 100;
        cout << "[" << setw(3) << setfill(' ') << progress << "%]\r";
    }
    cout << "\n";
#endif

    COUT << "Verifying Module ... " << endl;
    llvm_gen->VerifyGeneratedModule();

    COUT << "Writing Addressing Table ... " << endl;
    llvm_gen->WriteAddressingTable();

    COUT << "Optimizing LLVM Instructions ..." << endl;
    llvm_gen->OptimizeModule();

    COUT << "Writing Binary Configurations ... " << endl;
    llvm_gen->WriteBinaryConfigs(reader);

    COUT << "Writing Output Bitcode ..." << endl;
    llvm_gen->WriteBitcodeFile();
#endif

    /*
    ExecutePacketList_t * exec_list = execute_packet_list.GetExecutePacketList();
    for(ExecutePacketList_ConstIterator_t EPLI = exec_list->begin(), EPLE = exec_list->end();
        EPLI != EPLE; ++EPLI)
    {
        if(llvm_gen->GenerateLLVM(*EPLI))
        {
            DOUT << "Error: Generating LLVM Code" << endl;
            return (-1);
        }
    }
    */
    /*
    for(InstructionList_ConstIterator_t ILI = instruction_list.GetInstructionList()->begin(),
        ILE = instruction_list.GetInstructionList()->end(); ILI != ILE; ++ILI)
    {
        if(llvm_gen->GenerateLLVM(*ILI))
        {
            DOUT << "Error: Generating LLVM Code" << endl;
            return (-1);
        }
    }*/

    return (EXIT_SUCCESS);
}

