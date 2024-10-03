//Created by Adam Wheeler

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "instruction.h"
#include "bof.h"
#include "utilities.h"
#include "machine_types.h"
#include "machine.h"
#define MEMORY_SIZE_IN_WORDS 32768
#define NUM_REGISTERS 8
#define GP_REG 0    //$gp register
#define SP_REG 1    //$sp register
#define FP_REG 2    //$fp register
#define RA_REG 7    //$ra register
word_type GPR[NUM_REGISTERS];
// Special registers
address_type PC;  // Program Counter
word_type HI;  // High parts of multiplication/division
word_type LO;  // Low parts of multiplication/division

bool tracing = true;

static union mem_u {
    word_type words[MEMORY_SIZE_IN_WORDS];
    uword_type uwords[MEMORY_SIZE_IN_WORDS];
    bin_instr_t instrs[MEMORY_SIZE_IN_WORDS];
}memory;

bool flag = false;

int main(int argc, char* argv[])
{
    BOFFILE bof;

    if (argc < 2)
    {
        fprintf(stderr, "Error: no BOF file argument");
        return 1;
    }
    else if (strcmp(argv[1], "-p") == 0 && argc == 3)
    {
        bof = bof_read_open(argv[2]);
        BOFHeader header = loadHeader(bof);
        initializeRegisters(header);
        check_invariants();
        printASM(bof, header);
        return 0;
    }
    else
    {
        bof = bof_read_open(argv[1]);
        BOFHeader header = loadHeader(bof);

        for (int i = 0; i < header.text_length; i++) {
            bin_instr_t instruction = instruction_read(bof);

            memory.instrs[i] = instruction;
        }

        int data_start_index = header.data_start_address;
        for (int i = 0; i < header.data_length; i++) {
            word_type word = bof_read_word(bof);

            memory.words[data_start_index + i] = word;
        }

        initializeRegisters(header);
        check_invariants();

        //loops through instructions until exit code is reached
        while (!flag)
        {
            check_invariants();
            if (tracing == true) {
                trace(PC, memory.instrs[PC], header, bof);
            }

            handleInstruction(memory.instrs[PC], bof);

            PC += 1;
            check_invariants();
        }

        bof_close(bof);
        return 0;
    }
    return 0;
}

void trace(address_type addr, bin_instr_t instr, BOFHeader header, BOFFILE bof)
{

    printf("      PC: %d\n", addr);

    printf("GPR[$gp]: %d \t", GPR[0]);
    printf("GPR[$sp]: %d \t", GPR[1]);
    printf("GPR[$fp]: %d \t", GPR[2]);
    printf("GPR[$r3]: %d \t", GPR[3]);
    printf("GPR[$r4]: %d \t", GPR[4]);
    printf("\n");
    printf("GPR[$r5]: %d \t", GPR[5]);
    printf("GPR[$r6]: %d \t", GPR[6]);
    printf("GPR[$ra]: %d \n", GPR[7]);

    // Printing from $gp to $sp
    bool firstzero = false;
    for (int i = 0; i < GPR[GP_REG]+50; i++)
    {
        int mem_index = GPR[GP_REG] + i;

        //Check if memory in bounds
        if (mem_index < 0 || mem_index >= MEMORY_SIZE_IN_WORDS) {
            fprintf(stderr, "Memory out of bounds: %d\n", mem_index);
            exit(5);
        }

        if (memory.words[mem_index] == 0) {
            if (!firstzero) {
                //first 0 print
                printf("%d: 0	    ", mem_index);
                firstzero = true;
            }
            continue;
        }
        else {
            //Not 0, print
            printf("%d: %d	    ", mem_index, memory.words[mem_index]);
            firstzero = false;
        }
    }

    if (firstzero) {
        printf(" ... ");
    }

    printf("\n");

    int count;

    // Printing from $sp to $fp
    firstzero = false;
    for (int i = GPR[SP_REG]; i <= GPR[FP_REG]; i++)
    {
        count = i;
        //SP and GP are the same
        if (GPR[SP_REG] == GPR[FP_REG]) {
            printf("\n");
            printf("%d: %d	    ", i, memory.words[i]);
            break;
        }

        //Check for memory in bounds
        if (i < 0 || i >= MEMORY_SIZE_IN_WORDS) {
            fprintf(stderr, "Memory out of bounds: %d\n", i);
            exit(5);
        }

        if (memory.words[i] == 0) {
            if (!firstzero) {
                //first 0 print
                printf("%d: 0	    ", i);
                firstzero = true;
            }
            continue;
        }
        else {
            //Not 0, print
            printf("%d: %d	    ", i, memory.words[i]);
            firstzero = false;
        }
    }

    if (firstzero && count != GPR[FP_REG]) {
        printf(" ... ");
    }

    printf("\n==>      %d: %s\n", addr, instruction_assembly_form(addr, instr));
}

