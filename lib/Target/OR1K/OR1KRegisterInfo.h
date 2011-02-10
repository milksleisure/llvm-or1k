//===- SparcRegisterInfo.h - Sparc Register Information Impl ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the OR1K implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef OR1KREGISTERINFO_H
#define OR1KREGISTERINFO_H

#include "llvm/Target/TargetRegisterInfo.h"
#include "OR1KGenRegisterInfo.h.inc"

namespace llvm {

class OR1KSubtarget;
class TargetInstrInfo;
class Type;

struct OR1KRegisterInfo : public OR1KGenRegisterInfo {
  OR1KSubtarget &Subtarget;
  const TargetInstrInfo &TII;
  
  OR1KRegisterInfo(OR1KSubtarget &st, const TargetInstrInfo &tii);

  /// Code Generation virtual methods...  
  const unsigned *getCalleeSavedRegs(const MachineFunction *MF = 0) const;

  BitVector getReservedRegs(const MachineFunction &MF) const;

  bool hasFP(const MachineFunction &MF) const;

  void eliminateCallFramePseudoInstr(MachineFunction &MF,
                                     MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator I) const;

  void eliminateFrameIndex(MachineBasicBlock::iterator II,
                           int SPAdj, RegScavenger *RS = NULL) const;

  void processFunctionBeforeFrameFinalized(MachineFunction &MF) const;

  void emitPrologue(MachineFunction &MF) const;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const;
  
  // Debug information queries.
  unsigned getRARegister() const;
  unsigned getFrameRegister(const MachineFunction &MF) const;

  // Exception handling queries.
  unsigned getEHExceptionRegister() const;
  unsigned getEHHandlerRegister() const;

  int getDwarfRegNum(unsigned RegNum, bool isEH) const;
};

} // end namespace llvm

#endif
