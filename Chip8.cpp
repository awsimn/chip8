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

    /**
     * HOW THIS ROUTING ENGINE WORKS (Notes for me):
     *
     * 1. An opcode comes in (e.g., 0xF033).
     * 2. The main Cycle() function grabs the first digit ('F') and uses it as an array index.
     * 3. table[0xF] stores the memory address of the TableF() router function.
     * 4. We jump to TableF(). Inside it, we isolate the last two digits ('33').
     * 5. tableF[0x33] stores the memory address of the actual worker function: OP_Fx33().
     *
     * SYNTAX BREAKDOWN: ((*this).*(tableF[index]))();
     * - (*this)       = Get this active emulator object.
     * - .*            = The bridge operator. Connect the object to the function pointer, dereferencing Chip8Func.
     * - tableF[index] = The variable holding the raw memory address of the code.
     * - ()            = Pull the trigger and execute the function.
     */

    // Allocate the routing functions and opcode functions into the correct memory addresses
    table[0x0] = &Chip8::Table0;
    table[0x1] = &Chip8::OP_1nnn;
    table[0x2] = &Chip8::OP_2nnn;
    table[0x3] = &Chip8::OP_3xkk;
    table[0x4] = &Chip8::OP_4xkk;
    table[0x5] = &Chip8::OP_5xy0;
    table[0x6] = &Chip8::OP_6xkk;
    table[0x7] = &Chip8::OP_7xkk;
    table[0x8] = &Chip8::Table8;
    table[0x9] = &Chip8::OP_9xy0;
    table[0xA] = &Chip8::OP_Annn;
    table[0xB] = &Chip8::OP_Bnnn;
    table[0xC] = &Chip8::OP_Cxkk;
    table[0xD] = &Chip8::OP_Dxyn;
    table[0xE] = &Chip8::TableE;
    table[0xF] = &Chip8::TableF;

    // Fill the empty cells with OP_NULL for safety purposes
    for (size_t i = 0; i <= 0xE; i++)
    {
        table0[i], table8[i], tableE[i] = &Chip8::OP_NULL;
    }

    for (size_t i = 0; i < 0x65; i++)
    {
        tableF[i] = &Chip8::OP_NULL;
    }

    table0[0x0] = &Chip8::OP_00E0;
    table0[0xE] = &Chip8::OP_00EE;

    table8[0x0] = &Chip8::OP_8xy0;
    table8[0x1] = &Chip8::OP_8xy1;
    table8[0x2] = &Chip8::OP_8xy2;
    table8[0x3] = &Chip8::OP_8xy3;
    table8[0x4] = &Chip8::OP_8xy4;
    table8[0x5] = &Chip8::OP_8xy5;
    table8[0x6] = &Chip8::OP_8xy6;
    table8[0x7] = &Chip8::OP_8xy7;
    table8[0xE] = &Chip8::OP_8xyE;

    tableE[0x1] = &Chip8::OP_ExA1;
    tableE[0xE] = &Chip8::OP_Ex9E;

    tableF[0x07] = &Chip8::OP_Fx07;
    tableF[0x0A] = &Chip8::OP_Fx0A;
    tableF[0x15] = &Chip8::OP_Fx15;
    tableF[0x18] = &Chip8::OP_Fx18;
    tableF[0x1E] = &Chip8::OP_Fx1E;
    tableF[0x29] = &Chip8::OP_Fx29;
    tableF[0x33] = &Chip8::OP_Fx33;
    tableF[0x55] = &Chip8::OP_Fx55;
    tableF[0x65] = &Chip8::OP_Fx65;
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
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t compareByte = opcode & 0x00FFu;

    if (VRegisters[Vx] == compareByte)
    {
        programCounter += 2;
    }
}

// SNE instruction (Vx, compareByte) - Same masking and bitmasking flow as SE instruction
// If value NOT equal then increment PC
void Chip8::OP_4xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t compareByte = opcode & 0x00FFu;

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
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (VRegisters[Vx] == VRegisters[Vy])
    {
        programCounter += 2;
    }
}

