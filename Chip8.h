#include <cstdint>
#include <fstream>
#include <chrono>
#include <random>

const unsigned int START_ADDRESS = 0x200;

class Chip8
{
public:
    uint8_t VRegisters[16]{}; // 16 8 bit V registers that hold values to undergo computation during emulator execution
    uint16_t indexRegister{}; // 16 bit index register that can hold resource files
    uint16_t programCounter{};// 16 bit program counter that holds the value of current ROM instruction
    uint8_t memory[4096]{};   // 4KB virtual memory segmented into 3 sections(Interpreter, Fontset, ROM)
    uint16_t stack[16]{};     // 16 level stack that helps keep track of CALL, RET instructions
    uint8_t stackPointer{};   // Stack pointer that helps store and retrieve memory addresses for CALL,RET
    uint8_t delayTimer{};
    uint8_t soundTimer{};
    uint8_t keyboard[16]{};
    uint32_t video[64 * 32]{};
    uint16_t opcode;

    void LoadROM(char const* filename);
    void OP_00E0();
    void OP_00EE();
    void OP_1nnn();
    void OP_2nnn();
    void OP_3xkk();
    void OP_4xkk();
    void OP_5xy0();
    void OP_6xkk();
    void OP_7xkk();
    void OP_8xy0();

private:
    std::default_random_engine randomGenerator;
    std::uniform_int_distribution <uint8_t> randomByte {0, 255};
};