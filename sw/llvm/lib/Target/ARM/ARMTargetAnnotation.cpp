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

#include <iomanip>
#include "llvm/Target/TargetAnnotationInfo.h"

#include "ARM.h"
#include "ARMInstrInfo.h"
#include "ARMTargetMachine.h"
#include "ARMAddressingModes.h"
#include "llvm/Support/Debug.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace std;

bool PRINT_WARNINGS;

static cl::opt<bool, true> PrintAnnotationWarnings("print-annotation-warnings",
       cl::desc("Print warnings during annoatation pass."),
       cl::location(PRINT_WARNINGS), cl::init(false));

//#define PRINT_SUMMARY

#define DERR(flag) \
  if(flag) cerr

namespace ARMCycleEstimation {
    enum ARM9InstrClasses {
        ARM9_GENERIC        = 0, 
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
    ARM9InstrClasses ARM9InstrClassAssignment[ARM::INSTRUCTION_LIST_END] = {ARM9_GENERIC};
        
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

    ARM9InstrClassAssignment[ARM::PHI               ] = ARM9_INVALID;           // Should not be present in Code. 
    ARM9InstrClassAssignment[ARM::INLINEASM	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::DBG_LABEL	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::EH_LABEL	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::GC_LABEL	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::DECLARE           ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::EXTRACT_SUBREG    ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::INSERT_SUBREG	    ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::IMPLICIT_DEF	    ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::SUBREG_TO_REG	    ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::ADCri	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::ADCrr	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::ADCrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::ADDSri	        ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::ADDSrr	        ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::ADDSrs	        ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::ADDri	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::ADDrr	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::ADDrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::ADJCALLSTACKDOWN	] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::ADJCALLSTACKUP	] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::ANDri	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::ANDrr	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::ANDrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::B	                ] = ARM9_BRANCH;
    ARM9InstrClassAssignment[ARM::BICri	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::BICrr	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::BICrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::BL	            ] = ARM9_BRANCH_LINK;
    ARM9InstrClassAssignment[ARM::BLX	            ] = ARM9_BRANCH_LINK;
    ARM9InstrClassAssignment[ARM::BL_pred	        ] = ARM9_BRANCH_LINK;
    ARM9InstrClassAssignment[ARM::BR_JTadd	        ] = ARM9_BRANCH;
    ARM9InstrClassAssignment[ARM::BR_JTm	        ] = ARM9_BRANCH;
    ARM9InstrClassAssignment[ARM::BR_JTr	        ] = ARM9_BRANCH;
    ARM9InstrClassAssignment[ARM::BX	            ] = ARM9_BRANCH;
    ARM9InstrClassAssignment[ARM::BX_RET	        ] = ARM9_BRANCH;
    ARM9InstrClassAssignment[ARM::Bcc	            ] = ARM9_BRANCH;
    ARM9InstrClassAssignment[ARM::CLZ	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::CMNnzri	        ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::CMNnzrr	        ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::CMNnzrs	        ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::CMNri	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::CMNrr	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::CMNrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::CMPnzri	        ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::CMPnzrr	        ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::CMPnzrs	        ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::CMPri	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::CMPrr	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::CMPrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::CONSTPOOL_ENTRY	] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::DWARF_LOC	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::EORri	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::EORrr	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::EORrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::FABSD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FABSS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FADDD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FADDS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FCMPED	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FCMPES	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FCMPEZD	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FCMPEZS	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FCPYD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FCPYDcc	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FCPYS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FCPYScc	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FCVTDS	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FCVTSD	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FDIVD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FDIVS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FLDD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FLDMD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FLDMS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FLDS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FMACD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FMACS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FMDRR	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FMRRD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FMRS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FMSCD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FMSCS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FMSR	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FMSTAT	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FMULD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FMULS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FNEGD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FNEGDcc	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FNEGS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FNEGScc	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FNMACD	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FNMACS	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FNMSCD	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FNMSCS	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FNMULD	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FNMULS	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FSITOD	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FSITOS	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FSQRTD	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FSQRTS	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FSTD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FSTMD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FSTMS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FSTS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FSUBD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FSUBS	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FTOSIZD	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FTOSIZS	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FTOUIZD	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FTOUIZS	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FUITOD	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::FUITOS	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::LDM	            ] = ARM9_LDM_NPC;
    ARM9InstrClassAssignment[ARM::LDM_RET	        ] = ARM9_LDM_PC;
    ARM9InstrClassAssignment[ARM::LDR	            ] = ARM9_LDR_NPC;
    ARM9InstrClassAssignment[ARM::LDRB	            ] = ARM9_LDRB;
    ARM9InstrClassAssignment[ARM::LDRB_POST	        ] = ARM9_LDRB;
    ARM9InstrClassAssignment[ARM::LDRB_PRE	        ] = ARM9_LDRB;
    ARM9InstrClassAssignment[ARM::LDRD	            ] = ARM9_LDRD;
    ARM9InstrClassAssignment[ARM::LDRH	            ] = ARM9_LDRB;
    ARM9InstrClassAssignment[ARM::LDRH_POST	        ] = ARM9_LDRB;
    ARM9InstrClassAssignment[ARM::LDRH_PRE	        ] = ARM9_LDRB;
    ARM9InstrClassAssignment[ARM::LDRSB	            ] = ARM9_LDRB;
    ARM9InstrClassAssignment[ARM::LDRSB_POST	    ] = ARM9_LDRB;
    ARM9InstrClassAssignment[ARM::LDRSB_PRE	        ] = ARM9_LDRB;
    ARM9InstrClassAssignment[ARM::LDRSH	            ] = ARM9_LDRB;
    ARM9InstrClassAssignment[ARM::LDRSH_POST	    ] = ARM9_LDRB;
    ARM9InstrClassAssignment[ARM::LDRSH_PRE	        ] = ARM9_LDRB;
    ARM9InstrClassAssignment[ARM::LDR_POST	        ] = ARM9_LDR_NPC;
    ARM9InstrClassAssignment[ARM::LDR_PRE	        ] = ARM9_LDR_NPC;
    ARM9InstrClassAssignment[ARM::LDRcp	            ] = ARM9_LDR_NPC;
    ARM9InstrClassAssignment[ARM::LEApcrel	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::LEApcrelJT	    ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::MLA	            ] = ARM9_MUL_MLA;
    ARM9InstrClassAssignment[ARM::MOVCCi	        ] = ARM9_GENERIC;  /* Classify This Instruction to More Relevent Class Like ALU Class */ 
    ARM9InstrClassAssignment[ARM::MOVCCr	        ] = ARM9_GENERIC;  /* Classify This Instruction to More Relevent Class Like ALU Class */
    ARM9InstrClassAssignment[ARM::MOVCCs	        ] = ARM9_GENERIC;  /* Classify This Instruction to More Relevent Class Like ALU Class */
    ARM9InstrClassAssignment[ARM::MOVi	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::MOVi2pieces	    ] = ARM9_MOV2PIECES;
    ARM9InstrClassAssignment[ARM::MOVr	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::MOVrx	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::MOVs	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::MOVsra_flag	    ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::MOVsrl_flag	    ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::MUL	            ] = ARM9_MUL_MLA;
    ARM9InstrClassAssignment[ARM::MVNi	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::MVNr	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::MVNs	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::ORRri	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::ORRrr	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::ORRrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::PICADD	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::PICLD	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::PICLDB	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::PICLDH	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::PICLDSB	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::PICLDSH	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::PICLDZB	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::PICLDZH	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::PICSTR	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::PICSTRB	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::PICSTRH	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::PKHBT	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::PKHTB	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::REV	            ] = ARM9_GENERIC;  /* Classify This Instruction to More Relevent Class Like ALU Class */
    ARM9InstrClassAssignment[ARM::REV16	            ] = ARM9_GENERIC;  /* Classify This Instruction to More Relevent Class Like ALU Class */
    ARM9InstrClassAssignment[ARM::REVSH	            ] = ARM9_GENERIC;  /* Classify This Instruction to More Relevent Class Like ALU Class */
    ARM9InstrClassAssignment[ARM::RSBSri	        ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::RSBSrs	        ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::RSBri	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::RSBrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::RSCri	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::RSCrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::SBCri	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::SBCrr	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::SBCrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::SMLABB	        ] = ARM9_SMUL_SMLA_XY;
    ARM9InstrClassAssignment[ARM::SMLABT	        ] = ARM9_SMUL_SMLA_XY;
    ARM9InstrClassAssignment[ARM::SMLATB	        ] = ARM9_SMUL_SMLA_XY;
    ARM9InstrClassAssignment[ARM::SMLATT	        ] = ARM9_SMUL_SMLA_XY;
    ARM9InstrClassAssignment[ARM::SMLAWB	        ] = ARM9_SMUL_SMLA_XY;
    ARM9InstrClassAssignment[ARM::SMLAWT	        ] = ARM9_SMUL_SMLA_XY;
    ARM9InstrClassAssignment[ARM::SMULBB	        ] = ARM9_SMUL_SMLA_XY;
    ARM9InstrClassAssignment[ARM::SMULBT	        ] = ARM9_SMUL_SMLA_XY;
    ARM9InstrClassAssignment[ARM::SMULTB	        ] = ARM9_SMUL_SMLA_XY;
    ARM9InstrClassAssignment[ARM::SMULTT	        ] = ARM9_SMUL_SMLA_XY;
    ARM9InstrClassAssignment[ARM::SMULWB	        ] = ARM9_SMUL_SMLA_XY;
    ARM9InstrClassAssignment[ARM::SMULWT	        ] = ARM9_SMUL_SMLA_XY;
    ARM9InstrClassAssignment[ARM::SMLAL	            ] = ARM9_SMLAL_XY;
    ARM9InstrClassAssignment[ARM::SMULL	            ] = ARM9_SMLAL_XY;
    ARM9InstrClassAssignment[ARM::UMLAL	            ] = ARM9_SMLAL_XY;
    ARM9InstrClassAssignment[ARM::UMULL	            ] = ARM9_SMLAL_XY;
    ARM9InstrClassAssignment[ARM::UMAAL	            ] = ARM9_SMLAL_XY;
    ARM9InstrClassAssignment[ARM::SMMLA	            ] = ARM9_SMUL_SMLA_XY;          // Reconsider the Classification of this Instruction. 
    ARM9InstrClassAssignment[ARM::SMMLS	            ] = ARM9_SMUL_SMLA_XY;          // Reconsider the Classification of this Instruction. 
    ARM9InstrClassAssignment[ARM::SMMUL	            ] = ARM9_SMUL_SMLA_XY;          // Reconsider the Classification of this Instruction. 
    ARM9InstrClassAssignment[ARM::STM	            ] = ARM9_STM;
    ARM9InstrClassAssignment[ARM::STR	            ] = ARM9_STR_BH;
    ARM9InstrClassAssignment[ARM::STRB	            ] = ARM9_STR_BH;
    ARM9InstrClassAssignment[ARM::STRB_POST	        ] = ARM9_STR_BH;
    ARM9InstrClassAssignment[ARM::STRB_PRE	        ] = ARM9_STR_BH;
    ARM9InstrClassAssignment[ARM::STRD	            ] = ARM9_STRD;     
    ARM9InstrClassAssignment[ARM::STRH	            ] = ARM9_STR_BH;
    ARM9InstrClassAssignment[ARM::STRH_POST	        ] = ARM9_STR_BH;
    ARM9InstrClassAssignment[ARM::STRH_PRE	        ] = ARM9_STR_BH;
    ARM9InstrClassAssignment[ARM::STR_POST	        ] = ARM9_STR_BH;
    ARM9InstrClassAssignment[ARM::STR_PRE	        ] = ARM9_STR_BH;
    ARM9InstrClassAssignment[ARM::SUBSri	        ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::SUBSrr	        ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::SUBSrs	        ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::SUBri	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::SUBrr	            ] = ARM9_ALU;
    ARM9InstrClassAssignment[ARM::SUBrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::SXTABrr	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::SXTABrr_rot	    ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::SXTAHrr	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::SXTAHrr_rot	    ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::SXTBr	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::SXTBr_rot	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::SXTHr	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::SXTHr_rot	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::TEQri	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::TEQrr	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::TEQrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::TPsoft	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::TSTri	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::TSTrr	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::TSTrs	            ] = ARM9_ALU_RSL;
    ARM9InstrClassAssignment[ARM::UXTABrr	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::UXTABrr_rot	    ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::UXTAHrr	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::UXTAHrr_rot	    ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::UXTB16r	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::UXTB16r_rot	    ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::UXTBr	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::UXTBr_rot	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::UXTHr	            ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::UXTHr_rot	        ] = ARM9_GENERIC;
    ARM9InstrClassAssignment[ARM::tADC	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tADDS	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tADDhirr	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tADDi3	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tADDi8	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tADDrPCi	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tADDrSPi	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tADDrr	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tADDspi	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tADJCALLSTACKDOWN	] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tADJCALLSTACKUP	] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tAND	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tASRri	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tASRrr	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tB	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tBIC	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tBL	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tBLXi	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tBLXr	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tBR_JTr	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tBX	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tBX_RET	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tBX_RET_vararg	] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tBcc	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tBfar	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tCMN	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tCMNNZ	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tCMPNZi8	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tCMPNZr	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tCMPi8	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tCMPr	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tEOR	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLDR	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLDRB	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLDRH	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLDRSB	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLDRSH	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLDRcp	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLDRpci	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLDRspi	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLEApcrel	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLEApcrelJT	    ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLSLri	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLSLrr	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLSRri	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tLSRrr	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tMOVCCr	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tMOVi8	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tMOVr	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tMUL	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tMVN	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tNEG	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tORR	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tPICADD	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tPOP	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tPOP_RET	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tPUSH	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tREV	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tREV16	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tREVSH	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tROR	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tRestore	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSBC	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSTR	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSTRB	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSTRH	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSTRspi	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSUBS	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSUBi3	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSUBi8	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSUBrr	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSUBspi	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSXTB	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSXTH	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tSpill	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tTPsoft	        ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tTST	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tUXTB	            ] = ARM9_THUMB;
    ARM9InstrClassAssignment[ARM::tUXTH	            ] = ARM9_THUMB;
}

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

        if(MO.isRegister())
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
        case ARM9_GENERIC:
            DERR(PRINT_WARNINGS) << "ARM9_GENERIC Class Instruction: " << MachInstr.getDesc().Name << std::endl;
        break;

        case ARM9_ALU:              // +2 if Rd is pc
        case ARM9_ALU_RSL:          // +2 more if Rd is pc
            if(ARMRegisterInfo::getRegisterNumbering(MachInstr.getOperand(0).getReg()) == 15)        // +2 if Rd is PC
                extraCycles = 2;
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
            DERR(PRINT_WARNINGS) << "ARM9_INVALID Instruction: " << MachInstr.getDesc().Name << std::endl;
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

                if(MO.isMachineBasicBlock())
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

        annotationDB->setType( (MBB.pred_empty() ? MBB_ENTRY:0) | (MBB.succ_empty() ? MBB_RETURN:0) );
        //annotationDB->setType( MBB_DEFAULT );
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
                if(CurrInstr.getOperand(i).isRegister())
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
                if(CurrInstr.getOperand(i).isRegister())
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
                if(CurrInstr.getOperand(i).isRegister())
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
                if(CurrInstr.getOperand(i).isRegister())
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
                if(CurrInstr.getOperand(i).isRegister())
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
                if(CurrInstr.getOperand(i).isRegister())
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
                    if(CurrInstr->getOperand(i).isRegister())
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
                    if(CurrInstr->getOperand(i).isRegister())
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
    unsigned int instrCycleCount = 0;
    ARM9InstrClasses instructionClass = ARM9_GENERIC;

    // Identify the Class of Current Instruction. 
    instructionClass = ARM9InstrClassAssignment[MachInstr.getOpcode()];

    // Get Basic Class Based Cycle Estimate 
    instrCycleCount = ARM9ClassCycles [instructionClass]; 

    // Calculate the Extra Number of Cycles Needed in Special Cases. 
    instrCycleCount += calculateExtraCycles(MachInstr, instructionClass);
    
    return(instrCycleCount);
}

