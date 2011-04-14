/*************************************************************************************
 * File   : ARMTargetAnnotation.cpp,     
 *
 * Copyright (C) 2007 TIMA Laboratory
 * Author(s) :      Patrice, GERIN patrice.gerin@imag.fr
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

#define DEBUG_TYPE "annotation"
#include <iostream>
#include <iomanip>
#include "llvm/Target/TargetAnnotationInfo.h"

#include "ARM.h"
#include "ARMInstrInfo.h"
#include "ARMBaseInfo.h"
#include "ARMTargetMachine.h"
#include "ARMAddressingModes.h"
#include "llvm/Support/Debug.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace std;
using namespace ARM; 

bool PRINT_WARNINGS;

static cl::opt<bool, true> PrintAnnotationWarnings("print-annotation-warnings",
       cl::desc("Print warnings during annoatation pass."),
       cl::location(PRINT_WARNINGS), cl::init(false));

//#define PRINT_SUMMARY

#define DERR(flag) \
  if(flag) cerr

namespace ARMCycleEstimation {
    enum ARM9InstrClasses {
        ARM9_UNKNOWN        = 0,
        ARM9_GENERIC,
        ARM9_ALU, 
        ARM9_ALU_RSL,
        ARM9_BRANCH,
        ARM9_BRANCH_LINK,
        ARM9_CDP, 
        ARM9_LDC, 
        ARM9_LDRB, 
        ARM9_LDR_NPC, 
        ARM9_LDR_PC, 
        ARM9_LDRD,
        ARM9_LDM_NPC, 
        ARM9_LDM_PC, 
        ARM9_MCR, 
        ARM9_MCRR, 
        ARM9_MRC_NPC, 
        ARM9_MRC_PC, 
        ARM9_MRRC,
        ARM9_MRS, 
        ARM9_MSR, 
        ARM9_MUL_MLA, 
        ARM9_MULS_MLAS, 
        ARM9_XMULL_XMLAL, 
        ARM9_XMULLS_XMLALS, 
        ARM9_PLD, 
        ARM9_QX_ADDSUB, 
        ARM9_SMUL_SMLA_XY,
        ARM9_SMLAL_XY,
        ARM9_STC, 
        ARM9_STR_BH, 
        ARM9_STRD,
        ARM9_STM, 
        ARM9_SWI, 
        ARM9_SWP_B,
        ARM9_MOV2PIECES, 
        ARM9_THUMB,
        ARM9_INVALID,
        ARM9_TOTAL_TYPES
    };

    int ARM9ClassCycles [ARM9_TOTAL_TYPES] = {0};
    ARM9InstrClasses ARM9InstrClassAssignment[INSTRUCTION_LIST_END] = {ARM9_UNKNOWN};
        
    void initInstrClassCycles();
    void initInstrClassAssignment();
    unsigned int getRegOperandsCount(const MachineInstr &MachInstr);
    unsigned int getInstrAddressingMode(const MachineInstr &MachInstr);
    unsigned int isShiftedOffset(const MachineInstr &MachInstr);
    unsigned int calculateExtraCycles(const MachineInstr &MachInstr, ARM9InstrClasses &instructionClass);
    unsigned int estimateDependencyCycles(const MachineInstr &PrevInstr, const MachineInstr &CurrInstr);    
    unsigned int getDataLoadCount(MachineBasicBlock &MBB);
    unsigned int getDataStoreCount(MachineBasicBlock &MBB);
    unsigned int estimateInstructionCycles(const MachineInstr &MachInstr);  

    // For Debug
    unsigned int getShiftOp(const MachineOperand &MO);

}

void ARMCycleEstimation :: initInstrClassAssignment()
{
    for (unsigned int instrIndex = 0; instrIndex < INSTRUCTION_LIST_END; instrIndex++)
    {
        switch(instrIndex)
        {
            case PHI:                   case INLINEASM:             case PROLOG_LABEL:          case EH_LABEL:
            case GC_LABEL:              case KILL:                  case EXTRACT_SUBREG:        case INSERT_SUBREG:
            case IMPLICIT_DEF:          case SUBREG_TO_REG:         case COPY_TO_REGCLASS:      case DBG_VALUE:
            case REG_SEQUENCE:          case COPY:                  case ADJCALLSTACKDOWN:      case ADJCALLSTACKUP:
            case ATOMIC_CMP_SWAP_I16:   case ATOMIC_CMP_SWAP_I32:   case ATOMIC_CMP_SWAP_I8:    case ATOMIC_LOAD_ADD_I16:
            case ATOMIC_LOAD_ADD_I32:   case ATOMIC_LOAD_ADD_I8:    case ATOMIC_LOAD_AND_I16:   case ATOMIC_LOAD_AND_I32:
            case ATOMIC_LOAD_AND_I8:    case ATOMIC_LOAD_NAND_I16:  case ATOMIC_LOAD_NAND_I32:  case ATOMIC_LOAD_NAND_I8:
            case ATOMIC_LOAD_OR_I16:    case ATOMIC_LOAD_OR_I32:    case ATOMIC_LOAD_OR_I8:     case ATOMIC_LOAD_SUB_I16:
            case ATOMIC_LOAD_SUB_I32:   case ATOMIC_LOAD_SUB_I8:    case ATOMIC_LOAD_XOR_I16:   case ATOMIC_LOAD_XOR_I32:
            case ATOMIC_LOAD_XOR_I8:    case ATOMIC_SWAP_I16:       case ATOMIC_SWAP_I32:       case ATOMIC_SWAP_I8:
            case BCCZi64:               case BCCi64:                case CLREX:                 case CONSTPOOL_ENTRY:
            case CPS:                   case DBG:                   case DMB_MCR:               case DMBsy:
            case DMBvar:                case DSB_MCR:               case DSBsy:                 case DSBvar:
            case ISBsy:                 case Int_eh_sjlj_longjmp:   case Int_eh_sjlj_setjmp:    case Int_eh_sjlj_setjmp_nofp:
            case LDREX:                 case LDREXB:                case LDREXD:                case LDREXH:
            case MOVPCLR:               case MOVPCRX:               case QASX:                  case QSAX:
            case REV:                   case REV16:                 case REVSH:                 case RFE:
            case RFEW:                  case SADD16:                case SADD8:                 case SASX:
            case SEL:                   case SETENDBE:              case SETENDLE:              case SHADD16:
            case SHADD8:                case SHASX:                 case SHSAX:                 case SHSUB16:
            case SHSUB8:                case SMC:                   case SMLABT:                case SMLAD:
            case SMLADX:                case SMLALBT:               case SMLALD:                case SMLALDX:
            case SMLALTB:               case SMLALTT:               case SMLATB:                case SMLATT:
            case SMLAWT:                case SMLSD:                 case SMLSDX:                case SMLSLD:
            case SMLSLDX:               case SMMLA:                 case SMMLAR:                case SMMLS:
            case SMMLSR:                case SMMUL:                 case SMMULR:                case SMUAD:
            case SMUADX:                case SMULBT:                case SMULTB:                case SMULTT:
            case SMULWT:                case SMUSD:                 case SMUSDX:
            case SRS:                   case SRSW:                  case SSAT:                  case SSAT16:
            case SSAX:                  case SSUB16:                case SSUB8:                 case STC2L_OFFSET:
            case STC2L_OPTION:          case STC2L_POST:            case STC2L_PRE:             case STC2_OFFSET:
            case STC2_OPTION:           case STC2_POST:             case STC2_PRE:              case STCL_OFFSET:
            case STCL_OPTION:           case STCL_POST:             case STCL_PRE:              case STC_OFFSET:
            case STC_OPTION:            case STC_POST:              case STC_PRE:              
            case STRBT:                 case STREX:                 case STREXB:                case STREXD:
            case STREXH:                case SXTAB16rr:             case SXTAB16rr_rot:         case SXTABrr:
            case SXTABrr_rot:           case SXTAHrr:               case SXTAHrr_rot:           case SXTB16r:
            case SXTB16r_rot:           case SXTBr:                 case SXTBr_rot:             case SXTHr:
            case SXTHr_rot:             case TAILJMPd:              case TAILJMPdND:            case TAILJMPdNDt:
            case TAILJMPdt:             case TAILJMPr:              case TAILJMPrND:            case TCRETURNdi:
            case TCRETURNdiND:          case TCRETURNri:            case TCRETURNriND:          case UADD16:
            case UADD8:                 case UASX:                  case UHADD16:               case UHADD8:
            case UHASX:                 case UHSAX:                 case UHSUB16:               case UHSUB8:
            case UQADD16:               case UQADD8:                case UQASX:                 case UQSAX:
            case UQSUB16:               case UQSUB8:                case USAD8:                 case USADA8:
            case USAT:                  case USAT16:                case USAX:                  case USUB16:
            case USUB8:                 case UXTAB16rr:             case UXTAB16rr_rot:         case UXTABrr:
            case UXTABrr_rot:           case UXTAHrr:               case UXTAHrr_rot:           case UXTB16r:
            case UXTB16r_rot:           case UXTBr:                 case UXTBr_rot:             case UXTHr:
            case UXTHr_rot:           
                // We do *NOT* support these instructions for Performance Estimation as we Annotate for ARM9Erev2 (ARMv5TE)
                // Most of the above instructions are either for ARMv6 or SIMD instructions for Cortex Series Processors.
                // Refer to the following files.
                // AETC2010-21_ARM_Cortex-M4_IanJohnson.pdf, QRC0001_UAL.pdf, ARM9E-S Core Technical Reference Manual
                // ARMGenCodeEmitter.inc, ARMGenInstrInfo.inc, ARMGenInstrNames.inc
                ARM9InstrClassAssignment[instrIndex] = ARM9_INVALID;            
                break;

            case BKPT:                  case BMOVPCRX:              case BMOVPCRXr9:            case FCONSTD:
            case FCONSTS:               case FMSTAT:                case LEApcrel:              case LEApcrelJT:
            case MOVCCi:                case MOVCCi16:              case MOVCCr:                case MOVCCs:
            case MOVTi16:               case MOVi16:                case MOVi32imm:             case NOP:
            case PICADD:                case PICLDR:                case PICLDRB:               case PICLDRH:
            case PICLDRSB:              case PICLDRSH:              case PICSTR:                case PICSTRB:
            case PICSTRH:               case PKHBT:                 case PKHTB:                 case SVC:
            case SWP:                   case SWPB:                  case TPsoft:                case TRAP:
                ARM9InstrClassAssignment[instrIndex] = ARM9_GENERIC;            // We count *ONE* cycle for these instructions
                break;

            case ADCSSri:               case ADCSSrr:               case ADCri:                 case ADCrr:
            case ADDSri:                case ADDSrr:                case ADDri:                 case ADDrr:
            case ANDSri:                case ANDSrr:                case ANDri:                 case ANDrr:
            case BICri:                 case BICrr:                 case CLZ:                   case CMNzri:
            case CMNzrr:                case CMPri:                 case CMPrr:                 case CMPzri:
            case CMPzrr:                case EORri:                 case EORrr:                 case MOVi:
            case MOVr:                  case MOVr_TC:               case MOVrx:                 case MOVs:
            case MOVsra_flag:           case MOVsrl_flag:           case MVNi:                  case MVNr:
            case MVNs:                  case ORRri:                 case ORRrr:                 case RSBri:                     
            case RSBrr:                 case RSCri:                 case RSCrr:                 case SBCri:                 
            case SBCrr:                 case SUBSri:                case SUBSrr:                case SUBri:
            case SUBrr:                 case TEQri:                 case TEQrr:                 case TSTri:
            case TSTrr:
                ARM9InstrClassAssignment[instrIndex] = ARM9_ALU;
                break;

            case ADCSSrs:               case ADCrs:                 case ADDSrs:                case ADDrs:
            case ANDSrs:                case ANDrs:                 case BICrs:                 case CMNzrs:
            case CMPrs:                 case CMPzrs:                case EORrs:                 case ORRrs:
            case RSBrs:                 case RSCrs:                 case SBCrs:                 case SUBSrs:
            case SUBrs:                 case TEQrs:                 case TSTrs:
                ARM9InstrClassAssignment[instrIndex] = ARM9_ALU_RSL;            // Arithematic Instructions with Register Shift
                break;

            case B:                     case BRIND:                 case BR_JTadd:              case BR_JTm:
            case BR_JTr:                case BX:                    case BXJ:                   case BX_RET:
            case BXr9:                  case Bcc:
                ARM9InstrClassAssignment[instrIndex] = ARM9_BRANCH;
                break;

            case BL:                    case BLX:                   case BLXr9:                 case BL_pred:
            case BLr9:                  case BLr9_pred:
                ARM9InstrClassAssignment[instrIndex] = ARM9_BRANCH_LINK;        // Branch and Link
                break;

            case CDP:                   case CDP2:
                ARM9InstrClassAssignment[instrIndex] = ARM9_CDP;
                break;

            case LDC2L_OFFSET:          case LDC2L_OPTION:          case LDC2L_POST:            case LDC2L_PRE:
            case LDC2_OFFSET:           case LDC2_OPTION:           case LDC2_POST:             case LDC2_PRE:
            case LDCL_OFFSET:           case LDCL_OPTION:           case LDCL_POST:             case LDCL_PRE:
            case LDC_OFFSET:            case LDC_OPTION:            case LDC_POST:              case LDC_PRE:
                ARM9InstrClassAssignment[instrIndex] = ARM9_LDC;
                break;

            case LDM:                   case LDM_UPD:
                ARM9InstrClassAssignment[instrIndex] = ARM9_LDM_NPC;
                break;

            case LDM_RET:
                ARM9InstrClassAssignment[instrIndex] = ARM9_LDM_PC;
                break;

            case LDR:                   case LDRT:                  case LDR_POST:              case LDR_PRE:
            case LDRcp:
                // Every LDR is considered as Not Loading PC; The fact that an instruction is loading PC is decided later on. 
                ARM9InstrClassAssignment[instrIndex] = ARM9_LDR_NPC;
                break;

            case LDRB:                  case LDRBT:                 case LDRB_POST:             case LDRB_PRE:
            case LDRH:                  case LDRHT:                 case LDRH_POST:             case LDRH_PRE:
            case LDRSB:                 case LDRSBT:                case LDRSB_POST:            case LDRSB_PRE:
            case LDRSH:                 case LDRSHT:                case LDRSH_POST:            case LDRSH_PRE:
                ARM9InstrClassAssignment[instrIndex] = ARM9_LDRB;
                break;

            case LDRD:                  case LDRD_POST:             case LDRD_PRE:
                ARM9InstrClassAssignment[instrIndex] = ARM9_LDRD;
                break;

            case MCR:                   case MCR2:
                ARM9InstrClassAssignment[instrIndex] = ARM9_MCR;
                break; 

            case MCRR:                  case MCRR2:
                ARM9InstrClassAssignment[instrIndex] = ARM9_MCRR;
                break;

            case MLA:                   case MUL:
                ARM9InstrClassAssignment[instrIndex] = ARM9_MUL_MLA;
                break;

            case MOVi2pieces:
                ARM9InstrClassAssignment[instrIndex] = ARM9_MOV2PIECES;
                break; 

            case MRC:                   case MRC2:
                ARM9InstrClassAssignment[instrIndex] = ARM9_MRC_NPC;
                break;

            case MRRC:                  case MRRC2:
                ARM9InstrClassAssignment[instrIndex] = ARM9_MRRC;
                break;

            case MRS:                   case MRSsys:
                ARM9InstrClassAssignment[instrIndex] = ARM9_MRS;
                break;
                
            case MSR:                   case MSRi:                  case MSRsys:                    case MSRsysi:
                ARM9InstrClassAssignment[instrIndex] = ARM9_MSR;
                break;


            case PLDWi:                 case PLDWr:                 case PLDi:                      case PLDr:
            case PLIi:                  case PLIr:
                ARM9InstrClassAssignment[instrIndex] = ARM9_PLD;
                break;

            case QADD:                  case QADD16:                case QADD8:                     case QDADD:
            case QDSUB:                 case QSUB:                  case QSUB16:                    case QSUB8:
                ARM9InstrClassAssignment[instrIndex] = ARM9_QX_ADDSUB;
                break;

            case SMLABB:                case SMLALBB:               case SMLAWB:                    case SMULBB:
            case SMULWB:
                ARM9InstrClassAssignment[instrIndex] = ARM9_SMUL_SMLA_XY;
                break;

            case STM:                    case STM_UPD:
                // STM_UPD is Essentially an STM instruction with update to SP Register.
                ARM9InstrClassAssignment[instrIndex] = ARM9_STM;
                break;

            case STR:                   case STRB:                  case STRB_POST:                 case STRB_PRE:
            case STRH:                  case STRHT:                 case STRH_POST:                 case STRH_PRE:
            case STRT:                  case STR_POST:              case STR_PRE:
                ARM9InstrClassAssignment[instrIndex] = ARM9_STR_BH;

            case STRD:                  case STRD_POST:             case STRD_PRE:
                ARM9InstrClassAssignment[instrIndex] = ARM9_STRD;
                break;

            case SMLAL:                 case SMULL:                 case UMAAL:                     case UMLAL:
            case UMULL:
                ARM9InstrClassAssignment[instrIndex] = ARM9_SMLAL_XY;
                break;

            // All of the following instructions are Thumb-2 Instructions;
            // We treat them as default case i.e. As Thumb Instructions and don't expect them in Generated Code.
            case BFC:                   case BFI:                   case MLS:                       case RBIT:
            case RSBSri:                case RSBSrs:                case RSCSri:                    case RSCSrs:
            case SBCSSri:               case SBCSSrr:               case SBCSSrs:
            case SBFX:                  case SEV:                   case UBFX:                      case WFE:
            case WFI:                   case YIELD:
            default:
                // Default Case for All the rest of Instructions including Thumb Instructions.
                // We don't Handle Thumb Instructions for Annotation Purpose
                ARM9InstrClassAssignment[instrIndex] = ARM9_INVALID;
                break;
        }
    }

    // Check if All of the Instructions have been Assigned A Class?
    // Add An Assert Later HERE
    for (unsigned int instrIndex = 0; instrIndex < INSTRUCTION_LIST_END; instrIndex++)
    {
        if (ARM9InstrClassAssignment[instrIndex] == ARM9_UNKNOWN)
            cout << "Unknown Class for Instruction: " << instrIndex << endl;
    }

} // initInstrClassAssignment

#if 0
void ARMCycleEstimation :: initInstrClassAssignment()
{
    // NOTES: 
    // COMPILERS CANNOT GENERATE THESE INSTRCUTION (Usually these instrcution are written in Assembly directly). 
    // So we don't need to classify Instructions According to them. 
    // ARM9_CDP;                   // COPROCESSOR DATA PROCESSING 
    // ARM9_LDC;                   // LOAD COPROCESSOR
    // ARM9_MCR;                   // MOVE TO COPROCESSOR FROM ARM
    // ARM9_MRC_NPC;               // MOVE TO ARM FROM COPROCESSOR NOT LOADING PC
    // ARM9_MRC_PC;                // MOVE TO ARM FROM COPROCESSOR LOADING PC

    // Instructions for these classes not found in LLVM
    /*
        ARM9_MCRR, 
        ARM9_MRRC,
        ARM9_MRS, 
        ARM9_MSR, 
        ARM9_MULS_MLAS, 
        ARM9_XMULL_XMLAL, 
        ARM9_XMULLS_XMLALS, 
        ARM9_PLD, 
        ARM9_QX_ADDSUB, 
        ARM9_STC, 
        ARM9_SWI, 
        ARM9_SWP_B,
    */

}
#endif

