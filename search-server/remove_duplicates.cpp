#include "remove_duplicates.h"
#include <iostream>
#include <set>
#include <vector>

void RemoveDuplicates(SearchServer& server) {
    std::set<std::string> words_document;
    std::vector<int> id_documents_remove;
    for(const int i : server){
        std::string words;
        for(auto [word, freq] : server.GetWordFrequencies(i)){
            words += word;
        }
        if (!words_document.count(words)) {
            words_document.insert(words);
        } else {
            std::cout << std::string("Found duplicate document id ") << i << '\n';
            id_documents_remove.push_back(i);
        }
    }
    for(int i : id_documents_remove){
        server.RemoveDocument(i);
    }
}
