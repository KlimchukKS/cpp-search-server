#pragma once

#include <stdexcept>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <utility>
#include <execution>
#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);

    explicit SearchServer(std::string_view stop_words_text);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status,
                     const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&,
                                           std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&,
                                           std::string_view raw_query,
                                           DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&,
                                           std::string_view raw_query) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&,
                                           std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&,
                                           std::string_view raw_query,
                                           DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&,
                                           std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::vector<int>::iterator begin();

    std::vector<int>::iterator end();

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query,
                                                                            int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&,
                                                                            std::string_view raw_query,
                                                                            int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&,
                                                                            std::string_view raw_query,
                                                                            int document_id) const;

    const std::map<std::string_view, double> GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);

    void RemoveDocument(const std::execution::parallel_policy&, int document_id);

private:
    const double kEpsilon = 1e-6;
    const int kMaxResultDocumentCount = 5;
    const std::size_t kMaxBucket = 10;

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::unordered_set<std::string> words_documents_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> word_frequency_by_document_id_;
    std::map<int, DocumentData> documents_;
    std::vector<int> documents_id_;

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view word;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text, bool is_parallel_policy = false) const;

    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&,
                                           const Query& query,
                                           DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument(std::string("Some of stop words are invalid"));
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const {
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);

    auto it = next(matched_documents.begin(), (matched_documents.size() > kMaxResultDocumentCount) ? kMaxResultDocumentCount : matched_documents.size());

    std::partial_sort(matched_documents.begin(), it, matched_documents.end(),
                      [&](const Document& lhs, const Document& rhs) {
                          if (std::abs(lhs.relevance - rhs.relevance) < kEpsilon) {
                              return lhs.rating > rhs.rating;
                          }

                          return lhs.relevance > rhs.relevance;
                      });

    if (matched_documents.size() > kMaxResultDocumentCount) {
        matched_documents.resize(kMaxResultDocumentCount);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&,
                                       std::string_view raw_query,
                                       DocumentPredicate document_predicate) const {
    return FindTopDocuments(raw_query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&,
                                       std::string_view raw_query,
                                       DocumentPredicate document_predicate) const {
    Query query = ParseQuery(raw_query);

    std::vector<Document> matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);

    auto it = next(matched_documents.begin(), (matched_documents.size() > kMaxResultDocumentCount) ? kMaxResultDocumentCount : matched_documents.size());

    std::partial_sort(std::execution::par, matched_documents.begin(), it, matched_documents.end(),
                      [&](const Document& lhs, const Document& rhs) {
                          if (std::abs(lhs.relevance - rhs.relevance) < kEpsilon) {
                              return lhs.rating > rhs.rating;
                          }

                          return lhs.relevance > rhs.relevance;
                      });

    if (matched_documents.size() > kMaxResultDocumentCount) {
        matched_documents.resize(kMaxResultDocumentCount);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view &word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&,
                                       const Query& query,
                                       DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> concurrent_document_to_relevance(kMaxBucket);

    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
                  [&](const std::string_view &word) {
        if (word_to_document_freqs_.count(word)) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            std::for_each(std::execution::par,
                          word_to_document_freqs_.at(word).begin(),
                          word_to_document_freqs_.at(word).end(),
                          [&](const std::pair<int, double> &pair) {
                const auto &document_data = documents_.at(pair.first);
                if (document_predicate(pair.first, document_data.status,
                                       document_data.rating)) {
                    concurrent_document_to_relevance[pair.first].ref_to_value +=
                            pair.second * inverse_document_freq;
                }
            });
        }
    });

    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
                  [&](const std::string_view &word) {
        if (word_to_document_freqs_.count(word)) {
            std::for_each(std::execution::par, word_to_document_freqs_.at(word).begin(),
                          word_to_document_freqs_.at(word).end(),
                          [&](const std::pair<int, double> &pair) {
                concurrent_document_to_relevance.Erase(pair.first);
            });
        }
    });

    std::map<int, double> document_to_relevance = std::move(concurrent_document_to_relevance.BuildOrdinaryMap());

    std::vector<Document> matched_documents(document_to_relevance.size());

    std::transform(std::execution::par, document_to_relevance.begin(), document_to_relevance.end(), matched_documents.begin(),
                   [&] (const std::pair<int, double>& pair) {
                       return Document{pair.first, pair.second, documents_.at(pair.first).rating};});

    return matched_documents;
}
