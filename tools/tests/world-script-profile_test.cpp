#define WORLD_SCRIPT_PROFILE_TEST
#include "../check-world-script-profile.cpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr std::size_t kImageSize = 2048 * 128;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "world-script-profile-test: " << message << '\n';
        std::exit(1);
    }
}

bool accepts(const std::vector<uint8_t>& bytes,
             const std::vector<uint8_t>& text = {0, 0},
             std::string* diagnostic = nullptr) {
    std::string error;
    const bool result = validateWorldScriptProfile(bytes, text, error);
    if (diagnostic) *diagnostic = error;
    return result;
}

void put(std::vector<uint8_t>& image, std::size_t slot,
         const std::vector<uint8_t>& script) {
    require(script.size() <= 128, "fixture exceeds one 128-byte slot");
    std::copy(script.begin(), script.end(), image.begin() + slot * 128);
}

void put(std::vector<uint8_t>& image, std::size_t slot,
         std::initializer_list<uint8_t> script) {
    put(image, slot, std::vector<uint8_t>(script));
}
}

int main() {
    std::vector<uint8_t> image(kImageSize, 0);
    put(image, 0, {static_cast<uint8_t>(VmOpcode::If), 1, 2, 3, 4, 5,
                   static_cast<uint8_t>(VmOpcode::End)});
    put(image, 32, {static_cast<uint8_t>(VmOpcode::TpIf), 1, 2, 3, 4, 5, 6, 7, 8,
                    static_cast<uint8_t>(VmOpcode::End)});
    put(image, 33, {static_cast<uint8_t>(VmOpcode::End)});
    require(accepts(image), "canonical three-script image rejected");
    require(accepts(std::vector<uint8_t>(kImageSize, 0)), "all-zero image rejected");

    image.assign(kImageSize, 0);
    put(image, 1, {static_cast<uint8_t>(VmOpcode::If), 0xff, 0xff, 0xff, 0xff, 0xff,
                   static_cast<uint8_t>(VmOpcode::End)});
    require(accepts(image), "0xFF operands treated as commands");

    const uint8_t unsupported[] = {
        static_cast<uint8_t>(VmOpcode::Msg), static_cast<uint8_t>(VmOpcode::TMsg),
        static_cast<uint8_t>(VmOpcode::SMsg), static_cast<uint8_t>(VmOpcode::Tp),
        static_cast<uint8_t>(VmOpcode::SetFlag), static_cast<uint8_t>(VmOpcode::UnsetFlag),
        static_cast<uint8_t>(VmOpcode::ReadFlag), 42
    };
    for (uint8_t opcode : unsupported) {
        image.assign(kImageSize, 0);
        put(image, 7, {opcode, static_cast<uint8_t>(VmOpcode::End)});
        std::string diagnostic;
        require(!accepts(image, {0, 0}, &diagnostic), "unsupported opcode accepted");
        require(diagnostic.find("slot 7 offset 0") != std::string::npos,
                "unsupported opcode diagnostic lacks slot/offset");
        require(diagnostic.find("-DCGFX_FULL_WORLD_VM") != std::string::npos,
                "unsupported opcode diagnostic lacks full-VM remedy");
    }

    image.assign(kImageSize, 0);
    std::vector<uint8_t> noEnd(21 * 6, static_cast<uint8_t>(VmOpcode::If));
    for (std::size_t i = 0; i < 21; ++i) {
        std::fill(noEnd.begin() + i * 6 + 1,
                  noEnd.begin() + i * 6 + 6, 1);
    }
    noEnd.push_back(static_cast<uint8_t>(VmOpcode::TpIf));
    noEnd.push_back(1);
    require(noEnd.size() == 128, "missing-End fixture must fill exactly one slot");
    put(image, 9, noEnd);
    std::string diagnostic;
    require(!accepts(image, {0, 0}, &diagnostic), "missing-End/truncated slot accepted");
    require(diagnostic.find("slot 9 offset 126") != std::string::npos,
            "truncation diagnostic lacks slot/offset");
    require(!accepts(std::vector<uint8_t>(kImageSize - 1, 0)), "short image accepted");
    image.push_back(0);
    require(!accepts(image), "long image accepted");

    image.assign(kImageSize, 0);
    require(accepts(image, {0, 0}), "two-zero-byte text framing rejected");
    for (const std::vector<uint8_t>& text : {
             std::vector<uint8_t>{}, std::vector<uint8_t>{0},
             std::vector<uint8_t>{0, 1}, std::vector<uint8_t>{0, 0, 0}}) {
        require(!accepts(image, text, &diagnostic), "invalid text framing accepted");
        require(diagnostic.find("text.bin") != std::string::npos,
                "text framing diagnostic omits text.bin");
        require(diagnostic.find("-DCGFX_FULL_WORLD_VM") != std::string::npos,
                "text framing diagnostic lacks full-VM remedy");
    }

    std::cout << "world-script-profile-test: profile, opcode, size, and text framing cases passed\n";
}
