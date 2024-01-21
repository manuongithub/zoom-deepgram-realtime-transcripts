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

    const string& m_version = "1.0.2";
    const string& m_name = "Zoom Meeting SDK for Linux Sample v" + m_version;

    CLI::App m_app;

    CLI::App* m_rawRecordAudioCmd;
    string m_audioDir="out";
    string m_audioFile;
    bool m_separateParticipantAudio;

    CLI::App* m_rawRecordVideoCmd;
    string m_videoDir="out";
    string m_videoFile;

    string m_joinUrl;
    string m_meetingId;
    string m_password;
    string m_displayName = "Zoom Meeting Bot";

    string m_clientId;
    string m_clientSecret;

    string m_zoomHost = "https://zoom.us";
    string m_joinToken;

    bool m_isMeetingStart;

    // Add this member variable to store deepgram-api-key
    string m_deepgramApiKey;

public:
    Config();

    int read(int ac, char **av);

    // Add this method to read deepgram-api-key from the config file
    void readDeepgramApiKeyFromConfig();

    bool parseUrl(const string& join_url);

    const string& meetingId() const;
    const string& password() const;
    const string& displayName() const;

    const string& joinToken() const;
    const string& joinUrl() const;

    const string& clientId() const;
    const string& clientSecret() const;

    const string& zoomHost() const;

    // Add this method to get deepgram-api-key
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

Config::Config() :
    m_app(m_name, "zoomsdk"),
    m_rawRecordAudioCmd(m_app.add_subcommand("RawAudio", "Enable Audio Raw Recording")),
    m_rawRecordVideoCmd(m_app.add_subcommand("RawVideo", "Enable Video Raw Recording"))
{
    m_app.set_config("--config")->default_str("config.ini");

    m_app.add_option("-m, --meeting-id", m_meetingId, "Meeting ID of the meeting");
    m_app.add_option("-p, --password", m_password, "Password of the meeting");
    m_app.add_option("-n, --display-name", m_displayName, "Display Name for the meeting")->capture_default_str();

    m_app.add_option("--host", m_zoomHost, "Host Domain for the Zoom Meeting")->capture_default_str();
    m_app.add_option("-u, --join-url", m_joinUrl, "Join or Start a Meeting URL");
    m_app.add_option("-t, --join-token", m_joinToken, "Join the meeting with App Privilege using a token");

    m_app.add_option("--client-id", m_clientId, "Zoom Meeting Client ID")->required();
    m_app.add_option("--client-secret", m_clientSecret, "Zoom Meeting Client Secret")->required();

    m_app.add_flag("-s, --start", m_isMeetingStart, "Start a Zoom Meeting");

    m_rawRecordAudioCmd->add_option("-f, --file", m_audioFile, "Output PCM audio file");
    m_rawRecordAudioCmd->add_option("-d, --dir", m_audioDir, "Audio Output Directory");
    m_rawRecordAudioCmd->add_flag("-s, --separate-participants", m_separateParticipantAudio, "Output to separate PCM files for each participant");

    m_rawRecordVideoCmd->add_option("-f, --file", m_videoFile, "Output YUV video file");
    m_rawRecordVideoCmd->add_option("-d, --dir", m_videoDir, "Video Output Directory");
}

int Config::read(int ac, char **av) {
    try {
        m_app.parse(ac, av);
    } catch( const CLI::CallForHelp &e ){
        exit(m_app.exit(e));
    } catch (const CLI::ParseError& err) {
        return m_app.exit(err);
    } 

    if (!m_joinUrl.empty())
        parseUrl(m_joinUrl);

    // Read deepgram-api-key from config file
    //readDeepgramApiKeyFromConfig();

    return 0;
}

// Add this method to read deepgram-api-key from the config file
std::optional<std::string> Config::readDeepgramApiKeyFromConfig() {
    std::ifstream configFile("config.ini");
    if (!configFile.is_open()) {
        // Handle error: unable to open config file
        return std::nullopt;
    }

    std::string line;
    while (std::getline(configFile, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            if (key == "deepgram-api-key") {
                m_deepgramApiKey = value;
                return value;  // Optionally return the value if needed
            }
        }
    }

    // Handle error: key not found
    return std::nullopt;
}

bool Config::parseUrl(const string& join_url) {
    auto url = ada::parse<ada::url>(join_url);

    if (!url) {
        cerr << "unable to parse join URL" << endl;
        return false;
    }

    string token, lastRoute;
    istringstream ss(static_cast<string>(url->get_pathname()));

    while (getline(ss, token, '/')) {
        if(token.empty()) continue;

        m_isMeetingStart = token == "s";

        if (lastRoute == "j" || lastRoute == "s") {
            m_meetingId = token;
            break;
        }

        lastRoute = token;
    }

    if (m_meetingId.empty()) return false;

    ada::url_search_params search(url->get_search());
    if (!search.has("pwd")) return false;
    m_password = move(*search.get(string_view("pwd")));

    return true;
}

const string& Config::meetingId() const {
    return m_meetingId;
}

const string& Config::password() const {
    return m_password;
}

const string& Config::displayName() const {
    return m_displayName;
}

const string& Config::joinToken() const {
    return m_joinToken;
}

const string& Config::joinUrl() const {
    return m_joinUrl;
}

const string& Config::clientId() const {
    return m_clientId;
}

const string& Config::clientSecret() const {
    return m_clientSecret;
}

const string& Config::zoomHost() const {
    return m_zoomHost;
}

// Add this method to get deepgram-api-key
const string& Config::deepgramApiKey() const {
    return m_deepgramApiKey;
}

bool Config::isMeetingStart() const {
    return m_isMeetingStart;
}

bool Config::useRawRecording() const {
    return useRawAudio() || useRawVideo();
}

bool Config::useRawAudio() const {
    return !m_audioFile.empty() || m_separateParticipantAudio;
}

bool Config::useRawVideo() const {
    return !m_videoFile.empty();
}

const string& Config::audioFile() const {
    return m_audioFile;
}

const string& Config::videoFile() const {
    return m_videoFile;
}

const string& Config::audioDir() const {
    return m_audioDir;
}

const string& Config::videoDir() const {
    return m_videoDir;
}

bool Config::separateParticipantAudio() const {
    return m_separateParticipantAudio;
}
