#include "process_queries.h"

#include <vector>
#include <string>
#include <numeric>
#include <execution>

#include "document.h"
#include "search_server.h"


std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
    std::vector<std::vector<Document>> result(queries.size());
    transform_inclusive_scan( std::execution::par,
        queries.begin(), queries.end(),
        result.begin(),
        [](std::vector<Document> lhs, std::vector<Document> rhs){
            return rhs;
        },
        [&search_server](const std::string& query) {
            return search_server.FindTopDocuments(std::execution::par, query); }
    );
    return result;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
    std::vector<std::vector<Document>> docs_to_queries = ProcessQueries(search_server, queries);
    std::vector<Document> result;
    for(auto docs : docs_to_queries){
        for(auto doc : docs){
            result.push_back(doc);
        }
    }
    return result;
} 