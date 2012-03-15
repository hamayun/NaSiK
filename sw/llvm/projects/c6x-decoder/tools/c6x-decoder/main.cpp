
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

static string ToolDescription = "LLVM Based Static Binary Translator\n";

static cl::opt<std::string> InputBinaryFilename     (cl::Positional, cl::desc("<Target Binary Name>"), cl::Required);
static cl::opt<std::string> OutputAsmFilename       ("o", cl::desc("Output Assembly File [Target Disassembled Code]"),
                                                     cl::value_desc("filename"), cl::init("/dev/null"));
static cl::opt<std::string> InputBinaryFormat       ("ibf", cl::desc("Input Binary Format {COFF,RAW} [Default:COFF]"),
                                                     cl::value_desc("format"), cl::init("COFF"));
static cl::opt<std::string> ISAFilename             ("isa", cl::desc("ISA Behavior File [LLVM Bitcode]"),
                                                     cl::value_desc("filename"), cl::Required);
static cl::opt<LLVMCodeGenLevel_t> CodeGenLevel     (cl::desc("Code Generation Level:"), cl::init(LLVM_CG_BB_LVL),
                                                     cl::values(
                                                     clEnumValN(LLVM_CG_EP_LVL, "EP", "Execute Packet Level Code Generation"),
                                                     clEnumValN(LLVM_CG_BB_LVL, "BB", "Basic Block Level Code Generation [Mix-mode i.e. BB + EP]"),
                                                     clEnumValEnd));
static cl::bits<LLVMCGOBitVector_t> CodeGenOptionsBV(cl::desc("Code Generation Options:"),
                                                     cl::values(
                                                     clEnumValN(LLVM_CGO_INLINE_BIT, "inline", "Inline All ISA Function During Code Generation"),
                                                     clEnumValN(LLVM_CGO_FUF_INLINE_BIT, "finline", "Inline the Frequently Used Functions (requires the inline option)"),
                                                     clEnumValN(LLVM_CGO_OPT_MOD_BIT, "mopt", "Enable Optimization at Module Level"),
                                                     clEnumValN(LLVM_CGO_OPT_FUN_BIT, "fopt", "Enable Optimization at Function Level"),
                                                     clEnumValN(LLVM_CGO_OPT_SPE_BIT, "sopt", "Enable Special Optimizations (If Any)"),
                                                     clEnumValEnd));