void check_invariants()
{
    //0 <= GPR[$gp] < GPR[$sp]?
    if (GPR[0] < 0 || GPR[0] >= GPR[1])
    {
        fprintf(stderr, "Invariant: 0 <= GPR[$gp] < GPR[$sp]\n");
        exit(EXIT_FAILURE);
    }

    //GPR[$sp] <= GPR[$fp]?
    if (GPR[1] > GPR[2])
    {
        fprintf(stderr, "Invariant: GPR[$sp] <= GPR[$fp]\n");
        exit(EXIT_FAILURE);
    }

    //GPR[$fp] < MEMORY_SIZE_IN_WORDS?
    if (GPR[2] >= MEMORY_SIZE_IN_WORDS)
    {
        fprintf(stderr, "Invariant: GPR[$fp] < MEMORY_SIZE_IN_WORDS\n");
        exit(EXIT_FAILURE);
    }

    //0 <= PC < MEMORY_SIZE_IN_WORDS?
    if (PC < 0 || PC >= MEMORY_SIZE_IN_WORDS)
    {
        fprintf(stderr, "Invariant: 0 <= PC < MEMORY_SIZE_IN_WORDS\n");
        fprintf(stderr, "%d\n", PC);
        exit(EXIT_FAILURE);
    }
}

BOFHeader loadHeader(BOFFILE bof)
{
    BOFHeader header = bof_read_header(bof);

    return header;
}



void initializeRegisters(BOFHeader header)
{
    GPR[GP_REG] = header.data_start_address; //sets $gp to start of the data section
    GPR[SP_REG] = header.stack_bottom_addr;  //sets $sp to the bottom of the stack
    GPR[FP_REG] = header.stack_bottom_addr;  //sets $fp to the bottom of the stack
    GPR[RA_REG] = 0;  //clears return address $ra
    //sets $pc to the text start address
    //fprintf(stderr, "%d\n", PC);
    PC = header.text_start_address;
    //fprintf(stderr, "%d\n", PC);

    //clear general purpose registers 3-6
    for (int i = 3; i <= 6; i++)
    {
        GPR[i] = 0;
    }
    //clear hi/lo registers
    HI = 0;
    LO = 0;
}

void printASM(BOFFILE bof, BOFHeader header)
{

    // Read and print each instruction in the text section
    address_type addr = header.text_start_address;

    for (size_t i = 0; i < header.text_length; i++)
    {
        //reads instructions from BOF
        bin_instr_t instruction = instruction_read(bof);
        //converts to assembly
        const char* assembly_form = instruction_assembly_form(addr, instruction);

        //prints the assembly to stdout
        printf("     %d: %s\n", addr, assembly_form);

        //goes to next instruction address
        addr++;
    }

    int data_start_index = header.data_start_address;
    for (int i = 0; i < header.data_length; i++) {
        word_type word = bof_read_word(bof);

        memory.words[data_start_index + i] = word;
    }
    
    int count;

    // Printing from $gp to $sp
    bool firstzero = false;
    for (int i = 0; i < GPR[GP_REG] + 50; i++)
    {
        int mem_index = GPR[GP_REG] + i;

        count = i;

        //Check if memory in bounds
        if (mem_index < 0 || mem_index >= MEMORY_SIZE_IN_WORDS) {
            fprintf(stderr, "Memory out of bounds: %d\n", mem_index);
            exit(5);
        }

        if (memory.words[mem_index] == 0) {
            if (!firstzero) {
                //first 0 print
                printf("     %d: 0", mem_index);
                firstzero = true;
            }
            continue;
        }
        else {
            //Not 0, print
            printf("     %d: %d", mem_index, memory.words[mem_index]);
            firstzero = false;
        }
    }

    if (firstzero) {
        if (count >= 4) {
            printf("\n\t ... ");
        }
        else {
            printf(" ... ");
        }
        
    }

    bof_close(bof);
}



