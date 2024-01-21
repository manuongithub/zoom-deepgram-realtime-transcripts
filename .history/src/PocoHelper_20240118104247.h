#ifndef POCOHELPER_H
#define POCOHELPER_H

#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPMessage.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/NetException.h"
#include "Poco/Buffer.h"
#include "Poco/URI.h"
#include "Poco/Logger.h"

extern "C"
{
#include "binn.h"
}

class PocoHelper
{
    WebSocket* m_psock;
    HTTPSClientSession* cs;
    HTTPRequest* request;
    HTTPResponse response;
    Logger& logHelper = Logger::get("TestLogger");
    int msgSize = 8000;
    Buffer<char>* container;
    Poco::URI* uri;

public:
    PocoHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders = {}, const std::string& encoding = "S16LE", int sampleRate = 32000, int channels = 1);

    ~PocoHelper();

    void send_buffer(char* buffer, unsigned int bufferLen);

    void receive_buffer();

    void close();
};

#endif // POCOHELPER_H