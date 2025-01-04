/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
typedef struct Scoreboard
{
    int busy[16];
}Scoreboard;

static Scoreboard scoreboard;


int stall_flag;
int reached_halt;
APEX_CPU *btb = NULL;

static int 
flag_check(int val, APEX_CPU*cpu)
{
    cpu->zero_flag = FALSE;cpu->positive_flag = FALSE;cpu->negative_flag = FALSE;
    if(val == 0)
    {
        cpu->zero_flag = TRUE;
        return cpu->zero_flag;
    }
    else if(val > 0)
    {
        cpu->positive_flag = TRUE;
        return cpu->positive_flag;
    }
    else
    {
        cpu->negative_flag = TRUE;
        return cpu->negative_flag;
    }
}

static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_ADDL:
        case OPCODE_SUBL:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd,stage->rs1, stage->imm);
            break;
        }

        case OPCODE_MOVC:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
            break;
        }

        case OPCODE_LOAD:
        case OPCODE_JALR:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_LOADP:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_STORE:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }

        case OPCODE_STOREP:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }
        case OPCODE_CML:
        case OPCODE_JUMP:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rs1,
                   stage->imm);
            break;
        }


        case OPCODE_CMP:
        {
            printf("%s,R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2);
            break;
        }

        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        case OPCODE_BN:
        case OPCODE_BNN:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_HALT:
        {
            printf("%s", stage->opcode_str);
            break;
        }

        case OPCODE_NOP:
        {
            printf("%s", stage->opcode_str);
            break;
        }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;
    printf("\nP:%d,N:%d,Z:%d\n",cpu->positive_flag, cpu->negative_flag, cpu->zero_flag);

    printf("---------------------");

    for (int i = 0; i < BTB_SIZE; ++i)
    {
        printf("\ni_address: %d hbit0: %d hbit1: %d taddress: %d\n",btb->BTBentry[i].i_address,
        btb->BTBentry[i].h_bits[0],btb->BTBentry[i].h_bits[1],btb->BTBentry[i].t_address);
    }

    for(int i=0;i<4096;i++)
    {
        if(cpu->data_memory[i] != 0)
        {
            printf("\nMEM[%d] = %d\n",i, cpu->data_memory[i]);
        }
    }
    // if(cpu->data_memory[i])
    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");

    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");
}




int 
BTBHit(APEX_CPU *cpu, int pc)
{
    
    for (int i = 0; i < BTB_SIZE; i++)
    {
        // printf("\nbtbentry instruction address: %d\n", btb->BTBentry[i].i_address);
        // printf("\nactual instruction address: %d\n", pc);
       
        if (btb->BTBentry[i].i_address == pc)
        {
            cpu->fetch.btb_index = i;
            if((btb->BTBentry[i].h_bits[0] == 1 &&  btb->BTBentry[i].h_bits[1] == 0) || 
            (btb->BTBentry[i].h_bits[0] == 1 &&  btb->BTBentry[i].h_bits[1] == 1) )
            {
                cpu->fetch.predict_taken = 1;
            }
            else
            {
                cpu->fetch.predict_taken = 0;
            }
            // Use the second history bit as the prediction
            cpu->target_address = btb->BTBentry[i].t_address;
            return i; // BTB hit
        }
    }
    // BTB miss
    return -1;
}


void flipbits(int index, int a_taken)
{
    if(btb->BTBentry[index].h_bits[0] == 0 && btb->BTBentry[index].h_bits[1] == 0)
        {
            if(a_taken)
            {
                btb->BTBentry[index].h_bits[1] = 1;
            }
            
        }
        else if(btb->BTBentry[index].h_bits[0] == 1 && btb->BTBentry[index].h_bits[1] == 0)
        {
            if(a_taken)
            {
                btb->BTBentry[index].h_bits[1] = 1;
            }
            else
            {
                btb->BTBentry[index].h_bits[0] = 0;
                btb->BTBentry[index].h_bits[1] = 1;
            }
        }
        else if(btb->BTBentry[index].h_bits[0] == 1 && btb->BTBentry[index].h_bits[1] == 1)
        {
            if(!a_taken)
            {
                btb->BTBentry[index].h_bits[1] = 0;
            }
        }
        else
        {
            if(a_taken)
            {
                btb->BTBentry[index].h_bits[0] = 1;
                btb->BTBentry[index].h_bits[1] = 0;
            }
            else
            {
                btb->BTBentry[index].h_bits[1] = 0;
            }
        }
}