int main (int argc, char ** argv)
{
    native::InstructionList           iList;
    native::Instruction            * pInstr = NULL;
    native::BinaryReader           * reader = NULL;
    native::InstructionDecoder    * decoder = NULL;
    native::LLVMGenerator        * llvm_gen = NULL;
    native::ExecutePacketList_t * exec_list = NULL;
    ofstream               * p_out_asm_file = NULL;
    uint32_t               * section_handle = NULL;
    uint32_t                 instr_address  = 0x0;
    uint32_t total_bbs = 0, total_pkts = 0, curr_bb = 0, curr_pkt = 0, progress = 0;

    cl::ParseCommandLineOptions(argc, argv, ToolDescription.c_str());
    native::LLVMCodeGenOptions_t CodeGenOptions = (native::LLVMCodeGenOptions_t) CodeGenOptionsBV.getBits();

    if(InputBinaryFormat.compare("COFF") == 0)
    {
        reader = new COFFBinaryReader(InputBinaryFilename.c_str());
    }
    else if(InputBinaryFormat.compare("RAW") == 0)
    {
        reader = new RawBinaryReader(InputBinaryFilename.c_str());
    }
    ASSERT(reader != NULL, "Unknown Input Binary Format Specified.");

    p_out_asm_file = new ofstream(OutputAsmFilename.c_str(), ios::out);
    if(!p_out_asm_file->is_open())
    {
        DOUT << "Error: Opening Output File: " << OutputAsmFilename << endl;
        return (EXIT_FAILURE);
    }

    section_handle = reader->GetSectionHandle(".text");
    instr_address  = reader->GetSectionStartAddress(section_handle);

    COUT << "Reading Input Binary ... " << endl;
    while ((pInstr = reader->Read(section_handle, instr_address)))
    {
        iList.PushBack(pInstr);
        instr_address += 4;
    }

    if(InputBinaryFormat.compare("COFF") == 0)
    {
        string sections_to_dump[] = {".text", ".data", ".cinit", ".const", ""};

        // Dump the different sections of input binary to files for loading to KVM Memory
        for(int i = 0; strcmp(sections_to_dump[i].c_str(), "") != 0; i++)
        {
            reader->DumpSection(InputBinaryFilename, InputBinaryFilename, sections_to_dump[i]);
        }
    }

    FetchPacketList fpList(& iList);
    ExecutePacketList epList(& iList);

    // Create a New Decoder Object.
    // Depending upon the Type of Input Binary; (For the moment C62x only)
    decoder = new C62xInstructionDecoder();

    COUT << "Decoding Binary Instructions ..." << endl;
    for(InstructionList_Iterator_t ILI = iList.GetList()->begin(), ILE = iList.GetList()->end(); ILI != ILE; ++ILI)
    {
        DecodedInstruction * dec_instr = decoder->DecodeInstruction(*ILI);
        ASSERT(dec_instr != NULL, "Instruction Decoding Failed !!!")
    }

    // Mark All Statically Known Branch Target Execute Packets.
    iList.MarkBranchTargets();

    BasicBlockList bbList (& epList);
    total_bbs = bbList.GetSize();

    if(CodeGenLevel == LLVM_CG_BB_LVL)
    {
        // Here We Filter out the BasicBlock First Packets from the Execute Packet List;
        // So there are not duplications of Target Addresses
        bbList.RemoveRedundantEPs(& epList);
        bbList.Print(p_out_asm_file);
    }
    else if(CodeGenLevel == LLVM_CG_EP_LVL)
    {
        epList.Print(p_out_asm_file);
    }

    exec_list = epList.GetList();
    total_pkts = exec_list->size();
    progress   = 0;

    if(CodeGenLevel == LLVM_CG_BB_LVL)
    {
        llvm_gen = new LLVMGenerator(ISAFilename, CodeGenLevel, CodeGenOptions);
        ASSERT(llvm_gen != NULL, "Error Creating LLVM Generator Object!!!");

        const BasicBlockList_t * bb_list = bbList.GetList();
        COUT << "Generating LLVM (BB Level) ... " << setw(4) << total_bbs << " Basic Blocks ... ";

        curr_bb  = 0;
        for(BasicBlockList_ConstIterator_t BBLCI = bb_list->begin(), BBLCE = bb_list->end(); BBLCI != BBLCE; ++BBLCI)
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

    curr_pkt = 0;
    if(!llvm_gen) llvm_gen = new LLVMGenerator(ISAFilename, CodeGenLevel, CodeGenOptions);
    ASSERT(llvm_gen != NULL, "Error Creating LLVM Generator Object!!!");

    COUT << "Generating LLVM (EP Level) ... " <<  setw(4) << total_pkts << " Execute Packets ... ";
    for(ExecutePacketList_ConstIterator_t EPLI = exec_list->begin(), EPLE = exec_list->end(); EPLI != EPLE; ++EPLI)
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

    COUT << "Verifying Module ... " << endl;              llvm_gen->VerifyGeneratedModule();
    COUT << "Writing Addressing Table ... " << endl;      llvm_gen->WriteAddressingTable();
    COUT << "Optimizing LLVM Instructions ..." << endl;   llvm_gen->OptimizeModule();
    COUT << "Writing Binary Configurations ... " << endl; llvm_gen->WriteBinaryConfigs(reader);
    COUT << "Writing Output Bitcode ..." << endl;         llvm_gen->WriteBitcodeFile();

    return (EXIT_SUCCESS);
}