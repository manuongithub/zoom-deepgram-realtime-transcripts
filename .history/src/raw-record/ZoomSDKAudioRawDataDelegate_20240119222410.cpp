#include "ZoomSDKAudioRawDataDelegate.h"
#include "PocoHelper.h"

ZoomSDKAudioRawDataDelegate::ZoomSDKAudioRawDataDelegate(bool useMixedAudio)
    : m_useMixedAudio(useMixedAudio), m_deepgramWebSocketURL("wss://api.deepgram.com/v1/listen"),
      m_dgApiKey("0bb0c4d0ce6bed9e9540336bdabfe08217d38530"),
      m_extraHeaders({{"Authorization", "Token " + m_dgApiKey}}),
      m_pocoHelper(),
      m_initialized(false) // Initialize the flag to false
{
}

void ZoomSDKAudioRawDataDelegate::initializePocoHelper()
{
    if (!m_initialized) {
        m_pocoHelper.initialize(m_deepgramWebSocketURL, m_extraHeaders, "linear16", 32000, 1);
        m_initialized = true;
    }
}


void ZoomSDKAudioRawDataDelegate::onMixedAudioRawDataReceived(AudioRawData* data)
{
    if (!m_useMixedAudio)
        return;

    /*if (m_dir.empty())
        return Log::error("Output Directory cannot be blank");

    if (m_filename.empty())
        m_filename = "test.pcm";

    std::stringstream path;
    // path << m_dir << "/" << m_filename;
    path << m_dir << "/tumba1.pcm";

    writeToFile(path.str(), data);*/

    initializePocoHelper(); // Ensure PocoHelper is initialized before calling send_buffer
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