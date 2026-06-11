#include "Chip8.h"
#include <iostream>
#include <fstream>
#include <cstring>

// Start address of the ROM file storage section of memory
const unsigned int START_ADDRESS = 0x200;

const unsigned int FONTSET_SIZE = 80;
// Start address of the fontset section of memory
const unsigned int FONTSET_START_ADDRESS = 0x50;
uint8_t fontset[FONTSET_SIZE] =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8() : randomGenerator(std::chrono::system_clock::now().time_since_epoch().count())
{
    programCounter = START_ADDRESS;

    // Upon initializing the class load the fontset into memory
    for (unsigned int i = 0; i < FONTSET_SIZE; i++)
    {
        memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }
}

void Chip8::LoadROM(char const *filename)
{
    // Open an ifstream to allow ROM file to be read as a binary file and point to the end of the file(ate - AT THE END)
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (file.is_open())
    {
        // Get size of the ROM file and create a buffer based on ROM file's size
        std::streampos size = file.tellg();
        char *buffer = new char[size];

        // Go back to the beginning of the file and read the whole file into the newly created buffer
        file.seekg(std::ios::beg);
        file.read(buffer, size);
        file.close();

        // Copy the ROM data from the temporary buffer into the Chip8's virtual memory array
        for (long i = 0; i < size; i++)
        {
            memory[START_ADDRESS + i] = buffer[i];
        }

        // Free allocated heap memory for the buffer and ensure no dangling pointers are left
        delete[] buffer;
        buffer = nullptr;
    }
    else
    {
        std::cout << "File failed to open.";
    }
}

// CLS - Clear the video array(representing the emulator display) and set all values to 0 to represent an empty screen
void Chip8::OP_00E0()
{
    std::memset(video, 0, sizeof(video));
}

// RET instruction - return the program counter(PC) to the memory address at the end of the stack (LIFO)
void Chip8::OP_00EE()
{
    stackPointer -= 1;
    programCounter = stack[stackPointer];
}

// JP  instruction - Use bitmasking to get the memory address emulator needs to jump to
// Set the PC to jump to that address
void Chip8::OP_1nnn()
{
    uint16_t jumpAddress = opcode & 0x0FFFu;
    programCounter = jumpAddress;
}

// CALL instruction - Set the current stack slot to the return address (next instruction) to save it
// Increment stack pointer(SP) to get ready for next CALL
// Use bitmasking to isolate address PC needs to jump to and set PC to that address
void Chip8::OP_2nnn()
{
    stack[stackPointer] = programCounter;
    stackPointer += 1;
    uint16_t callAddress = opcode & 0x0FFFu;
    programCounter = callAddress;
}

// SE instruction (Vx, compareByte) - Use bitmasking + bitshifting to isolate Vx and normalise value to number of VRs(16)
// Use bitmasking again to isolate comparison value against value inside VR
// If value equal then increment PC
void Chip8::OP_3xkk()
{
    uint16_t Vx = (opcode & 0x0F00u) >> 8u;
    uint16_t compareByte = opcode & 0x00FFu;

    if (VRegisters[Vx] == compareByte)
    {
        programCounter += 2;
    }
}

// SNE instruction (Vx, compareByte) - Same masking and bitmasking flow as SE instruction
// If value NOT equal then increment PC
void Chip8::OP_4xkk()
{
    uint16_t Vx = (opcode & 0x0F00u) >> 8u;
    uint16_t compareByte = opcode & 0x00FFu;

    if (VRegisters[Vx] != compareByte)
    {
        programCounter += 2;
    }
}

// SE instruction (Vx, Vy) - Same masking and bitmasking flow as SE instruction but on two different parts of the opcode
// Opcode x value is situated in second nibble so needs 8u shift. Opcode y value is third nibble so needs 4u shift
// If values equal then increment PC
void Chip8::OP_5xy0()
{
    uint16_t Vx = (opcode & 0x0F00u) >> 8u;
    uint16_t Vy = (opcode & 0x00F0u) >> 4u;

    if (VRegisters[Vx] == VRegisters[Vy])
    {
        programCounter += 2;
    }
}

// LD instruction(Vx, updateValue) - Isolate the target VR index and the raw byte value via bitmasking and bitshifting
// Assign the byte value directly into the Vx slot in the VRegisters array
void Chip8::OP_6xkk()
{
    uint16_t Vx = (opcode & 0x0F00u) >> 8u;
    uint16_t updateValue = opcode & 0x00FFu;

    VRegisters[Vx] = updateValue;
}

// ADD instruction (Vx, addValue) - Isolate the target VR index and the raw byte value via bitmasking and bitshifting
// Add the byte value to the value inside the VR at index Vx
void Chip8::OP_7xkk()
{
    uint16_t Vx = (opcode & 0x0F00u) >> 8u;
    uint16_t addValue = opcode & 0x00FFu;

    VRegisters[Vx] += addValue;
}

// LD instruction(Vx, Vy) - Isolate the target VR indexes(Vx, Vy) via bitmasking and bitshifting
// Assign the Vy value directly into the Vx slot in the VRegisters array
void Chip8::OP_8xy0()
{
    uint16_t Vx = (opcode & 0x0F00u) >> 8u;
    uint16_t Vy = (opcode & 0x00F0u) >> 4u;

    VRegisters[Vx] = VRegisters[Vy];
}

// OR instruction (Vx, Vy) - Isolate VR indexes via bitmasking and bitshifting
// Use bitwise OR to apply OR on corresponding Vx and Vy bits to TURN THEM ON or LEAVE THEM OFF
void Chip8::OP_8xy1()
{
    uint16_t Vx = (opcode & 0x0F00u) >> 8u;
    uint16_t Vy = (opcode & 0x00F0u) >> 4u;

    VRegisters[Vx] |= VRegisters[Vy];
}

// AND instruction (Vx, Vy) - Isolate VR indexes via bitmasking and bitshifting
// Use bitwise AND to apply AND on corresponding Vx and Vy bits to LEAVE THEM ON or TURN THEM OFF
void Chip8::OP_8xy2()
{
    uint16_t Vx = (opcode & 0x0F00u) >> 8u;
    uint16_t Vy = (opcode & 0x00F0u) >> 4u;

    VRegisters[Vx] &= VRegisters[Vy];
}

// XOR instruction (Vx, Vy) - Isolate VR indexes via bitmasking and bitshifting
// Use bitwise XOR to apply AND on corresponding Vx and Vy bits to TURN THEM BOTH OFF or LEAVE ONE ON
void Chip8::OP_8xy3()
{
    uint16_t Vx = (opcode & 0x0F00u) >> 8u;
    uint16_t Vy = (opcode & 0x00F0u) >> 4u;

    VRegisters[Vx] ^= VRegisters[Vy];
}

// ADD instruction (Vx, Vy) - Isolate VR indexes via bitmasking and bitshifting
// Calculate sum of values inside both registers. Use bitwise AND to convert sum back to 8 bit and store in Vx
//If sum > 255 set register Vf to 1 else 0
void Chip8::OP_8xy4()
{
    uint16_t Vx = (opcode & 0x0F00u) >> 8u;
    uint16_t Vy = (opcode & 0x00F0u) >> 4u;

    uint16_t total = VRegisters[Vx] + VRegisters[Vy];

    VRegisters[Vx] = total & 0xFFu;

    if (total > 255U)
    {
        VRegisters[0xF] = 1;
    }
    else
    {
        VRegisters[0xF] = 0;
    }

}