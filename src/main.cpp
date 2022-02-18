#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fstream>
#include <filesystem>
#include <iterator>
#include <sstream>
#include <string>
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <memory>

#include "Options.hpp"

#include "colors.h"

using namespace std;

namespace DcsEmbler {

struct FreeDel {
    const void operator()(void* p) {
        free(p);
    }
};

struct Label {
    /// Holds the value of the (zero-based) index of the instruction this is.
    /// Just counts up for each instruction, starting at 0.
    int instructionIndex = 0;
    /// Holds the (one-based) line number this label was found on.
    int declaredOnLine = 0;
};

const char* Options::outputFormatForBinary = ".bin.riscv5i";
const char* Options::outputFormatForHex = ".hex.riscv5i";

Options opts;

auto instructionIndexToAddress(int instructionIndex) -> int {
    return instructionIndex * 4 + *opts.startOfMemory;
}

struct LabelSet : public unordered_map<string, Label> {
    auto prettyPrint() -> void {
        cout << "Labels and their values:\n";
        for (const auto& [ labelName, label ] : *this) {
            const auto& [ instructionIndex, lineNumberDeclared ] = label;

            const auto instructionAddress = instructionIndexToAddress(instructionIndex);
            printf("Label " GREENC("%s")
                            " -> "
                            YELLOWC("instruction no. %i (0x%x)")
                            "/"
                            MAGENTAC("address %i (0x%x)")
                            "/"
                            CYANC("line %i") "\n",
                labelName.c_str(),
                instructionIndex, instructionIndex,
                instructionAddress, instructionAddress,
                lineNumberDeclared);
        }
    }
};

FILE* out = nullptr;
int instructionIndex = 0;
LabelSet labels{};

auto immediateTo2ByteSignedOffset(int immediateAddressOfByte, int currentInstructionIndex) -> int {
    return (immediateAddressOfByte - instructionIndexToAddress(currentInstructionIndex)) / 2;
}

auto labelTo2ByteSignedOffset(Label l, int currentInstructionIndex) -> int {
    /// in number of bytes
    const int labelAddress = instructionIndexToAddress(l.instructionIndex);
    /// in number of bytes
    const int currentAddress = instructionIndexToAddress(currentInstructionIndex);
    return (labelAddress - currentAddress) / 2;
}

auto fileSize(FILE* f) -> off_t {
    auto start = ftell(f);

    fseek(f, 0L, SEEK_END);
    auto ret = ftello(f);

    fseek(f, start, SEEK_SET);

    return ret;
}

struct File {
    const char* const text;
    char* cursor;
    off_t size;

    File(char* text, off_t size) : text(text), cursor(text), size(size) {};

