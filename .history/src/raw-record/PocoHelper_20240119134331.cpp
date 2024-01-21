#include "PocoHelper.h"
#include <Poco/ConsoleChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/AutoPtr.h>
#include <Poco/Base64Encoder.h>
#include <Poco/StreamConverter.h>

#include <thread>
#include <chrono>


PocoHelper::PocoHelper() : uri(nullptr), cs(nullptr), request(nullptr), m_psock(nullptr), container(nullptr) {
    // Default constructor
}

PocoHelper::PocoHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
    : uri(nullptr), cs(nullptr), request(nullptr), m_psock(nullptr), container(nullptr)
{
    initialize(wsEndPoint, extraHeaders, encoding, sampleRate, channels);
}

PocoHelper::~PocoHelper()
{
    close();
}

void PocoHelper::initialize(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
{
    std::stringstream logStream;
    uri = new Poco::URI(wsEndPoint);

    // Append query parameters to the WebSocket URL
    std::string queryString = "encoding=" + encoding + "&sample_rate=" + std::to_string(sampleRate) + "&channels=" + std::to_string(channels) + "&model=nova-2" + "&smart_format=true";
    uri->setQuery(queryString);

    logStream.str("");
    logStream << "WebSocket Query String: " << queryString;
    Log::info(logStream.str());

    Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", Poco::Net::Context::VERIFY_NONE, 9, true);

    cs = new Poco::Net::HTTPSClientSession(uri->getHost(), 443, context);

    request = new Poco::Net::HTTPRequest(Poco::Net::HTTPRequest::HTTP_GET, uri->getPathEtc(), Poco::Net::HTTPMessage::HTTP_1_1);

    // Add extra headers
    for (const auto& header : extraHeaders) {
        request->set(header.first, header.second);
    }

    m_psock = new Poco::Net::WebSocket(*cs, *request, response);

    // Check if the socket is open
    if (m_psock->impl() && m_psock->impl()->initialized()) {
        logStream.str("");
        logStream << "WebSocket connection opened successfully.";
        Log::info(logStream.str());
    } else {
        logStream.str("");
        logStream << "WebSocket connection failed to open.";
        Log::error(logStream.str());
    }

    m_psock->setBlocking(false);
    m_psock->setReceiveTimeout(Poco::Timespan(10, 0));
    m_psock->setSendTimeout(Poco::Timespan(10, 0));
    container = new Poco::Buffer<char>(4096);

    if (m_psock->poll(Poco::Timespan(1, 0), Poco::Net::WebSocket::SELECT_READ))
    {
        receive_buffer();
    }
}

void PocoHelper::send_buffer(char* buffer, unsigned int bufferLen)
{
    logStream.str("");
    logStream << "Current msgSize: " << msgSize;
    Log::info(logStream.str());

    try
    {
        bool isSocketReady = m_psock->poll(Poco::Timespan(2, 0), Poco::Net::WebSocket::SELECT_WRITE);

        logStream.str("");
        logStream << "Socket is ready: " << (isSocketReady ? "true" : "false");
        Log::info(logStream.str());

        if (msgSize <= 0 && isSocketReady)
        {
            logStream.str("");
            logStream << "Entering the condition block";
            Log::info(logStream.str());

            msgSize = 4096;

            logStream.str("");
            logStream << "Sending to deepgram";
            Log::info(logStream.str());

            // Send the raw buffer data to the socket
            //m_psock->sendFrame(buffer, bufferLen, Poco::Net::WebSocket::FRAME_BINARY);
            m_psock->sendFrame(container->begin(), container->size(), Poco::Net::WebSocket::FRAME_BINARY);
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long>(1 * 100)));

            // Print sent bytes to console
            logStream.str("");
            logStream << "Sent bytes: " << container->size();
            Log::info(logStream.str());

            // Print received data to console
            receive_buffer();

            container = new Poco::Buffer<char>(msgSize);
        }

        if (*buffer)
        {
            container->append(buffer, bufferLen);
            msgSize -= bufferLen;

            logStream.str("");
            logStream << "Updated msgSize: " << msgSize;
            Log::info(logStream.str());
        }
    }
    catch (const Poco::Net::WebSocketException& ex)
    {
        std::string msg(ex.what());
        std::stringstream logStream;
        logStream << "WebSocketException Exception: " << msg;
        Log::error(logStream.str());

        
    }
}

void PocoHelper::receive_buffer()
{
    try
    {
        const int bufferSize = 256;
        char readMsg[bufferSize];

        int bytesRead = m_psock->receiveBytes(readMsg, bufferSize);

        if (bytesRead > 0)
        {
            // Log the received message to Poco Logger
            std::stringstream logStream;
            logStream << "Received message: " << std::string(readMsg, bytesRead);
            Log::info(logStream.str());
        }
        else if (bytesRead == 0)
        {
            // Connection closed gracefully
            Log::info("WebSocket connection closed gracefully.");
        }
        else
        {
            // Handle error or connection closure
            Log::error("Error receiving message from WebSocket.");
        }
    }
    catch (const Poco::Net::WebSocketException& ex)
    {
        std::string msg(ex.what());
        std::stringstream logStream;
        logStream << "WebSocketException caught: " << msg;
        Log::error(logStream.str());
    }
    catch (const Poco::Exception& ex)
    {
        std::string msg(ex.what());
        std::stringstream logStream;
        logStream << "Exception: " << msg;
        Log::error(logStream.str());
    }
}

void PocoHelper::close()
{
    try
    {
        m_psock->shutdown();
    }
    catch (...)
    {
        std::stringstream logStream;
        logStream << "Closing failed." << std::endl;
    }
}