void ARMCycleEstimation :: initInstrClassCycles()
{
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_GENERIC]        = 1; 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_ALU]            = 1;    // +2 if Rd is pc
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_ALU_RSL]        = 2;    // 2 because ALU = 1 and Register Shift/Logical Operation = 1. +2 more if Rd is pc
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_BRANCH]         = 1;    // +2 as Branch Penalty; If the Branch is Taken; At Inter MBB Level        
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_BRANCH_LINK]    = 3;    // Branch Link Instrcutions are Always Considered as Taken, so 3 Cycles            
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_CDP]            = 1 +   ARM9ClassCycles [ARMCycleEstimation :: ARM9_BRANCH];     // 1 + B
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_LDC]            =       ARM9ClassCycles [ARMCycleEstimation :: ARM9_BRANCH];     // +N Later on 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_LDRB]           = 1;    // Rd is not available for two cycles. +1 if the load offset is shifted.
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_LDR_NPC]        = 1;    // Rd is not available for two cycles. +1 if the load offset is shifted.
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_LDR_PC]         = 5;    // +1 if the load offset is shifted.
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_LDRD]           = 2;    // R(d+1) is not available for one cycle.
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_LDM_NPC]        = 0;    // +N Later on, +1 if N = 1 or the last loaded register used in the next cycle 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_LDM_PC]         = 4;    // +N Later on   
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_MCR]            = 1 +   ARM9ClassCycles [ARMCycleEstimation :: ARM9_BRANCH]; 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_MCRR]           = 2 +   ARM9ClassCycles [ARMCycleEstimation :: ARM9_BRANCH];
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_MRC_NPC]        = 1 +   ARM9ClassCycles [ARMCycleEstimation :: ARM9_BRANCH];     // Rd is not available for one cycle. 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_MRC_PC]         = 4 +   ARM9ClassCycles [ARMCycleEstimation :: ARM9_BRANCH]; 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_MRRC]           = 2 +   ARM9ClassCycles [ARMCycleEstimation :: ARM9_BRANCH];     // Rn is not available for one cycle. 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_MRS]            = 2; 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_MSR]            = 1;    // +2 if any of the csx fields are updated.  
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_MUL_MLA]        = 2;    // Rd is not available for one cycle, except as an accumulator input for a multiply accumulate. 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_MULS_MLAS]      = 4;
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_XMULL_XMLAL]    = 3;    // RdHi is not available for one cycle, except as an accumulator input for a multiply accumulate. 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_XMULLS_XMLALS]  = 5;
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_PLD]            = 1;
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_QX_ADDSUB]      = 1;    // Rd is not available for one cycle. 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_SMUL_SMLA_XY]   = 1;    // Rd is not available for one cycle except as an accumulator input for a multiply accumulate. 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_SMLAL_XY]       = 1;    // RdHi is not available for one cycle except as an accumulator input for a multiply accumulate. 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_STC]            =       ARM9ClassCycles [ARMCycleEstimation :: ARM9_BRANCH];      // +N Later on
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_STR_BH]         = 1;    // +1 if a shifted offset is used.  
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_STRD]           = 2; 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_STM]            = 0;    // +N Later on, +1 if N = 1. 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_SWI]            = 3; 
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_SWP_B]          = 2;    // Rd is not available for one cycle.
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_MOV2PIECES]     = 2;
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_THUMB]          = 0;
    ARM9ClassCycles [ARMCycleEstimation :: ARM9_INVALID]        = 0;
}

