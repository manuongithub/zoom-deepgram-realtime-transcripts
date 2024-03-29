// DeepgramWSHelper.h
#ifndef DEEPGRAMWSHELPER_H
#define DEEPGRAMWSHELPER_H

#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/SocketReactor.h>
#include <Poco/Net/SocketNotification.h>
#include <Poco/Net/NetException.h>
#include <Poco/URI.h>
#include <Poco/Buffer.h>
#include <Poco/Logger.h>
#include <map>
#include <sstream>
#include "../util/Log.h"
#include <Poco/Thread.h>

class DeepgramWSHelper : public Poco::Runnable {
public:
    DeepgramWSHelper(); 
    DeepgramWSHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels);
    ~DeepgramWSHelper();

    void initialize(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels);

    void send_buffer(char* buffer, unsigned int bufferLen);
    void receive_buffer();
    void close();

    void run() override;

private:
    Poco::URI* uri;
    Poco::Net::HTTPSClientSession* cs;
    Poco::Net::HTTPRequest* request;
    Poco::Net::HTTPResponse response;
    Poco::Thread reactorThread;
    Poco::Net::WebSocket* m_psock;
    Poco::Net::SocketReactor* reactor;

    void runReactor();
    void onSocketReadable(Poco::Net::ReadableNotification* pNf);

    std::stringstream logStream;
};

#endif // DEEPGRAMWSHELPER_H
