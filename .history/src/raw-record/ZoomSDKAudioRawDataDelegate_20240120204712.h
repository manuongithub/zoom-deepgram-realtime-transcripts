// ZoomSDKAudioRawDataDelegate.h
#ifndef ZOOMSDKAUDIORAWDATADELEGATE_H
#define ZOOMSDKAUDIORAWDATADELEGATE_H

#include <fstream>
#include <sstream>
#include "AudioRawData.h"
#include "../util/Log.h"

class ZoomSDKAudioRawDataDelegate {
public:
    ZoomSDKAudioRawDataDelegate(bool useMixedAudio);
    ~ZoomSDKAudioRawDataDelegate() = default;

    void setDeepgramApiKey(const std::string& apiKey);

    void initializePocoHelper(int sampleRate, int channelCount);

    void onMixedAudioRawDataReceived(AudioRawData* data);

    void onOneWayAudioRawDataReceived(AudioRawData* data, uint32_t node_id);

    void onShareAudioRawDataReceived(AudioRawData* data);

    void setDir(const std::string& dir);

    void setFilename(const std::string& filename);

private:
    void writeToFile(const std::string& path, AudioRawData* data);

    bool m_useMixedAudio;
    bool m_initialized;
    std::string m_dgApiKey;
    std::string m_deepgramWebSocketURL;
    std::map<std::string, std::string> m_extraHeaders;
    DeepgramWSHelper m_pocoHelper;
    std::string m_dir;
    std::string m_filename;
};

#endif // ZOOMSDKAUDIORAWDATADELEGATE_H
