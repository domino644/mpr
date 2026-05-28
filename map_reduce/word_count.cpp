#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <chrono>

inline uint64_t hash_fast(const char *str, size_t len) {
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < len; ++i) {
        hash ^= str[i];
        hash *= 0x00000100000001B3;
    }
    return hash;
}

struct WordSlot {
    const char *start = nullptr;
    uint32_t len = 0;
    uint32_t count = 0;
};

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

alignas(64) constexpr CharLut is_word_char = generate_word_lut();

void word_count_ultra(const char *data, size_t size) {
    const size_t MAP_SIZE = 1 << 22;
    const size_t MASK = MAP_SIZE - 1;
    std::vector<WordSlot> hash_map(MAP_SIZE);

    const char *ptr = data;
    const char *end = data + size;
    size_t unique_words = 0;

    while (ptr < end) {
        while (ptr < end && !is_word_char[static_cast<unsigned char>(*ptr)]) {
            ptr++;
        }
        if (ptr >= end) break;

        const char *word_start = ptr;
        while (ptr < end && is_word_char[static_cast<unsigned char>(*ptr)]) {
            ptr++;
        }
        size_t word_len = ptr - word_start;

        uint64_t hash = hash_fast(word_start, word_len);
        size_t idx = hash & MASK;

        while (true) {
            if (hash_map[idx].count == 0) {
                hash_map[idx] = { word_start, static_cast<uint32_t>(word_len), 1 };
                unique_words++;
                break;
            }
            if (hash_map[idx].len == word_len &&
                std::memcmp(hash_map[idx].start, word_start, word_len) == 0) {
                hash_map[idx].count++;
                break;
            }
            idx = (idx + 1) & MASK;
        }
    }
    std::cout << "Przetwarzanie zakonczone. Unikalnych slow: " << unique_words << "\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Uzycie: " << argv << " <sciezka_do_pliku>\n";
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        std::cerr << "Nie mozna otworzyc pliku.\n";
        return 1;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        close(fd);
        return 1;
    }
    size_t length = sb.st_size;

    if (length == 0) {
        std::cout << "Plik jest pusty.\n";
        close(fd);
        return 0;
    }

    void *addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        std::cerr << "Blad mmap.\n";
        close(fd);
        return 1;
    }

    const char *data = static_cast<const char *>(addr);

    auto start = std::chrono::high_resolution_clock::now();

    word_count_ultra(data, length);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    std::cout << "Czas wykonania: " << duration.count() << " ms\n";

    munmap(addr, length);
    close(fd);
    return 0;
}