void handleInstruction(bin_instr_t instr, BOFFILE bof)
{
    //Identify the instruction type (computation, immediate, jump, etc.)
    instr_type type = instruction_type(instr);

    switch (type)
    {
        //opcode 0
    case comp_instr_type:
        switch (instr.comp.func)
        {
        case NOP_F: //No operation
            break;
        case ADD_F: //addition
            ADD(instr.comp);
            break;
        case SUB_F: //subtraction
            SUB(instr.comp);
            break;
        case CPW_F: //Copy Word
            CPW(instr.comp);
            break;
        case AND_F: //AND
            AND(instr.comp);
            break;
        case BOR_F: //OR
            BOR(instr.comp);
            break;
        case NOR_F: //NOR
            NOR(instr.comp);
            break;
        case XOR_F: //XOR
            XOR(instr.comp);
            break;
        case LWR_F: //loads the word to register
            LWR(instr.comp);
            break;
        case SWR_F: //stores the word from register
            SWR(instr.comp);
            break;
        case SCA_F: //stores computed address
            SCA(instr.comp);
            break;
        case LWI_F: //load word indirect
            LWI(instr.comp);
            break;
        case NEG_F: //negate
            NEG(instr.comp);
            break;
        default:
            bail_with_error("Invalid instruction: %d", instr.comp.func);
        }
        break;

        //opcode 1
    case other_comp_instr_type:
        switch (instr.othc.func)
        {
        case LIT_F://load immediate
            LIT(instr.othc);
            break;
        case ARI_F: //add immediate to register
            ARI(instr.othc);
            break;
        case SRI_F: //subtract immediate from register
            SRI(instr.othc);
            break;
        case MUL_F: //multiply
            MUL(instr.othc);
            break;
        case DIV_F: //divide
            DIV(instr.othc);
            break;
        case CFHI_F://copy HI
            CFHI(instr.othc);
            break;
        case CFLO_F: //copy LO
            CFLO(instr.othc);
            break;
        case SLL_F:  //shift lef
            SLL(instr.othc);
            break;
        case SRL_F:  //shift right
            SRL(instr.othc);
            break;
        case JMP_F:  //jump
            JMP(instr.othc);
            break;
        case CSI_F: //call subroutine indirect
            CSI(instr.othc);
            break;
        case JREL_F: //jump relative
            JREL(instr.othc);
            break;
            //case SYS_F:  //syscall
                //SYS(instr.othc);
                //break;
        default:
            bail_with_error("Invalid other computation instruction: %d", instr.othc.func);
        }
        break;

        //opcode 2-6
    case immed_instr_type:
        switch (instr.immed.op)
        {
        case ADDI_O:  //addi
            ADDI(instr.immed);
            break;
        case ANDI_O:  //AND immediate
            ANDI(instr.immed);
            break;
        case BORI_O: //OR immediate
            BORI(instr.immed);
            break;
        case NORI_O: //NOR immediate
            NORI(instr.immed);
            break;
        case XORI_O: //XOR immediate
            XORI(instr.immed);
            break;
        case BEQ_O:  //branch equal 
            BEQ(instr.immed);
            break;
        case BGEZ_O: //branch >=0
            BGEZ(instr.immed);
            break;
        case BGTZ_O: //branch >0
            BGTZ(instr.immed);
            break;
        case BLEZ_O: //branch <=0
            BLEZ(instr.immed);
            break;
        case BLTZ_O: //branch <0
            BLTZ(instr.immed);
            break;
        case BNE_O: //branch !=
            BNE(instr.immed);
            break;
        default:
            bail_with_error("Invalid immediate instruction: %d", instr.immed.op);
        }
        break;

        //opcode 13-15
    case jump_instr_type:
        switch (instr.jump.op)
        {
        case JMPA_O: //jump absolute
            JMPA(instr.jump);
            break;
        case CALL_O:  //call subroutine
            CALL(instr.jump);
            break;
        case RTN_O:  //return from subroutine
            RTN(instr.jump);
            break;
        default:
            bail_with_error("Invalid jump opcode: %d", instr.jump.op);
        }
        break;

        //opcode 1 function 15
    case syscall_instr_type:
        switch (instr.syscall.code)
        {
        case exit_sc: //exit
            EXIT(instr.syscall);
            break;
        case print_str_sc: //print string
            PSTR(instr.syscall);
            break;
        case print_char_sc: //print char
            PCH(instr.syscall);
            break;
        case read_char_sc: //read char
            RCH(instr.syscall);
            break;
        case start_tracing_sc: //start tracing
            STRA(instr.syscall);
            break;
        case stop_tracing_sc: //stop tracing
            NOTR(instr.syscall);
            break;
        default:
            bail_with_error("Invalid system call: %d", instr.syscall.code);
        }
        break;

    default:
        bail_with_error("Invalid instruction type: %d", type);
    }
}