unsigned int ARMCycleEstimation :: getRegOperandsCount(const MachineInstr &MachInstr)
{
    int  noOfOperands = MachInstr.getNumOperands();
    int  i, N = 0;

    // Start from 1 because 0 is the Base Register
    for(i = 1; i < noOfOperands; i++)
    {
        MachineOperand MO = MachInstr.getOperand(i); 

        if(MO.isReg())
            if(MO.getReg() != 0)        // 0 is %reg0 and is not counted for N
                N++;
    }

    return N;
}

unsigned int ARMCycleEstimation::getInstrAddressingMode(const MachineInstr &MachInstr)
{
    const TargetInstrDesc &Desc = MachInstr.getDesc();

    switch (Desc.TSFlags & ARMII::AddrModeMask) 
    {
        case ARMII::AddrModeNone:
            return 0;
            
        case ARMII::AddrMode1:
            return 1;
            
        case ARMII::AddrMode2:
            return 2;
            
        case ARMII::AddrMode3:
            return 3;
            
        case ARMII::AddrMode4:
            return 4;

        default:
            cout << "Unknown Addressing Mode" << endl;
            
  }

  return 0;
}

unsigned int ARMCycleEstimation::getShiftOp(const MachineOperand &MO){
  switch (ARM_AM::getAM2ShiftOpc(MO.getImm())) {
  default: assert(0 && "Unknown shift opc!");
  case ARM_AM::asr:
        cout << "\t (asr)";
        return 2;
  case ARM_AM::lsl: 
        cout << "\t (lsl)";
        return 0;
  case ARM_AM::lsr: 
        cout << "\t (lsr)";
        return 1;
  case ARM_AM::ror:
  case ARM_AM::rrx:
        cout << "\t (ror/rrx)";
        return 3;
  }
  return 0;
}