    ~File() {
        free((void*) text);
    }
};

auto readEntireFile(const char* filepath) -> File {
    FILE* f = fopen(filepath, "r");
    off_t size = fileSize(f);

    char* text = (char*) malloc(sizeof(char) * size);
    fread(text, sizeof(char), size, f);

    fclose(f);

    return File{text, size};
}

auto getNextLine(File& f, char*& current_line) -> bool {
    if (current_line == f.cursor)
        return false;

    char* next_newline = strchr(f.cursor, '\n');

    if (next_newline == nullptr) {
        current_line = f.cursor;
    } else {
        *next_newline = '\0';
        next_newline++;
        current_line = f.cursor;
        f.cursor = next_newline;
    }

    return true;
}

auto isComment(char* first_token) -> bool {
    return first_token[0] == '#';
}

auto isLabel(char* first_token) -> bool {
    auto len = strlen(first_token);
    if (len == 0) {
        return false;
    } else {
        return first_token[len - 1] == ':';
    }
}

#include <cctype>

auto toLower(char* s) -> char* {
    for (char* c = s; *c != '\0'; c++) {
        if (not islower(*c)) *c = tolower(*c);
    }
    return s;
}

auto regToNum(char* token) -> int {
    // Registers are denoted as x1, x2, ..., x30, x31, x32.
    // So just do...
    return atoi(token + 1);
}

auto doIFormatInstruction(char* tokens[], int tokenCount, int lineNumber,
                                  int opcode, int funct3,
                                  int setImm_5_11To = -1) -> unsigned int {
        const auto target = 0x00310093;
        //    e.g. target = addi x1 x2 3
        //                                      funct3      OP-IMM
        //                 |---- imm ----| |-x1-||-| |-x2-||-opco-|
        //                  0000 0000 0011 0001 0000 0000 0101 0011
        unsigned int instruction = 0x00000000;

        unsigned int immediate = atoi(tokens[3]);

        if (setImm_5_11To != -1) {
            unsigned int immediate_0_4 = (immediate & 0b11111);
            immediate = (setImm_5_11To << 5) | (immediate_0_4);
        }

        // imm[11:0] goes to [31:20]
        instruction = immediate; // immediate is most significant bits.

        // rs1 goes to [19:15]
        const auto rs1 = regToNum(tokens[2]); // TODO This order might be wrong.
        instruction = (instruction << 5) | rs1;

        // funct3 goes to [14:12]
        instruction = (instruction << 3) | funct3;

        // rd goes to [11:7]
        const auto rd = regToNum(tokens[1]);
        instruction = (instruction << 5) | rd;

        // opcode goes to [6:0]
        instruction = (instruction << 7) | opcode; // opcode is least significant bits.

        return instruction;
}

auto doRFormatInstruction(char* tokens[], int tokenCount, int lineNumber,
                          int opcode, int funct3, int funct7) -> unsigned int {
        // Format: R-type.
        //         target = 0x003150b3;
        //    e.g. target = srl x1 x2 x3
        //                              R-type format
        //                                      funct3
        //                  |funct7||-rs2| |-rs1||-| |-rd-||-opco-|
        //                  0000 0000 0011 0001 0101 0000 1011 0111

        unsigned int instruction = 0x00000000;

        // funct7
        instruction = funct7;

        // rs2
        const auto rs2 = regToNum(tokens[3]);
        instruction = (instruction << 5) | rs2;

        // rs1
        const auto rs1 = regToNum(tokens[2]);
        instruction = (instruction << 5) | rs1;

        // funct3
        instruction = (instruction << 3) | funct3;

        // rd
        const auto rd = regToNum(tokens[1]);
        instruction = (instruction << 5) | rd;

        // opcode
        instruction = (instruction << 7) | opcode;

        return instruction;
}

auto doUFormatInstruction(char* tokens[], int tokenCount,
                          int lineNumber, int opcode) -> unsigned int {
    // Format: U-type.
    // Instruction: lui <rd> <immediate value>
    //         target = 0x000030b7;
    //         target = lui x1 3
    //                              U-type format
    //                 |-------imm[31:12]------| |-rd-||-opco-|
    //                  0000 0000 0000 0000 0011 0000 1011 0111
    //                 <-- constant value -----> <reg-><-LUI-->
    unsigned int immediate = atoi(tokens[2]);

    unsigned int instruction = 0x00000000;

    // imm goes to [31:12]
    instruction = immediate; // immediate is most significant bits.

    // rd goes to [11:7]
    const auto rd = regToNum(tokens[1]);
    instruction = (instruction << 5) | rd;

    // opcode goes to [6:0]
    instruction = (instruction << 7) | opcode; // opcode is least significant bits.

    return instruction;
}

auto doSFormatInstruction(char* tokens[], int tokenCount, int lineNumber,
                          int opcode, int funct3) -> unsigned int {
        // Format: S-type
        //         target = SW x1, 3(x2)
        // for us, target = SW x1 3 x2
        //                              S-type format
        //                          source  base    offset[4:0]
        //                 offset[11:5]         funct3      opcode
        //                  |offset||-rs2| |-rs1||-| |----||------|
        //                  0000 0000 0001 0001 0010 0001 1010 0011

        const int immediate_offset = atoi(tokens[2]);

        if (abs(immediate_offset) > 0b111111111111) {
            // TODO Error
            cerr << "Error: S format instruction offset too big!\n";
            exit(EXIT_FAILURE);
        }

        unsigned int instruction = 0x00000000;

        const unsigned int offset_11_5 = (immediate_offset & (0b111111100000)) >> 7;
        instruction = offset_11_5;

        // Source register
        const auto dataSourceRegister_rs2 = regToNum(tokens[1]);
        instruction = (instruction << 5) | dataSourceRegister_rs2;

        // Base register
        const auto baseRegister_rs1 = regToNum(tokens[3]); // TODO Ordering wrong?
        instruction = (instruction << 5) | baseRegister_rs1;

        instruction = (instruction << 3) | funct3;

        const unsigned int offset_4_0  = (immediate_offset & (0b000000011111));
        instruction = (instruction << 5) | offset_4_0;

        instruction = (instruction << 7) | opcode;

        return instruction;
}

auto doBFormatInstruction(char* tokens[], int tokenCount, int lineNumber,
                          int opcode, int funct3) -> unsigned int {
        // Format: B-type
        char* destination = tokens[3];

        string destinationStr{destination};
        int immediate_offset;

        if (labels.contains(destinationStr)) {
            immediate_offset = labelTo2ByteSignedOffset(labels[destinationStr], instructionIndex);
        } else {
            immediate_offset = immediateTo2ByteSignedOffset(atoi(tokens[3]), instructionIndex);
        }

        if (abs(immediate_offset) > 0b11111111111111111111) {
            puts("Address jump too big - we haven't implemented JALR generation yet.");
            exit(EXIT_FAILURE);
        } else if (abs(immediate_offset) > 0b11111111111) {
            // Branch jump range (+- 4Kib) not big enough, so we need to use a branch and a jump.
            // Unconditional jumps have a range of +- 1Mib (20 bit offset, 1 for sign, so 2^19).
            // If we're in that range, then we need to generate a branch and a jump.
            // If we're outside of that range, then we can generate a JALR.
            // But for that, we'd need to sac a register???
            // So put it in x5?? It's called an 'alternate link register' in the spec, so...
            // Yeah, maybe?

            // Hah, as it turns out, gcc doesn't even ever generate a beq instruction.
            puts("Address jump too big - we haven't implemented branch and jump or branch and JALR generation yet.");
            exit(EXIT_FAILURE);
        }

        unsigned int off_12   = (immediate_offset & 0b100000000000) >> (11);
        unsigned int off_11   = (immediate_offset & 0b010000000000) >> (10);
        unsigned int off_10_5 = (immediate_offset & 0b001111110000) >> (4);
        unsigned int off_4_1  = (immediate_offset & 0b000000001111) >> (0);

        // Build the instruction
        unsigned int instruction = 0;
        instruction = off_12;
        instruction = (instruction << 6) | off_10_5;
        instruction = (instruction << 5) | regToNum(tokens[2]);
        instruction = (instruction << 5) | regToNum(tokens[1]);
        instruction = (instruction << 3) | funct3;
        instruction = (instruction << 4) | off_4_1;
        instruction = (instruction << 1) | off_11;
        instruction = (instruction << 7) | opcode;

        return instruction;
}

auto emitInstruction(unsigned int it) -> void {
    instructionIndex++;

    switch (*opts.format) {
        case Format::binary:
        case Format::bin:
            fwrite(&it, sizeof(unsigned int), 1, out);
            break;
        case Format::hexadecimal:
        case Format::hex:
            fprintf(out, "0x%08x\n", it);
            break;
    }
}

auto parseInstructionFrom(char* tokens[], int tokenCount, int lineNumber) -> bool {
    char* opcode = toLower(tokens[0]);

    unsigned int instruction = 0x00000000;

    bool matchedAnInstruction = true;

    if (strlen(opcode) < 1) {
        // TODO When?
        cout << "strlen opcode < 1: " << opcode << '\n';
        return false;
    }

    if (opcode[0] == '.') {
        // TODO Test
        // This is something like the metadata output by gcc.
        // For example, compiling a simple C program will produce:
        /*
            .file	"test.c"
            .option nopic
            .attribute arch, "rv32i2p0"
            .attribute unaligned_access, 0
            .attribute stack_align, 16
            .text
            .align	2
            .globl	main
            .type	main, @function
         */
        // At the top of the file.
        // We ignore these, so just return true to suggest that we're happy to continue.
        return true;
    }
    //region I-type instructions
    else if (strcmp("addi", opcode) == 0) {
        // ADDI (Addition Immediate)
        // Add sign-extended 12-bit imm to register rs1, storing result in rd.
        // Instruction: addi <rd> <rs1> <imm>
        const int machineOpcode = 0b0010011;
        const int funct3 = 0x00;
        instruction = doIFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("xori", opcode) == 0) {
        // XORI (Exclusive Or Immediate)
        // Instruction: xori <rd> <rs1> <imm>
        const int machineOpcode = 0b0010011;
        const int funct3 = 0x04;
        instruction = doIFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("ori", opcode) == 0) {
        // ORI (Or Immediate)
        // Instruction: ori <rd> <rs1> <imm>
        const int machineOpcode = 0b0010011;
        const int funct3 = 0x06;
        instruction = doIFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("andi", opcode) == 0) {
        // ANDI (And Immediate)
        // Instruction: andi <rd> <rs1> <imm>
        const int machineOpcode = 0b0010011;
        const int funct3 = 0x07;
        instruction = doIFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("slli", opcode) == 0) {
        // SLTI (Shift Left Logical Immediate)
        // Instruction: slli <rd> <rs1> <imm>
        const int machineOpcode = 0b0010011;
        const int funct3 = 0x01;
        const int setImm_5_11To = 0x00;
        instruction = doIFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, setImm_5_11To);
    } else if (strcmp("srli", opcode) == 0) {
        // SLRI (Shift Right Logical Immediate)
        // Instruction: slri <rd> <rs1> <imm>
        const int machineOpcode = 0b0010011;
        const int funct3 = 0x05;
        const int setImm_5_11To = 0x00;
        instruction = doIFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, setImm_5_11To);
    } else if (strcmp("srai", opcode) == 0) {
        // SRAI (Shift Right Arith Immediate)
        // Instruction: srai <rd> <rs1> <imm>
        const int machineOpcode = 0b0010011;
        const int funct3 = 0x05;
        const int setImm_5_11To = 0x20;
        instruction = doIFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, setImm_5_11To);
    } else if (strcmp("slti", opcode) == 0) {
        // SLTI (Set Less Than Immediate)
        // Instruction: slti <rd> <rs1> <imm>
        const int machineOpcode = 0b0010011;
        const int funct3 = 0x02;
        instruction = doIFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("jalr", opcode) == 0) {
        // JALR (Jump and Link Reg)
        // Instruction: jalr <rd> <imm> <rs1>
        char* reorderedTokens[4];
        reorderedTokens[0] = nullptr;
        reorderedTokens[1] = tokens[1]; // rd
        reorderedTokens[2] = tokens[3]; // rs1
        reorderedTokens[3] = tokens[2]; // immediate value
        const int machineOpcode = 0b1100111;
        const int funct3 = 0x00;
        instruction = doIFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("ecall", opcode) == 0) {
        // ECALL (Environment Call)
        // Instruction: ecall
        const int machineOpcode = 0b1110011;
        const int funct3 = 0x00;
        tokens[3] = (char*)"0";
        instruction = doIFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("ebreak", opcode) == 0) {
        // EBREAK (Environment Break)
        // Instruction: ecall
        const int machineOpcode = 0b1110011;
        const int funct3 = 0x00;
        tokens[3] = (char*)"1";
        instruction = doIFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("sltiu", opcode) == 0) {
        // SLTI (Set Less Than Immediate Unsigned)
        // Instruction: sltiu <rd> <rs1> <imm>
        const int machineOpcode = 0b0010011;
        const int funct3 = 0x03;
        instruction = doIFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("lw", opcode) == 0) {
        // LW (Load Word).
        // Load a 32-bit value from memory into rd.
        // Format: I-type
        // Instruction: lw <rd> <immediate offset> <register of base address>

        //         target = LW x1, 3(x2)
        // for us, target = LW x1 3 x2
        //                              I-type format
        //                                       width  dest
        //                                  base funct3 rd
        //                  |---offset---| |----||-| |----||-opco-|
        //                  0000 0000 0011 0001 0010 0000 1000 0011

        const auto target = 0x00312083;
        char* reorderedTokens[4];
        reorderedTokens[0] = nullptr;
        reorderedTokens[1] = tokens[1]; // rd
        reorderedTokens[2] = tokens[3]; // rs1
        reorderedTokens[3] = tokens[2]; // immediate value
        const int funct3 = 0b010;
        const int machineOpcode = 0b0000011;

        instruction = doIFormatInstruction(reorderedTokens, tokenCount, lineNumber,
                                           machineOpcode, funct3);
    } else if (strcmp("lh", opcode) == 0) {
        // LH (Load Half).
        // Load a 16-bit value from memory into rd.
        // Format: I-type
        // Instruction: lh <rd> <immediate offset> <register of base address>

        char* reorderedTokens[4];
        reorderedTokens[0] = nullptr;
        reorderedTokens[1] = tokens[1]; // rd
        reorderedTokens[2] = tokens[3]; // rs1
        reorderedTokens[3] = tokens[2]; // immediate value

        const int funct3 = 0b001;
        const int machineOpcode = 0b0000011;

        instruction = doIFormatInstruction(reorderedTokens, tokenCount, lineNumber,
                                           machineOpcode, funct3);
    } else if (strcmp("lb", opcode) == 0) {
        // LB (Load Byte).
        // Load a 8-bit value from memory into rd.
        // Format: I-type
        // Instruction: lb <rd> <immediate offset> <register of base address>

        char* reorderedTokens[4];
        reorderedTokens[0] = nullptr;
        reorderedTokens[1] = tokens[1]; // rd
        reorderedTokens[2] = tokens[3]; // rs1
        reorderedTokens[3] = tokens[2]; // immediate value

        const int funct3 = 0b000;
        const int machineOpcode = 0b0000011;

        instruction = doIFormatInstruction(reorderedTokens, tokenCount, lineNumber,
                                           machineOpcode, funct3);
    } else if (strcmp("lbu", opcode) == 0) {
        // LB (Load Byte Unsigned).
        // Load a 8-bit value from memory into rd.
        // Format: I-type
        // Instruction: lb <rd> <immediate offset> <register of base address>

        char* reorderedTokens[4];
        reorderedTokens[0] = nullptr;
        reorderedTokens[1] = tokens[1]; // rd
        reorderedTokens[2] = tokens[3]; // rs1
        reorderedTokens[3] = tokens[2]; // immediate value

        const int funct3 = 0b100; // 0x4
        const int machineOpcode = 0b0000011;

        instruction = doIFormatInstruction(reorderedTokens, tokenCount, lineNumber,
                                           machineOpcode, funct3);
    } else if (strcmp("lhu", opcode) == 0) {
        // LH (Load Half Unsigned).
        // Load a 16-bit value from memory into rd.
        // Format: I-type
        // Instruction: lb <rd> <immediate offset> <register of base address>

        char* reorderedTokens[4];
        reorderedTokens[0] = nullptr;
        reorderedTokens[1] = tokens[1]; // rd
        reorderedTokens[2] = tokens[3]; // rs1
        reorderedTokens[3] = tokens[2]; // immediate value

        const int funct3 = 0b101; // 0x5
        const int machineOpcode = 0b0000011;

        instruction = doIFormatInstruction(reorderedTokens, tokenCount, lineNumber,
                                           machineOpcode, funct3);
    }
    //endregion I-type instructions

    //region J-type instructions
    else if (strcmp("jal", opcode) == 0) {
        // JAL (Jump And Link)
        // Jump to the specified location, placing PC+4 into rd.
        // Format: J-type.
        // Instruction: jal <rd> <immediate value - address>
        char* destination = tokens[2];

        //         target = fd5ff0ef
        //         _start = 0x10054
        //         target = JAL x1, _start
        //                              J-type format
        //                     imm_10_1     imm_19_12
        //                 imm20  |     imm11  |
        //                  |     |      |     |
        //                  ||----------|||--------|
        //                  |--------imm off-------| |-rd-||-opco-|
        //         target = 1111 1101 0101 1111 1111 0000 1110 1111
        string destinationStr{destination};
        int immediate;
        if (labels.contains(destinationStr)) {
            immediate = labelTo2ByteSignedOffset(labels[destinationStr], instructionIndex);
            //const int labelDestinationInstructionIndex = labels[destinationStr].instructionIndex;
            //const int difference = (labelDestinationInstructionIndex - instructionIndex) / 2;
            //immediate = difference;
        } else {
            // const int immediateDestinationInstructionIndex = atoi(tokens[2]);
            // const int difference = (immediateDestinationInstructionIndex - instructionIndex) / 2;
            // immediate = difference;

            // const int immediateDestinationInstructionIndex = atoi(tokens[2]);
            // //const int difference = (immediateDestinationInstructionIndex - instructionIndex) / 2;
            // const int difference = (immediateDestinationInstructionIndex / 2 - instructionIndex);
            // immediate = difference;

            immediate = immediateTo2ByteSignedOffset(atoi(tokens[2]), instructionIndex);

            //const int immediateDestinationInstructionIndex = atoi(tokens[2]);
            //const int difference = (immediateDestinationInstructionIndex - instructionIndex) / 2;
            //const int difference = (immediateDestinationInstructionIndex / 2 - instructionIndex);
            //immediate = immediateDestinationInstructionIndex;
        }

        // TODO
        if (immediate > 0b11111111111111111111) {
            puts("Address jump too big.");
            exit(EXIT_FAILURE);
        }

        const unsigned int immediate_20 = (immediate &    0b10000000000000000000);
        const unsigned int immediate_10_1 = (immediate &  0b00000000001111111111) << 9;
        const unsigned int immediate_11 = (immediate &    0b00000000010000000000) >> 2;
        const unsigned int immediate_19_12 = (immediate & 0b01111111100000000000) >> 11;

        // const int immediate_20 = (immediate &    0b10000000000000000000) >> 19;
        // const int immediate_10_1 = (immediate &  0b00000000001111111111);
        // const int immediate_11 = (immediate &    0b00000000010000000000) >> 10;
        // const int immediate_19_12 = (immediate & 0b01111111100000000000) >> 11;

        instruction = immediate_20 | immediate_19_12 | immediate_11 | immediate_10_1;

        // 0xfebff0ef    1111 1110 1011 1111 1111 | 0000 1110 1111
        // 0xfdbff0ef    1111 1101 1011 1111 1111 | 0000 1110 1111
        //               1

        // instruction = immediate_20;
        // instruction = (instruction << 10) | immediate_10_1;
        // instruction = (instruction << 1) | immediate_11;
        // instruction = (instruction << 8) | immediate_19_12;

        const auto rd = regToNum(tokens[1]);
        instruction = (instruction << 5) | rd;

        const int JAL = 0b1101111;
        instruction = (instruction << 7) | JAL;
    }
    //endregion J-type instructions

    //region U-type instructions
    else if (strcmp("lui", opcode) == 0) {
        // LUI (Load Upper Immediate).
        // Load a 32-bit constant to top 20 bits of register rd, filling rest with zeroes.
        // Format: U-type.
        // Instruction: lui <rd> <immediate value>

        const auto target = 0x000030b7;
        //         target = lui x1 3
        //                              U-type format
        //                 |-------imm[31:12]------| |-rd-||-opco-|
        //                  0000 0000 0000 0000 0011 0000 1011 0111
        //                 <-- constant value -----> <reg-><-LUI-->
        const int LUI = 0b0110111;
        unsigned int immediate = atoi(tokens[2]);

        // imm goes to [31:12]
        instruction = immediate; // immediate is most significant bits.

        // rd goes to [11:7]
        const auto rd = regToNum(tokens[1]);
        instruction = (instruction << 5) | rd;

        // opcode goes to [6:0]
        instruction = (instruction << 7) | LUI; // opcode is least significant bits.
    } else if (strcmp("auipc", opcode) == 0) {
        // AUIPC (Add Upper IMM To PC).
        // Format: U-type.
        // Instruction: auipc <rd> <immediate value>
        // Post-condition (C): rd = PC + (imm << 12) && PC updated.

        const int AUIPC = 0b0010111;

        // imm goes to [31:12]
        unsigned int immediate = atoi(tokens[2]);
        instruction = immediate; // immediate is most significant bits.

        // rd goes to [11:7]
        const auto rd = regToNum(tokens[1]);
        instruction = (instruction << 5) | rd;

        // opcode goes to [6:0]
        instruction = (instruction << 7) | AUIPC; // opcode is least significant bits.
    }
    //endregion U-type instructions

    //region R-type instructions
    else if (strcmp("add", opcode) == 0) {
        // ADD.
        // Format: R-type.
        // Instruction: add <rd> <rs1> <rs2>
        const int machineOpcode = 0b0110011;
        const int funct7 = 0b0000000;
        const int funct3 = 0b0000;
        instruction = doRFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, funct7);
    } else if (strcmp("sub", opcode) == 0) {
        // SUB.
        // Format: R-type.
        // Instruction: sub <rd> <rs1> <rs2>
        const int machineOpcode = 0b0110011;
        const int funct7 = 0x20;
        const int funct3 = 0b0000;
        instruction = doRFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, funct7);
    } else if (strcmp("xor", opcode) == 0) {
        // XOR.
        // Format: R-type.
        // Instruction: xor <rd> <rs1> <rs2>
        const int machineOpcode = 0b0110111;
        const int funct7 = 0b0000000;
        const int funct3 = 0b0100; // 0x4
        instruction = doRFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, funct7);
    } else if (strcmp("or", opcode) == 0) {
        // OR.
        // Format: R-type.
        // Instruction: or <rd> <rs1> <rs2>
        const int machineOpcode = 0b0110011;
        const int funct7 = 0b0000000;
        const int funct3 = 0b0110; // 0x6
        instruction = doRFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, funct7);
    } else if (strcmp("and", opcode) == 0) {
        // AND.
        // Format: R-type.
        // Instruction: or <rd> <rs1> <rs2>
        const int machineOpcode = 0b0110011;
        const int funct7 = 0b0000000;
        const int funct3 = 0b0111; // 0x7
        instruction = doRFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, funct7);
    } else if (strcmp("sll", opcode) == 0) {
        // SLL. (Shift Left Logical)
        // Logical left shift of the value in rs1 by the amount in the lower 5 bits of rs2, putting
        // the result in rd.
        // Format: R-type.
        // Instruction: or <rd> <rs1> <rs2>
        const int machineOpcode = 0b0110011;
        const int funct7 = 0b0000000;
        const int funct3 = 0b0001; // 0x1
        instruction = doRFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, funct7);
    } else if (strcmp("srl", opcode) == 0) {
        // SRL (Shift Right Logical).
        // Logical right shift of the value in rs1 by the amount in the lower 5 bits of rs2, putting
        // the result in rd.
        // Format: R-type.
        // Instruction: srl <rd> <rs1> <rs2>

        const auto target = 0x003150b3; // target = srl x1 x2 x3
        //                              R-type format
        //                                      funct3
        //                  |funct7||-rs2| |-rs1||-| |-rd-||-opco-|
        //                  0000 0000 0011 0001 0101 0000 1011 0111
        const int machineOpcode = 0b0110011;
        const int funct7 = 0b0000000;
        const int funct3 = 0b0101;
        instruction = doRFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, funct7);
    } else if (strcmp("sra", opcode) == 0) {
        // SRA. (Shift Right Arithmetic)
        // Format: R-type.
        // Instruction: SRA <rd> <rs1> <rs2>
        const int machineOpcode = 0b0110011;
        const int funct7 = 0b0010100; // 0x20
        const int funct3 = 0b0101; // 0x5
        instruction = doRFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, funct7);
    } else if (strcmp("slt", opcode) == 0) {
        // SLT. (Set Less Than)
        // Format: R-type.
        // Instruction: SLT <rd> <rs1> <rs2>
        const int machineOpcode = 0b0110011;
        const int funct7 = 0b0000000; // 0x00
        const int funct3 = 0b0010; // 0x2
        instruction = doRFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, funct7);
    } else if (strcmp("sltu", opcode) == 0) {
        // SLTU. (Set Less Than)
        // Format: R-type.
        // Instruction: SLT <rd> <rs1> <rs2>
        const int machineOpcode = 0b0110011;
        const int funct7 = 0b0000000; // 0x00
        const int funct3 = 0b0011; // 0x3
        instruction = doRFormatInstruction(tokens, tokenCount, lineNumber,
                                           machineOpcode, funct3, funct7);
    }
    //endregion R-type instructions

    //region S-type instructions
    else if (strcmp("sw", opcode) == 0) {
        // SW (Store Word).
        // Store a 32-bit value from the register rs2 to memory.
        // Format: S-type
        // Instruction: sw <

        const auto target = 0x001121a3;
        //         target = SW x1, 3(x2)
        // for us, target = SW x1 3 x2
        //                              S-type format
        //                          source  base    offset[4:0]
        //                 offset[11:5]         funct3      opcode
        //                  |offset||-rs2| |-rs1||-| |----||------|
        //                  0000 0000 0001 0001 0010 0001 1010 0011
        //         ours:    0000 0000 0001 0000 0

        const int machineOpcode = 0b0100011;
        const int funct3 = 0b010;
        instruction = doSFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("sh", opcode) == 0) {
        // SH (Store Half).
        // Store a 16-bit value from the register rs2 to memory.
        // Format: S-type
        const int machineOpcode = 0b0100011;
        const int funct3 = 0b001;
        instruction = doSFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("sb", opcode) == 0) {
        // SB (Store Byte).
        // Store a 8-bit value from the register rs2 to memory.
        // Format: S-type
        const int machineOpcode = 0b0100011;
        const int funct3 = 0b000;
        instruction = doSFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    }
    //endregion S-type instructions

    //region B-type instructions
    else if (strcmp("beq", opcode) == 0) {
        // BEQ (Branch if Equal).
        // Format: B-type
        const int machineOpcode = 0b1100011;
        const int funct3 = 0b000; // 0x00
        instruction = doBFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("bne", opcode) == 0) {
        // BNE (Branch Not Equal).
        // Format: B-type
        const int machineOpcode = 0b1100011;
        const int funct3 = 0b001; // 0x01
        instruction = doBFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("blt", opcode) == 0) {
        // BLT (Branch Less Than).
        // Format: B-type
        const int machineOpcode = 0b1100011;
        const int funct3 = 0b100; // 0x04
        instruction = doBFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("bge", opcode) == 0) {
        // BLT (Branch Greater Than Or Equal to).
        // Format: B-type
        const int machineOpcode = 0b1100011;
        const int funct3 = 0x5;
        instruction = doBFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("bltu", opcode) == 0) {
        // BLTU (Branch Less Than Unsigned).
        // Format: B-type
        const int machineOpcode = 0b1100011;
        const int funct3 = 0b110; // 0x06
        instruction = doBFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    } else if (strcmp("bgeu", opcode) == 0) {
        // BGEU (Branch Greater Than or Equal To Unsigned).
        // Format: B-type
        const int machineOpcode = 0b1100011;
        const int funct3 = 0b111; // 0x07
        instruction = doBFormatInstruction(tokens, tokenCount, lineNumber, machineOpcode, funct3);
    }
    //endregion B-type instructions

    //region Pseudoinstructions
    else if (strcmp("mv", opcode) == 0) {
        // (Pseudoinstruction)
        // Move the value in rs1 to rd.
        // Instruction: mv <rd>, <rs1>
        // Real instruction: addi <rd>, <rs1>, 0
        tokens[0] = const_cast<char*>("addi");
        tokens[1] = tokens[1];
        tokens[2] = tokens[2];
        tokens[3] = const_cast<char*>("0");
        return parseInstructionFrom(tokens, 4, lineNumber);
    } else if (strcmp("jr", opcode) == 0) {
        // JR pseudoinstruction - jump register
        // Instruction: jr rs
        // Real instruction: jalr x0, 0(rs)
        tokens[0] = const_cast<char*>("jalr");
        tokens[3] = tokens[1];
        tokens[1] = const_cast<char*>("x0");
        tokens[2] = const_cast<char*>("0");
        return parseInstructionFrom(tokens, 4, lineNumber);
    } else if (strcmp("nop", opcode) == 0 or strcmp("noop", opcode) == 0) {
        // nop is just an alias for addi x0, x0, 0
        tokens[0] = const_cast<char*>("addi");
        tokens[1] = const_cast<char*>("x0");
        tokens[2] = const_cast<char*>("x0");
        tokens[3] = const_cast<char*>("0");
        return parseInstructionFrom(tokens, 4, lineNumber);
    }
    //endregion Pseudoinstructions

    //region Corner cases
    else if (strcmp("li", opcode) == 0) {
        // TOUCH NOCOMMIT
        cout << REDC("On li\n");
        // TODO Test
        // Now for the fun bit - the corner case.
        // Source: Slides 53 onwards https://inst.eecs.berkeley.edu/~cs61c/resources/su18_lec/Lecture7.pdf
        // This generates two instructions,
        // One LUI and one ADDI.
        // Not only that, but there's a corner case due to the sign extend of ADDI.

        const int immediate = atoi(tokens[2]);
        const int immediate_31_12 = immediate & (0b11111111111111111111000000000000);
        const int immediate_11_0  = immediate & (0b00000000000000000000111111111111);
        const int immediate_11    = immediate & (0b00000000000000000000100000000000);

        if (immediate_11 == 1) {
            // This is the corner case - the highest bit of the low 12 bits is 1, so the sign extend
            // will cause the ADDI to subtract -1 from the upper 20.
            // Therefore, we add 1 to the upper 20 to counteract this.

            // Do `lui rd (upper_20_bits - 1)`
            tokens[0] = const_cast<char*>("lui");
            tokens[1] = tokens[1]; // Register destination is unchanged
            string upper = to_string(immediate_31_12 - 1);
            tokens[2] = const_cast<char*>(upper.c_str());
            cout << REDC("First inner parsi\n");
            parseInstructionFrom(tokens, 3, lineNumber);

            // Do `addi rd lower_12_bits`
            tokens[0] = const_cast<char*>("addi");
            tokens[1] = tokens[1]; // Register destination is unchanged
            string lower = to_string(immediate_11_0);
            tokens[2] = const_cast<char*>(lower.c_str());


            cout << REDC("Returning with parse from tokens, 3\n");
            return parseInstructionFrom(tokens, 3, lineNumber);
        } else {
            // Do `lui rd upper_20_bits`
            tokens[0] = const_cast<char*>("lui");
            tokens[1] = tokens[1]; // Register destination is unchanged
            string upper = to_string(immediate_31_12);
            tokens[2] = const_cast<char*>(upper.c_str());
            cout << REDC("First inner parsi\n");
            parseInstructionFrom(tokens, 3, lineNumber);

            // Do `addi rd lower_12_bits`
            tokens[0] = const_cast<char*>("addi");
            tokens[1] = tokens[1]; // Register destination is unchanged
            string lower = to_string(immediate_11_0);
            tokens[2] = const_cast<char*>(lower.c_str());

            cout << REDC("Returning with parse from tokens, 3\n");
            return parseInstructionFrom(tokens, 3, lineNumber);
        }
    }
    //endregion Corner cases
    else {
        matchedAnInstruction = false;
    }

    if (matchedAnInstruction) {
        emitInstruction(instruction);
        if (*opts.verbose) {
            printf("%-6s -> 0x%08x \n", opcode, instruction);
        }
        return true;
    } else {
        return false;
    }
}

