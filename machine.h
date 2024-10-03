

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "instruction.h"
#include "bof.h"
#include "utilities.h"
#include "machine_types.h"
#include "disasm.h"








// Function declarations
BOFHeader loadHeader(BOFFILE bof);
void initializeRegisters(BOFHeader header);
void printASM(BOFFILE bof, BOFHeader header);
void handleInstruction(bin_instr_t instr, BOFFILE bof);
void check_invariants();
void trace(address_type addr, bin_instr_t instr, BOFHeader header, BOFFILE bof);
void fill_Data(BOFFILE bof);


// Instructions
void NEG(comp_instr_t instruction);
void SCA(comp_instr_t instruction);
void XORI(immed_instr_t instruction);
void NOR(comp_instr_t instruction);
void BOR(comp_instr_t instruction);
void BLEZ(immed_instr_t instruction);
void NORI(immed_instr_t instruction);
void JREL(other_comp_instr_t instruction);
void XOR(comp_instr_t instruction);
void SRL(other_comp_instr_t instruction);

void LWI(comp_instr_t instruction);
void BGTZ(immed_instr_t instruction);
void AND(comp_instr_t instruction);
void SRI(other_comp_instr_t instruction);
void BEQ(immed_instr_t instruction);
void SLL(other_comp_instr_t instruction);
void ADD(comp_instr_t instruction);
void JMP(other_comp_instr_t instruction);
void SUB(comp_instr_t instruction);
void DIV(other_comp_instr_t instruction);
void CPW(comp_instr_t instruction);
void LWR(comp_instr_t instruction);
void SWR(comp_instr_t instruction);
void LIT(other_comp_instr_t instruction);
void ARI(other_comp_instr_t instr);
void MUL(other_comp_instr_t instruction);
void CFHI(other_comp_instr_t instruction);
void CFLO(other_comp_instr_t instruction);
void CSI(other_comp_instr_t instruction);
void ADDI(immed_instr_t instruction);
void ANDI(immed_instr_t instruction);
void BORI(immed_instr_t instruction);
void BGEZ(immed_instr_t instruction);
void BLTZ(immed_instr_t instr);
void BNE(immed_instr_t instruction);
void JMPA(jump_instr_t instruction);
void CALL(jump_instr_t instruction);
void RTN(jump_instr_t instruction);
void EXIT(syscall_instr_t instr);
void PSTR(syscall_instr_t instr);
void PCH(syscall_instr_t instr);
void RCH(syscall_instr_t instr);
void STRA(syscall_instr_t instruction);
void NOTR(syscall_instr_t instruction);

// Debugging tool
void debugTool(BOFHeader header);


