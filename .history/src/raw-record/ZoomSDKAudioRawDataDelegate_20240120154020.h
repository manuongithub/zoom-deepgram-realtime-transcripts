// ZoomSDKAudioRawDataDelegate.h

#ifndef MEETING_SDK_LINUX_SAMPLE_ZOOMSDKAUDIORAWDATADELEGATE_H
#define MEETING_SDK_LINUX_SAMPLE_ZOOMSDKAUDIORAWDATADELEGATE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include "zoom_sdk_raw_data_def.h"
#include "rawdata/rawdata_audio_helper_interface.h"
#include "../util/Log.h"
#include "PocoHelper.h"

using namespace std;
using namespace ZOOMSDK;

class ZoomSDKAudioRawDataDelegate : public IZoomSDKAudioRawDataDelegate {
    string m_dir = "out";
    string m_filename = "test.pcm";
    bool m_useMixedAudio;
    bool m_initialized;
    string m_deepgramWebSocketURL;
    string m_dgApiKey;
    std::map<std::string, std::string> m_extraHeaders;
    PocoHelper m_pocoHelper;
    
    void writeToFile(const string& path, AudioRawData* data);
    void initializePocoHelper(int sampleRate, int channels);

public:
    ZoomSDKAudioRawDataDelegate(bool useMixedAudio);

    void setDeepgramApiKey(const std::string& apiKey);
    void setDir(const string& dir);
    void setFilename(const string& filename);

    void onMixedAudioRawDataReceived(AudioRawData* data) override;
    void onOneWayAudioRawDataReceived(AudioRawData* data, uint32_t node_id) override;
    void onShareAudioRawDataReceived(AudioRawData* data) override;
};

#endif //MEETING_SDK_LINUX_SAMPLE_ZOOMSDKAUDIORAWDATADELEGATE_H
