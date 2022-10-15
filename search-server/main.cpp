void TestAddingDocumentsAndSearchingDocumentsRequest(){
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL_HINT(server.GetDocumentCount(), 1,
                      "The document was not added"s);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL_HINT(doc0.id, doc_id,
                      "No document was found by request"s);
}
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                "Stop words must be excluded from documents"s);
}
void TestExcludeMinusWords() {
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("cat -city"s).empty(),
                    "Documents containing minus words from a search query should not be included in search results."s);
    }
}
void TestForCorrectCalculationDocumentRating(){
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("cat"s);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL_HINT(doc0.rating, 2,
                      "The rating of the added document should be equal to the arithmetic average of the document ratings."s);
}
void TestForSortingFoundDocumentsRelevance(){
    SearchServer server;
    server.AddDocument(0, "white cat fashion collar"s,        DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(1, "fluffy cat fluffy tail"s,       DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(2, "groomed dog expressive eyes"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(3, "house bee yellow black stripes"s,         DocumentStatus::ACTUAL, {1, 2, 3});
    bool is_sorted = true;
    double relevance_buffer = 1;
    for(const auto& doc : server.FindTopDocuments("fluffy groomed cat"s)){
        if(doc.relevance > relevance_buffer){
            is_sorted = false;
        }
        relevance_buffer = doc.relevance;
    }
    ASSERT_HINT(is_sorted,
                "Found documents should be sorted in descending order"s);
}
void TestForCorrectlyCalculatingRelevanceDocuments(){
    SearchServer server;
    server.AddDocument(0, "white cat fashion collar", DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(1, "fluffy cat fluffy tail", DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(2, "groomed dog expressive eyes", DocumentStatus::ACTUAL, {1, 2, 3});
    const auto found_docs = server.FindTopDocuments("fluffy groomed cat"s);
    const double epsilon = 1e-4;
    ASSERT_HINT(abs(found_docs[0].relevance - 0.6507) < epsilon,
                "Relevance is not calculated correctly"s);
    ASSERT_HINT(abs(found_docs[1].relevance - 0.2746) < epsilon,
                "Relevance is not calculated correctly"s);
    ASSERT_HINT(abs(found_docs[2].relevance - 0.1014) < epsilon,
                "Relevance is not calculated correctly"s);
}
void TheTestForFindingDocumentsStatus(){
    SearchServer server;
    server.AddDocument(0, "white cat fashion collar", DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(1, "fluffy cat fluffy tail", DocumentStatus::IRRELEVANT, {1, 2, 3});
    server.AddDocument(2, "groomed dog expressive eyes", DocumentStatus::BANNED, {1, 2, 3});
    server.AddDocument(3, "groomed house bee yellow black stripes", DocumentStatus::REMOVED, {1, 2, 3});
    {
        const auto found_docs = server.FindTopDocuments("fluffy groomed cat"s);
        ASSERT_HINT(!found_docs.empty(),
                    "Documents with the status of a ACTUAL are not found"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u,
                          "Number of documents with ACTUAL status does not match the input"s);
    }{
        const auto found_docs = server.FindTopDocuments("fluffy groomed cat"s, DocumentStatus::ACTUAL);
        ASSERT_HINT(!found_docs.empty(),
                    "Documents with the status of a ACTUAL are not found"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u,
                          "Number of documents with ACTUAL status does not match the input"s);
    }{
        const auto found_docs = server.FindTopDocuments("fluffy groomed cat"s, DocumentStatus::IRRELEVANT);
        ASSERT_HINT(!found_docs.empty(),
                    "Documents with the status of a IRRELEVANT are not found"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u,
                          "Number of documents with IRRELEVANT status does not match the input"s);
    }{
        const auto found_docs = server.FindTopDocuments("fluffy groomed cat"s, DocumentStatus::BANNED);
        ASSERT_HINT(!found_docs.empty(),
                    "Documents with the status of a BANNED are not found"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u,
                          "Number of documents with BANNED status does not match the input"s);
    }{
        const auto found_docs = server.FindTopDocuments("fluffy groomed cat"s, DocumentStatus::REMOVED);
        ASSERT_HINT(!found_docs.empty(),
                    "Documents with the status of a REMOVED are not found"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u,
                          "Number of documents with REMOVED status does not match the input"s);
    }
}
void TestMatchDocument(){
    SearchServer server;
    server.AddDocument(0, "white cat fashion collar", DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(1, "fluffy cat fluffy tail", DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(2, "groomed dog expressive eyes", DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(3, "groomed house bee yellow black stripes", DocumentStatus::ACTUAL, {1, 2, 3});
    const int document_count = server.GetDocumentCount();
    ASSERT_EQUAL_HINT(document_count, 4,
                      "The number of documents does not match the input"s);
    {
        const auto [words, status] = server.MatchDocument("fluffy groomed cat"s, 0);
        ASSERT(!words.empty());
        ASSERT_EQUAL(words.size(), 1);
        ASSERT_EQUAL_HINT(words[0], "cat"s,
                          "The document does not match the search query"s);
    }{
        const auto [words, status] = server.MatchDocument("fluffy groomed cat"s, 1);
        ASSERT(!words.empty());
        ASSERT_EQUAL(words.size(), 2);
        ASSERT_EQUAL_HINT(words[0], "cat"s,
                          "The document does not match the search query"s);
        ASSERT_EQUAL_HINT(words[1], "fluffy"s,
                          "The document does not match the search query"s);
    }{
        const auto [words, status] = server.MatchDocument("fluffy groomed cat -dog"s, 2);
        ASSERT_HINT(words.empty(),
                          "Documents that have stop words are not correctly processed, the word vector must be empty."s);
    }{
        const auto [words, status] = server.MatchDocument("cat"s, 3);
        ASSERT_HINT(words.empty(),
                    "The document does not match the query, the word vector must be empty."s);
    }{
        const auto [words, status] = server.MatchDocument(""s, 0);
        ASSERT_HINT(words.empty(),
                    "The document does not match the query, the word vector must be empty."s);
    }
}
void TestFilteringSearchResultsUsingPredicate() {
    SearchServer server;
    server.AddDocument(0, "white cat fashion collar", DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "fluffy cat fluffy tail", DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "groomed dog expressive eyes", DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "fluffy house bee yellow black stripes", DocumentStatus::BANNED, {9});
    {
        const auto found_docs = server.FindTopDocuments("fluffy groomed cat"s,
                                                        [](int document_id, DocumentStatus status, int rating){ return status == DocumentStatus::ACTUAL; });
        ASSERT(!found_docs.empty());
        ASSERT_EQUAL(found_docs.size(), 3u);
        for(const auto& doc: found_docs){
            if(doc.id == 3){
                bool sorted_from_status = false;
                ASSERT_HINT(sorted_from_status,
                            "No sorting from status"s);
            }
        }
    }
    {
        const auto found_docs = server.FindTopDocuments("fluffy groomed cat"s,
                                                        [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        ASSERT(!found_docs.empty());
        ASSERT_EQUAL(found_docs.size(), 2u);
        for(const auto& doc: found_docs){
            ASSERT_EQUAL_HINT(doc.id % 2, 0,
                              "No sorting from id"s);
        }
    }
    {
        const auto found_docs = server.FindTopDocuments("fluffy groomed cat"s,
                                                        [](int document_id, DocumentStatus status, int rating) { return rating > 8; });
        ASSERT(!found_docs.empty());
        ASSERT_EQUAL(found_docs.size(), 1u);
        for(const auto& doc: found_docs){
            if(doc.rating < 8){
                bool sorted_from_rating = false;
                ASSERT_HINT(sorted_from_rating,
                            "No sorting from rating"s);
            }
        }
    }
}

void TestSearchServer() {
    RUN_TEST(TestAddingDocumentsAndSearchingDocumentsRequest);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWords);
    RUN_TEST(TestForCorrectCalculationDocumentRating);
    RUN_TEST(TestForSortingFoundDocumentsRelevance);
    RUN_TEST(TestForCorrectlyCalculatingRelevanceDocuments);
    RUN_TEST(TheTestForFindingDocumentsStatus);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestFilteringSearchResultsUsingPredicate);
}
