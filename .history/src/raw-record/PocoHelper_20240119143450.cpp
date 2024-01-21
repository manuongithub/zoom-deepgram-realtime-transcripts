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

PocoHelper::PocoHelper() : uri(nullptr), cs(nullptr), request(nullptr), m_psock(nullptr), container(nullptr), msgSize(0) {
    // Default constructor
}

PocoHelper::PocoHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
    : uri(nullptr), cs(nullptr), request(nullptr), m_psock(nullptr), container(nullptr), msgSize(0)
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
    Poco::URI uri(wsEndPoint);

    // Append query parameters to the WebSocket URL
    std::string queryString = "?encoding=" + encoding + "&sample_rate=" + std::to_string(sampleRate) + "&channels=" + std::to_string(channels);
    uri.setQuery(queryString);

    logStream.str("");
    logStream << "WebSocket Query String: " << queryString;
    Log::info(logStream.str());

    Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", Poco::Net::Context::VERIFY_NONE, 9, true);

    // Create a WebSocket
    m_psock = new Poco::Net::WebSocket(uri.toString(), "", context);

    // Set additional WebSocket properties if needed
    m_psock->setBlocking(false);
    m_psock->setReceiveTimeout(Poco::Timespan(10, 0));
    // ...

    // Container for storing data
    container = new Poco::Buffer<char>(4096);
}

void PocoHelper::sendWebSocketData(const char* data, std::size_t size)
{
    try
    {
        // Send data to the WebSocket
        m_psock->sendFrame(data, size, Poco::Net::WebSocket::FRAME_BINARY);
        // Handle other logic if needed...
    }
    catch (const Poco::Net::WebSocketException& ex)
    {
        std::stringstream logStream;
        logStream << "WebSocketException caught: " << ex.displayText();
        Log::error(logStream.str());
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
            sendWebSocketData(container->begin(), container->size());

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
    std::stringstream logStream;
    logStream << "trying to receive messages from deepgram now";
    Log::info(logStream.str());
    try
    {
        const int bufferSize = 256;
        char readMsg[bufferSize];

        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long>(3 * 100)));

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
            Log::info("WebSocket connection got nothing back");
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
