#include "request_queue.h"
#include "document.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server){
    }

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus search_status) {
    return AddFindRequest(raw_query, [search_status](int document_id, DocumentStatus status, int rating)
                                     { return status == search_status; });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
    return empty_requests_;
}
