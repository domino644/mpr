#include <iostream>
#include <string>

struct CharLut {
    bool data[256]{};
    constexpr bool operator[](size_t idx) const { return data[idx]; }
};

constexpr CharLut generate_word_lut() {
    CharLut lut{};
    for (int i = 0; i < 256; ++i) {
        if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || (i >= '0' && i <= '9')) {
            lut.data[i] = true;
        }
    }
    return lut;
}

alignas(64) const CharLut is_word_char = generate_word_lut();

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    std::string line;
    while (std::getline(std::cin, line)) {
        const char *ptr = line.data();
        const char *end = ptr + line.size();

        while (ptr < end) {
            while (ptr < end && !is_word_char[static_cast<unsigned char>(*ptr)]) ptr++;
            if (ptr >= end) break;

            const char *word_start = ptr;
            while (ptr < end && is_word_char[static_cast<unsigned char>(*ptr)]) ptr++;

            std::cout.write(word_start, ptr - word_start);
            std::cout << "\t1\n";
        }
    }
    return 0;
}