#ifndef MEETING_SDK_LINUX_SAMPLE_CONFIG_H
#define MEETING_SDK_LINUX_SAMPLE_CONFIG_H

#include <iostream>
#include <fstream>
#include <iterator>
#include <codecvt>
#include <algorithm>
#include <locale>
#include <string>
#include <ada.h>
#include <CLI/CLI.hpp>

using namespace std;
using namespace ada;

class Config {

    const string m_version = "1.0.2";
    const string m_name = "Zoom Meeting SDK for Linux Sample v" + m_version;

    CLI::App m_app;

    CLI::App* m_rawRecordAudioCmd;
    string m_audioDir = "out";
    string m_audioFile;
    bool m_separateParticipantAudio;

    CLI::App* m_rawRecordVideoCmd;
    string m_videoDir = "out";
    string m_videoFile;

    string m_joinUrl;
    string m_meetingId;
    string m_password;
    string m_displayName = "Zoom Meeting Bot";

    string m_clientId;
    string m_clientSecret;

    string m_zoomHost = "https://zoom.us";
    string m_joinToken;

    // Addition of deepgram-api-key
    string m_deepgramApiKey;

    bool m_isMeetingStart;

public:
    Config();

    int read(int ac, char **av);

    // Add this method to read deepgram-api-key from the config file
    std::optional<std::string> readDeepgramApiKeyFromConfig();

    bool parseUrl(const string& join_url);

    const string& meetingId() const;
    const string& password() const;
    const string& displayName() const;

    const string& joinToken() const;
    const string& joinUrl() const;

    const string& clientId() const;
    const string& clientSecret() const;

    const string& zoomHost() const;

    // Getter for deepgram-api-key
    const string& deepgramApiKey() const;

    bool isMeetingStart() const;

    bool useRawRecording() const;

    bool useRawAudio() const;
    bool useRawVideo() const;

    const string& audioFile() const;
    const string& videoFile() const;

    const string& audioDir() const;
    const string& videoDir() const;

    bool separateParticipantAudio() const;
};

#endif //MEETING_SDK_LINUX_SAMPLE_CONFIG_H
