#include <stdint.h>
#include <stdio.h>

enum
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, /* program counter */
    R_COND,
    R_COUNT
};

uint16_t registers[R_COUNT];
uint16_t memory[UINT16_MAX];

enum
{
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
};

/* Condition Flags */
/**
 * It's a mystery. I've no clue at all why you need to bitshift these :)
 * 
 */
enum
{
    FL_POS = 1 << 0, /* P */
    FL_ZRO = 1 << 1, /* Z */
    FL_NEG = 1 << 2, /* N */
};

enum
{
    TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT = 0x21,   /* output a character */
    TRAP_PUTS = 0x22,  /* output a word string */
    TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25   /* halt the program */
};

/**
 * @brief 
 *  might need a bit refactoring, but this is how it made more sense to me. 
 *  e.g:
 *  
 *  instruction                     = 0000 0111 0000 0000 = 0x700 = 1792d
 *  start_bit                       = 0000 0001 0000 0000 = 0x9 = 9d (location)
 *  bit_count_to_mask               = 0000 0000 0000 0111 = 0x7 = 3d
 *  bit_count_to_mask << start_bit  = 0000 0111 0000 0000 = 0x700 = 1792d
 *  instruction & mask              = 0000 0111 0000 0000 = 0x700 = 1792d
 *  >> start_bit                    = 0000 0000 0000 0111
 * @param start_bit - starting from this position
 * @param bit_count_to_mask - how many bits we need to get
 * @param instruction 
 * @return uint16_t 
 */
uint16_t extract_instruction_bits(uint16_t start_bit, uint16_t bit_count_to_mask, uint16_t instruction)
{
    uint16_t mask = bit_count_to_mask << start_bit;
    uint16_t result = (instruction & mask) >> start_bit;

    return result;
}

/**
 * @brief https://justinmeiners.github.io/lc3-vm/#1:6
 *  x = x + FFFF << bit_count
 *  e.g
 *  if the x is negative number: 0001 0000 (16d) and bitcount is 5d
 *  then we do the following:
 *  x = 0000 0000 0001 0000 + (1111 1111 1111 1111 << 5 = 1111 1111 1110 0000)
 *  x = 0000 0000 0001 0000 + 1111 1111 1110 0000
 *  x = 1111 1111 1111 0000
 *  x = 16 in decimal
 * 
 * else, we just pass a 16 bit positive number through as it is and do nothing
 * @param x presumably negative value that we need to extend to accomodate the word length of the machine, 16 bit
 * @param bit_count this denotes the amount of bits that matter in uint16_t x var, e.g: if this was 5, then the 5 right most bit will be preserved 
 * @return uint16_t 
 */