void actual(APEX_CPU *cpu, int actual_taken, int predict_taken, int btb_hit_bit, int index)
{
    if(actual_taken)
    {
        if(btb_hit_bit)
        {
            if(predict_taken)
            {
                flipbits(index, actual_taken);
            }
            else
            {
                flipbits(index, actual_taken);
                cpu->pc = cpu->execute.pc + cpu->execute.imm;
                cpu->fetch_from_next_cycle = TRUE;
                cpu->decode.has_insn = FALSE;
                cpu->fetch.has_insn = TRUE;
            }
        }
        else
        {
            flipbits(index, actual_taken);
                cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    cpu->fetch_from_next_cycle = TRUE;
                    cpu->decode.has_insn = FALSE;
                    cpu->fetch.has_insn = TRUE;
        }
    }

    else
    {
        if(btb_hit_bit)
        {
            if(predict_taken)
            {
                flipbits(index, actual_taken);
                cpu->pc = cpu->execute.pc + 4;
                    cpu->fetch_from_next_cycle = TRUE;
                    cpu->decode.has_insn = FALSE;
                    cpu->fetch.has_insn = TRUE;
                    // cpu->pc = cpu->execute.pc + 4;
            }
        }
        else
        {
            flipbits(index, actual_taken);
        }
    }
}

// int stalling(APEX_CPU *cpu)
// {
//     if(cpu->memory.opcode == OPCODE_STOREP)
//     {
//         if(cpu->decode.rs1 == cpu->memory.rd)
//         return 1;
//         if(cpu->decode.rs2 == cpu->memory.rd)
//         return 1;
//     }

//     if(cpu->memory.opcode == OPCODE_LOADP)
//     {
//          if(cpu->decode.rs1 == cpu->memory.rd || cpu->decode.rs1 == cpu->memory.rs1)
//         return 1;
//         if(cpu->decode.rs2 == cpu->memory.rd || cpu->decode.rs2 == cpu->memory.rs1)
//         return TRUE;
//     }
// }

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    cpu->fetch.btb_hit_bit = 0;
    
    if(cpu->pc <= (cpu->code_memory_size - 1)*4 + 4000)
    {
    APEX_Instruction *current_ins;
    if (cpu->fetch.has_insn)
    {     
        
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }


        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.imm = current_ins->imm;

        
        printf("\nstallflag: %d\n", stall_flag);
        /* Update PC for next instruction */
        
        /* Copy data from fetch latch to decode latch*/
        if(stall_flag==0){
            int btb_hit = BTBHit(cpu, cpu->pc);
            /* If BTB hit, update PC to the predicted target address */
            if (btb_hit != -1)
            {
                printf("\nbtb hit true\n");
                cpu->fetch.btb_hit_bit = 1;
                printf("\npredict taken: %d\n", cpu->fetch.predict_taken);
                if(cpu->fetch.predict_taken)
                {
                cpu->pc = cpu->target_address;
                // printf("target pc: %d", cpu->pc);
                // cpu->fetch_from_next_cycle = TRUE;
                }
                else
                {
                    cpu->pc += 4;
                }
            }
            else
            {
                cpu->fetch.btb_hit_bit = 0;
                cpu->pc += 4;
            }
            cpu->decode = cpu->fetch;
        }
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Fetch", &cpu->fetch);
        }
        }
        else
        {
            cpu->fetch.has_insn = FALSE;
            return;
        }

        /* Stop fetching new instructions if HALT is fetched */
        // if (cpu->fetch.opcode == OPCODE_HALT)
        // {
        //     cpu->fetch.has_insn = FALSE;
        // }
    }
}

int stall_check(APEX_CPU *cpu)
{
    if(!cpu->stall_0_check)
    {
        if(cpu->memory.opcode == OPCODE_STOREP)
                    {
                        if(cpu->decode.rs1 == cpu->memory.rs2)
                        return 1;
                        if(cpu->decode.rs2 == cpu->memory.rs2)
                        return 1;
                    }

                    if(cpu->memory.opcode == OPCODE_LOADP)
                    {
                        if(cpu->decode.rs1 == cpu->memory.rd || cpu->decode.rs1 == cpu->memory.rs1)
                        return 1;
                        if(cpu->decode.rs2 == cpu->memory.rd || cpu->decode.rs2 == cpu->memory.rs1)
                        return 1;
                    }
    }
    return 0;
}

