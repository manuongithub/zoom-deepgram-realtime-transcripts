#include "PocoHelper.h"
#include <fstream> // Include the necessary header for file operations
#include <Poco/PatternFormatter.h>
#include <Poco/FormattingChannel.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/FileChannel.h>

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

    // Create a pattern formatter
    Poco::AutoPtr<Poco::PatternFormatter> pPatternFormatter = new Poco::PatternFormatter("%Y-%m-%d %H:%M:%S.%c %N[%P]:%s:%q:%t");

    // Create a channel for both console and file logging
    Poco::AutoPtr<Poco::ConsoleChannel> pConsoleChannel = new Poco::ConsoleChannel;
    Poco::AutoPtr<Poco::FileChannel> pFileChannel = new Poco::FileChannel("/tmp/meeting-sdk-linux-sample/out/received_data.txt");
    pFileChannel->setProperty("path", "/tmp/meeting-sdk-linux-sample/out/received_data.txt");

    // Create a formatting channel and set the channels
    Poco::AutoPtr<Poco::FormattingChannel> pFormattingChannel = new Poco::FormattingChannel(pPatternFormatter, pConsoleChannel);
    pFormattingChannel->setChannel(pFileChannel);

    // Set the channel for the logger
    logHelper.setChannel(pFormattingChannel);

    m_psock = new Poco::Net::WebSocket(*cs, *request, response);

    m_psock->setBlocking(false);
    m_psock->setReceiveTimeout(Poco::Timespan(3, 0));
    m_psock->setSendTimeout(Poco::Timespan(3, 0));
    container = new Poco::Buffer<char>(msgSize);

    if (m_psock->poll(Poco::Timespan(1, 0), Poco::Net::WebSocket::SELECT_READ))
    {
        receive_buffer();
    }
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

            std::cout << "Sending to deepgram";

            Poco::Buffer<char>* tempBuffer = container;
            m_psock->sendFrame(tempBuffer->begin(), tempBuffer->size(), Poco::Net::WebSocket::FRAME_BINARY);

            // Print sent bytes to console
            std::cout << "Sent bytes: " << tempBuffer->size() << std::endl;

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
            // Print exception message to console
            std::cerr << "Exception: " << ex.displayText() << std::endl;
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

        // Open a file for writing (you may need to adjust the file path)
        std::ofstream outputFile("/tmp/meeting-sdk-linux-sample/out/received_data.txt", std::ios::app);

        if (outputFile.is_open())
        {
            // Write the received message to the file
            outputFile << "Received message: " << readMsg << std::endl;

            // Close the file
            outputFile.close();
        }
        else
        {
            // Handle file opening failure
            std::cerr << "Error opening file for writing!" << std::endl;
        }

        // Also log the received message to Poco Logger
        logHelper.information(readMsg);
    }
    catch (const Poco::Exception& ex)
    {
        std::string msg(ex.what());
        logHelper.information("Exception: " + msg);
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
        logHelper.information("closing failed.");
    }
}