void NEG(comp_instr_t instruction)
{
    reg_num_type rt = instruction.rt;
    reg_num_type rs = instruction.rs;
    offset_type ot = instruction.ot;
    offset_type os = instruction.os;
    memory.words[GPR[rt] + machine_types_formOffset(ot)] = -memory.words[GPR[rs] + machine_types_formOffset(os)];
}

void SCA(comp_instr_t instruction)
{
    reg_num_type rt = instruction.rt;
    reg_num_type rs = instruction.rs;
    offset_type ot = instruction.ot;
    offset_type os = instruction.os;
    memory.words[GPR[rt] + machine_types_formOffset(ot)] = GPR[rs] + machine_types_formOffset(os);
}

void XORI(immed_instr_t instruction)
{
    reg_num_type reg = instruction.reg;
    arg_type immediate = instruction.immed;
    offset_type o = instruction.offset;

    memory.uwords[GPR[reg] + machine_types_formOffset(o)] = memory.uwords[GPR[reg] + machine_types_formOffset(o)] ^ machine_types_zeroExt(immediate);
}

void NOR(comp_instr_t instruction)
{
    reg_num_type rs = instruction.rs;
    reg_num_type rt = instruction.rt;
    offset_type os = instruction.os;
    offset_type ot = instruction.ot;


    memory.uwords[GPR[rt] + machine_types_formOffset(ot)] = ~(memory.uwords[GPR[SP_REG]] | memory.uwords[GPR[rs] + machine_types_formOffset(os)]);
}

void BOR(comp_instr_t instruction)
{
    reg_num_type rs = instruction.rs;
    reg_num_type rt = instruction.rt;
    offset_type os = instruction.os;
    offset_type ot = instruction.ot;

    memory.uwords[GPR[rt] + machine_types_formOffset(ot)] = memory.uwords[GPR[SP_REG]] | memory.uwords[GPR[rs] + machine_types_formOffset(os)];
}

void BLEZ(immed_instr_t instruction)
{
    reg_num_type rs = instruction.reg;
    offset_type o = instruction.offset;
    immediate_type imm = instruction.immed;
    if (memory.words[GPR[rs] + machine_types_formOffset(o)] <= 0)
    {
        PC = (PC - 1) + machine_types_formOffset(imm);
    }
}

