#pragma once

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

#include <string>
#include <string_view>
#include <set>
#include <vector>
#include <tuple>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <execution>
#include <initializer_list>
#include <mutex>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    SearchServer() = default;
    
    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words);

    SearchServer(const std::string_view stop_words_text);
    
    SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentStatus status) const;
    
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    int GetDocumentCount() const;

    int GetDocumentId(int index) const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    std::vector<int>::iterator begin();

    std::vector<int>::iterator end();

    template<typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);
    
    void RemoveDocument(int document_id){
        RemoveDocument(std::execution::seq, document_id);
    }
    
    template<typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(ExecutionPolicy&& policy, const std::string_view raw_query, int document_id) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;

    std::set<std::string> GetStopWords() const;
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord;
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> doc_id_to_words_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;
    const std::map<std::string_view, double> dummy_;

    bool IsStopWord(const std::string& word) const;

    static bool IsValidWord(const std::string& word) {
        return std::none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    QueryWord ParseQueryWord(const std::string& text) const;

    Query ParseQuery(const std::string& text) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <class DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&& policy, const Query& query, DocumentPredicate document_predicate) const;
};



template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
: stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), SearchServer::IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}

template <class DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
    }
    
template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query) const{
    return FindTopDocuments(policy, raw_query, [](int document_id, DocumentStatus document_status, int rating) {
                                                return document_status == DocumentStatus::ACTUAL;});	
}
    
template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentStatus status) const{
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                                                return document_status == status;});	    
}

template <class ExecutionPolicy, class DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(static_cast<std::string>(raw_query));
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);
    sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
                                                        if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                                                            return lhs.rating > rhs.rating;
                                                        } else {
                                                            return lhs.relevance > rhs.relevance;
                                                        }});
	
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <class DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const SearchServer::Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> map_lock(10);    
    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), 
        [document_predicate, &map_lock, this] (const std::string& word){
            if (word_to_document_freqs_.count(word)) {
                const double inverse_document_freq = SearchServer::ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                       map_lock[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
        }});	
    std::map<int, double> document_to_relevance = map_lock.BuildOrdinaryMap();
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word)) {
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
    }
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }    
    return matched_documents;
}

template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id){
    if(!documents_.count(document_id)) return;
    documents_.erase(document_id);
    for(const auto [word_sv,_] : doc_id_to_words_freqs_[document_id]){
        std::string word = static_cast<std::string>(word_sv);
        word_to_document_freqs_[word].erase(document_id);
        if(word_to_document_freqs_[word].empty()){
            word_to_document_freqs_.erase(word);
        }
    }
    doc_id_to_words_freqs_.erase(document_id);
    std::remove(policy, document_ids_.begin(), document_ids_.end(), document_id);
}
    
template<typename ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(ExecutionPolicy&& policy, const std::string_view raw_query, int document_id) const {
    const auto query = ParseQuery(static_cast<std::string>(raw_query));
    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());
    std::mutex m;
    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(),
        [&m,document_id, &raw_query, &matched_words, this](const std::string& word) {
        if (word_to_document_freqs_.count(word)) {
        if (word_to_document_freqs_.at(word).count(document_id)) {
            std::lock_guard<std::mutex> guard(m);
            matched_words.push_back(raw_query.substr(raw_query.find(word), word.size()));
        }
    }});    
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word)) {
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
    }
    return { matched_words, documents_.at(document_id).status };
}
    