// LD instruction(Vx, updateValue) - Isolate the target VR index and the raw byte value via bitmasking and bitshifting
// Assign the byte value directly into the Vx slot in the VRegisters array
void Chip8::OP_6xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t updateValue = opcode & 0x00FFu;

    VRegisters[Vx] = updateValue;
}

// ADD instruction (Vx, addValue) - Isolate the target VR index and the raw byte value via bitmasking and bitshifting
// Add the byte value to the value inside the VR at index Vx
void Chip8::OP_7xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t addValue = opcode & 0x00FFu;

    VRegisters[Vx] += addValue;
}

// LD instruction(Vx, Vy) - Isolate the target VR indexes(Vx, Vy) via bitmasking and bitshifting
// Assign the Vy value directly into the Vx slot in the VRegisters array
void Chip8::OP_8xy0()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    VRegisters[Vx] = VRegisters[Vy];
}

// OR instruction (Vx, Vy) - Isolate VR indexes via bitmasking and bitshifting
// Use bitwise OR to apply OR on corresponding Vx and Vy bits to TURN THEM ON or LEAVE THEM OFF
void Chip8::OP_8xy1()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    VRegisters[Vx] |= VRegisters[Vy];
}

// AND instruction (Vx, Vy) - Isolate VR indexes via bitmasking and bitshifting
// Use bitwise AND to apply AND on corresponding Vx and Vy bits to LEAVE THEM ON or TURN THEM OFF
void Chip8::OP_8xy2()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    VRegisters[Vx] &= VRegisters[Vy];
}

// XOR instruction (Vx, Vy) - Isolate VR indexes via bitmasking and bitshifting
// Use bitwise XOR to apply AND on corresponding Vx and Vy bits to TURN THEM BOTH OFF or LEAVE ONE ON
void Chip8::OP_8xy3()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    VRegisters[Vx] ^= VRegisters[Vy];
}

// ADD instruction (Vx, Vy) - Isolate VR indexes via bitmasking and bitshifting
// Calculate sum of values inside both registers. Use bitwise AND to convert sum back to 8 bit and store in Vx
// If sum > 255 set register Vf to 1 else 0
void Chip8::OP_8xy4()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

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

// SUB instruction (Vx, Vy) - Isolate VR indexes via bitmasking and bitshifting
// Calculate difference of values inside both registers(Vx - Vy) and set the Vx to the resultant
// If Vx > Vy set register Vf to 1 else 0
void Chip8::OP_8xy5()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (VRegisters[Vx] >= VRegisters[Vy])
    {
        VRegisters[0xF] = 1;
    }
    else
    {
        VRegisters[0xF] = 0;
    }

    uint8_t diff = VRegisters[Vx] - VRegisters[Vy];
    VRegisters[Vx] = diff;
}

// SHR instruction (Vx) - Isolate VR index via bitmasking and bitshifting
// Set Vf to the value of the last bit in VRegister[Vx] via bitmasking
// This sets Vf to either 1 or 0 depending on what the last bit of VRegister[Vx] contained
// Set VRegister[Vx] to new value equal to old value with all bits shifted right once
void Chip8::OP_8xy6()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    VRegisters[0xF] = VRegisters[Vx] & 0x1u;

    VRegisters[Vx] = VRegisters[Vx] >> 1u;
}

// SUBN instruction (Vx, Vy) - Isolate VR indexes via bitmasking and bitshifting
// Calculate difference of values inside both registers(Vy - Vx) and set the Vx to the resultant
// If Vy > Vx set register Vf to 1 else 0
void Chip8::OP_8xy7()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (VRegisters[Vy] >= VRegisters[Vx])
    {
        VRegisters[0xF] = 1;
    }
    else
    {
        VRegisters[0xF] = 0;
    }

    uint8_t diff = VRegisters[Vy] - VRegisters[Vx];
    VRegisters[Vx] = diff;
}