unsigned int ARMCycleEstimation::isShiftedOffset(const MachineInstr &MachInstr)
{
    const MachineOperand &MO2 = MachInstr.getOperand(2);
    const MachineOperand &MO3 = MachInstr.getOperand(3);
    const MachineOperand &MO4 = MachInstr.getOperand(4);

    if (!MO2.getReg())  // is immediate
        return 0;  

    assert(TargetRegisterInfo::isPhysicalRegister(MO2.getReg()));
    if(MO3.isImm())
    {
        //if (unsigned ShImm = ARM_AM::getAM2Offset(MO3.getImm())) 
        if (ARM_AM::getAM2Offset(MO3.getImm())) 
        {
            //cout << "\tShiftOp: " << getShiftOp(MO3);
            //cout << "\tShiftImm: " << ShImm << endl;
            return 1;
        }
    }
    else if(MO4.isImm())
    {
        if (ARM_AM::getAM2Offset(MO4.getImm())) 
        {
            return 1;
        }
    }
    
    return 0;
}

unsigned int ARMCycleEstimation :: calculateExtraCycles(const MachineInstr &MachInstr, ARM9InstrClasses &instructionClass)
{
    unsigned int extraCycles = 0;
    
    switch (instructionClass)
    {
        case ARM9_UNKNOWN:
            DERR(PRINT_WARNINGS) << "Unknown ARM Instruction: " << MachInstr.getDesc().Name << std::endl;
            // assert(0 && "Unknown ARM Instruction !!!");
            break;

        case ARM9_GENERIC:
            DERR(PRINT_WARNINGS) << "Warning [Counted 1 Cycle for Annotation]: ARM9_GENERIC Class Instruction: " << MachInstr.getDesc().Name << std::endl;
        break;

        case ARM9_ALU:              // +2 if Rd is pc
        case ARM9_ALU_RSL:          // +2 more if Rd is pc
            if(ARMRegisterInfo::getRegisterNumbering(MachInstr.getOperand(0).getReg()) == 15)        // +2 if Rd is PC
                extraCycles = 2;
            //ARMRegisterInfo::getRegisterNumbering();
            //MachInstr.getOperand(0).getReg();
            
            //ARMRegisterInfo::
        break;

        case ARM9_BRANCH:       
        case ARM9_BRANCH_LINK:               
        break;

        case ARM9_CDP:
        case ARM9_LDC:              // +N Later on
        case ARM9_MCR:
        case ARM9_MCRR:    
        case ARM9_MRC_NPC:          // Rd is not available for one cycle. 
        case ARM9_MRC_PC:
        case ARM9_MRRC:
        case ARM9_MRS:
        case ARM9_MSR:              // +2 if any of the csx fields are updated
        case ARM9_STC:              // +N Later on
        case ARM9_SWI:
        case ARM9_SWP_B:            // Rd is not available for one cycle.
            DERR(PRINT_WARNINGS) << "ARM 9 Un-used Instruction Classes: " << MachInstr.getDesc().Name << std::endl;
        break;

        case ARM9_LDRB:             // Rd is not available for two cycles. +1 if the load offset is shifted.
            if(getInstrAddressingMode(MachInstr) == 2)
            {
                 if(isShiftedOffset(MachInstr))
                {
                    extraCycles++;
                }
            }
        break;

        case ARM9_LDRD:
        case ARM9_STRD:
        break;
            
        case ARM9_LDR_NPC:          // Rd is not available for two cycles. +1 if the load offset is shifted.
            if(ARMRegisterInfo::getRegisterNumbering(MachInstr.getOperand(0).getReg()) == 15)
            {
                instructionClass = ARM9_LDR_PC;     // +1 if the load offset is shifted.
                extraCycles = 4;
            }

            if(getInstrAddressingMode(MachInstr) == 2)
            {
                if(isShiftedOffset(MachInstr))
                {
                    extraCycles++;
                }
            }
        break;

        case ARM9_LDR_PC:           // This is empty because this class has not been assigned to any Instructions.
        break; 
            
        case ARM9_LDM_NPC:          // +N Later on, +1 if N = 1 or the last loaded register used in the next cycle
        {
            int  N = getRegOperandsCount(MachInstr);
            if(N == 1) 
                N++;

            extraCycles = N;
        }
        break;
            
        case ARM9_LDM_PC:           // +N Later on
        {
            int  N = getRegOperandsCount(MachInstr);
            extraCycles = N;
        }
        break;

        case ARM9_MUL_MLA:          // Rd is not available for one cycle, except as an accumulator input for a multiply accumulate.
                                    // No Early Termination, but Check for Rd Availability. 
        break;

        case ARM9_MULS_MLAS: 
        break;

        case ARM9_PLD:
        break;

        case ARM9_QX_ADDSUB:
        break;

        case ARM9_SMUL_SMLA_XY:
        case ARM9_SMLAL_XY:
        break;
            
        case ARM9_XMULL_XMLAL:
        case ARM9_XMULLS_XMLALS:
        break;
            
        case ARM9_STR_BH:           // +1 if a shifted offset is used.
            if(getInstrAddressingMode(MachInstr) == 2)
            {
                 if(isShiftedOffset(MachInstr))
                {
                    extraCycles++;
                }
            }
        break;

        case ARM9_STM:              // +N Later on, +1 if N = 1.
        {
            int  N = getRegOperandsCount(MachInstr);
            if(N == 1)
                N++;

            extraCycles = N;
        }
        break;
            
        case ARM9_MOV2PIECES:
        break;
            
        case ARM9_THUMB:
            DERR(PRINT_WARNINGS) << "ARM9_THUMB Unsupported Instructions: " << MachInstr.getDesc().Name << std::endl;
        break;
            
        case ARM9_INVALID:
            // Invalid means that we were not expecting this Instruction, So Ignored from Annotation.
            DERR(PRINT_WARNINGS) << "Warning [Ignored for Annotation]: ARM9_INVALID Instruction : " << MachInstr.getDesc().Name << std::endl;
            // assert(0 && "Unknown ARM Instruction !!!");
        break;
            
        default:
            DERR(PRINT_WARNINGS) << "ARM 9 Unknown Instruction: " << MachInstr.getDesc().Name << std::endl;
        break;            
    }

    return extraCycles;
}

