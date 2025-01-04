/*
 * apex_cpu.h
 * Contains APEX cpu pipeline declarations
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_

#include "apex_macros.h"

/* Format of an APEX instruction  */
typedef struct APEX_Instruction
{
    char opcode_str[128];
    int opcode;
    int rd;
    int rs1;
    int rs2;
    int imm;
} APEX_Instruction;


/* Model of CPU stage latch */
typedef struct CPU_Stage
{
    int pc;
    char opcode_str[128];
    int opcode;
    int rs1;
    int rs2;
    int rd;
    int imm;
    int rs1_value;
    int rs2_value;
    int result_buffer;
    int memory_address;
    int has_insn;
    int btb_hit_bit;
    int btb_index;
    int predict_taken;
} CPU_Stage;

typedef struct BTBentry
{
    int i_address;
    int h_bits[2];
    int t_address;
    int valid;
}BTBentry;

/* Model of APEX CPU */
typedef struct APEX_CPU
{
    int pc;                        /* Current program counter */
    int clock;                     /* Clock cycles elapsed */
    int insn_completed;            /* Instructions retired */
    int regs[REG_FILE_SIZE];       /* Integer register file */
    int code_memory_size;          /* Number of instruction in the input file */
    APEX_Instruction *code_memory; /* Code Memory */
    int data_memory[DATA_MEMORY_SIZE]; /* Data Memory */
    int single_step;               /* Wait for user input after every cycle */
    int zero_flag;                 /* {TRUE, FALSE} Used by BZ and BNZ to branch */
    int positive_flag;
    int negative_flag;
    int print_i_flag;
    int print_r_flag;
    int fetch_from_next_cycle;
    int stall_at_exe;
    int stall_at_decode;
    int target_address;
    int actual_taken;
    int stall_0_check;
    BTBentry BTBentry[BTB_SIZE];

    /* Pipeline stages */
    CPU_Stage fetch;
    CPU_Stage decode;
    CPU_Stage execute;
    CPU_Stage memory;
    CPU_Stage writeback;
} APEX_CPU;

APEX_Instruction *create_code_memory(const char *filename, int *size);
APEX_CPU *APEX_cpu_init(const char *filename);
void APEX_cpu_run(APEX_CPU *cpu, int num_of_cycles);
void APEX_cpu_stop(APEX_CPU *cpu);
void display(APEX_CPU *cpu);
int BTBHit(APEX_CPU *cpu, int pc);
void flipbits(int index, int a_taken);
void actual(APEX_CPU *cpu, int actual_taken, int predict_taken, int btb_hit_bit, int index);
void load_store(APEX_CPU *cpu);
int stall_check(APEX_CPU *cpu);
#endif