// SHL instruction (Vx) - Isolate VR index via bitmasking and bitshifting
// Get the first bit of VRegisters[Vx] through bitmasking (0x80u = 1000 0000) and shift down by seven to reach end
// This sets Vf to either 1 or 0 depending on what the first bit of VRegister[Vx] contained
// Set VRegister[Vx] to new value equal to old value with all bits shifted left once
void Chip8::OP_8xyE()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    VRegisters[0xF] = (VRegisters[Vx] & 0x80u) >> 7u;

    VRegisters[Vx] = VRegisters[Vx] << 1u;
}

// SNE (Vx, Vy) - Isolate VR indexes via bitmasking and bitshifting
// If VRegister values for Vx and Vy are not the same, double increment PC
void Chip8::OP_9xy0()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0X00F0u) >> 4u;

    if (VRegisters[Vx] != VRegisters[Vy])
    {
        programCounter += 2;
    }
}

// LD (I, addr) - Isolate address via bitmasking
// Set indexRegister to point to that address
void Chip8::OP_Annn()
{
    uint16_t address = opcode & 0x0FFFu;

    indexRegister = address;
}

// JP (V0, addr) - Isolate address via bitmasking
// Set programCounter to resultant address of sum of value in V0 and isolated address
void Chip8::OP_Bnnn()
{
    uint16_t address = opcode & 0x0FFFu;

    programCounter = VRegisters[0] + address;
}

// RND (Vx, value) - Isolate Vx address via bitmasking and bitshifting
// Isolate value of kk(value) via bitmasking
// Set VRegister number Vx to the value of randomly generated byte masked by value(kk)
void Chip8::OP_Cxkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value = opcode && 0x00FFu;

    VRegisters[Vx] = randomByte(randomGenerator) & value;
}

// DRAW instruction - (Vx, Vy, height)
void Chip8::OP_Dxyn()
{
    // Get Vx, Vy and height values via bitmasking and bitshifting(where needed)
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    uint8_t height = (opcode & 0x000Fu);

    // Wrap graphics around screen upon overflow via modulus operator
    uint8_t xPosition = VRegisters[Vx] % VIDEO_WIDTH;
    uint8_t yPosition = VRegisters[Vy] % VIDEO_HEIGHT;

    // Set collisions to off initially
    VRegisters[0xF] = 0;

    // Outer loop controls the row of the sprite being drawn
    for (unsigned int row = 0; row < height; row++)
    {
        // Get the sprite's location in memory from the indexRegister and the relevant section being drawn from row number
        uint8_t spriteByte = memory[indexRegister + row];

        // Inner loop controls the column of the sprite currently being drawn
        for (unsigned int col = 0; col < 8; col++)
        {
            // Checks each column meaning each pixel of the sprite seperately
            uint8_t spritePixel = spriteByte & (0x80u >> col);
            // Stores the physical memory address of the video array store on the PC
            // that is relevant to the section of the Chip8 screen being rendered
            // Basically gets the pixel on the screen that corresponds to the location of that pixel in-game
            uint32_t *screenPixel = &video[(yPosition + row) * VIDEO_WIDTH + (xPosition + col)];

            // If the current pixel is active/being drawn check for collisions
            if (spritePixel)
            {
                // Check if pixel is already white(means collision has occured)
                if (*screenPixel == 0xFFFFFFFF)
                {
                    VRegisters[0xF] = 1;
                }

                // Trigger an XOR to toggle white to black(or active to inactive) and vice versa
                *screenPixel ^= 0xFFFFFFFF;
            }
        }
    }
}

// SKP instruction - (Vx)
void Chip8::OP_Ex9E()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t keyPressed = VRegisters[Vx];

    // If the key that is pressed has the same decimal value as the number inside Vx then skip next instruction
    if (keyboard[keyPressed])
    {
        programCounter += 2;
    }
}