auto handleLine(char* nextToken, int lineNumber) -> void {
    char* nothing = (char*)"";

    char* tokens[5] = {nothing, nothing, nothing, nothing, nothing};
    size_t tokenCount = 0;

    while(nextToken != nullptr && tokenCount < 5) {
        tokens[tokenCount] = nextToken;
        tokenCount++;

        nextToken = strtok(nullptr, " ,");
    }

    if (*opts.verbose) {
        printf("[%3i]: ", lineNumber);
    }

    bool hasLabel = false;

    if (tokenCount == 0) {
        // Nothing on the line.
        if (*opts.verbose) puts("");
        return;
    }
    if (isComment(tokens[0])) {
        fputs("Comment line starting with > ", stdout);
        puts(tokens[0]);

        return; // Do nothing.
    } else if (isLabel(tokens[0])) {
        char* labelName = tokens[0];
        labelName[strlen(labelName) - 1] = '\0';
        string labelNameStr{labelName};

        hasLabel = true;

        if (tokenCount > 1) {
            // Print out label info.
            printf("(labelled as " GREENC("%s") " -> " YELLOWC("ins index 0%x") ")",
                   labelName, instructionIndex);

            // Remove the label for instruction processing.
            tokens[0] = tokens[1];
            tokens[1] = tokens[2];
            tokens[2] = tokens[3];
            tokens[3] = tokens[4];

            // And continue along.
        } else {
            // It's just a label line.
            printf("Label " GREENC("%s")
                           " -> "
                            YELLOWC("ins index 0x%x") "/" CYANC("line %i") "\n",
                   labelName, instructionIndex, lineNumber);
            return;
        }
    }

    bool didEmitInstruction = parseInstructionFrom(tokens, tokenCount, lineNumber);

    if (!didEmitInstruction) {
        printf( RED "Error:"
                RESET " Failed to match instruction "
                RESET "'" YELLOW "%s" RESET"'.\n", tokens[0]);
        exit(EXIT_FAILURE);
    }
}

