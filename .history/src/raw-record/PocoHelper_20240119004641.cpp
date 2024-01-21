#include "PocoHelper.h"
#include <Poco/ConsoleChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/AutoPtr.h>
#include <sstream>


PocoHelper::PocoHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
{
    std::stringstream logStream;
    uri = new Poco::URI(wsEndPoint);

    // Append query parameters to the WebSocket URL
    std::string queryString = "?encoding=" + encoding + "&sample_rate=" + std::to_string(sampleRate) + "&channels=" + std::to_string(channels);
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
    if (m_psock->impl() && m_psock->impl()->initialized()) {}
        logStream.str("");
        logStream << "WebSocket connection opened successfully.";
        Log::info(logStream.str());
    } else {
        logStream.str("");
        logStream << "WebSocket connection failed to open.";
        Log::error(logStream.str());
    }

    m_psock->setBlocking(false);
    m_psock->setReceiveTimeout(Poco::Timespan(3, 0));
    m_psock->setSendTimeout(Poco::Timespan(3, 0));
    container = new Poco::Buffer<char>(msgSize);

    if (m_psock->poll(Poco::Timespan(1, 0), Poco::Net::WebSocket::SELECT_READ))
    {
        receive_buffer();
    }

    // Additional log message for debugging
    logStream.str("");
    logStream << "PocoHelper initialized.";
    Log::info(logStream.str());
}

PocoHelper::~PocoHelper()
{
    m_psock->close();
    delete m_psock;
    delete container;
    delete uri;
    delete request;
    delete cs;
}

void PocoHelper::send_buffer(char* buffer, unsigned int bufferLen)
{
    logStream.str("");
    logStream << "Buffer length before sending: " << bufferLen;
    Log::info(logStream.str());
    try
    {
        if (msgSize <= 0 && m_psock->poll(Poco::Timespan(2, 0), Poco::Net::WebSocket::SELECT_WRITE))
        {
            msgSize = 8000;

            logStream.str("");
            logStream << "Sending to deepgram";
            Log::info(logStream.str());

            Poco::Buffer<char>* tempBuffer = container;
            m_psock->sendFrame(tempBuffer->begin(), tempBuffer->size(), Poco::Net::WebSocket::FRAME_BINARY);

            // Print sent bytes to console
            logStream.str("");
            logStream << "Sent bytes: " << tempBuffer->size();
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
        std::stringstream logStream;
        logStream << "WebSocketException caught: " << ex.displayText();
    }
}

void PocoHelper::receive_buffer()
{
    try
    {
        char readMsg[256];
        int flags = 0;

        int rlen = m_psock->receiveFrame(&readMsg, 256, flags);

        // Log the received message to Poco Logger
        std::stringstream logStream;
        logStream << "Received message: " << readMsg;
        Log::info(logStream.str());
    }
    catch (const Poco::Exception& ex)
    {
        std::string msg(ex.what());
        std::stringstream logStream;
        logStream << "Exception: " << msg;
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
