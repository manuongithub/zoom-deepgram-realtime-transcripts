#include "DeepgramJsonParser.h"

#include "DeepgramJsonParser.h"

DeepgramResults DeepgramJsonParser::parse(const std::string& jsonString) {
    DeepgramResults results;

    try {
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var result = parser.parse(jsonString);
        Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();

        // Parsing logic for the new structure
        results.metadata.transaction_key = object->getObject("metadata")->optValue<std::string>("transaction_key", "");
        results.metadata.request_id = object->getObject("metadata")->optValue<std::string>("request_id", "");
        results.metadata.sha256 = object->getObject("metadata")->optValue<std::string>("sha256", "");
        results.metadata.created = object->getObject("metadata")->optValue<std::string>("created", "");
        results.metadata.duration = object->getObject("metadata")->optValue<int>("duration", 0);
        results.metadata.channels = object->getObject("metadata")->optValue<int>("channels", 0);


        // Parsing models array
        try {
            Poco::JSON::Object::Ptr metadataObject = object->getObject("metadata");
            if (metadataObject) {
                Poco::JSON::Array::Ptr modelsArray = metadataObject->getArray("models");
                if (modelsArray) {
                    for (int i = 0; i < modelsArray->size(); ++i) {
                        Poco::Dynamic::Var modelVar = modelsArray->get(i);
                        if (!modelVar.isEmpty())
                            results.metadata.models.push_back(modelVar.convert<std::string>());
                    }
                }
            } 
        } catch (Poco::Exception& e) {
            std::cerr << "Poco Exception while parsing 'models': " << e.displayText() << std::endl;
            std::cerr << "Error parsing 'models'. JSON: " << jsonString << std::endl;
        } catch (std::exception& e) {
            std::cerr << "Standard Exception while parsing 'models': " << e.what() << std::endl;
            std::cerr << "Error parsing 'models'. JSON: " << jsonString << std::endl;
        } catch (...) {
            std::cerr << "Unknown error parsing 'models'" << std::endl;
            std::cerr << "Error parsing 'models'. JSON: " << jsonString << std::endl;
        }


        if (object->has("type")) {
            results.type = object->getValue<std::string>("type");
        }
        

        // Parsing channel_index array
        Poco::JSON::Array::Ptr channelIndexArray = object->getArray("channel_index");
        for (int i = 0; i < channelIndexArray->size(); ++i) {
            Poco::Dynamic::Var indexVar = channelIndexArray->get(i);
            if (!indexVar.isEmpty())
                results.channel_index.push_back(indexVar.convert<int>());
        }


        results.duration = object->optValue<double>("duration", 0.0);
        results.start = object->optValue<double>("start", 0.0);
        results.is_final = object->optValue<bool>("is_final", false);
        results.speech_final = object->optValue<bool>("speech_final", false);

        // Parsing Channel
        Poco::JSON::Object::Ptr channelObject = object->getObject("channel");
        if (channelObject) {
            // Parsing Alternatives

            Poco::JSON::Array::Ptr alternativesArray = channelObject->getArray("alternatives");
            for (std::size_t i = 0; i < alternativesArray->size(); ++i) {
                Poco::JSON::Object::Ptr alternativeObject = alternativesArray->getObject(i);
                Alternative alternative;
                alternative.transcript = alternativeObject->getValue<std::string>("transcript");
                alternative.confidence = alternativeObject->getValue<double>("confidence");

                // Parsing Words

                Poco::JSON::Array::Ptr wordsArray = alternativeObject->getArray("words");
                for (std::size_t j = 0; j < wordsArray->size(); ++j) {
                    Poco::JSON::Object::Ptr wordObject = wordsArray->getObject(j);
                    Word word;
                    word.word = wordObject->getValue<std::string>("word");
                    word.start = wordObject->getValue<double>("start");
                    word.end = wordObject->getValue<double>("end");
                    word.confidence = wordObject->getValue<double>("confidence");
                    alternative.words.push_back(word);
                }

                results.channel.alternatives.push_back(alternative);
            }

            // Parsing Search

            Poco::JSON::Array::Ptr searchArray = channelObject->getArray("search");
            for (std::size_t i = 0; i < searchArray->size(); ++i) {
                Poco::JSON::Object::Ptr searchObject = searchArray->getObject(i);
                Search search;
                search.query = searchObject->getValue<std::string>("query");

                // Parsing Hits

                Poco::JSON::Array::Ptr hitsArray = searchObject->getArray("hits");
                for (std::size_t j = 0; j < hitsArray->size(); ++j) {
                    Poco::JSON::Object::Ptr hitObject = hitsArray->getObject(j);
                    Hit hit;
                    hit.confidence = hitObject->getValue<double>("confidence");
                    hit.start = hitObject->getValue<double>("start");
                    hit.end = hitObject->getValue<double>("end");
                    hit.snippet = hitObject->getValue<std::string>("snippet");
                    search.hits.push_back(hit);
                }

                results.channel.search.push_back(search);
            }
        }

    } catch (Poco::Exception& e) {
        //std::cerr << "Poco Exception: " << e.displayText() << std::endl;
    } catch (std::exception& e) {
        //std::cerr << "Standard Exception: " << e.what() << std::endl;
    } catch (...) {
        //std::cerr << "Unknown error parsing JSON" << std::endl;
    }

    return results;
}
