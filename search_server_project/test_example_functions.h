#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <iostream>


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << "(" << line << "): " << func << ": ";
        std::cout << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
        std::cout << t << " != " << u << ".";
        if (!hint.empty()) {
            std::cout << " Hint: " << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line, const std::string& hint) ;

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))


void TestFindQueryWords() ;

void TestExcludeStopWordsFromAddedDocumentContent() ;

void TestMinusWords();

void TestMatching() ;

void TestAvetageRating() ;

void TestPredicate() ;

void TestFindStatus() ;

void TestRelevanceCalc() ;

void TestRelevanceSort() ;

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() ;

// --------- Окончание модульных тестов поисковой системы -----------

