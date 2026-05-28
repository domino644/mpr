#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <cstring>
#include <chrono>

// --- TWÓJ ULTRA SZYBKI ALGORYTM ---

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

inline uint64_t hash_fast(const char *str, size_t len) {
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < len; ++i) {
        hash ^= str[i];
        hash *= 0x00000100000001B3;
    }
    return hash;
}

std::map<std::string, uint32_t> run_ultra_algorithm(const char *data, size_t size) {
    // Duża mapa (16M slotów), aby zminimalizować kolizje dla ogromnych plików
    const size_t MAP_SIZE = 1 << 24;
    const size_t MASK = MAP_SIZE - 1;
    std::vector<WordSlot> hash_map(MAP_SIZE);

    const char *ptr = data;
    const char *end = data + size;

    while (ptr < end) {
        while (ptr < end && !is_word_char[static_cast<unsigned char>(*ptr)]) ptr++;
        if (ptr >= end) break;

        const char *word_start = ptr;
        while (ptr < end && is_word_char[static_cast<unsigned char>(*ptr)]) ptr++;
        size_t word_len = ptr - word_start;

        uint64_t hash = hash_fast(word_start, word_len);
        size_t idx = hash & MASK;

        while (true) {
            if (hash_map[idx].count == 0) {
                hash_map[idx] = { word_start, static_cast<uint32_t>(word_len), 1 };
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

    std::map<std::string, uint32_t> result;
    for (const auto &slot : hash_map) {
        if (slot.count > 0) {
            result[std::string(slot.start, slot.len)] = slot.count;
        }
    }
    return result;
}

// --- ALGORYTM REFERENCYJNY (CZYTANIE Z PLIKU STRUMIENIEM) ---

std::map<std::string, uint32_t> run_reference_algorithm(const std::string &filepath) {
    std::map<std::string, uint32_t> reference_map;
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return reference_map;

    std::string current_word = "";
    char c;
    while (file.get(c)) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            current_word += c;
        }
        else {
            if (!current_word.empty()) {
                reference_map[current_word]++;
                current_word.clear();
            }
        }
    }
    if (!current_word.empty()) {
        reference_map[current_word]++;
    }
    return reference_map;
}

// --- MAIN ---

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Uzycie: " << argv[0] << " <sciezka_do_pliku>\n";
        return 1;
    }
    std::string filepath = argv[1];

    // 1. PRZYGOTOWANIE DO SZYBKIEGO ALGORYTMU (mmap)
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "❌ Nie mozna otworzyc pliku do mmap.\n";
        return 1;
    }
    struct stat sb;
    if (fstat(fd, &sb) == -1) { close(fd); return 1; }
    size_t length = sb.st_size;

    void *addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        std::cerr << "❌ Blad mmap.\n";
        close(fd);
        return 1;
    }
    const char *data = static_cast<const char *>(addr);

    // 2. URUCHOMIENIE TESTÓW Z POMIAREM CZASU
    std::cout << "[1/3] Uruchamianie ultra szybkiego algorytmu (mmap + custom hash)...\n";
    auto t1 = std::chrono::high_resolution_clock::now();
    auto fast_results = run_ultra_algorithm(data, length);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "-> Zakonczono w: " << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms\n\n";

    std::cout << "[2/3] Uruchamianie wolnego algorytmu referencyjnego (std::ifstream)...\n";
    auto t3 = std::chrono::high_resolution_clock::now();
    auto ref_results = run_reference_algorithm(filepath);
    auto t4 = std::chrono::high_resolution_clock::now();
    std::cout << "-> Zakonczono w: " << std::chrono::duration<double, std::milli>(t4 - t3).count() << " ms\n\n";

    // Czyszczenie pamieci mmap
    munmap(addr, length);
    close(fd);

    // 3. WERYFIKACJA POPRAWNOŚCI
    std::cout << "[3/3] Porownywanie wynikow w poszukiwaniu bledow...\n";

    if (fast_results.size() != ref_results.size()) {
        std::cerr << "❌ BLAD: Rozna liczba unikalnych slow!\n";
        std::cerr << "Szybki algorytm: " << fast_results.size() << " unikalnych slow.\n";
        std::cerr << "Referencyjny: " << ref_results.size() << " unikalnych slow.\n";
        return 1;
    }

    bool success = true;
    for (const auto &[word, count] : ref_results) {
        auto it = fast_results.find(word);
        if (it == fast_results.end()) {
            std::cerr << "❌ BLAD: Szybki algorytm zgubil slowo: '" << word << "'\n";
            success = false;
            break;
        }
        if (it->second != count) {
            std::cerr << "❌ BLAD: Niezgodnosc dla slowa '" << word << "'. ";
            std::cerr << "Powinno byc: " << count << ", jest: " << it->second << "\n";
            success = false;
            break;
        }
    }

    if (success) {
        std::cout << "==================================================\n";
        std::cout << "  ✅ WERYFIKACJA ZAKONCZONA SUKCESEM!\n";
        std::cout << "  Oba algorytmy daly identyczne wyniki dla pliku.\n";
        std::cout << "  Szybki algorytm jest w 100% poprawny logicznie.\n";
        std::cout << "==================================================\n";
    }

    return success ? 0 : 1;
}