void NORI(immed_instr_t instruction)
{
    reg_num_type reg = instruction.reg;
    immediate_type imm = instruction.immed;
    offset_type o = instruction.offset;

    memory.uwords[GPR[reg] + machine_types_formOffset(o)] = ~(memory.uwords[GPR[reg] + machine_types_formOffset(o)] | machine_types_zeroExt(imm));
}

void JREL(other_comp_instr_t instruction)
{

    arg_type o = instruction.arg;

    PC = ((PC - 1) + machine_types_formOffset(o));
}

void XOR(comp_instr_t instruction)
{
    reg_num_type rs = instruction.rs;
    reg_num_type rt = instruction.rt;
    offset_type ot = instruction.ot;
    offset_type os = instruction.os;

    memory.uwords[GPR[rt] + machine_types_formOffset(ot)] = memory.uwords[GPR[SP_REG]] ^ memory.uwords[GPR[rs] + machine_types_formOffset(os)];

}

void SRL(other_comp_instr_t instruction)
{
    reg_num_type reg = instruction.reg;
    arg_type shift = instruction.arg;
    offset_type o = instruction.offset;

    memory.uwords[GPR[reg] + machine_types_formOffset(o)] = memory.uwords[GPR[SP_REG]] >> shift;

}

void LWI(comp_instr_t instruction) //Load word indirect
{
    reg_num_type rt = instruction.rt;
    offset_type ot = instruction.ot;
    reg_num_type rs = instruction.rs;
    offset_type os = instruction.os;

    memory.words[GPR[rt] + machine_types_formOffset(ot)] = memory.words[memory.words[GPR[rs] + machine_types_formOffset(os)]];
}

void BGTZ(immed_instr_t instruction) //Branch > 0
{
    reg_num_type r = instruction.reg;
    offset_type o = instruction.offset;
    immediate_type i = instruction.immed;
    if (memory.words[GPR[r] + machine_types_formOffset(o)] > 0)
    {
        PC = (PC - 1) + machine_types_formOffset(i);
    }
}

void AND(comp_instr_t instruction) //Bitwise And
{
    reg_num_type rt = instruction.rt;
    offset_type ot = instruction.ot;
    reg_num_type rs = instruction.rs;
    offset_type os = instruction.os;
    memory.uwords[GPR[rt] + machine_types_formOffset(ot)] = memory.uwords[GPR[SP_REG]] & (memory.uwords[GPR[rs] + machine_types_formOffset(os)]);
}

void SRI(other_comp_instr_t instruction) //Subtract immediate from register
{
    reg_num_type r = instruction.reg;
    arg_type i = instruction.arg;
    word_type sign_extended_imm = machine_types_sgnExt(i);
    GPR[r] = GPR[r] - sign_extended_imm;
}

void BEQ(immed_instr_t instruction) //Branch on Equal
{
    reg_num_type r = instruction.reg;
    offset_type o = instruction.offset;
    immediate_type i = instruction.immed;

    if (memory.words[GPR[SP_REG]] == memory.words[GPR[r] + machine_types_formOffset(o)])
    {
        PC = (PC - 1) + machine_types_formOffset(i);
    }
}

void SLL(other_comp_instr_t instruction) //shift left
{
    reg_num_type t = instruction.reg;
    offset_type o = instruction.offset;
    arg_type h = instruction.arg;
    memory.uwords[GPR[t] + machine_types_formOffset(o)] = memory.uwords[GPR[SP_REG]] << h;
}

void ADD(comp_instr_t instruction) //Add
{
    reg_num_type rs = instruction.rs;
    reg_num_type rt = instruction.rt;
    offset_type ot = instruction.ot;
    offset_type os = instruction.os;
    memory.words[GPR[rt] + machine_types_formOffset(ot)] = memory.words[GPR[SP_REG]] + (memory.words[GPR[rs] + machine_types_formOffset(os)]);
}