auto huntForLabels(char* nextToken, int lineNumber) -> void {
    char* nothing = (char*)"";

    array<char*, 5> tokens = {nothing, nothing, nothing, nothing, nothing};
    size_t tokenCount = 0;

    while(nextToken != nullptr && tokenCount < 5) {
        tokens[tokenCount] = nextToken;
        tokenCount++;

        nextToken = strtok(nullptr, " ,");
    }

    if (tokenCount == 0) {
        // Nothing on the line.
        return;
    }
    if (isComment(tokens[0])) {
        return; // Do nothing.
    } else if (isLabel(tokens[0])) {
        char* labelName = tokens[0];
        labelName[strlen(labelName) - 1] = '\0';
        string labelNameStr = string{labelName};

        labels[labelNameStr] = Label{instructionIndex, lineNumber};

        if (tokenCount > 1) instructionIndex++;
    } else {
        instructionIndex++;
    }
}

} // namespace DCSembler

[[nodiscard]]
auto readFileToString(const string& filePath) -> string {
    if (not filesystem::is_regular_file(filePath)) {
        return {};
    }

    ifstream f{filePath, ios_base::in | ios_base::binary};

    if (not f.is_open()) {
        return {};
    }

    ostringstream ss{};
    ss << f.rdbuf();

    return ss.str();
}

auto main(int argc, char** argv) -> int {
    using namespace DcsEmbler;

    opts = Options::parseFrom(argc, argv);

    string text = readFileToString(*opts.inputFileName);
    istringstream lines(text);
    string line;

    //region{{{ Building labels
    for (int lineNumber = 1; getline(lines, line); lineNumber++){
        char* token = strtok(const_cast<char*>(line.c_str()), " \t,()");
        huntForLabels(token, lineNumber);
    }
    //endregion}}}

    /// Reset the lines stream and the instruction index.
    lines = istringstream{text};
    instructionIndex = 0;

    /// Open output file
    out = fopen(opts.getOutputFileName().c_str(), "w");
    if (not out) {
        cerr << " [Error]: Failed to open output file. Path attempted: '" << opts.getOutputFileName() << "'\n";
        return EXIT_FAILURE;
    }

    //region{{{ Emit instructions
    for (int lineNumber = 1; getline(lines, line); lineNumber++) {
        string currentLine = line; // Make a copy of the string for strtok to use.
        char* token = strtok(const_cast<char*>(currentLine.c_str()), " \t,()");
        handleLine(token, lineNumber);
    }
    //endregion}}}

    labels.prettyPrint();

    return EXIT_SUCCESS;
}
