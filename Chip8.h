#include <cstdint>
#include <fstream>
#include <chrono>
#include <random>

const unsigned int START_ADDRESS = 0x200;

class Chip8
{
public:
    uint8_t VRegisters[16]{};
    uint16_t indexRegister{};
    uint16_t programCounter{};
    uint8_t memory[4096]{};
    uint16_t stack[16]{};
    uint8_t stackPointer{};
    uint8_t delayTimer{};
    uint8_t soundTimer{};
    uint8_t keyboard[16]{};
    uint32_t video[64 * 32]{};
    uint16_t opcode;

    void LoadROM(char const* filename);
    void OP_00E0();
    void OP_00EE();
    void OP_1nnn();

private:
    std::default_random_engine randomGenerator;
    std::uniform_int_distribution <uint8_t> randomByte {0, 255};
};