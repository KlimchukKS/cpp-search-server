#include "search_server.h"
#include <cmath>
#include <algorithm>

SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {
}

SearchServer::SearchServer(std::string_view stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status,
                               const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument(std::string("Invalid document_id"));
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view& word : words) {
        auto [iter_word, b] = words_documents_.insert(std::string (word));
        word_to_document_freqs_[*iter_word][document_id] += inv_word_count;
        word_frequency_by_document_id_[document_id][*iter_word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    documents_id_.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&,
                                       std::string_view raw_query,
                                       DocumentStatus status) const {
    return FindTopDocuments(raw_query, status);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&,
                                       std::string_view raw_query) const {
    return FindTopDocuments(raw_query);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&,
                                                     std::string_view raw_query,
                                                     DocumentStatus status) const {
    return FindTopDocuments(std::execution::par, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&,
                                                     std::string_view raw_query) const {
    return FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::vector<int>::iterator SearchServer::begin() {
    return documents_id_.begin();
}

std::vector<int>::iterator SearchServer::end() {
    return documents_id_.end();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query,
                                                                                 int document_id) const {
    const auto query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words;

    for (auto word : query.minus_words) {
        if (word_frequency_by_document_id_.at(document_id).count(word)) {
            return {std::vector<std::string_view>{}, documents_.at(document_id).status};
        }
    }

    for (auto word : query.plus_words) {
        if (word_frequency_by_document_id_.at(document_id).count(word)) {
            matched_words.push_back(word);
        }
    }

    std::sort(matched_words.begin(), matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&,
                                                                                      std::string_view raw_query,
                                                                                      int document_id) const {
    return MatchDocument(raw_query, document_id);
}
// Возвращает все плюс слова из запроса которые есть в документе и его статус по id
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&,
                                                                                      std::string_view raw_query,
                                                                                      int document_id) const {
    auto query = ParseQuery(raw_query, true);

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [this, document_id] (const std::string_view& word) {
        return word_frequency_by_document_id_.at(document_id).count(word);
    })) {
        return {std::vector<std::string_view>{}, documents_.at(document_id).status};
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());
    std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
                 matched_words.begin(), [this, document_id](const auto& word) {
                     return word_frequency_by_document_id_.at(document_id).count(word);
                 });
    std::sort(std::execution::par, matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(std::execution::par, matched_words.begin(), matched_words.end()), matched_words.end());

    if (matched_words[0].empty()) { matched_words.erase(matched_words.begin()); }

    return {matched_words, documents_.at(document_id).status};
}

const std::map<std::string_view, double> SearchServer::GetWordFrequencies(int document_id) const {
    std::map<std::string_view, double> result;
    for (auto [first, second] : word_frequency_by_document_id_.at(document_id)) {
        result[first] = second;
    }
    return result;
}

void SearchServer::RemoveDocument(int document_id) {
    for(auto [word, freq] : word_frequency_by_document_id_[document_id]){
        word_to_document_freqs_[word].erase(document_id);
        if(word_to_document_freqs_[word].empty()){
            word_to_document_freqs_.erase(word);
            words_documents_.erase(std::string(word));
        }
    }

    word_frequency_by_document_id_.erase(document_id);
    documents_.erase(document_id);

    std::vector<int> documents_id_copy;
    documents_id_copy.reserve((documents_id_.size() - 1));

    auto it(std::find(documents_id_.begin(), documents_id_.end(), document_id));
    if (it != documents_id_.end()) {
        documents_id_.erase(it);
    }
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    std::vector<std::string_view> words(word_frequency_by_document_id_[document_id].size());

    std::transform(std::execution::par, word_frequency_by_document_id_[document_id].begin(),
                   word_frequency_by_document_id_[document_id].end(), words.begin(),
                   [](const std::pair<std::string_view, double>& p) {
        return p.first;
    });

    std::for_each(std::execution::par, words.begin(), words.end(), [document_id, this](std::string_view word){
        word_to_document_freqs_[word].erase(document_id);
        words_documents_.erase(std::string(word));
    });

    word_frequency_by_document_id_.erase(document_id);
    documents_.erase(document_id);

    auto it(std::find(std::execution::par, documents_id_.begin(), documents_id_.end(), document_id));
    if (it != documents_id_.end()) {
        documents_id_.erase(it);
    }
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word);
}

bool SearchServer::IsValidWord(const std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument(std::string("Word is invalid"));
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view word) const {
    if (word.empty()) {
        throw std::invalid_argument(std::string("Query word is empty"));
    }
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(std::string(word))) {
        throw std::invalid_argument(std::string("Query word is invalid"));
    }
    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text, bool is_parallel_policy) const {
    Query result;

    for (std::string_view& word : SplitIntoWords(text)) {
        auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.word);
            } else {
                result.plus_words.push_back(query_word.word);
            }
        }
    }

    if (!is_parallel_policy) {
        std::sort(result.minus_words.begin(), result.minus_words.end());
        std::sort(result.plus_words.begin(), result.plus_words.end());
        result.minus_words.erase(
                std::unique(result.minus_words.begin(), result.minus_words.end()),
                result.minus_words.end());
        result.plus_words.erase(
                std::unique(result.plus_words.begin(), result.plus_words.end()),
                result.plus_words.end());
    }

    return result;
}


double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
