#include <iostream>
#include <string>
#include <cstdint>

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    std::string line;
    std::string current_word = "";
    uint64_t current_count = 0;

    while (std::getline(std::cin, line)) {
        size_t tab_pos = line.find('\t');
        if (tab_pos == std::string::npos) continue;

        std::string word = line.substr(0, tab_pos);
        uint64_t count = std::stoull(line.substr(tab_pos + 1));

        if (current_word == word) {
            current_count += count;
        }
        else {
            if (!current_word.empty()) {
                std::cout << current_word << "\t" << current_count << "\n";
            }
            current_word = word;
            current_count = count;
        }
    }

    if (!current_word.empty()) {
        std::cout << current_word << "\t" << current_count << "\n";
    }

    return 0;
}