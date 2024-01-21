#include "PocoHelper.h"
#include <Poco/ConsoleChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/AutoPtr.h>
#include <Poco/Util/Application.h>
#include <iostream>
#include <sstream>

PocoHelper::PocoHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
    : logStream(std::stringstream())
{
    uri = new Poco::URI(wsEndPoint);

    // Append query parameters to the WebSocket URL
    std::string queryString = "?encoding=" + encoding + "&sample_rate=" + std::to_string(sampleRate) + "&channels=" + std::to_string(channels);
    uri->setQuery(queryString);

    Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", Poco::Net::Context::VERIFY_NONE, 9, true);

    cs = new Poco::Net::HTTPSClientSession(uri->getHost(), 443, context);

    request = new Poco::Net::HTTPRequest(Poco::Net::HTTPRequest::HTTP_GET, uri->getPathEtc(), Poco::Net::HTTPMessage::HTTP_1_1);

    // Add extra headers
    for (const auto& header : extraHeaders) {
        request->set(header.first, header.second);
    }

    m_psock = new Poco::Net::WebSocket(*cs, *request, response);

    m_psock->setBlocking(false);
    m_psock->setReceiveTimeout(Poco::Timespan(3, 0));
    m_psock->setSendTimeout(Poco::Timespan(3, 0));
    container = new Poco::Buffer<char>(msgSize);

    if (m_psock->poll(Poco::Timespan(1, 0), Poco::Net::WebSocket::SELECT_READ))
    {
        receive_buffer();
    }

    // Configure Poco Logger
    Poco::AutoPtr<Poco::PatternFormatter> pPatternFormatter = new Poco::PatternFormatter("%Y-%m-%d %H:%M:%S.%c %N[%P]:%s:%q:%t");
    Poco::AutoPtr<Poco::ConsoleChannel> pConsoleChannel = new Poco::ConsoleChannel;

    // Create Poco::FileChannel
    Poco::AutoPtr<Poco::FileChannel> pFileChannel = new Poco::FileChannel("/tmp/meeting-sdk-linux-sample/out/received_data.txt");
    pFileChannel->setProperty("path", "/tmp/meeting-sdk-linux-sample/out/received_data.txt");

    // Check file channel creation
    if (pFileChannel)
    {
        logStream << "File channel created." << std::endl;
    }
    else
    {
        logStream << "Error creating file channel!" << std::endl;
    }

    // Create Poco::FormattingChannel
    Poco::AutoPtr<Poco::FormattingChannel> pFormattingChannel = new Poco::FormattingChannel(pPatternFormatter, pConsoleChannel);

    // Set file channel for formatting channel
    if (pFormattingChannel)
    {
        pFormattingChannel->setChannel(pFileChannel);
        logStream << "Formatting channel created and set." << std::endl;
    }
    else
    {
        logStream << "Error creating formatting channel!" << std::endl;
    }

    // Set formatting channel for logger
    // Additional log message for debugging
    logStream << "PocoHelper initialized." << std::endl;
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
    logStream << "inside send_buffer" << std::endl;
    try
    {
        if (msgSize <= 0 && m_psock->poll(Poco::Timespan(2, 0), Poco::Net::WebSocket::SELECT_WRITE))
        {
            msgSize = 8000;

            logStream << "Sending to deepgram" << std::endl;

            // Send the raw audio data directly
            m_psock->sendFrame(buffer, bufferLen, Poco::Net::WebSocket::FRAME_BINARY);

            // Print sent bytes to console
            logStream << "Sent bytes: " << bufferLen << std::endl;

            // Print received data to console
            receive_buffer();

            container = new Poco::Buffer<char>(msgSize);
        }

        if (buffer)
        {
            // Append the raw audio data to the container
            container->append(buffer, bufferLen);
            msgSize -= bufferLen;
        }
    }
    catch (const Poco::Net::WebSocketException& ex)
    {
        switch (ex.code())
        {
        case Poco::Net::WebSocket::ErrorCodes::WS_ERR_UNAUTHORIZED:
            logStream << "Unauthorized socket!" << std::endl;
            break;
        case Poco::Net::WebSocket::ErrorCodes::WS_ERR_PAYLOAD_TOO_BIG:
            logStream << "Payload too big..." << std::endl;
            break;
        case Poco::Net::WebSocket::ErrorCodes::WS_ERR_INCOMPLETE_FRAME:
            logStream << "Incomplete Frame" << std::endl;
            break;
        default:
            // Print exception message to console
            logStream << "Exception: " << ex.displayText() << std::endl;
            break;
        }
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
        logStream << "Received message: " << std::string(readMsg) << std::endl;
    }
    catch (const Poco::Exception& ex)
    {
        std::string msg(ex.what());
        logStream << "Exception: " << msg << std::endl;
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
        logStream << "Closing failed." << std::endl;
    }
}