void load_store(APEX_CPU *cpu)
{
    if (cpu->writeback.opcode == OPCODE_STOREP){
                       
                        if((cpu->decode.rs1 == cpu->writeback.rs2)){
                            cpu->decode.rs1_value = cpu->writeback.result_buffer;
                        }
                        if((cpu->decode.rs2 == cpu->writeback.rs2)){
                            cpu->decode.rs2_value = cpu->writeback.result_buffer;
                            // printf("\nfirst rs2 value: %d\n", cpu->decode.rs2_value);
                        }
                    }

                    if (cpu->memory.opcode == OPCODE_STOREP){
                        if((cpu->decode.rs1 == cpu->memory.rs2)){
                            cpu->decode.rs1_value = cpu->memory.result_buffer;
                        }
                        if((cpu->decode.rs2 == cpu->memory.rs2)){
                            cpu->decode.rs2_value = cpu->memory.result_buffer;
                        }
                    }

        if(cpu->writeback.opcode == OPCODE_LOADP)
        {
            if((cpu->decode.rs1 == cpu->writeback.rs1)){
                    cpu->decode.rs1_value = cpu->writeback.rs1_value;
                }
            else if(cpu->decode.rs1 == cpu->writeback.rd)
            {
                cpu->decode.rs1_value = cpu->data_memory[cpu->writeback.memory_address];
            }
            if((cpu->decode.rs2 == cpu->writeback.rs1))
            {
                cpu->decode.rs2_value = cpu->writeback.result_buffer;
                // printf("\nfirst rs2 value: %d\n", cpu->decode.rs2_value);
            }
            else if(cpu->decode.rs2 == cpu->writeback.rd)
            {
                cpu->decode.rs2_value = cpu->data_memory[cpu->writeback.memory_address];
            }
        }

            if (cpu->memory.opcode == OPCODE_LOADP)
            {
                if((cpu->decode.rs1 == cpu->memory.rs1)){
                cpu->decode.rs1_value = cpu->memory.rs1_value;
                }
            else if(cpu->decode.rs1 == cpu->memory.rd)
            {
                cpu->decode.rs1_value = cpu->memory.memory_address;
            }
            if((cpu->decode.rs2 == cpu->memory.rs1))
            {
                cpu->decode.rs2_value = cpu->memory.result_buffer;
                // printf("\nfirst rs2 value: %d\n", cpu->decode.rs2_value);
            }
            else if(cpu->decode.rs2 == cpu->memory.rd)
            {
                cpu->decode.rs2_value = cpu->memory.memory_address;
            }
            }
}
/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu)
{
    // printf("\n%d, %d, %d\n", scoreboard.busy[cpu->decode.rd], scoreboard.busy[cpu->decode.rs1], scoreboard.busy[cpu->decode.rs2]);
    if (cpu->decode.has_insn)
    {
        // stall_flag = 1;

        switch (cpu->decode.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            case OPCODE_MUL:
            case OPCODE_DIV:
            {
                // if(scoreboard.busy[cpu->decode.rd] != 1 && scoreboard.busy[cpu->decode.rs1] != 1 && 
                // scoreboard.busy[cpu->decode.rs2] != 1)
                // {
                //     scoreboard.busy[cpu->decode.rd] = 1;

                // stalling logic
                // if(cpu->execute.opcode == OPCODE_LOAD || cpu->execute.opcode == OPCODE_LOADP)
                // {
                //     // if(scoreboard.busy[cpu->decode.rs1] == 1 || scoreboard.busy[cpu->decode.rs1] == 1 )
                //     // {
                //     //     stall_flag = 1;
                //     //     cpu->decode.has_insn = TRUE;
                //     //     return;
                //     // }
                //     if((cpu->decode.rs1 == cpu->memory.rd && cpu->memory.has_insn) || (cpu->decode.rs2 == cpu->memory.rd && cpu->memory.has_insn)){
                //         stall_flag = 1;
                //         // cpu->decode.has_insn = TRUE;
                //         return;
                //     }

                    stall_flag = stall_check(cpu);
                   
                if(cpu->decode.rs1 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->writeback.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->execute.rd == cpu->decode.rs1 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->execute.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];

                    }

               if(cpu->decode.rs2 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs2_value = cpu->writeback.result_buffer;
                    }
                    else if(cpu->decode.rs2 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs2_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->execute.rd == cpu->decode.rs2 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs2_value = cpu->execute.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];

                    }

                    load_store(cpu);
                // }
                // else
                // {
                //     stall_flag = 1;
                // }
                break;
            }

            case OPCODE_ADDL:
            case OPCODE_SUBL:
            {
                // if(scoreboard.busy[cpu->decode.rd] != 1 && scoreboard.busy[cpu->decode.rs1] !=1)
                // {
                //     stall_flag = 0;
                    // scoreboard.busy[cpu->decode.rd] = 1;
                    stall_flag = stall_check(cpu);

                    if(cpu->decode.rs1 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->writeback.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->execute.rd == cpu->decode.rs1 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->execute.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];

                    }

                    // TODO: check everywhere
                    // if (cpu->execute.opcode == OPCODE_STOREP){
                    //     if(cpu->decode.rs1 == cpu->execute.rs2)
                    //     {
                    //         cpu->decode.rs1_value = cpu->execute.result_buffer;
                    //     }
                    // }

                     load_store(cpu);
                // }
                // else
                // {
                //     stall_flag = 1;
                // }
                
                // stall_flag = 0;
                break;
            }
            case OPCODE_LOAD:
            {
                // if(scoreboard.busy[cpu->decode.rd] != 1 && scoreboard.busy[cpu->decode.rs1] !=1)
                // {
                //     stall_flag = 0;
                    // scoreboard.busy[cpu->decode.rd] = 1;
                    stall_flag = stall_check(cpu);

                    if(cpu->execute.rd == cpu->decode.rs1 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->execute.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->writeback.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];

                    }

                     load_store(cpu);
                // }
                // else
                // {
                //     stall_flag = 1;
                // }
                
                // stall_flag = 0;
                break;
            }
            case OPCODE_LOADP:
            {
                // if(scoreboard.busy[cpu->decode.rd] != 1 && scoreboard.busy[cpu->decode.rs1] != 1)
                // {
                //     stall_flag = 0;
                    // scoreboard.busy[cpu->decode.rd] = 1;

                   stall_flag = stall_check(cpu);


                //     scoreboard.busy[cpu->decode.rs1] = 1;
                    if(cpu->execute.rd == cpu->decode.rs1 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->execute.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->writeback.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];

                    }
                    // if(cpu->memory.opcode == OPCODE_STOREP)
                    // {
                    //     if(cpu->decode.rs1 == cpu->memory.rs2)
                    //     {
                    //         cpu->decode.rs1 = cpu->memory.rs2_value;
                    //     }
                    // }
                    // if (cpu->execute.opcode == OPCODE_STOREP){
                    // //printf("entered here\n %d %d %d", cpu->decode.rs1, cpu->execute.rs2, cpu->execute.has_insn);
                    //     if((cpu->decode.rs1 == cpu->execute.rs2)){
                    // //printf("entered here as well\n");
                    //         cpu->decode.rs1_value = cpu->execute.result_buffer;
                    //     }
                    // }

                     load_store(cpu);
                    
                    
                //printf("dewcode %d %d\n", cpu->decode.rs1_value, cpu->decode.rs2_value);

                // }
                // else
                // {
                //     stall_flag = 1;
                // }
                
                // stall_flag = 0;
                break;
            }
            case OPCODE_STORE:
            {
                // if(scoreboard.busy[cpu->decode.rs2] != 1 && scoreboard.busy[cpu->decode.rs1] != 1)
                // {
                    
                //     stall_flag = 0;

                stall_flag = stall_check(cpu);

                if(cpu->decode.rs1 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->writeback.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->execute.rd == cpu->decode.rs1 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->execute.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];

                    }

               if(cpu->decode.rs2 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs2_value = cpu->writeback.result_buffer;
                    }
                    else if(cpu->decode.rs2 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs2_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->execute.rd == cpu->decode.rs2 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs2_value = cpu->execute.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];

                    }

                     load_store(cpu);
                // }
                // else
                // {
                //     stall_flag = 1;
                // }
                // stall_flag = 0;
                break;
            }

            case OPCODE_STOREP:
            {
                // if(scoreboard.busy[cpu->decode.rs2] != 1 && scoreboard.busy[cpu->decode.rs1 !=1])
                // {
                //     stall_flag = 0;
                //     scoreboard.busy[cpu->decode.rs2] = 1;
                stall_flag = stall_check(cpu);


                if(cpu->decode.rs1 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->writeback.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->execute.rd == cpu->decode.rs1 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->execute.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];

                    }

               if(cpu->decode.rs2 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs2_value = cpu->writeback.result_buffer;
                    }
                    else if(cpu->decode.rs2 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs2_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->execute.rd == cpu->decode.rs2 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs2_value = cpu->execute.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];

                    }

                     load_store(cpu);

                    // printf("\nmem instruction: %s", cpu->memory.opcode_str);
                    // printf("\nwrite instruction: %s", cpu->writeback.opcode_str);
                    //  if (cpu->writeback.opcode == OPCODE_STOREP){
                    //     printf("entered here");
                    // //printf("entered here\n %d %d %d", cpu->decode.rs1, cpu->execute.rs2, cpu->execute.has_insn);
                    //     if((cpu->decode.rs1 == cpu->writeback.rs2)){
                    //         cpu->decode.rs1_value = cpu->writeback.result_buffer;
                    //     }
                    //     if((cpu->decode.rs2 == cpu->writeback.rs2)){
                    //         cpu->decode.rs2_value = cpu->writeback.result_buffer;
                    //         printf("\nfirst rs2 value: %d\n", cpu->decode.rs2_value);
                    //     }
                    // }

                    // if (cpu->memory.opcode == OPCODE_STOREP){
                    // //printf("entered here\n %d %d %d", cpu->decode.rs1, cpu->execute.rs2, cpu->execute.has_insn);
                    //     if((cpu->decode.rs1 == cpu->memory.rs2)){
                    //         cpu->decode.rs1_value = cpu->memory.result_buffer;
                    //     }
                    //     if((cpu->decode.rs2 == cpu->memory.rs2)){
                    //         cpu->decode.rs2_value = cpu->memory.result_buffer;
                    //     }
                    // }

                    // printf("rs1 value: %d", cpu->decode.rs1_value);
                    // printf("rs2 value: %d", cpu->decode.rs2_value);
                // }
                // else
                // {
                //     stall_flag = 1;
                // }
                
                // stall_flag = 0;
                break;
            }

            case OPCODE_CMP:
            {
                

                stall_flag = stall_check(cpu);
                   
                
                if(cpu->decode.rs1 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->writeback.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->execute.rd == cpu->decode.rs1 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->execute.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];

                    }

               if(cpu->decode.rs2 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs2_value = cpu->writeback.result_buffer;
                    }
                    else if(cpu->decode.rs2 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs2_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->execute.rd == cpu->decode.rs2 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs2_value = cpu->execute.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];

                    }

                     load_store(cpu);
                // }
                // else
                // {
                //     stall_flag = 1;
                // }

                // stall_flag = 0;
                break;
            }

            case OPCODE_CML:
            {
                // if(scoreboard.busy[cpu->decode.rs1] != 1)
                // {
                //     stall_flag = 0;

                stall_flag = stall_check(cpu);
                
                    if(cpu->decode.rs1 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->writeback.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->execute.rd == cpu->decode.rs1 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->execute.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];

                    }

                     load_store(cpu);
                // }
                // else
                // {
                //     stall_flag = 1;
                // }

                // stall_flag = 0;
                break;
            }

            case OPCODE_MOVC:
            {
                // printf("rd: %d, score for rd %d", cpu->decode.rd, scoreboard.busy[cpu->decode.rd]);
                // if(scoreboard.busy[cpu->decode.rd] != 1)
                // {
                //     printf("here");
                //     stall_flag = 0;
                    scoreboard.busy[cpu->decode.rd] = 1;

                // }
                // else
                // {
                //     stall_flag = 1;
                // }
                // stall_flag = 0;
                break;
            }

            case OPCODE_JALR:
            
            {
                // if(scoreboard.busy[cpu->decode.rd] != 1 && scoreboard.busy[cpu->decode.rs1] != 1)
                // {
                //     stall_flag = 0;
                //     scoreboard.busy[cpu->decode.rd] = 1;

                stall_flag = stall_check(cpu);

                    if(cpu->execute.rd == cpu->decode.rs1 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->execute.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->writeback.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];

                    }

                     load_store(cpu);
                // }
                // else
                // {
                //     stall_flag = 1;
                // }
                // stall_flag = 0;
                break;
            }
            


            case OPCODE_JUMP:
            {
                // if(scoreboard.busy[cpu->decode.rs1] != 1)
                // {
                //     stall_flag = 0;
                stall_flag = stall_check(cpu);


                    if(cpu->execute.rd == cpu->decode.rs1 && cpu->execute.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->execute.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->memory.rd && cpu->memory.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->memory.result_buffer;
                    }
                    else if(cpu->decode.rs1 == cpu->writeback.rd && cpu->writeback.has_insn)
                    {
                        cpu->decode.rs1_value = cpu->writeback.result_buffer;
                    }
                    else
                    {
                        cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];

                    }

                     load_store(cpu);
                // }
                // else
                // {
                //     stall_flag = 1;
                // }
                
                // stall_flag = 0;
                break;
            }

            case OPCODE_NOP:
            {
                // stall_flag = 0;
                break;
            }

            case OPCODE_BP:
            case OPCODE_BNP:
            case OPCODE_BZ:
            case OPCODE_BNZ:
            // case OPCODE_BN:
            // case OPCODE_BNN:
            {
                // stall_flag = 0;
                printf("Stall flag at decode is %d:\n",stall_flag);
                int slot = 0;
                //int btb_hit = BTBLookup(btb, cpu->decode.pc);   
                if(!cpu->decode.btb_hit_bit)
                {
                     for (int i = 0; i < BTB_SIZE; ++i) 
                    {
                        if(!btb->BTBentry[i].valid)
                        {
                            slot = i;
                            break;
                        }
                    }
                        btb->BTBentry[slot].valid = 1;
                        btb->BTBentry[slot].i_address = cpu->decode.pc;
                         if (strcmp(cpu->decode.opcode_str, "BNZ") == 0 || strcmp(cpu->decode.opcode_str, "BP") == 0) {
                            btb->BTBentry[slot].h_bits[0] = 1;
                            btb->BTBentry[slot].h_bits[1] = 1;
                        } else {
                            btb->BTBentry[slot].h_bits[0] = 0;
                            btb->BTBentry[slot].h_bits[1] = 0;
                        }

                cpu->decode.btb_index = slot;
                }


                break;
            }

         cpu->fetch.has_insn = TRUE;
        }

        
        if(cpu->decode.opcode == OPCODE_HALT){
            cpu->fetch.has_insn = FALSE;
            // stall_flag = 0;
        }
        /* Copy data from decode latch to execute latch*/
        if(stall_flag == 0){

        cpu->execute = cpu->decode;
        cpu->stall_0_check = FALSE;
        cpu->decode.has_insn = FALSE;
        }
        
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode/RF", &cpu->decode);
        }
    }
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{
    if (cpu->execute.has_insn)
    {
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {
            case OPCODE_ADD:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value + cpu->execute.rs2_value;
        
                /* Set the zero flag based on the result buffer */
                flag_check(cpu->execute.result_buffer, cpu);
                break;
            }
            case OPCODE_SUB:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.rs2_value;
                flag_check(cpu->execute.result_buffer,cpu);
                break;
            }

            case OPCODE_ADDL:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.imm;
                printf("ADDL forwarded value %d", cpu->execute.result_buffer);
                flag_check(cpu->execute.result_buffer, cpu);
                break;
            }

             case OPCODE_SUBL:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.imm;
                flag_check(cpu->execute.result_buffer, cpu);
                break;
            }
            
            case OPCODE_MUL:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value * cpu->execute.rs2_value;
                flag_check(cpu->execute.result_buffer, cpu);
                break;
            }

            case OPCODE_DIV:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value / cpu->execute.rs2_value;
                flag_check(cpu->execute.result_buffer, cpu);
                break;
            }

            case OPCODE_AND:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value & cpu->execute.rs2_value;
                printf("\nAND result: %d",cpu->execute.result_buffer);
                flag_check(cpu->execute.result_buffer, cpu);
                break;
            }

            case OPCODE_OR:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value | cpu->execute.rs2_value;
                flag_check(cpu->execute.result_buffer, cpu);
                break;
            }

            case OPCODE_XOR:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value ^ cpu->execute.rs2_value;
                flag_check(cpu->execute.result_buffer, cpu);
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->execute.memory_address
                    = cpu->execute.rs1_value + cpu->execute.imm;
                break;
            }
            case OPCODE_LOADP:
            {
                cpu->execute.memory_address
                    = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->execute.rs1_value = cpu->execute.rs1_value + 4;
                break;
            }
            case OPCODE_STORE:
            {
                cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.imm;
                break;         
            }
            case OPCODE_STOREP:
            {
                cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.imm;
                cpu->execute.result_buffer = cpu->execute.rs2_value + 4;
                break;
            }

            case OPCODE_CMP:
            {
                if(cpu->execute.rs1_value == cpu->execute.rs2_value)
                {
                    cpu->zero_flag = TRUE;
                    cpu->positive_flag = FALSE;
                    cpu->negative_flag = FALSE;
                }
                else if(cpu->execute.rs1_value > cpu->execute.rs2_value)
                {
                    cpu->positive_flag = TRUE;
                    cpu->negative_flag = FALSE;
                    cpu->zero_flag = FALSE;
                }
                else
                {
                    cpu->negative_flag = TRUE;
                    cpu->positive_flag = FALSE;
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_CML:
            {
                if(cpu->execute.rs1_value == cpu->execute.imm)
                {
                     cpu->zero_flag = TRUE;
                    cpu->positive_flag = FALSE;
                    cpu->negative_flag = FALSE;
                }
                else if(cpu->execute.rs1_value > cpu->execute.imm)
                {
                     cpu->positive_flag = TRUE;
                    cpu->negative_flag = FALSE;
                    cpu->zero_flag = FALSE;
                }
                else
                {
                    cpu->negative_flag = TRUE;
                    cpu->positive_flag = FALSE;
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_JALR:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->fetch.has_insn = FALSE;
                cpu->decode.has_insn = FALSE;
                break;

            }

            case OPCODE_BZ:
            {
                btb->BTBentry[cpu->execute.btb_index].t_address = cpu->execute.pc + cpu->execute.imm;

                printf("\ntarget address: %d\n", btb->BTBentry[cpu->execute.btb_index].t_address);
                // printf("zero flag: %d",cpu->zero_flag);
                if (cpu->zero_flag == TRUE)
                {
                    cpu->actual_taken = TRUE;
                    actual(cpu, cpu->actual_taken, cpu->execute.predict_taken, cpu->execute.btb_hit_bit, cpu->execute.btb_index);
                 
                }
                else
                {
                    cpu->actual_taken = FALSE;
                    actual(cpu, cpu->actual_taken, cpu->execute.predict_taken, cpu->execute.btb_hit_bit, cpu->execute.btb_index);
                }
                break;
            }

            case OPCODE_BNZ:
            {

                btb->BTBentry[cpu->execute.btb_index].t_address = cpu->execute.pc + cpu->execute.imm;
                printf("\ntarget address: %d\n", btb->BTBentry[cpu->execute.btb_index].t_address);
                // printf("zero flag: %d",cpu->zero_flag);
                if (cpu->zero_flag == FALSE)
                {
                    cpu->actual_taken = TRUE;
                    actual(cpu, cpu->actual_taken, cpu->execute.predict_taken, cpu->execute.btb_hit_bit, cpu->execute.btb_index);
                 
                }
                else
                {
                    cpu->actual_taken = FALSE;
                    actual(cpu, cpu->actual_taken, cpu->execute.predict_taken, cpu->execute.btb_hit_bit, cpu->execute.btb_index);
                }
                
                break;
            }

            case OPCODE_BP:
            {
                btb->BTBentry[cpu->execute.btb_index].t_address = cpu->execute.pc + cpu->execute.imm;
                printf("\ntarget address: %d\n", btb->BTBentry[cpu->execute.btb_index].t_address);
                // printf("zero flag: %d",cpu->zero_flag);
                if (cpu->positive_flag == TRUE)
                {
                    cpu->actual_taken = TRUE;
                    actual(cpu, cpu->actual_taken, cpu->execute.predict_taken, cpu->execute.btb_hit_bit, cpu->execute.btb_index);
                 
                }
                else
                {
                    cpu->actual_taken = FALSE;
                    actual(cpu, cpu->actual_taken, cpu->execute.predict_taken, cpu->execute.btb_hit_bit, cpu->execute.btb_index);
                }
                break;
            }

            case OPCODE_BNP:
            {
                btb->BTBentry[cpu->execute.btb_index].t_address = cpu->execute.pc + cpu->execute.imm;
                printf("\ntarget address: %d\n", btb->BTBentry[cpu->execute.btb_index].t_address);
                // printf("zero flag: %d",cpu->zero_flag);
                if (cpu->positive_flag == FALSE)
                {
                    cpu->actual_taken = TRUE;
                    actual(cpu, cpu->actual_taken, cpu->execute.predict_taken, cpu->execute.btb_hit_bit, cpu->execute.btb_index);
                 
                }
                else
                {
                    cpu->actual_taken = FALSE;
                    actual(cpu, cpu->actual_taken, cpu->execute.predict_taken, cpu->execute.btb_hit_bit, cpu->execute.btb_index);
                }
                break;
            }

            case OPCODE_JUMP:
            {
                cpu->pc = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->fetch_from_next_cycle = TRUE;
                cpu->decode.has_insn = FALSE;
                //cpu->fetch.has_insn = TRUE;
                break;
            }

            case OPCODE_MOVC: 
            {
                cpu->execute.result_buffer = cpu->execute.imm + 0;
                printf("MOvc rd value: %d",cpu->execute.result_buffer);
                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_NOP:
            {
                break;
            }
        }

        /* Copy data from execute latch to memory latch*/

        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Execute", &cpu->execute);
        }
    }
}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu)
{
    if (cpu->memory.has_insn)
    {
        switch (cpu->memory.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_XOR:
            {
                /* No work for ADD */
                break;
            }

            case OPCODE_ADDL:
            case OPCODE_SUBL:
            {
                break;
            }

            case OPCODE_MUL:
            {
                break;
            }

            case OPCODE_DIV:
            {
                break;
            }

            case OPCODE_LOAD:
            {
                /* Read from data memory */
                cpu->memory.result_buffer
                    = cpu->data_memory[cpu->memory.memory_address];
                scoreboard.busy[cpu->decode.rd] = 0;
                break;
            }

            case OPCODE_LOADP:
            {
                cpu->memory.result_buffer
                    = cpu->data_memory[cpu->memory.memory_address];
                scoreboard.busy[cpu->decode.rd] = 0;
                if(stall_flag)
                {
                    stall_flag = 0;
                    cpu->stall_0_check = TRUE;
                }
                break;
            }

            case OPCODE_STORE:
            {
                cpu->data_memory[cpu->memory.memory_address] = cpu->memory.rs1_value;
                break;
            }

            case OPCODE_STOREP:
            {
                cpu->data_memory[cpu->memory.memory_address] = cpu->memory.rs1_value;
                if(stall_flag)
                {
                    stall_flag = 0;
                    cpu->stall_0_check = TRUE;
                }
                break;
            }

            case OPCODE_JALR:
            {
                cpu->memory.result_buffer = cpu->memory.pc + 4;
                cpu->pc = cpu->memory.memory_address;
                cpu->fetch.has_insn = TRUE;
                break;
            }

            case OPCODE_JUMP:
            {
                break;
            }

            case OPCODE_NOP:
            {
                break;
            }
        }

        /* Copy data from memory latch to writeback latch*/
       
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;
        
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Memory", &cpu->memory);
        }
    }
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu)
{
    if (cpu->writeback.has_insn)
    {
       
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_XOR:
            case OPCODE_AND:
            case OPCODE_OR:
            {
                // scoreboard.busy[cpu->writeback.rd] = 0;
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                printf("write back res: %d",cpu->regs[cpu->writeback.rd]);
                break;
            }

            case OPCODE_ADDL:
            case OPCODE_SUBL:
            {
                // scoreboard.busy[cpu->writeback.rd] = 0;
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_MUL:
            {
                // scoreboard.busy[cpu->writeback.rd] = 0;
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_DIV:
            {
                // scoreboard.busy[cpu->writeback.rd] = 0;
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_LOADP:
            {
                // scoreboard.busy[cpu->writeback.rs1] = 0;
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->regs[cpu->writeback.rs1] = cpu->writeback.rs1_value;
                break;
            }

            case OPCODE_STORE:
            {
                break;
            }

            case OPCODE_STOREP:
            {
                // scoreboard.busy[cpu->writeback.rs2] = 0;
                cpu->regs[cpu->writeback.rs2] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_MOVC: 
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                // scoreboard.busy[cpu->writeback.rd] = 0;
                break;
            }

            case OPCODE_JALR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_JUMP:
            case OPCODE_CMP:
            case OPCODE_CML:
            {
                break;
            }

            case OPCODE_NOP:
            {
                break;
            }
        }

        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Writeback", &cpu->writeback);
        }

        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            // return TRUE;
            reached_halt = 1;
        }
    }

    /* Default */
    return 0;
}
void display(APEX_CPU *cpu)
{
    print_stage_content("fetch",&cpu->fetch);
    print_stage_content("decode",&cpu->decode);
    print_stage_content("execute",&cpu->execute);
    print_stage_content("memory",&cpu->memory);
    print_stage_content("writeback",&cpu->writeback);
    print_reg_file(cpu);
}
/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    
    btb = calloc(1, sizeof(APEX_CPU));

    if(!btb)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    cpu->clock = 1;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    for( int i=0; i<4; i++)
    {
        btb->BTBentry[i].i_address = 0;
        btb->BTBentry[i].h_bits[0] = 0;
        btb->BTBentry[i].h_bits[1] = 0;
        btb->BTBentry[i].t_address = 0;
    }


    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_run(APEX_CPU *cpu, int num_of_cycles)
{
    char user_prompt_val;
    while(1)
    {

         if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            user_prompt_val = getchar();
            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            // break;
        }

    

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);

        if (reached_halt){
            break;
        }
        cpu->clock++;
        if(!cpu->single_step && cpu->clock>=num_of_cycles){
            break;
        }

    }


}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}