void ARMTargetMachine::InitializeMBBAnnotation()
{
    ARMCycleEstimation::initInstrClassCycles();
    ARMCycleEstimation::initInstrClassAssignment();
}

const TargetAnnotationDB * ARMTargetMachine::MBBBranchPenaltyAnnotation(MachineBasicBlock &MBB, MachineBasicBlock &SuccMBB)
{
    TargetAnnotationDB *branchPenaltyAnnotationDB = NULL;

    MachineBasicBlock::const_iterator FirstInstr, LastInstr, CurrInstr;

    if(MBB.size() > 0)
    {
        ARMCycleEstimation::ARM9InstrClasses instructionClass;

        LastInstr = MBB.end();
        --LastInstr;

        instructionClass = ARMCycleEstimation::ARM9InstrClassAssignment[(*LastInstr).getOpcode()];
        if(instructionClass == ARMCycleEstimation::ARM9_BRANCH)
        {
            int numOperands = LastInstr->getNumOperands();
    
            for(int i=0; i < numOperands; i++)
            {
                MachineOperand MO = LastInstr->getOperand(i);

                if(MO.isMBB())
                {
                    MachineBasicBlock* TargetMBB = MO.getMBB();

                    if(TargetMBB->getNumber() ==  SuccMBB.getNumber())
                    {
                        branchPenaltyAnnotationDB = new TargetAnnotationDB();
                        assert( branchPenaltyAnnotationDB && "NULL pointer!\n" );    
                    
                        branchPenaltyAnnotationDB->setType(MBB_BRANCHPENALTY);
                        branchPenaltyAnnotationDB->setCycleCount(2);
                        break;
                    }
                }
            }
        }
    }

    return (branchPenaltyAnnotationDB);
}


