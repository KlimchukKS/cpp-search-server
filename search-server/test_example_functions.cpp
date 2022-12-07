#include "test_example_functions.h"

#include <iostream>
#include <string>
#include <cmath>
#include "search_server.h"
#include "remove_duplicates.h"

using namespace std;

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

void TestGetWordFrequencies() {
    SearchServer server("in the"s);
    server.AddDocument(1, "fluffy cat fluffy tail"s,       DocumentStatus::ACTUAL, {1, 2, 3});
    const map<string, double>& words_freq = server.GetWordFrequencies(1);
    const double epsilon = 1e-4;
    ASSERT_HINT(std::abs((words_freq.at("fluffy"s) - 2. / 4.) < epsilon),
                "Word frequencies is not calculated correctly"s);
    ASSERT_HINT(std::abs((words_freq.at("cat"s) - 1. / 4.) < epsilon),
                "Word frequencies is not calculated correctly"s);
}

void TestRemoveDocument() {
    SearchServer server("in the"s);
    server.AddDocument(0, "white cat fashion collar", DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "white cat fashion collar", DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(2, "fluffy cat fluffy tail", DocumentStatus::ACTUAL, {7, 2, 7});
    server.RemoveDocument(0);
    ASSERT_EQUAL_HINT(distance(server.begin(), server.end()), 2,
                "Document not remove"s);
}

void TestRemoveDuplicates() {
    SearchServer search_server("and with"s);

    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    RemoveDuplicates(search_server);

    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;

    ASSERT_EQUAL_HINT(search_server.GetDocumentCount(), 5,
                      "Duplicate documents are incorrectly deleted"s);
}

#define RUN_TEST(func) func(); cerr << #func << " OK"s << endl

void TestSearchServer() {
    RUN_TEST(TestGetWordFrequencies);
    RUN_TEST(TestRemoveDocument);
    RUN_TEST(TestRemoveDuplicates);
}
