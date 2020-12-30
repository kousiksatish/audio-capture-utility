#include <napi.h>
#include <thread>
#include "../headers/audio-encoder.h"

class SoundCaptureUtility : public Napi::ObjectWrap<SoundCaptureUtility>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    SoundCaptureUtility(const Napi::CallbackInfo &info);
    ~SoundCaptureUtility();

private:
    static Napi::FunctionReference constructor;
    std::thread nativeThread;
    Napi::ThreadSafeFunction tsfn;
    bool isClosing;

    void StopListener(const Napi::CallbackInfo &info);
    void StartListener(const Napi::CallbackInfo &info);

    AudioEncoder *_encoder;
};