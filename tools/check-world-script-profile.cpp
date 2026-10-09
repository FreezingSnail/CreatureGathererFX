#include "../src/vm/opcodes.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
constexpr std::size_t kSlotCount = 2048;
constexpr std::size_t kSlotSize = 128;
constexpr char kFullVmRemedy[] =
    "; compile with -DCGFX_FULL_WORLD_VM to allow full VM content";

bool validateWorldScriptProfile(const std::vector<uint8_t>& image,
                                const std::vector<uint8_t>& text,
                                std::string& error) {
    if (image.size() != kSlotCount * kSlotSize) {
        error = "scripts.bin has " + std::to_string(image.size()) +
                " bytes; expected exactly " +
                std::to_string(kSlotCount * kSlotSize) + kFullVmRemedy;
        return false;
    }
    if (text.size() != 2 || text[0] != 0 || text[1] != 0) {
        error = "text.bin must contain exactly two zero bytes for empty script text" +
                std::string(kFullVmRemedy);
        return false;
    }

    for (std::size_t slot = 0; slot < kSlotCount; ++slot) {
        const std::size_t base = slot * kSlotSize;
        bool populated = false;
        for (std::size_t i = 0; i < kSlotSize; ++i) {
            if (image[base + i] != 0) {
                populated = true;
                break;
            }
        }
        if (!populated) continue;

        std::size_t offset = 0;
        bool ended = false;
        while (offset < kSlotSize) {
            const std::size_t commandOffset = offset;
            const VmOpcode opcode = static_cast<VmOpcode>(image[base + offset++]);
            std::size_t operandBytes = 0;
            switch (opcode) {
            case VmOpcode::If:
                operandBytes = 5;
                break;
            case VmOpcode::TpIf:
                operandBytes = 8;
                break;
            case VmOpcode::End:
                ended = true;
                break;
            default:
                error = "slot " + std::to_string(slot) + " offset " +
                        std::to_string(commandOffset) +
                        ": unsupported opcode " +
                        std::to_string(static_cast<unsigned>(image[base + commandOffset])) +
                        kFullVmRemedy;
                return false;
            }
            if (ended) break;
            if (operandBytes > kSlotSize - offset) {
                error = "slot " + std::to_string(slot) + " offset " +
                        std::to_string(commandOffset) +
                        ": truncated operands" + std::string(kFullVmRemedy);
                return false;
            }
            offset += operandBytes;
        }
        if (!ended) {
            error = "slot " + std::to_string(slot) + " offset " +
                    std::to_string(kSlotSize) + ": missing End" +
                    kFullVmRemedy;
            return false;
        }
    }
    return true;
}

bool readImage(const std::string& path, std::vector<uint8_t>& image) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::cerr << "world-script-profile: cannot open " << path << '\n';
        return false;
    }
    image.assign(std::istreambuf_iterator<char>(input),
                 std::istreambuf_iterator<char>());
    return true;
}
} // namespace

#ifndef WORLD_SCRIPT_PROFILE_TEST
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: check-world-script-profile <scripts.bin> <text.bin>\n";
        return 2;
    }
    std::vector<uint8_t> image, text;
    if (!readImage(argv[1], image) || !readImage(argv[2], text)) return 2;
    std::string error;
    if (!validateWorldScriptProfile(image, text, error)) {
        std::cerr << "world-script-profile: " << error << '\n';
        return 1;
    }
    std::cout << "world-script-profile: " << kSlotCount
              << " slots, compact If/TpIf/End profile valid\n";
    return 0;
}
#endif
