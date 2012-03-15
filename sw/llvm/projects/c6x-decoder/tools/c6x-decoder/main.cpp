
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

void print_usage(char **argv)
{
    cout << argv[0] << " <input_binary> <output_asm> <raw/coff> <gen_code_level> <isa_path>" << endl;
    return;
}

int main (int argc, char ** argv)
{
    string input_binary, output_asm, input_format, cg_lvl_str, isa_path;

    if(argc != 6)
    {
        print_usage(argv);
        return (EXIT_FAILURE);
    }

    input_binary   = argv[1];
    output_asm     = argv[2];
    input_format   = argv[3];
    cg_lvl_str     = argv[4];
    isa_path       = argv[5];

    LLVMCodeGenLevel_t code_gen_lvl;
    LLVMCodeGenOption_t code_gen_opt = LLVM_CGO_NONE;
    code_gen_opt = (LLVMCodeGenOption_t) (code_gen_opt | LLVM_CGO_OPTIMIZE_MOD);
    code_gen_opt = (LLVMCodeGenOption_t) (code_gen_opt | LLVM_CGO_INLINE);
    code_gen_opt = (LLVMCodeGenOption_t) (code_gen_opt | LLVM_CGO_FUF_INLINE);

    if(cg_lvl_str.compare("EP") == 0)             code_gen_lvl = LLVM_CG_EP_LVL;
    else if (cg_lvl_str.compare("BB") == 0)       code_gen_lvl = LLVM_CG_BB_LVL;
    else ASSERT(0, "Unknown Code Generation Level");

    BinaryReader * reader = NULL;
    if(input_format.compare("coff") == 0)
        reader = new COFFBinaryReader(input_binary);
    else if(input_format.compare("raw") == 0)
        reader = new RawBinaryReader(input_binary);
    ASSERT(reader != NULL, "Unknown Input Binary Format Specified.");

    uint32_t *section_handle = reader->GetSectionHandle(".text");
    uint32_t  instr_address  = reader->GetSectionStartAddress(section_handle);

    ofstream  *p_output = new ofstream(output_asm.c_str(), ios::out);
    if(!p_output->is_open())
    {
        DOUT << "Error: Opening Output File: " << output_asm << endl;
        return (EXIT_FAILURE);
    }

    COUT << "Reading Input Binary ... " << endl;
    InstructionList iList;
    native :: Instruction * pInstr = NULL;
    while ((pInstr = reader->Read(section_handle, instr_address)))
    {
        iList.PushBack(pInstr);
        instr_address += 4;
    }

    if(input_format.compare("coff") == 0)
    {
        string sections_to_dump[] = {".text", ".data", ".cinit", ".const", ""};

        // Dump the different sections of input binary to files for loading to KVM Memory
        for(int i = 0; strcmp(sections_to_dump[i].c_str(), "") != 0; i++)
        {
            reader->DumpSection(input_binary, input_binary, sections_to_dump[i]);
        }
    }

    FetchPacketList fpList(& iList);
    ExecutePacketList epList(& iList);

    // Create a New Decoder Object.
    InstructionDecoder * decoder = new C62xInstructionDecoder();

    COUT << "Decoding Binary Instructions ..." << endl;
    for(InstructionList_Iterator_t ILI = iList.GetList()->begin(),
        ILE = iList.GetList()->end(); ILI != ILE; ++ILI)
    {
        DecodedInstruction * dec_instr = decoder->DecodeInstruction(*ILI);
        ASSERT(dec_instr != NULL, "Instruction Decoding Failed !!!")
    }

    // Mark All Statically Known Branch Target Instructions.
    iList.MarkBranchTargets();

    BasicBlockList bbList (& epList);
    uint32_t total_bbs = bbList.GetSize();

    if(code_gen_lvl == LLVM_CG_BB_LVL)
    {
        // Here We Filter out the BasicBlock First Packets from the Execute Packet List;
        // So there are not duplications of Target Addresses
        bbList.RemoveRedundantEPs(& epList);
        bbList.Print(p_output);
    }
    else if(code_gen_lvl == LLVM_CG_EP_LVL)
    {
        epList.Print(p_output);
    }

    ExecutePacketList_t * exec_list = epList.GetList();
    uint32_t total_pkts = exec_list->size();
    uint32_t progress   = 0;
    LLVMGenerator * llvm_gen = NULL;

    if(code_gen_lvl == LLVM_CG_BB_LVL)
    {
        uint32_t curr_bb    = 0;
        llvm_gen = new LLVMGenerator(isa_path, code_gen_lvl, code_gen_opt);

        const BasicBlockList_t * bb_list = bbList.GetList();
        COUT << "Generating LLVM (BB Level) ... " << setw(4) << total_bbs << " Basic Blocks ... ";

        for(BasicBlockList_ConstIterator_t BBLCI = bb_list->begin(), BBLCE = bb_list->end();
            BBLCI != BBLCE; ++BBLCI)
        {
            if(llvm_gen->GenerateLLVM_BBLevel(*BBLCI))
            {
                DOUT << "Error: Generating LLVM Code" << endl;
                return (-1);
            }

            progress = ++curr_bb / total_bbs * 100;
            cout << "[" << setw(3) << setfill(' ') << progress << "%]\b\b\b\b\b\b";
        }
        cout << "\n";
    }

    uint32_t curr_pkt = 0;

    if(!llvm_gen)
    {
        llvm_gen = new LLVMGenerator(isa_path, code_gen_lvl, code_gen_opt);
    }

    COUT << "Generating LLVM (EP Level) ... " <<  setw(4) << total_pkts << " Execute Packets ... ";
    for(ExecutePacketList_ConstIterator_t EPLI = exec_list->begin(), EPLE = exec_list->end();
        EPLI != EPLE; ++EPLI)
    {
        if(llvm_gen->GenerateLLVM_EPLevel(*EPLI))
        {
            DOUT << "Error: Generating LLVM Code" << endl;
            return (-1);
        }

        progress = ++curr_pkt / total_pkts * 100;
        cout << "[" << setw(3) << setfill(' ') << progress << "%]\b\b\b\b\b\b";
    }
    cout << "\n";

    ASSERT(llvm_gen != NULL, "Unknown Code Generation Level !!!");

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

    return (EXIT_SUCCESS);
}

