#include "document.h"

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}

std::ostream& operator<<(std::ostream& output,  const Document& document) { 
    output << std::string("{ document_id = ") << document.id << std::string(", relevance = ")
           << document.relevance << std::string(", rating = ") << document.rating << std::string(" }");
    return output;
}