void JMP(other_comp_instr_t instruction) //Jump
{
    reg_num_type s = instruction.reg;
    offset_type o = instruction.offset;
    PC = memory.uwords[GPR[s] + machine_types_formOffset(o)];
}

void SUB(comp_instr_t instruction) //Subtract
{
    reg_num_type rt = instruction.rt;
    offset_type ot = instruction.ot;
    reg_num_type rs = instruction.rs;
    offset_type os = instruction.os;
    memory.words[GPR[rt] + machine_types_formOffset(ot)] = memory.words[GPR[SP_REG]] - (memory.words[GPR[rs] + machine_types_formOffset(os)]);
}

void DIV(other_comp_instr_t instruction) //Divide
{
    reg_num_type s = instruction.reg;
    offset_type o = instruction.offset;
    HI = memory.words[GPR[SP_REG]] % (memory.words[GPR[s] + machine_types_formOffset(o)]);
    LO = memory.words[GPR[SP_REG]] / (memory.words[GPR[s] + machine_types_formOffset(o)]);
}

void CPW(comp_instr_t instruction) //Copy Word
{
    reg_num_type rt = instruction.rt;
    offset_type ot = instruction.ot;
    reg_num_type rs = instruction.rs;
    offset_type os = instruction.os;
    memory.words[GPR[rt] + machine_types_formOffset(ot)] = memory.words[GPR[rs] + machine_types_formOffset(os)];
}

void LWR(comp_instr_t instruction) //Load Word into Register
{
    reg_num_type rt = instruction.rt;
    reg_num_type rs = instruction.rs;
    offset_type os = instruction.os;
    GPR[rt] = memory.words[GPR[rs] + machine_types_formOffset(os)];
}

void LIT(other_comp_instr_t instruction) //Literal (load)
{
    reg_num_type t = instruction.reg;
    offset_type o = instruction.offset;
    arg_type I = instruction.arg;
    memory.words[GPR[t] + machine_types_formOffset(o)] = machine_types_sgnExt(I);
}

//NOT SURE
void MUL(other_comp_instr_t instruction) //Multiply
{
    //reg_num_type s = instruction.reg;
    //offset_type o = instruction.offset;
    //[GPR[s] + formOffset(o)] (HI, LO) = memory.words[GPR[SP_REG]] * (memory.words[GPR[s] + formOffset(o)]);
    return;
}

void CFHI(other_comp_instr_t instruction) //Copy from HI
{
    reg_num_type t = instruction.reg;
    offset_type o = instruction.offset;
    memory.words[GPR[t] + machine_types_formOffset(o)] = HI;
}

void ADDI(immed_instr_t instruction) //Add immediate
{
    reg_num_type r = instruction.reg;
    offset_type o = instruction.offset;
    immediate_type i = instruction.immed;

    memory.words[GPR[r] + machine_types_formOffset(o)] = (memory.words[GPR[r] + machine_types_formOffset(o)]) + machine_types_sgnExt(i);

}

void BGEZ(immed_instr_t instruction) //Branch >= 0
{
    reg_num_type r = instruction.reg;
    offset_type o = instruction.offset;
    immediate_type i = instruction.immed;
    if (memory.words[GPR[r] + machine_types_formOffset(o)] >= 0)
    {
        PC = (PC - 1) + machine_types_formOffset(i);
    }
}

void JMPA(jump_instr_t instruction) //Jump to given address
{
    address_type a = instruction.addr;
    PC = machine_types_formAddress(PC - 1, a);
}

void CALL(jump_instr_t instruction) //Call subroutine
{
    address_type a = instruction.addr;
    GPR[RA_REG] = PC;
    PC = machine_types_formAddress(PC - 1, a);
}

// Return from subroutine
void RTN(jump_instr_t instruction)
{
    PC = GPR[RA_REG];
}

// Add register immediate
void ARI(other_comp_instr_t instr)
{
    reg_num_type r = instr.reg;
    arg_type i = instr.arg;

    GPR[r] = GPR[r] + machine_types_sgnExt(i);
}

