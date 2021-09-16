# SearchServer
Simple search server

Add stop words and documents then search with key words.
Example adding document:
  AddDocument(search_server, 1, "smooth cat smooth tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
Example searching:
  FindTopDocuments(search_server, "lazy -dog"s);
