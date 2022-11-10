#pragma once

#include "search_server.h"
#include <vector>
#include <string>
#include "document.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> documents = search_server_.FindTopDocuments(raw_query, document_predicate);
        if(documents.empty() && (total_requests <= min_in_day_)){
            ++no_result_requests;
        }else{
            if(no_result_requests > 0 && total_requests >= min_in_day_){
                --no_result_requests;
            }
        }
        ++total_requests;
        return documents;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query) {
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }
    int GetNoResultRequests() const;
private:
    int total_requests = 0;
    int no_result_requests = 0;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
}; 