// Bitwise or immediate
void BORI(immed_instr_t instr)
{
    reg_num_type r = instr.reg;
    offset_type o = instr.offset;
    immediate_type i = instr.immed;

    memory.uwords[GPR[r] + machine_types_formOffset(o)] = (memory.uwords[GPR[r] + machine_types_formOffset(o)]) | machine_types_zeroExt(i);
}

void ANDI(immed_instr_t instr)
{
    reg_num_type r = instr.reg;
    offset_type o = instr.offset;
    immediate_type i = instr.immed;

    memory.uwords[GPR[r] + machine_types_formOffset(o)] = (memory.uwords[GPR[r] + machine_types_formOffset(o)] & machine_types_zeroExt(i));
}

// Branch < 0
void BLTZ(immed_instr_t instr)
{
    reg_num_type r = instr.reg;
    offset_type o = instr.offset;
    immediate_type i = instr.immed;

    if (memory.words[GPR[r] + machine_types_formOffset(o)] < 0) {
        PC = (PC - 1) + machine_types_formOffset(i);
    }
}

// Branch not equal
void BNE(immed_instr_t instr)
{
    reg_num_type r = instr.reg;
    offset_type o = instr.offset;
    immediate_type i = instr.immed;

    if (memory.words[GPR[SP_REG]] != (memory.words[GPR[r] + machine_types_formOffset(o)])) {
        PC = (PC - 1) + machine_types_formOffset(i);
    }

}

// Store word from register
void SWR(comp_instr_t instr)
{
    reg_num_type rt = instr.rt;
    offset_type ot = instr.ot;
    reg_num_type rs = instr.rs;
    //offset_type os = instr.os;

    memory.words[GPR[rt] + machine_types_formOffset(ot)] = GPR[rs];
}

// Copy from LO
void CFLO(other_comp_instr_t instr)
{
    reg_num_type t = instr.reg;
    offset_type o = instr.offset;
    //func_type func = instr.func;

    memory.words[GPR[t] + machine_types_formOffset(o)] = LO;
}

// Call subroutine indirectly
void CSI(other_comp_instr_t instr)
{
    reg_num_type s = instr.reg;
    offset_type o = instr.offset;

    GPR[RA_REG] = PC;
    PC = memory.words[GPR[s] + machine_types_formOffset(o)];
}

// Halt with given exit code
void EXIT(syscall_instr_t instr)
{
    offset_type o = instr.offset;

    exit(machine_types_sgnExt(o));
    flag = true;
}

// Print string
void PSTR(syscall_instr_t instr)
{
    reg_num_type s = instr.reg;
    offset_type o = instr.offset;

    memory.words[GPR[SP_REG]] = printf("%s", (char*)&memory.words[GPR[s] + machine_types_formOffset(o)]);
}

// Print char
void PCH(syscall_instr_t instr)
{
    reg_num_type s = instr.reg;
    offset_type o = instr.offset;

    memory.words[GPR[SP_REG]] = fputc(memory.words[GPR[s] + machine_types_formOffset(o)], stdout);
}

// Read char
void RCH(syscall_instr_t instr) {
    reg_num_type t = instr.reg;
    offset_type o = instr.offset;

    memory.words[GPR[t] + machine_types_formOffset(o)] = getc(stdin);
}

void STRA(syscall_instr_t instruction)
{
    tracing = true;
}

void NOTR(syscall_instr_t instruction)
{
    tracing = false;
}


//general function to help with debugging.
//add anything here to help with tracing
//NOT part of the project
void debugTool(BOFHeader header)
{
    printf("BOF File Header:\n");
    printf("Magic: %.4s\n", header.magic);
    printf("Text start address: %u\n", header.text_start_address);
    printf("Text length: %u words\n", header.text_length);
    printf("Data start address: %u\n", header.data_start_address);
    printf("Data length: %u words\n", header.data_length);
    printf("Stack bottom address: %u\n", header.stack_bottom_addr);
}
