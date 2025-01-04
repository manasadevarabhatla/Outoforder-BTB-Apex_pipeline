/*
 * main.c
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "apex_cpu.h"

int
main(int argc, char const *argv[])
{
    int choice;
    int n;
    int l;

    APEX_CPU *cpu;
    
    fprintf(stderr, "APEX CPU Pipeline Simulator v%0.1lf\n", 2.0);
    n = atoi(argv[3]);
    if (argc != 4)
    {
        fprintf(stderr, "APEX_Help: Usage %s <input_file> simulate <n>\n", argv[0]);
        exit(1);
    }

cpu = APEX_cpu_init(argv[1]);
            if((strcmp(argv[2],"simulate")) == 0)
            {
                APEX_cpu_run(cpu,n);
            }
        if (!cpu)
        {
            fprintf(stderr, "APEX_Error: Unable to initialize and simulate CPU\n");
            exit(1);
        }
while (1)
{
    //  cpu = APEX_cpu_init(argv[1]);
    //         if((strcmp(argv[2],"simulate")) == 0)
    //         {
    //             APEX_cpu_run(cpu,n);
    //         }
    //     if (!cpu)
    //     {
    //         fprintf(stderr, "APEX_Error: Unable to initialize and simulate CPU\n");
    //         exit(1);
    //     }
    fprintf(stderr, "\nEnter the choice of your command\n");
    fprintf(stderr, "1. Single Step\n");
    fprintf(stderr, "2. Display\n");
    fprintf(stderr, "3. Show Memory Address\n");
    fprintf(stderr, "4. Exit\n");
    scanf("%d", &choice);
    switch(choice)
    {
        // case 1: 
        // {
        //     cpu = APEX_cpu_init(argv[1]);
        //     if((strcmp(argv[1],"simulate")) == 0)
        //     {
        //         APEX_cpu_run(cpu,n);
        //     }
        // if (!cpu)
        // {
        //     fprintf(stderr, "APEX_Error: Unable to initialize and simulate CPU\n");
        //     exit(1);
        // }
        // break;
        // }

        // case 2:
        // {
        //     if(cpu!=NULL)
        //     {
        //         fprintf(stderr,"Enter the numer of cycles:\n");
        //         scanf("%d",&n);
        //         APEX_cpu_run(cpu,n);
        //     }
        //     else
        //     {
        //         fprintf(stderr,"You need to initialize your CPU first\n");
        //     }
        //     break;
        // }

        case 1:
        {
            if(cpu!=NULL)
            {
                cpu->single_step = ENABLE_SINGLE_STEP;
                APEX_cpu_run(cpu, n);
            }
            
            else
            {
                fprintf(stderr,"You need to initialize your CPU first\n");
            }
            break;
        }

        case 2:
        {
            if(cpu!=NULL)
            {
            display(cpu);
            for(int i=0;i<4096;i++)
            {
                if(cpu->data_memory[i] != 0)
                    {
                        printf("\nMEM[%d] = %d\n",i, cpu->data_memory[i]);
                    }
            }
            }
            else
            {
                fprintf(stderr,"You need to initialize your CPU first\n");
            }
            
            break;
        }

        case 3:
        {
            if(cpu!=NULL)
            {
                printf("Enter the location:\n");
                scanf("%d", &l);
                printf("MEM[%d] = %d",l,cpu->data_memory[l]);
            }
            else
            {
                fprintf(stderr,"You need to initialize your CPU first\n");
            }
            break;
        }

        case 4:
        {
            exit(0);
        }
    }
}
    APEX_cpu_stop(cpu);
    return 0;
}
