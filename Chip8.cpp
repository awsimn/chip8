#include "Chip8.h"
#include <iostream>
#include <fstream>
#include <cstring>

const unsigned int START_ADDRESS = 0x200;

const unsigned int FONTSET_SIZE = 80;
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

    for (unsigned int i = 0; i < FONTSET_SIZE; i++)
    {
        memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }
}

void Chip8::LoadROM(char const *filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (file.is_open())
    {
        std::streampos size = file.tellg();
        char *buffer = new char[size];

        file.seekg(std::ios::beg);
        file.read(buffer, size);
        file.close();

        for (long i = 0; i < size; i++)
        {
            memory[START_ADDRESS + i] = buffer[i];
        }

        delete[] buffer;
        buffer = nullptr;
    }
    else
    {
        std::cout << "File failed to open.";
    }
}

void Chip8::OP_00E0()
{
    std::memset(video, 0, sizeof(video));
}

void Chip8::OP_00EE()
{
    stackPointer -= 1;
    programCounter = stack[stackPointer];
}

void Chip8::OP_1nnn()
{
    uint16_t jumpAddress = opcode & 0x0FFFu;
    programCounter = jumpAddress;
}