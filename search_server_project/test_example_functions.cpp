#include "test_example_functions.h"
#include "search_server.h"
using namespace std;

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))


void TestFindQueryWords() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};
	SearchServer server;
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
	const auto found_docs = server.FindTopDocuments("in"s);
	ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Incorrect document search.");
	const Document& doc0 = found_docs[0];
	ASSERT_EQUAL_HINT(doc0.id, doc_id, "Incorrect document search.");
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
	SearchServer server("in the"s);
	//server.SetStopWords("in the"s);
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
	ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded.");
}

void TestMinusWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
	SearchServer server;
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
	ASSERT_HINT(server.FindTopDocuments("in -cat"s).empty(), "There should be no minus words in document.");
}

void TestMatching() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
    	SearchServer server;
    	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);		
		vector<string_view> matched_words = {"in"};
        const auto ans = server.MatchDocument(matched_words[0], doc_id);
		ASSERT_EQUAL_HINT(static_cast<string>(get<0>(ans)[0]), matched_words[0], "Error in matching documents.");
    }
    {
    	SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto ans = server.MatchDocument("in -cat"s, doc_id);
		//vector<string> matched_words = {};
		ASSERT_HINT(get<0>(ans).empty(), "Minus words should not be in matching list.");
    }
}

void TestAvetageRating() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = {5, -12, 2, 1};
	int sum = 0;
	for (int num : ratings){
		sum += num;
	}
	int average_rating = sum / static_cast<int>(ratings.size());
	SearchServer server;
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
	const auto found_docs = server.FindTopDocuments("in"s);
	ASSERT_EQUAL_HINT(found_docs[0].rating, average_rating, "Incorrect rating calculation.");
}

void TestPredicate() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};
	SearchServer server;
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
	const auto found_docs = server.FindTopDocuments("in"s, [](int document_id,
			DocumentStatus status, int rating) { return document_id % 2 == 0; });
	ASSERT_EQUAL_HINT(found_docs[0].id, doc_id, "Predicate search does not work.");
}

void TestFindStatus() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};
	SearchServer server;
	server.AddDocument(doc_id, content, DocumentStatus::REMOVED, ratings);
	const auto found_docs = server.FindTopDocuments("in"s, DocumentStatus::REMOVED);
	ASSERT_EQUAL_HINT(found_docs[0].id, doc_id, "Status search does not work.");
}

void TestRelevanceCalc() {
	SearchServer search_server;
	search_server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, {8, -3});
	search_server.AddDocument(1, "dog in the park"s, DocumentStatus::ACTUAL, {7, 2, 7});
	search_server.AddDocument(2, "smooth cat"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
	vector<Document> result = search_server.FindTopDocuments("cat"s);
	const double IDF = log(3 * 0.5);
	const double TF_0 = 1 * 0.25;
	const double TF_2 = 1 * 0.5;
	ASSERT_EQUAL_HINT(result[0].relevance, IDF * TF_2, "Wrong relevance calculation.");
	ASSERT_EQUAL_HINT(result[1].relevance, IDF * TF_0, "Wrong relevance calculation.");
}

void TestRelevanceSort() {
	SearchServer search_server;
	search_server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, {8, -3});
	search_server.AddDocument(1, "dog in the park"s, DocumentStatus::ACTUAL, {7, 2, 7});
	search_server.AddDocument(2, "smooth cat"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
	vector<Document> result = search_server.FindTopDocuments("cat"s);
	ASSERT_EQUAL_HINT(result[0].id, 2, "Relevance sort does not work.");
	ASSERT_EQUAL_HINT(result[1].id, 0, "Relevance sort does not work.");
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
	TestFindQueryWords();
    TestExcludeStopWordsFromAddedDocumentContent();
    TestMinusWords();
    TestMatching();
    TestAvetageRating();
    TestPredicate();
    TestFindStatus();
    TestRelevanceCalc();
    TestRelevanceSort();
}