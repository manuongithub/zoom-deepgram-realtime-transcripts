// ZoomSDKAudioRawDataDelegate.cpp
#include "ZoomSDKAudioRawDataDelegate.h"
#include "DeepgramWSHelper.h" // Updated include
#include "../Config.h"

ZoomSDKAudioRawDataDelegate::ZoomSDKAudioRawDataDelegate(bool useMixedAudio)
    : m_useMixedAudio(useMixedAudio),
      m_pocoHelper(),
      m_initialized(false)
{
    m_deepgramWebSocketURL = "wss://api.deepgram.com/v1/listen";
    m_extraHeaders = {{"Authorization", "Token " + m_dgApiKey}};
}

void ZoomSDKAudioRawDataDelegate::setDeepgramApiKey(const std::string& apiKey) {
    m_dgApiKey = apiKey;
    m_extraHeaders = {{"Authorization", "Token " + m_dgApiKey}};
}

void ZoomSDKAudioRawDataDelegate::initializePocoHelper(int sampleRate, int channelCount)
{
    if (!m_initialized) {
        m_pocoHelper.initialize(m_deepgramWebSocketURL, m_extraHeaders, "linear16", sampleRate, channelCount);
        m_initialized = true;
    }
}

void ZoomSDKAudioRawDataDelegate::onMixedAudioRawDataReceived(AudioRawData* data)
{
    if (!m_useMixedAudio)
        return;

    initializePocoHelper(data->GetSampleRate(), data->GetChannelNum());
    m_pocoHelper.send_buffer(data->GetBuffer(), data->GetBufferLen());
}

void ZoomSDKAudioRawDataDelegate::onOneWayAudioRawDataReceived(AudioRawData* data, uint32_t node_id)
{
    if (m_useMixedAudio)
        return;

    std::stringstream path;
    path << m_dir << "/node-" << node_id << ".pcm";
    writeToFile(path.str(), data);
}

void ZoomSDKAudioRawDataDelegate::onShareAudioRawDataReceived(AudioRawData* data)
{
    std::stringstream ss;
    ss << "Shared Audio Raw data: " << data->GetBufferLen() << "b at " << data->GetSampleRate() << "Hz";
    Log::info(ss.str());
}

void ZoomSDKAudioRawDataDelegate::writeToFile(const std::string& path, AudioRawData* data)
{
    static std::ofstream file;
    file.open(path, std::ios::out | std::ios::binary | std::ios::app);

    if (!file.is_open())
        return Log::error("failed to open audio file path: " + path);

    file.write(data->GetBuffer(), data->GetBufferLen());

    file.close();
    file.flush();

    std::stringstream ss;
    ss << "Writing " << data->GetBufferLen() << "b to " << path << " at " << data->GetSampleRate() << "Hz";
    Log::info(ss.str());
}

void ZoomSDKAudioRawDataDelegate::setDir(const std::string& dir)
{
    m_dir = dir;
}

void ZoomSDKAudioRawDataDelegate::setFilename(const std::string& filename)
{
    m_filename = filename;
}
