//===-- OR1KAsmPrinter.cpp - OR1K LLVM assembly writer ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to GAS-format SPARC assembly language.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "asm-printer"
#include "OR1K.h"
#include "OR1KInstrInfo.h"
#include "OR1KTargetMachine.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Target/Mangler.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace {
  class OR1KAsmPrinter : public AsmPrinter {
  public:
    explicit OR1KAsmPrinter(TargetMachine &TM, MCStreamer &Streamer)
      : AsmPrinter(TM, Streamer) {}

    virtual const char *getPassName() const {
      return "OR1K Assembly Printer";
    }

    void printOperand(const MachineInstr *MI, int opNum, raw_ostream &OS);
    void printMemOperand(const MachineInstr *MI, int opNum, raw_ostream &OS,
                         const char *Modifier = 0);
    void printCCOperand(const MachineInstr *MI, int opNum, raw_ostream &OS);

    virtual void EmitInstruction(const MachineInstr *MI) {
      SmallString<128> Str;
      raw_svector_ostream OS(Str);
      printInstruction(MI, OS);
      OutStreamer.EmitRawText(OS.str());
    }
    void printInstruction(const MachineInstr *MI, raw_ostream &OS);// autogen'd.
    static const char *getRegisterName(unsigned RegNo);

    bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                         unsigned AsmVariant, const char *ExtraCode,
                         raw_ostream &O);
    bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo,
                               unsigned AsmVariant, const char *ExtraCode,
                               raw_ostream &O);

    bool printGetPCX(const MachineInstr *MI, unsigned OpNo, raw_ostream &OS);
    
    virtual bool isBlockOnlyReachableByFallthrough(const MachineBasicBlock *MBB)
                       const;
  };
} // end of anonymous namespace

#include "OR1KGenAsmWriter.inc"

void OR1KAsmPrinter::printOperand(const MachineInstr *MI, int opNum,
                                   raw_ostream &O) {
}

void OR1KAsmPrinter::printMemOperand(const MachineInstr *MI, int opNum,
                                      raw_ostream &O, const char *Modifier) {
  printOperand(MI, opNum, O);

  // If this is an ADD operand, emit it like normal operands.
  if (Modifier && !strcmp(Modifier, "arith")) {
    O << ", ";
    printOperand(MI, opNum+1, O);
    return;
  }

  if (MI->getOperand(opNum+1).isImm() &&
      MI->getOperand(opNum+1).getImm() == 0)
    return;   // don't print "+0"

  O << "+";
  if (MI->getOperand(opNum+1).isGlobal() ||
      MI->getOperand(opNum+1).isCPI()) {
    O << "%lo(";
    printOperand(MI, opNum+1, O);
    O << ")";
  } else {
    printOperand(MI, opNum+1, O);
  }
}

bool OR1KAsmPrinter::printGetPCX(const MachineInstr *MI, unsigned opNum,
                                  raw_ostream &O) {
  std::string operand = "";
  const MachineOperand &MO = MI->getOperand(opNum);
  switch (MO.getType()) {
  default: llvm_unreachable("Operand is not a register");
  case MachineOperand::MO_Register:
    assert(TargetRegisterInfo::isPhysicalRegister(MO.getReg()) &&
           "Operand is not a physical register ");
    operand = "%" + StringRef(getRegisterName(MO.getReg())).lower();
    break;
  }

  unsigned mfNum = MI->getParent()->getParent()->getFunctionNumber();
  unsigned bbNum = MI->getParent()->getNumber();

  O << '\n' << ".LLGETPCH" << mfNum << '_' << bbNum << ":\n";
  O << "\tcall\t.LLGETPC" << mfNum << '_' << bbNum << '\n' ;

  O << "\t  sethi\t"
    << "%hi(_GLOBAL_OFFSET_TABLE_+(.-.LLGETPCH" << mfNum << '_' << bbNum 
    << ")), "  << operand << '\n' ;

  O << ".LLGETPC" << mfNum << '_' << bbNum << ":\n" ;
  O << "\tor\t" << operand  
    << ", %lo(_GLOBAL_OFFSET_TABLE_+(.-.LLGETPCH" << mfNum << '_' << bbNum
    << ")), " << operand << '\n';
  O << "\tadd\t" << operand << ", %o7, " << operand << '\n'; 
  
  return true;
}

void OR1KAsmPrinter::printCCOperand(const MachineInstr *MI, int opNum,
                                     raw_ostream &O) {
}

/// PrintAsmOperand - Print out an operand for an inline asm expression.
///
bool OR1KAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                      unsigned AsmVariant,
                                      const char *ExtraCode,
                                      raw_ostream &O) {
  if (ExtraCode && ExtraCode[0]) {
    if (ExtraCode[1] != 0) return true; // Unknown modifier.

    switch (ExtraCode[0]) {
    default: return true;  // Unknown modifier.
    case 'r':
     break;
    }
  }

  printOperand(MI, OpNo, O);

  return false;
}

bool OR1KAsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                            unsigned OpNo, unsigned AsmVariant,
                                            const char *ExtraCode,
                                            raw_ostream &O) {
  if (ExtraCode && ExtraCode[0])
    return true;  // Unknown modifier

  O << '[';
  printMemOperand(MI, OpNo, O);
  O << ']';

  return false;
}

/// isBlockOnlyReachableByFallthough - Return true if the basic block has
/// exactly one predecessor and the control transfer mechanism between
/// the predecessor and this block is a fall-through.
///
/// This overrides AsmPrinter's implementation to handle delay slots.
bool OR1KAsmPrinter::
isBlockOnlyReachableByFallthrough(const MachineBasicBlock *MBB) const {
  // If this is a landing pad, it isn't a fall through.  If it has no preds,
  // then nothing falls through to it.
  if (MBB->isLandingPad() || MBB->pred_empty())
    return false;
  
  // If there isn't exactly one predecessor, it can't be a fall through.
  MachineBasicBlock::const_pred_iterator PI = MBB->pred_begin(), PI2 = PI;
  ++PI2;
  if (PI2 != MBB->pred_end())
    return false;
  
  // The predecessor has to be immediately before this block.
  const MachineBasicBlock *Pred = *PI;
  
  if (!Pred->isLayoutSuccessor(MBB))
    return false;
  
  // Check if the last terminator is an unconditional branch.
  MachineBasicBlock::const_iterator I = Pred->end();
  while (I != Pred->begin() && !(--I)->getDesc().isTerminator())
    ; // Noop
  return I == Pred->end() || !I->getDesc().isBarrier();
}



// Force static initialization.
extern "C" void LLVMInitializeOR1KAsmPrinter() { 
  RegisterAsmPrinter<OR1KAsmPrinter> X(TheOR1KTarget);
}