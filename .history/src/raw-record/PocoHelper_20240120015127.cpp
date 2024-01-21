#include "PocoHelper.h"
#include <Poco/Net/SocketReactor.h>
#include <Poco/Net/SocketNotification.h>
#include <Poco/RunnableAdapter.h>
#include <Poco/Delegate.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>

PocoHelper::PocoHelper() : uri(nullptr), m_psock(nullptr), container(nullptr), reactor(nullptr) {
    // Default constructor
}

PocoHelper::PocoHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
    : uri(nullptr), m_psock(nullptr), container(nullptr), reactor(nullptr) {
    initialize(wsEndPoint, extraHeaders, encoding, sampleRate, channels);
}

PocoHelper::~PocoHelper() {
    close();
}

void PocoHelper::initialize(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
{
    std::stringstream logStream;
    uri = new Poco::URI(wsEndPoint);

    // Append query parameters to the WebSocket URL
    std::string queryString = "encoding=" + encoding + "&sample_rate=" + std::to_string(sampleRate) + "&channels=" + std::to_string(channels) + "&model=nova-2" + "&smart_format=true&diarize=true&utterances=true";
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
            // Check HTTP response status for successful authentication
            if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_SWITCHING_PROTOCOLS) {
                logStream.str("");
                logStream << "WebSocket connection opened successfully. Authentication successful.";
                Log::info(logStream.str());
            } else {
                logStream.str("");
                logStream << "WebSocket connection opened, but authentication failed. HTTP Status Code: " << response.getStatus();

                Log::error(logStream.str());
                // Handle authentication failure
                return;
            }
        } else {
            logStream.str("");
            logStream << "WebSocket connection failed to open.";
            Log::error(logStream.str());
            // Handle connection failure
            return;
        }

        reactor = new Poco::Net::SocketReactor();
        using PocoObserver = Poco::Observer<PocoHelper, Poco::Net::ReadableNotification>;
        reactor->addEventHandler(*m_psock, PocoObserver(*this, &PocoHelper::onSocketReadable));

        reactorThread.start(*this);

        m_psock->setBlocking(false);
        m_psock->setReceiveTimeout(Poco::Timespan(100, 0));
        m_psock->setSendTimeout(Poco::Timespan(100, 0));
        container = new Poco::Buffer<char>(4096);

    } catch (const Poco::Exception& ex) {
        std::string msg(ex.what());
        logStream << "Exception during WebSocket initialization: " << msg;
        Log::error(logStream.str());
        // Handle exception
    }
}


void PocoHelper::runReactor() {
    reactor->run();
}

void PocoHelper::onSocketReadable(Poco::Net::ReadableNotification* pNf) {
    receive_buffer();
}

void PocoHelper::send_buffer(char* buffer, unsigned int bufferLen) {
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

void PocoHelper::receive_buffer() {
    static std::string buffer; // Static buffer to persist data between calls

    try {
        const int bufferSize = 256;
        char readMsg[bufferSize];

        int bytesRead = m_psock->receiveBytes(readMsg, bufferSize);

        if (bytesRead > 0) {
            // Append the received data to the buffer
            buffer.append(readMsg, bytesRead);

            // Check if the buffer contains a complete JSON object
            size_t jsonEnd = buffer.find_last_of('}');
            if (jsonEnd != std::string::npos) {
                // Extract the JSON string from the buffer
                std::string jsonString = buffer.substr(0, jsonEnd + 1);

                // Parse the received JSON message
                Poco::JSON::Parser parser;
                Poco::Dynamic::Var result = parser.parse(jsonString);
                Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();

                // Clear the processed part of the buffer
                buffer = buffer.substr(jsonEnd + 1);

                // Rest of the parsing logic remains the same
                // ...

                // The rest of the code to check 'is_final' and print transcript
                // ...

            } else {
                Log::info("Buffering incomplete JSON object.");
            }
        } else if (bytesRead == 0) {
            // Connection closed gracefully
            Log::info("WebSocket connection got nothing back");
        } else {
            // Handle error or connection closure
            Log::error("Error receiving message from WebSocket.");
        }
    } catch (const Poco::JSON::JSONException& jsonEx) {
        std::string msg(jsonEx.displayText());
        std::stringstream logStream;
        logStream << "JSON Exception: " << msg;
        Log::error(logStream.str());
    } catch (const Poco::Net::WebSocketException& ex) {
        std::string msg(ex.what());
        std::stringstream logStream;
        logStream << "WebSocketException caught: " << msg;
        Log::error(logStream.str());
    } catch (const Poco::Exception& ex) {
        std::string msg(ex.what());
        std::stringstream logStream;
        logStream << "Exception: " << msg;
        Log::error(logStream.str());
    }
}

void PocoHelper::run() {
    runReactor();
}

void PocoHelper::close() {
    try {
        m_psock->shutdown();

        // Stop the reactor
        reactor->stop();
    } catch (...) {
        std::stringstream logStream;
        logStream << "Closing failed." << std::endl;
    }
}