const TargetAnnotationDB * ARMTargetMachine::MachineBasicBlockAnnotation(MachineBasicBlock &MBB)
{
    TargetAnnotationDB  *annotationDB = NULL;

    unsigned int mbbInstructionCount = 0;
    unsigned int mbbLoadCount = 0;
    unsigned int mbbStoreCount = 0;
    
    unsigned int mbbInstructionCycleCount = 0;
    unsigned int mbbDependencyCycleCount = 0;
    unsigned int mbbTotalCycles = 0;

    unsigned int prevInstrNo, currInstrNo;

    MachineBasicBlock::const_iterator FirstInstr, LastInstr;
    MachineBasicBlock::const_iterator CurrInstr, PrevInstr;
    
    FirstInstr = MBB.begin();
    LastInstr = MBB.end();    

    // Cycle Estimation for Each Instruction. 
    for(CurrInstr = FirstInstr; 
        CurrInstr != LastInstr; 
        CurrInstr++)
    {
        mbbInstructionCycleCount += ARMCycleEstimation::estimateInstructionCycles (*CurrInstr);    
        mbbInstructionCount++;
    }

    if(mbbInstructionCount >= 2)   // Atleast Two Instructions are needed to perform any kindof of Dependency Analysis in each MBB
    {
        currInstrNo = 0;
        prevInstrNo = currInstrNo++;

        // Cycle Estimation due to Intra-Instruction Dependencies.
        for (CurrInstr = FirstInstr, PrevInstr = CurrInstr++; 
             CurrInstr != LastInstr; 
             PrevInstr = CurrInstr++) 
        {
            unsigned int 	    dependencyCycles = 0;

#ifdef PRINT_SUMMARY
            cout << "[" << prevInstrNo << "]: " << *PrevInstr;
            prevInstrNo = currInstrNo++;
#endif            

            dependencyCycles = ARMCycleEstimation::estimateDependencyCycles(*PrevInstr, *CurrInstr);                       // Convert to Cumulative Count +=
            mbbDependencyCycleCount += dependencyCycles;
            
#ifdef PRINT_SUMMARY
            if(dependencyCycles)
            {
                cout << "*** Dependency Cycles: " << setw(3) << dependencyCycles << " ***" << endl;
            }
#endif            
        }

#ifdef PRINT_SUMMARY             
        cout << "[" << prevInstrNo << "]: " << *PrevInstr;
#endif
    }    

    mbbTotalCycles = mbbInstructionCycleCount + mbbDependencyCycleCount;

    // Calculate Load/Store Count for this MBB
    mbbLoadCount = ARMCycleEstimation::getDataLoadCount(MBB);
    mbbStoreCount = ARMCycleEstimation::getDataStoreCount(MBB);

#ifdef PRINT_SUMMARY    
    cout << "SUMMARY" << std::endl;
    cout << "==============================================================" << std::endl;
    cout << "Total Instructions: " << setw(3) << mbbInstructionCount; 
    cout << "\t\tTotal Loads: " << setw(3) << mbbLoadCount << "\t\t\tTotal Stores: " << setw(3) << mbbStoreCount << endl;
    cout << "Cycles Estimate: " << setw(3) << mbbInstructionCycleCount ;
    cout << "\t\tDependency Cycles: " << setw(3) << mbbDependencyCycleCount;
    cout << "\t\tTotal Cycles : " << setw(4) << mbbTotalCycles << "\t\tCPI: " << setw(4) << (float)mbbTotalCycles /(float) mbbInstructionCount << std::endl;
    cout <<"==============================================================" << std::endl << std::endl;
#endif

    if( mbbInstructionCycleCount > 0 )
    {
        annotationDB = new TargetAnnotationDB();
        assert( annotationDB && "NULL pointer!\n" );

        // The entry and return types will be handled in the Machine Independent Code
        annotationDB->setType(MBB_DEFAULT);
        //annotationDB->setType( (MBB.pred_empty() ? MBB_ENTRY : MBB_DEFAULT) | (MBB.succ_empty() ? MBB_RETURN : MBB_DEFAULT) );
        // cout << "MBB Name: " << MBB.getName().str() << "  Annotation DB Type: " << annotationDB->getType() << endl;

        annotationDB->setInstructionCount(mbbInstructionCount);
        annotationDB->setCycleCount(mbbTotalCycles);
        annotationDB->setLoadCount(mbbLoadCount);
        annotationDB->setStoreCount(mbbStoreCount);        
    }

    return(annotationDB);
}

