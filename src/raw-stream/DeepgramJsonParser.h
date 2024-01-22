#ifndef DEEPGRAM_JSON_PARSER_H
#define DEEPGRAM_JSON_PARSER_H

#include "Poco/JSON/Parser.h"
#include "Poco/JSON/Object.h"
#include "Poco/Dynamic/Var.h"
#include <iostream>
#include <vector>

struct Word {
    std::string word;
    double start;
    double end;
    double confidence;
};

struct Alternative {
    std::string transcript;
    double confidence;
    std::vector<Word> words;
};

struct Hit {
    double confidence;
    double start;
    double end;
    std::string snippet;
};

struct Search {
    std::string query;
    std::vector<Hit> hits;
};

struct Channel {
    std::vector<Alternative> alternatives;
    std::vector<Search> search;
};

struct Metadata {
    std::string transaction_key;
    std::string request_id;
    std::string sha256;
    std::string created;
    int duration;
    int channels;
    std::vector<std::string> models;
};

struct DeepgramResults {
    Metadata metadata;
    std::string type;
    std::vector<int> channel_index;
    double duration;
    double start;
    bool is_final;
    bool speech_final;
    Channel channel;
};

class DeepgramJsonParser {
public:
    static DeepgramResults parse(const std::string& jsonString);
};

#endif // DEEPGRAM_JSON_PARSER_H
