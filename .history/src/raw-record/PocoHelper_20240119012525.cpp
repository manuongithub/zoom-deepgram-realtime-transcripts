#include <Poco/ConsoleChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/AutoPtr.h>
#include <sstream>

class PocoHelper
{
private:
    Poco::URI* uri;
    Poco::Net::HTTPSClientSession* cs;
    Poco::Net::HTTPRequest* request;
    Poco::Net::WebSocket* m_psock;
    Poco::Buffer<char>* container;
    Poco::Net::HTTPResponse response;
    int msgSize;

    PocoHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
    {
        std::stringstream logStream;
        uri = new Poco::URI(wsEndPoint);

        std::string queryString = "?encoding=" + encoding + "&sample_rate=" + std::to_string(sampleRate) + "&channels=" + std::to_string(channels);
        uri->setQuery(queryString);

        logStream.str("");
        logStream << "WebSocket Query String: " << queryString;
        Log::info(logStream.str());

        Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", Poco::Net::Context::VERIFY_NONE, 9, true);

        cs = new Poco::Net::HTTPSClientSession(uri->getHost(), 443, context);

        request = new Poco::Net::HTTPRequest(Poco::Net::HTTPRequest::HTTP_GET, uri->getPathEtc(), Poco::Net::HTTPMessage::HTTP_1_1);

        for (const auto& header : extraHeaders) {
            request->set(header.first, header.second);
        }

        m_psock = new Poco::Net::WebSocket(*cs, *request, response);

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
        m_psock->setReceiveTimeout(Poco::Timespan(3, 0));
        m_psock->setSendTimeout(Poco::Timespan(3, 0));
        container = new Poco::Buffer<char>(640);

        if (m_psock->poll(Poco::Timespan(1, 0), Poco::Net::WebSocket::SELECT_READ)) {
            receive_buffer();
        }

        msgSize = 0; // Initialize msgSize
    }

public:
    static PocoHelper& getInstance(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
    {
        static PocoHelper instance(wsEndPoint, extraHeaders, encoding, sampleRate, channels);
        return instance;
    }

    PocoHelper(const PocoHelper&) = delete;
    PocoHelper& operator=(const PocoHelper&) = delete;

    ~PocoHelper()
    {
        m_psock->close();
        delete m_psock;
        delete container;
        delete uri;
        delete request;
        delete cs;
    }

    void send_buffer(char* buffer, unsigned int bufferLen)
    {
        std::stringstream logStream;
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

                msgSize = 640;

                logStream.str("");
                logStream << "Sending to deepgram";
                Log::info(logStream.str());

                Poco::Buffer<char>* tempBuffer = container;
                m_psock->sendFrame(tempBuffer->begin(), tempBuffer->size(), Poco::Net::WebSocket::FRAME_BINARY);

                logStream.str("");
                logStream << "Sent bytes: " << tempBuffer->size();
                Log::info(logStream.str());

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
            logStream.str("");
            logStream << "WebSocketException caught: " << ex.displayText();
            Log::error(logStream.str());
        }
    }

    void receive_buffer()
    {
        try
        {
            char readMsg[256];
            int flags = 0;

            int rlen = m_psock->receiveFrame(&readMsg, 256, flags);

            std::stringstream logStream;
            logStream << "Received message: " << readMsg;
            Log::info(logStream.str());
        }
        catch (const Poco::Exception& ex)
        {
            std::string msg(ex.what());
            std::stringstream logStream;
            logStream << "Exception: " << msg;
            Log::error(logStream.str());
        }
    }

    void close()
    {
        try
        {
            m_psock->shutdown();
        }
        catch (...)
        {
            std::stringstream logStream;
            logStream << "Closing failed." << std::endl;
            Log::error(logStream.str());
        }
    }
};
