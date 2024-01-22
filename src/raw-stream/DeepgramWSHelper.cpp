// DeepgramWSHelper.cpp
#include "DeepgramWSHelper.h"
#include "DeepgramJsonParser.h"
#include <Poco/Net/SocketReactor.h>
#include <Poco/Net/SocketNotification.h>
#include <Poco/RunnableAdapter.h>
#include <Poco/Delegate.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>


DeepgramWSHelper::DeepgramWSHelper() : uri(nullptr), m_psock(nullptr), reactor(nullptr) {
    // Default constructor
}

DeepgramWSHelper::DeepgramWSHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
    : uri(nullptr), m_psock(nullptr), reactor(nullptr) {
    initialize(wsEndPoint, extraHeaders, encoding, sampleRate, channels);
}

DeepgramWSHelper::~DeepgramWSHelper() {
    close();
}

void DeepgramWSHelper::initialize(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels) {
    std::stringstream logStream;
    uri = new Poco::URI(wsEndPoint);

    // Append query parameters to the WebSocket URL
    std::string queryString = "encoding=" + encoding + "&sample_rate=" + std::to_string(sampleRate) + "&channels=" + std::to_string(channels) + "&model=nova-2" + "&endpointing=10&smart_format=true&diarize=true&utterances=true";
    uri->setQuery(queryString);

    logStream.str("");
    logStream << "WebSocket Query String: " << queryString;
    Log::info(logStream.str());

    try {
        Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", Poco::Net::Context::VERIFY_NONE, 9, true);

        Poco::Net::HTTPSClientSession cs(uri->getHost(), 443, context);

        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, uri->getPathEtc(), Poco::Net::HTTPMessage::HTTP_1_1);

        // Add extra headers
        for (const auto& header : extraHeaders) {
            request.set(header.first, header.second);
        }

        Poco::Net::HTTPResponse response;
        m_psock = new Poco::Net::WebSocket(cs, request, response);

        // Check if the socket is open
        if (m_psock->impl() && m_psock->impl()->initialized()) {
            // NOP everything is good
        } else {
            logStream.str("");
            logStream << "WebSocket connection failed to open.";
            Log::error(logStream.str());
            

            // Check HTTP response status for successful authentication
            if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_SWITCHING_PROTOCOLS) {
                logStream.str("");
                logStream << "WebSocket connection opened successfully. Authentication successful.";
                Log::info(logStream.str());
            } else {
                logStream.str("");
                logStream << "WebSocket connection opened, but authentication failed. HTTP Status Code: " << response.getStatus();

                Log::error(logStream.str());
            }

            return;
        }

        reactor = new Poco::Net::SocketReactor();
        using PocoObserver = Poco::Observer<DeepgramWSHelper, Poco::Net::ReadableNotification>;
        reactor->addEventHandler(*m_psock, PocoObserver(*this, &DeepgramWSHelper::onSocketReadable));

        reactorThread.start(*this);

        m_psock->setBlocking(false);
        m_psock->setReceiveTimeout(Poco::Timespan(10, 0));
        m_psock->setSendTimeout(Poco::Timespan(10, 0));

    } catch (const Poco::Exception& ex) {
        std::string msg(ex.what());
        logStream << "Exception during WebSocket initialization: " << msg;
        Log::error(logStream.str());
        // Handle exception
    }
}

void DeepgramWSHelper::runReactor() {
    reactor->run();
}

void DeepgramWSHelper::onSocketReadable(Poco::Net::ReadableNotification* pNf) {
    receive_buffer();
}

void DeepgramWSHelper::send_buffer(char* buffer, unsigned int bufferLen) {
    try {
        bool isSocketReady = m_psock->poll(Poco::Timespan(2, 0), Poco::Net::WebSocket::SELECT_WRITE);

        if (isSocketReady) {
            // Send the raw buffer data to the socket
            m_psock->sendFrame(buffer, bufferLen, Poco::Net::WebSocket::FRAME_BINARY);
        }

        reactor->wakeUp();
    } catch (const Poco::Net::WebSocketException& ex) {
        // Indulging in the philosophical contemplation of potential exceptions
        std::string msg(ex.what());
        std::stringstream logStream;
        logStream << "WebSocketException Exception: " << msg;
        Log::error(logStream.str());
    }
}

void DeepgramWSHelper::receive_buffer() {
    try {
        std::vector<char> readBuffer(2048);
        std::vector<char> accumulatedData;
        int bytesRead = 0;  // Declare bytesRead before the do-while loop

        do {
            // Resize buffer as needed
            readBuffer.resize(2048);

            // Read data into the resized vector
            bytesRead = m_psock->receiveBytes(readBuffer.data(), readBuffer.size());

            if (bytesRead > 0) {
                // Append the received data to the accumulated data
                accumulatedData.insert(accumulatedData.end(), readBuffer.begin(), readBuffer.begin() + bytesRead);

            }
        } while (bytesRead > 0 && bytesRead == readBuffer.size());

        if (!accumulatedData.empty()) {
            // Log the raw JSON data received
            std::string receivedData(accumulatedData.begin(), accumulatedData.end());

            try {
                // Attempt to parse the received data as JSON
                DeepgramResults result = DeepgramJsonParser::parse(receivedData);

                // Check if the parsed JSON contains alternatives and transcript
                if (!result.channel.alternatives.empty() && !result.channel.alternatives[0].transcript.empty()) {
                    // Extract and print the transcript
                    std::string transcript = result.channel.alternatives[0].transcript;
                    Log::info("Transcript from JSON: " + transcript);
                }
            } catch (const Poco::JSON::JSONException& jsonEx) {
                // Log the JSON parsing exception
                Log::error("JSON Parsing Exception: " + jsonEx.message());
            }
        } else {
            Log::info("WebSocket connection got nothing back");
        }
    } catch (const Poco::Net::WebSocketException& ex) {
        std::string errorMsg = "WebSocketException caught: " + std::string(ex.displayText());
        Log::error(errorMsg);
    } catch (const Poco::Exception& ex) {
        std::string errorMsg = "Exception: " + std::string(ex.displayText());
        Log::error(errorMsg);
    }
}


void DeepgramWSHelper::run() {
    runReactor();
}

void DeepgramWSHelper::close() {
    try {
        m_psock->shutdown();

        // Stop the reactor
        reactor->stop();
    } catch (...) {
        std::stringstream logStream;
        logStream << "Closing failed." << std::endl;
        Log::error(logStream.str());
    }
}