uint16_t sign_extend(uint16_t x, int bit_count)
{

    if ((x >> (bit_count - 1)) & 1)
    {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

/**
 * @brief https://justinmeiners.github.io/lc3-vm/#1:6
 *  copy pasted, self explanatory
 * @param r 
 */
void update_flags(uint16_t r)
{
    if (registers[r] == 0)
    {
        registers[R_COND] = FL_ZRO;
    }
    else if (registers[r] >> 15) /* a 1 in the left-most bit indicates negative */
    {
        registers[R_COND] = FL_NEG;
    }
    else
    {
        registers[R_COND] = FL_POS;
    }
}

int main()
{
    memory[0] = OP_TRAP << 12;

    // 1. Load one instruction from memory at the address of the PC register.
    uint16_t PC_register = registers[R_PC];
    PC_register = 0;
    uint16_t instruction = memory[PC_register];
    instruction += TRAP_PUTS;

    // 2. Increment the PC register.
    PC_register++;
    registers[R_PC] = PC_register;

    // push in only the op-code part, as in: 0001 0000 0000 0000 will become = 0000 0000 0000 0001
    const uint16_t CURRENT_OPCODE = instruction >> 12;

    printf("Instruction after DR and mode bit %d\n", instruction);
    printf("\n");
    int loop_counter = 0;

    while (loop_counter < 1)
    {
        loop_counter++;
        switch (CURRENT_OPCODE)
        {
        case OP_BR:;
            {

                uint16_t PCoffset9 = 0x1FF;
                uint16_t n_bit_enabled = 0x0700;
                // 0000 0001 1111 1111
                instruction = instruction + PCoffset9;
                // 0000 1001 1111 1111
                instruction = instruction + n_bit_enabled;

                // instruction bits are empty, we can just push it towards the right
                uint16_t nzp_bits = instruction >> 0x9;

                // if n z or p bits are set
                if ((nzp_bits & registers[R_COND]))
                {
                    // I... I just figured it out, I can mask it with and operator aswell... Geezzzzzzzzzzz
                    // instruction can have what ever if you just AND the instruction and mask..........
                    // 0x1FF
                    // 0000 0001 1111 1111
                    // :)
                    uint16_t pc_offset = sign_extend(instruction & 0x1FF, 9);
                    registers[R_PC] += pc_offset;
                }

                printf("instruction branch %d\n", nzp_bits);
            }

            break;
        case OP_ADD:;
            {
                // --------------------------- building instruction for test case -----------------------------------------------------

                // fifth bit - modebit
                // 0001 0000 0001 0000
                instruction += 0b1 << 4;

                // DR - setting the register where we should save the result of the ADD operation
                // 0001 0000 0001 0000
                instruction += R_R0 << 8;

                // add 1 and 1 to R1 and R2 registers
                registers[R_R1] = 1;
                registers[R_R2] = 1;
                // put R1 register address into instruction starting from the sixth bit
                instruction += R_R1 << 5;
                // put R2 register address into instruction starting from the beginning
                instruction += R_R2; // remember to use imm5 if modebit is 0

                // --------------------------- END -----------------------------------------------------------------------------------

                // mode bit: 0000 0000 0001 0000 according the specs
                // this call gets the fifth bit out
                uint16_t mode_bit = extract_instruction_bits(4, 0x1, instruction);

                // destination register is 3 bit wide and starts from 9th bit
                // 0000 0111 0000 0000
                // to get three bits, we give 0x7 as an argument for the mask
                uint16_t dest_register = extract_instruction_bits(8, 0x7, instruction);

                uint16_t source_one = extract_instruction_bits(5, 0x3, instruction);
                if (mode_bit == 0)
                {
                    uint16_t source_two = extract_instruction_bits(0, 0x3, instruction);
                    registers[dest_register] = registers[source_one] + registers[source_two];
                }
                else
                {
                    uint16_t imm5 = extract_instruction_bits(0, 0x5, instruction);
                    registers[dest_register] = registers[source_one] + sign_extend(imm5, 5);
                }

                printf("Result stored into register : %d", registers[dest_register]);
                update_flags(dest_register);
            }
            break;
        case OP_LD:;
            {
                /**
                 * @brief An address is computed by sign-extending bits [8:0] to 16 bits and adding this
                    value to the incremented PC. The contents of memory at this address are loaded
                    into DR. The condition codes are set, based on whether the value loaded is
                    negative, zero, or positive.
                * 
                */
                uint16_t pc_offset9 = instruction & 0x1FF;
                uint16_t dest_reg = (instruction >> 9) & 0x7;
                uint16_t extended_offset9 = sign_extend(pc_offset9, 9);
                uint16_t read_addr = registers[R_PC] + extended_offset9;
                registers[dest_reg] = memory[read_addr];
                update_flags(dest_reg);
            }
            break;
        case OP_ST:;
            {
                // store source reg value into memory extended by offset
                uint16_t pc_offset9 = instruction & 0x1FF;
                uint16_t source_reg = (instruction >> 9) & 0x7;
                uint16_t extended_pc_offset9 = sign_extend(pc_offset9, 9);
                memory[registers[R_PC] + extended_pc_offset9] = registers[source_reg];
            }
            break;
        case OP_JSR:;
            {
                registers[R_R7] = registers[R_PC];
                uint16_t mode_bit = (instruction >> 11) & 1;
                if (mode_bit == 0)
                {
                    uint16_t r1 = (instruction >> 6) & 0x7;
                    registers[R_PC] = registers[r1]; /* JSRR */
                }
                else
                {
                    // mask
                    // 0000 0111 1111 1111
                    uint16_t long_pc_offset = sign_extend(instruction & 0x7FF, 11);
                    registers[R_PC] += long_pc_offset; /* JSR */
                }
            }
            break;
        case OP_AND:;
            {
                uint16_t mode_bit = extract_instruction_bits(4, 0x1, instruction);
                uint16_t dest_register = extract_instruction_bits(8, 0x7, instruction);

                uint16_t source_one = extract_instruction_bits(5, 0x3, instruction);
                if (mode_bit == 0)
                {
                    uint16_t source_two = extract_instruction_bits(0, 0x3, instruction);
                    registers[dest_register] = registers[source_one] & registers[source_two];
                }
                else
                {
                    uint16_t imm5 = extract_instruction_bits(0, 0x5, instruction);
                    registers[dest_register] = registers[source_one] & sign_extend(imm5, 5);
                }

                update_flags(dest_register);
            }
            break;
        case OP_LDR:;
            {
                uint16_t r0 = (instruction >> 9) & 0x7;
                uint16_t r1 = (instruction >> 6) & 0x7;
                uint16_t offset = sign_extend(instruction & 0x3F, 6);
                registers[r0] = memory[registers[r1] + offset];
                update_flags(r0);
            }
            break;
        case OP_STR:;
            {
                uint16_t r0 = (instruction >> 9) & 0x7;
                uint16_t r1 = (instruction >> 6) & 0x7;
                uint16_t offset = sign_extend(instruction & 0x3F, 6);
                memory[registers[r1] + offset] = registers[r0];
            }
            break;
        case OP_RTI:
            break;
        case OP_NOT:;
            {
                // 1001000100111111
                instruction += 0x13F;
                printf("instruction NOT %d\n", instruction);

                uint16_t dest = (instruction >> 9) & 0x7;
                uint16_t r1 = (instruction >> 6) & 0x7;
                registers[dest] = ~registers[r1];
                update_flags(dest);
            }
            break;
        case OP_LDI:;
            {

                // --------------------------- building instruction for test case -----------------------------------------------------
                // FFS, this shit starts from: 0000 1110 0000 0000
                // setting up the destination register
                instruction = R_R7 << 9;

                // setting up dummy data for offset9
                instruction += 0x1FF; // 0000 0001 1111 1111b - 511d

                // --------------------------- END -----------------------------------------------------------------------------------

                // get the 9 right most bits and
                uint16_t offset_address_for_new_PC = extract_instruction_bits(0, 0x1FF, instruction);
                uint16_t fetch_PC_addr = registers[R_PC] + sign_extend(offset_address_for_new_PC, 0x9);

                // fetch a value for the new program counter value from memory
                // 1. get the address where new PC is stored
                // 2. read the PC value from another location (like pointers in c)
                uint16_t address_read_from_memory = memory[fetch_PC_addr];
                uint16_t value_read_from_memory = memory[address_read_from_memory];

                // get dest register
                uint16_t dest_register = instruction >> 9;
                registers[dest_register] = value_read_from_memory;
            }
            break;
        case OP_STI:;
            {
                uint16_t pc_offset9 = instruction & 0x1FF;
                uint16_t source_reg = (instruction >> 9) & 0x7;
                uint16_t extended_pc_offset9 = sign_extend(pc_offset9, 9);

                uint16_t addr_location = registers[R_PC] + extended_pc_offset9;
                // e.g memory[mem[...]]
                // has address 0x0001 <= this is is the address we store reg[source]
                memory[memory[addr_location]] = registers[source_reg];
            }
            break;
        case OP_JMP:;
            {
                uint16_t r1 = (instruction >> 6) & 0x7;
                registers[R_PC] = registers[r1];
            }
            break;
        case OP_RES:
            break;
        case OP_LEA:;
            {
                uint16_t pc_offset9 = instruction & 0x1FF;
                uint16_t dest_reg = (instruction >> 9) & 0x7;
                uint16_t extended_pc_offset9 = sign_extend(pc_offset9, 9);
                registers[dest_reg] = registers[R_PC] + extended_pc_offset9;
            }
            break;
        case OP_TRAP:
            
            switch (instruction & 0xFF)
            {
            case TRAP_GETC:;
                {
                }
                break;
            case TRAP_OUT:

                break;
            case TRAP_PUTS:;
                {
                    /* one char per word */
                    char greeting[] = "hello";

                    memory[100] = greeting[0];
                    memory[101] = greeting[1];
                    memory[102] = greeting[2];
                    memory[103] = greeting[3];
                    memory[104] = greeting[4];

                    registers[R_R0] = 100;

                    uint16_t *c = memory + registers[R_R0];
            
                    while (*c)
                    {
                 
                        putc((char)*c, stdout);
                        ++c;
                    }
                    fflush(stdout);
                }
                break;
            case TRAP_IN:

                break;
            case TRAP_PUTSP:;
                {
                }
                break;
            case TRAP_HALT:

                break;
            }
            break;
        default:
            break;
        }
    }

    return 1;
}