// SKNP instruction - (Vx)
void Chip8::OP_ExA1()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t keyPressed = VRegisters[Vx];

    // If the key that is pressed does NOT have the same decimal value as the number inside Vx then skip next instruction
    if (!keyboard[keyPressed])
    {
        programCounter += 2;
    }
}

// LD (Vx, DT)
// Set VRegister number Vx value to the value of the delay timer
void Chip8::OP_Fx07()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    VRegisters[Vx] = delayTimer;
}

// LD (Vx, keyboard)
void Chip8::OP_Fx0A()
{
    // Get Vx via bitmasking and bitshifting
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Loop through each keyboard index to check if the key was pressed
    for (unsigned int i = 0; i < sizeof(keyboard); i++)
    {
        // If a key was pressed, store the value of that key into the VRegister corresponding to Vx and return
        if (keyboard[i])
        {
            VRegisters[Vx] = i;
            return;
        }
    }
    // If a key was not pressed, wait till key is pressed by repeating previous instruction in a loop
    programCounter -= 2;
}

// LD (delayTimer, Vx)
// Set delayTimer value to value of VRegister corresponding to Vx
void Chip8::OP_Fx15()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    delayTimer = VRegisters[Vx];
}

// LD (soundTimer, Vx)
// Set soundTimer value to value of VRegister corresponding to Vx
void Chip8::OP_Fx18()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    soundTimer = VRegisters[Vx];
}

// ADD (indexRegister, Vx)
// Set the value of indexRegister to the sum of indexRegister and VRegister[Vx]
void Chip8::OP_Fx1E()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    indexRegister = indexRegister + VRegisters[Vx];
}

// LD (F, Vx)
void Chip8::OP_Fx29()
{
    // Get Vx via bitmasking and bitshifting
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Store the corresponding VRegister value to Vx inside
    uint8_t spriteDigit = VRegisters[Vx];

    // Store the value of the corresponding digit inside indexRegister
    // while accounting for the fact that a fontset character is 5 bytes long so every digit takes up 5 addresses
    indexRegister = FONTSET_START_ADDRESS + (5 * spriteDigit);
}

// LD (B, Vx)
void Chip8::OP_Fx33()
{
    // Get Vx via bitmasking and bitshifting
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t number = VRegisters[Vx];

    // Get needed part of the number(upto 255), use module to extract and store part in appropriate memory address
    // Divide by 10 to get a floating point number that is automatically truncated
    // Repeat process for each part of the number(unit, ten, hundred)
    memory[indexRegister + 2] = number % 10;
    number = number / 10;

    memory[indexRegister + 1] = number % 10;
    number = number / 10;

    memory[indexRegister] = number;
}

// LD ([I], Vx)
void Chip8::OP_Fx55()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Store Vregisters ranging from V0 to Vx in memory at address starting at indexRegister
    for (uint8_t i = 0; i <= Vx; i++)
    {
        memory[indexRegister + i] = VRegisters[i];
    }
}

// LD (Vx, [I])
void Chip8::OP_Fx65()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Load values from memory starting at indexRegister into registers V0 through Vx inclusive
    for (uint8_t i = 0; i <= Vx; i++)
    {
        VRegisters[i] = memory[indexRegister + i];
    }
}

//-------------------------------ROUTER FUNCTIONS-------------------------------------------------
// Do nothing
void Chip8::OP_NULL()
{
}

void Chip8::Table0()
{
    ((*this).*(table0[opcode & 0x000Fu]))();
}

void Chip8::Table8()
{
    ((*this).*(table8[opcode & 0x000Fu]))();
}

void Chip8::TableE()
{
    ((*this).*(tableE[opcode & 0x000Fu]))();
}

void Chip8::TableF()
{
    ((*this).*(tableF[opcode & 0x00FFu]))();
}