unsigned int ARMCycleEstimation :: estimateDependencyCycles(const MachineInstr &PrevInstr, const MachineInstr &CurrInstr)
{
    ARM9InstrClasses instructionClass = ARM9InstrClassAssignment[PrevInstr.getOpcode()];
    unsigned int instrDependencyCycles = 0;
    
    switch(instructionClass)
    {
        case ARM9_LDRB: // Rd is not available for two cycles.
        {
            int prevInstrRd = PrevInstr.getOperand(0).getReg();
            int currInstrOperandCount = CurrInstr.getNumOperands();
            
            for(int i = 0; i < currInstrOperandCount; i++)
            {
                if(CurrInstr.getOperand(i).isReg())
                {
                    int currInstrReg = CurrInstr.getOperand(i).getReg();
                    if(currInstrReg == prevInstrRd)
                    {
                        instrDependencyCycles = 2;
                        break;
                    }
                }
            }
        }
        break;
            
        case ARM9_LDR_NPC: // Rd is not available for one cycle. 
        {
            int prevInstrRd = PrevInstr.getOperand(0).getReg();
            int currInstrOperandCount = CurrInstr.getNumOperands();
            
            for(int i = 0; i < currInstrOperandCount; i++)
            {
                if(CurrInstr.getOperand(i).isReg())
                {
                    int currInstrReg = CurrInstr.getOperand(i).getReg();
                    if(currInstrReg == prevInstrRd)
                    {
                        instrDependencyCycles = 1;
                        break;
                    }
                }
            }
        }
        break;

        case ARM9_LDRD:     // R(d+1) is not available for one cycle.
        {
            int prevInstrRd = PrevInstr.getOperand(1).getReg();  // Check this line
            int currInstrOperandCount = CurrInstr.getNumOperands();

            cout << "LDRD Instruction Found: Verify Dependency Analysis Code " << endl;
            
            for(int i = 0; i < currInstrOperandCount; i++)
            {
                if(CurrInstr.getOperand(i).isReg())
                {
                    int currInstrReg = CurrInstr.getOperand(i).getReg();
                    if(currInstrReg == prevInstrRd)
                    {
                        instrDependencyCycles = 1;
                        break;
                    }
                }
            }
        }
        break;
            
        case ARM9_LDM_NPC:  // +N Later on, +1 if N = 1 or the last loaded register used in the next cycle 
        {
            int prevInstrOperandCount = PrevInstr.getNumOperands();
            int currInstrOperandCount = CurrInstr.getNumOperands();
            int prevInstrLastReg = PrevInstr.getOperand(prevInstrOperandCount-1).getReg();

            for(int i = 0; i < currInstrOperandCount; i++)
            {
                if(CurrInstr.getOperand(i).isReg())
                {
                    int currInstrReg = CurrInstr.getOperand(i).getReg();
                    if(currInstrReg == prevInstrLastReg)
                    {
                        instrDependencyCycles = 1;
                        break;
                    }
                }
            }
        }
        break;

        case ARM9_MUL_MLA: // Rd is not available for one cycle, except as an accumulator input for a multiply accumulate. 
        case ARM9_SMUL_SMLA_XY: // Rd is not available for one cycle except as an accumulator input for a multiply accumulate. 
        {
            int prevInstrRd = PrevInstr.getOperand(0).getReg();
            int currInstrOperandCount = CurrInstr.getNumOperands();

            // TODO: Handle the except as an accumulator input for multiply accumulate case. 
            for(int i = 0; i < currInstrOperandCount; i++)
            {
                if(CurrInstr.getOperand(i).isReg())
                {
                    int currInstrReg = CurrInstr.getOperand(i).getReg();
                    if(currInstrReg == prevInstrRd)
                    {
                        instrDependencyCycles = 1;
                        break;
                    }
                }
            }
        }
        break;

        case ARM9_SMLAL_XY:  // RdHi is not available for one cycle except as an accumulator input for a multiply accumulate. 
        {
            int prevInstrRd = PrevInstr.getOperand(1).getReg();
            int currInstrOperandCount = CurrInstr.getNumOperands();

            // TODO: Handle the except as an accumulator input for multiply accumulate case. 
            for(int i = 0; i < currInstrOperandCount; i++)
            {
                if(CurrInstr.getOperand(i).isReg())
                {
                    int currInstrReg = CurrInstr.getOperand(i).getReg();
                    if(currInstrReg == prevInstrRd)
                    {
                        instrDependencyCycles = 1;
                        break;
                    }
                }
            }
        }
        break;

            
        case ARM9_MRC_NPC: // Rd is not available for one cycle. 
        case ARM9_MRRC:   // Rn is not available for one cycle. 
        case ARM9_XMULL_XMLAL: // RdHi is not available for one cycle, except as an accumulator input for a multiply accumulate.
        case ARM9_QX_ADDSUB: // Rd is not available for one cycle.  
        case ARM9_SWP_B: // Rd is not available for one cycle.
            break;

        default:
            break;
    }

    return (instrDependencyCycles);
}

