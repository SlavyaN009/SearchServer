#include "string_processing.h"



std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    size_t word_start = 0;
    for (size_t i = 0; i != text.size(); ++i) {
        if (text[i] == ' ') {
            if (word_start != i) {
                words.push_back(std::string{ &text[word_start], i - word_start });
            }
            word_start = i + 1;
        }
    }
    if (word_start != text.size()) {
        words.push_back(std::string{ &text[word_start], text.size() - word_start });
    }
    return words;
}
