#include "Poco/Net/HTTPRequest.h"
#include <Poco/Net/HTTPSClientSession.h>
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPMessage.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/NetException.h"
#include "Poco/Buffer.h"
#include "Poco/URI.h"
#include "Poco/Logger.h"
#include "PocoHelper.h"

extern "C"
{
#include "binn.h"
#pragma comment(lib,"binn.lib")
}

using Poco::Net::WebSocketException;
using Poco::Net::HTTPSClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::WebSocket;
using Poco::Buffer;
using Poco::Logger; 

class PocoHelper
{
    WebSocket* m_psock;
    HTTPSClientSession* cs;
    HTTPRequest* request;
    HTTPResponse response;
    Logger& logHelper;

    int msgSize = 8000;
    Buffer<char>* container;
    Poco::URI* uri;

public:
    PocoHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders = {}, const std::string& encoding = "S16LE", int sampleRate = 32000, int channels = 1)
        : logHelper(Logger::get("TestLogger"))
    {
        uri = new Poco::URI(wsEndPoint);

        // Append query parameters to the WebSocket URL
        std::string queryString = "?encoding=" + encoding + "&sample_rate=" + std::to_string(sampleRate) + "&channels=" + std::to_string(channels);
        uri->setQuery(queryString);

        cs = new HTTPSClientSession(uri->getHost(), 443);

        request = new HTTPRequest(HTTPRequest::HTTP_GET, uri->getPathEtc(), HTTPMessage::HTTP_1_1);

        // Add extra headers
        for (const auto& header : extraHeaders) {
            request->set(header.first, header.second);
        }

        m_psock = new WebSocket(*cs, *request, response);

        m_psock->setBlocking(false);
        m_psock->setReceiveTimeout(Poco::Timespan(3, 0));
        m_psock->setSendTimeout(Poco::Timespan(3, 0));
        container = new Buffer<char>(msgSize);

        if (m_psock->poll(Poco::Timespan(1, 0), Poco::Net::WebSocket::SelectMode::SELECT_READ))
        {
            receive_buffer();
        }
    }

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
        try
        {
            if (msgSize <= 0 && m_psock->poll(Poco::Timespan(2, 0), Poco::Net::WebSocket::SelectMode::SELECT_WRITE))
            {
                msgSize = 8000;

                Buffer<char>* tempBuffer = container;
                m_psock->sendFrame(tempBuffer->begin(), tempBuffer->size(), WebSocket::FRAME_BINARY);

                logHelper.information("Sent bytes: " + std::to_string(tempBuffer->size()));

                container = new Buffer<char>(msgSize);
            }

            if (m_psock->poll(Poco::Timespan(1, 0), Poco::Net::WebSocket::SelectMode::SELECT_READ))
            {
                receive_buffer();
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
            case WebSocket::ErrorCodes::WS_ERR_UNAUTHORIZED:
                logHelper.information("Unauthorized socket!");
                break;
            case WebSocket::ErrorCodes::WS_ERR_PAYLOAD_TOO_BIG:
                logHelper.information("Payload too big...");
                break;
            case WebSocket::ErrorCodes::WS_ERR_INCOMPLETE_FRAME:
                logHelper.information("Incomplete Frame");
                break;
            default:
                logHelper.information("Exception: " + ex.displayText());
                break;
            }
        }
    }

    void receive_buffer()
    {
        try
        {
            char readMsg[256];
            int flags = 0;

            int rlen = m_psock->receiveFrame(&readMsg, 256, flags);
            logHelper.information(readMsg);
        }
        catch (const Poco::Exception& ex)
        {
            std::string msg(ex.what());
            logHelper.information("Exception: " + msg);
        }
    }

    void close() {
        try {
            m_psock->shutdown();
        }
        catch (...) {
            logHelper.information("closing failed.");
        }
    }
};
