#pragma once
#include "search_server.h"
#include <string>
#include <vector>
#include <deque>

class RequestQueue {
public:
    RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus search_status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::string query;
        std::vector<Document> search_result;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer &search_server_;
    int empty_requests_ = 0;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    requests_.push_back({raw_query, search_server_.FindTopDocuments(raw_query, document_predicate)});
    if(requests_.back().search_result.empty()){
        ++empty_requests_;
    }
    if(requests_.size() > min_in_day_) {
        if(requests_.front().search_result.empty()){
            --empty_requests_;
        }
        requests_.pop_front();
    }
    return requests_.back().search_result;
}
