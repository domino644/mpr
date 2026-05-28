#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <cstring>
#include <random>
#include <algorithm>

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

// Zwraca wyniki jako standardową mapę do łatwego porównania
std::map<std::string, uint32_t> run_ultra_algorithm(const char *data, size_t size) {
    const size_t MAP_SIZE = 1 << 16; // Mniejsza mapa na potrzeby testu
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

    // Przepakuj wyniki do std::map w celu weryfikacji
    std::map<std::string, uint32_t> result;
    for (const auto &slot : hash_map) {
        if (slot.count > 0) {
            result[std::string(slot.start, slot.len)] = slot.count;
        }
    }
    return result;
}

// --- STANDARDOWY, GEOMEDIALNY ALGORYTM REFERENCYJNY ---

std::map<std::string, uint32_t> run_reference_algorithm(const std::string &text) {
    std::map<std::string, uint32_t> reference_map;
    std::string current_word = "";

    for (char c : text) {
        // Ta sama logika podziału na słowa
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

// --- GENERATOR TESTOWYCH DANYCH ---

std::string generate_test_data(size_t num_words) {
    std::vector<std::string> vocabulary = { "kot", "pies", "ptak", "program", "algorytm", "fast", "slow", "c++", "123", "test" };
    std::vector<char> separators = { ' ', ' ', '\n', '\t', ',', '.', '!', '-', '/' };

    std::mt19937 rng(42); // Stały seed dla powtarzalności testów
    std::uniform_int_distribution<size_t> word_dist(0, vocabulary.size() - 1);
    std::uniform_int_distribution<size_t> sep_dist(0, separators.size() - 1);

    std::string test_text;
    for (size_t i = 0; i < num_words; ++i) {
        test_text += vocabulary[word_dist(rng)];
        test_text += separators[sep_dist(rng)];
    }
    return test_text;
}

// --- FUNKCJA GŁÓWNA (PORÓWNANIE) ---

int main() {
    std::cout << "[1/3] Generowanie losowego tekstu testowego...\n";
    std::string test_text = generate_test_data(50000); // 50 tysięcy słów
    std::cout << "Wygenerowano tekst o wielkosci: " << test_text.size() << " bajtow.\n\n";

    std::cout << "[2/3] Uruchamianie obu algorytmow...\n";
    auto fast_results = run_ultra_algorithm(test_text.data(), test_text.size());
    auto ref_results = run_reference_algorithm(test_text);

    std::cout << "[3/3] Weryfikacja wynikow...\n";

    if (fast_results.size() != ref_results.size()) {
        std::cerr << "❌ BLAD: Rozna liczba unikalnych slow!\n";
        std::cerr << "Szybki algorytm znalazl: " << fast_results.size() << "\n";
        std::cerr << "Referencyjny znalazl: " << ref_results.size() << "\n";
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
            std::cerr << "❌ BLAD: Niezgodnosc liczby wystapien dla slowa '" << word << "'. ";
            std::cerr << "Powinno byc: " << count << ", a szybki algorytm naliczyl: " << it->second << "\n";
            success = false;
            break;
        }
    }

    if (success) {
        std::cout << "=========================================\n";
        std::cout << "  ✅ TEST ZALICZONY SUKCESEM! \n";
        std::cout << "  Szybki algorytm dziala w 100% poprawnie.\n";
        std::cout << "  Oba algorytmy zwrocily identyczne wyniki.\n";
        std::cout << "=========================================\n";
    }

    return success ? 0 : 1;
}