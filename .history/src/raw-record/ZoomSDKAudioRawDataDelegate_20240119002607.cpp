#include "ZoomSDKAudioRawDataDelegate.h"
#include "PocoHelper.h"

ZoomSDKAudioRawDataDelegate::ZoomSDKAudioRawDataDelegate(bool useMixedAudio) : m_useMixedAudio(useMixedAudio)
{

}

void ZoomSDKAudioRawDataDelegate::onMixedAudioRawDataReceived(AudioRawData *data) {
    if (!m_useMixedAudio) return;

    if (m_dir.empty())
        return Log::error("Output Directory cannot be blank");
    

    if (m_filename.empty()) 
        m_filename = "test.pcm";
    

    stringstream path;
    path << m_dir << "/" << m_filename;

    writeToFile(path.str(), data);

    // Replace 'YOUR_DEEPGRAM_WEBSOCKET_URL' and 'YOUR_DG_API_KEY' with actual values
    std::string deepgramWebSocketURL = "wss://api.deepgram.com/v1/listen";
    std::string dgApiKey = "0bb0c4d0ce6bed9e9540336bdabfe08217d38530";

    std::map<std::string, std::string> extraHeaders = {
        {"Authorization", "Token " + dgApiKey}
    };

    stringstream ss;
    ss << "Writing " << data->GetBufferLen() << " to " << path << " deepgram at " << data->GetSampleRate() << "Hz";

    PocoHelper pocoHelper(deepgramWebSocketURL, extraHeaders, "linear16" ,data->GetSampleRate(), data->getChannelNum());
    pocoHelper.send_buffer(data->GetBuffer(), data->GetBufferLen());
}



void ZoomSDKAudioRawDataDelegate::onOneWayAudioRawDataReceived(AudioRawData* data, uint32_t node_id) {
    if (m_useMixedAudio) return;

    stringstream path;
    path << m_dir << "/node-" << node_id << ".pcm";
    writeToFile(path.str(), data);
}

void ZoomSDKAudioRawDataDelegate::onShareAudioRawDataReceived(AudioRawData* data) {
    stringstream ss;
    ss << "Shared Audio Raw data: " << data->GetBufferLen() << "b at " << data->GetSampleRate() << "Hz";
    Log::info(ss.str());
}


void ZoomSDKAudioRawDataDelegate::writeToFile(const string &path, AudioRawData *data)
{
    static std::ofstream file;
	file.open(path, std::ios::out | std::ios::binary | std::ios::app);

	if (!file.is_open())
        return Log::error("failed to open audio file path: " + path);
	
    file.write(data->GetBuffer(), data->GetBufferLen());

    file.close();
	file.flush();

    stringstream ss;
    ss << "Writing " << data->GetBufferLen() << "b to " << path << " at " << data->GetSampleRate() << "Hz";

    Log::info(ss.str());
}

void ZoomSDKAudioRawDataDelegate::setDir(const string &dir)
{
    m_dir = dir;
}

void ZoomSDKAudioRawDataDelegate::setFilename(const string &filename)
{
    m_filename = filename;
}
