#include "PocoHelper.h"
#include <Poco/Thread.h>

PocoHelper::PocoHelper() : uri(nullptr), m_psock(nullptr), container(nullptr) {
    // Default constructor
}

PocoHelper::PocoHelper(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
    : uri(nullptr), m_psock(nullptr), container(nullptr) {
    initialize(wsEndPoint, extraHeaders, encoding, sampleRate, channels);
}

PocoHelper::~PocoHelper() {
    close();
}

void PocoHelper::initialize(std::string wsEndPoint, const std::map<std::string, std::string>& extraHeaders, const std::string& encoding, int sampleRate, int channels)
{
    std::stringstream logStream;
    uri = new Poco::URI(wsEndPoint);

    // Append query parameters to the WebSocket URL
    std::string queryString = "encoding=" + encoding + "&sample_rate=" + std::to_string(sampleRate) + "&channels=" + std::to_string(channels) + "&model=nova-2" + "&smart_format=true";
    uri->setQuery(queryString);

    logStream.str("");
    logStream << "WebSocket Query String: " << queryString;
    Log::info(logStream.str());

    try {
        Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", Poco::Net::Context::VERIFY_NONE, 9, true);

        Poco::Net::HTTPSClientSession cs(uri->getHost(), 443, context);

        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, uri->getPathEtc(), Poco::Net::HTTPMessage::HTTP_1_1);

        // Add extra headers
        for (const auto& header : extraHeaders) {
            request.set(header.first, header.second);
        }

        Poco::Net::HTTPResponse response;
        m_psock = new Poco::Net::WebSocket(cs, request, response);

        // Check if the socket is open
        if (m_psock->impl() && m_psock->impl()->initialized()) {
            // Check HTTP response status for successful authentication
            if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_SWITCHING_PROTOCOLS) {
                logStream.str("");
                logStream << "WebSocket connection opened successfully. Authentication successful.";
                Log::info(logStream.str());
            } else {
                logStream.str("");
                logStream << "WebSocket connection opened, but authentication failed. HTTP Status Code: " << response.getStatus();

                Log::error(logStream.str());
                // Handle authentication failure
                return;
            }
        } else {
            logStream.str("");
            logStream << "WebSocket connection failed to open.";
            Log::error(logStream.str());
            // Handle connection failure
            return;
        }

        m_psock->setBlocking(false);
        m_psock->setReceiveTimeout(Poco::Timespan(100, 0));
        m_psock->setSendTimeout(Poco::Timespan(100, 0));
        container = new Poco::Buffer<char>(4096);

        // Create two threads for sending and receiving
        Poco::Thread sendThread;
        Poco::Thread receiveThread;

        // Set thread names
        sendThread.setName("SendThread");
        receiveThread.setName("ReceiveThread");

        // Start the threads with appropriate member function pointers
        sendThread.start(sendThreadFunc, this);
        receiveThread.start(receiveThreadFunc, this);

        // Wait for both threads to finish (you might want to add a condition to break out of this loop)
        while (sendThread.isRunning() || receiveThread.isRunning()) {
            Poco::Thread::sleep(100);  // Adjust sleep duration as needed
        }
    } catch (const Poco::Exception& ex) {
        std::string msg(ex.what());
        logStream << "Exception during WebSocket initialization: " << msg;
        Log::error(logStream.str());
        // Handle exception
    }
}


static void PocoHelper::sendThreadFunc()
{
    while (true) {
        try {
            bool isSocketReady = m_psock->poll(Poco::Timespan(2, 0), Poco::Net::WebSocket::SELECT_WRITE);

            if (isSocketReady) {
                // Your existing send_buffer logic here
                // ...

                // Print received data to console
                // receive_buffer();
            }
        } catch (const Poco::Net::WebSocketException& ex) {
            std::string msg(ex.what());
            std::stringstream logStream;
            logStream << "WebSocketException Exception: " << msg;
            Log::error(logStream.str());
        }

        Poco::Thread::sleep(100);  // Sleep for a specified duration (adjust as needed)
    }
}

static void PocoHelper::receiveThreadFunc()
{
    while (true) {
        try {
            if (m_psock->poll(Poco::Timespan(1, 0), Poco::Net::WebSocket::SELECT_READ)) {
                // Your existing receive_buffer logic here
                // ...
            }

            Poco::Thread::sleep(100);  // Sleep for a specified duration (adjust as needed)
        } catch (const Poco::Net::WebSocketException& ex) {
            std::string msg(ex.what());
            std::stringstream logStream;
            logStream << "WebSocketException caught: " << msg;
            Log::error(logStream.str());
        } catch (const Poco::Exception& ex) {
            std::string msg(ex.what());
            std::stringstream logStream;
            logStream << "Exception: " << msg;
            Log::error(logStream.str());
        }
    }
}

void PocoHelper::send_buffer(char* buffer, unsigned int bufferLen) {
    try {
        bool isSocketReady = m_psock->poll(Poco::Timespan(2, 0), Poco::Net::WebSocket::SELECT_WRITE);

        if (isSocketReady) {
            // Send the raw buffer data to the socket immediately
            m_psock->sendFrame(buffer, bufferLen, Poco::Net::WebSocket::FRAME_BINARY);
            // Print received data to console
            //receive_buffer();
        }
    } catch (const Poco::Net::WebSocketException& ex) {
        std::string msg(ex.what());
        std::stringstream logStream;
        logStream << "WebSocketException Exception: " << msg;
        Log::error(logStream.str());
    }
}


void PocoHelper::receive_buffer() {
    std::stringstream logStream;
    logStream << "trying to receive messages from deepgram now";
    Log::info(logStream.str());
    try {
        const int bufferSize = 256;
        char readMsg[bufferSize];

        //std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long>(3 * 100)));

        int bytesRead = m_psock->receiveBytes(readMsg, bufferSize);

        if (bytesRead > 0) {
            // Log the received message to Poco Logger
            std::stringstream logStream;
            logStream << "Received message: " << std::string(readMsg, bytesRead);
            Log::info(logStream.str());
        } else if (bytesRead == 0) {
            // Connection closed gracefully
            Log::info("WebSocket connection got nothing back");
        } else {
            // Handle error or connection closure
            Log::error("Error receiving message from WebSocket.");
        }
    } catch (const Poco::Net::WebSocketException& ex) {
        std::string msg(ex.what());
        std::stringstream logStream;
        logStream << "WebSocketException caught: " << msg;
        Log::error(logStream.str());
    } catch (const Poco::Exception& ex) {
        std::string msg(ex.what());
        std::stringstream logStream;
        logStream << "Exception: " << msg;
        Log::error(logStream.str());
    }
}

void PocoHelper::close() {
    try {
        m_psock->shutdown();
    } catch (...) {
        std::stringstream logStream;
        logStream << "Closing failed." << std::endl;
    }
}
