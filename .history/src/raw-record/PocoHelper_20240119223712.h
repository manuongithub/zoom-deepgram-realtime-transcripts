#ifndef POCOHELPER_H
#define POCOHELPER_H

#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include "Poco/Net/NetException.h"
#include <Poco/URI.h>
#include <Poco/Buffer.h>
#include <Poco/Logger.h>
#include <map>
#include <sstream> // Add this line
#include "../util/Log.h"

class PocoHelper
{
public:
    PocoHelper(); // Default constructor
    PocoHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels);
    ~PocoHelper();

    void initialize(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels);

    void send_buffer(char* buffer, unsigned int bufferLen);
    void receive_buffer();
    void close();

private:
    Poco::URI* uri;
    Poco::Net::HTTPSClientSession* cs;
    Poco::Net::HTTPRequest* request;
    Poco::Net::HTTPResponse response;
    Poco::Net::WebSocket* m_psock;
    Poco::Buffer<char>* container;
    int msgSize = 2048;
    std::stringstream logStream;
};

#endif // POCOHELPER_H
