#include "PocoHelper.h"
#include <fstream>
#include <Poco/ConsoleChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/AutoPtr.h>

PocoHelper::PocoHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
    : logHelper(Poco::Logger::get("TestLogger"))
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

    // Use Poco::FileChannel instead of Poco::Util::FileChannel
    Poco::AutoPtr<Poco::FileChannel> pFileChannel = new Poco::FileChannel("/tmp/meeting-sdk-linux-sample/out/received_data.txt");
    pFileChannel->setProperty("path", "/tmp/meeting-sdk-linux-sample/out/received_data.txt");

    Poco::AutoPtr<Poco::FormattingChannel> pFormattingChannel = new Poco::FormattingChannel(pPatternFormatter, pConsoleChannel);
    pFormattingChannel->setChannel(pFileChannel);

    // Use setChannel method without a channel name
    logHelper.setChannel(pFormattingChannel);
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
    try
    {
        if (msgSize <= 0 && m_psock->poll(Poco::Timespan(2, 0), Poco::Net::WebSocket::SELECT_WRITE))
        {
            msgSize = 8000;

            logHelper.information("Sending to deepgram");

            Poco::Buffer<char>* tempBuffer = container;
            m_psock->sendFrame(tempBuffer->begin(), tempBuffer->size(), Poco::Net::WebSocket::FRAME_BINARY);

            // Log sent bytes
            logHelper.information("Sent bytes: " + std::to_string(tempBuffer->size()));

            // Log received data
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
        switch (ex.code())
        {
        case Poco::Net::WebSocket::ErrorCodes::WS_ERR_UNAUTHORIZED:
            logHelper.information("Unauthorized socket!");
            break;
        case Poco::Net::WebSocket::ErrorCodes::WS_ERR_PAYLOAD_TOO_BIG:
            logHelper.information("Payload too big...");
            break;
        case Poco::Net::WebSocket::ErrorCodes::WS_ERR_INCOMPLETE_FRAME:
            logHelper.information("Incomplete Frame");
            break;
        default:
            // Log exception message
            logHelper.error("Exception: " + ex.displayText());
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
        logHelper.information("Received message: " + std::string(readMsg));
    }
    catch (const Poco::Exception& ex)
    {
        // Log exception message
        logHelper.error("Exception: " + std::string(ex.what()));
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
        logHelper.information("Closing failed.");
    }
}