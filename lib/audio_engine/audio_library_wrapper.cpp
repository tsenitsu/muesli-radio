module audio_library_wrapper;

namespace audio_engine::audio_library_wrapper {

AudioLibraryWrapper::AudioLibraryWrapper(const LogCallback& logCallback)
 :  m_logCallback { logCallback },
    m_audioCallback { nullptr },
    m_inputAudioBuffer { nullptr },
    m_outputAudioBuffer { nullptr } {}

}
