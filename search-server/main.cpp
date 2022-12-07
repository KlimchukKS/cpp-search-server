#include <iostream>
#include "paginator.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "search_server.h"
#include "request_queue.h"
#include "document.h"
#include "log_duration.h"
#include "remove_duplicates.h"
#include "test_example_functions.h"

using namespace std;

int main() {
    {
        LOG_DURATION_STREAM("Time work program", std::cerr);
        SearchServer search_server(std::string("and in at"));
        RequestQueue request_queue(search_server);
        search_server.AddDocument(1, std::string("curly cat curly tail"), DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(2, std::string("curly dog and fancy costd::llar"), DocumentStatus::ACTUAL, {1, 2, 3});
        search_server.AddDocument(3, std::string("big cat fancy collar "), DocumentStatus::ACTUAL, {1, 2, 8});
        search_server.AddDocument(4, std::string("big dog sparrow Eugene"), DocumentStatus::ACTUAL, {1, 3, 2});
        search_server.AddDocument(5, std::string("big dog sparrow Vasiliy"), DocumentStatus::ACTUAL, {1, 1, 1});
        for (int i = 0; i < 1439; ++i) {
            request_queue.AddFindRequest(std::string("empty request"));
        }
        std::cout << std::string("empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
        request_queue.AddFindRequest(std::string("curly dog"));
        std::cout << std::string("empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
        request_queue.AddFindRequest(std::string("big collar"));
        std::cout << std::string("empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
        request_queue.AddFindRequest(std::string("sparrow"));
        std::cout << std::string("Total empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
    }

    return 0;
}