unsigned int ARMCycleEstimation::getDataLoadCount(MachineBasicBlock &MBB)
{
    unsigned int mbbLoadCount = 0;
    MachineBasicBlock::const_iterator FirstInstr, LastInstr, CurrInstr;

    FirstInstr = MBB.begin();
    LastInstr = MBB.end();

    for(CurrInstr = FirstInstr; CurrInstr != LastInstr; CurrInstr++)
    {
        ARM9InstrClasses instructionClass;
        unsigned int currInstrLoadCount = 0;

        instructionClass = ARM9InstrClassAssignment[CurrInstr->getOpcode()];
        switch(instructionClass)
        {
            // The following load only one data memory location
            case ARM9_LDRB:
            case ARM9_LDR_NPC:
            case ARM9_LDR_PC:
            currInstrLoadCount++;  
            mbbLoadCount += currInstrLoadCount;
            break;

            case ARM9_LDRD:
            currInstrLoadCount+=2;  
            mbbLoadCount += currInstrLoadCount;                
            break;

            // The following load multiple data memory locations    
            case ARM9_LDM_NPC:
            case ARM9_LDM_PC:
            {
                int operandCount = CurrInstr->getNumOperands();
                
                // The first parameter is a Register but it is the Base Address of Memory (Load/Store Address), so we start from the second operand.
                for(int i = 1; i < operandCount; i++)
                {
                    if(CurrInstr->getOperand(i).isReg())
                        if(CurrInstr->getOperand(i).getReg() != 0)
                            currInstrLoadCount++;
                }
            }
            mbbLoadCount += currInstrLoadCount;
            break;

            default:
            break;
        }
    }

    return(mbbLoadCount);
}

unsigned int ARMCycleEstimation::getDataStoreCount(MachineBasicBlock &MBB)
{
    unsigned int mbbStoreCount = 0;
    MachineBasicBlock::const_iterator FirstInstr, LastInstr, CurrInstr;

    FirstInstr = MBB.begin();
    LastInstr = MBB.end();

    for(CurrInstr = FirstInstr; CurrInstr != LastInstr; CurrInstr++)
    {
        ARM9InstrClasses instructionClass;
        unsigned int currInstrStoreCount = 0;

        instructionClass = ARM9InstrClassAssignment[CurrInstr->getOpcode()];

        switch(instructionClass)
        {
            case ARM9_STR_BH:
            currInstrStoreCount++;  
            mbbStoreCount += currInstrStoreCount;
            break;

            case ARM9_STRD:
            currInstrStoreCount+=2;  
            mbbStoreCount += currInstrStoreCount;                
            break;
    
            case ARM9_STM:
            {
                int operandCount = CurrInstr->getNumOperands();

                // The first parameter is a Register but it is the Base Address of Memory (Load/Store Address), so we start from the second operand.
                for(int i = 1; i < operandCount; i++)
                {
                    if(CurrInstr->getOperand(i).isReg())
                        if(CurrInstr->getOperand(i).getReg() != 0)
                            currInstrStoreCount++;
                }
            }
            mbbStoreCount += currInstrStoreCount;
            break;

            default:
            break;
        }
     }

     return(mbbStoreCount);
}

unsigned int ARMCycleEstimation :: estimateInstructionCycles(const MachineInstr &MachInstr)
{
    // Identify the Class of Current Instruction. 
    ARM9InstrClasses instructionClass = ARM9InstrClassAssignment[MachInstr.getOpcode()];

    // Get Basic Class Based Cycle Estimate 
    unsigned int instrCycleCount = ARM9ClassCycles [instructionClass];

    // Calculate the Extra Number of Cycles Needed in Special Cases. 
    instrCycleCount += calculateExtraCycles(MachInstr, instructionClass);
    
    return(instrCycleCount);
}

