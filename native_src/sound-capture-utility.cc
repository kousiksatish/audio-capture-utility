#include "../headers/sound-capture-utility.h"
#include <chrono>
#include <memory>

Napi::FunctionReference SoundCaptureUtility::constructor;

Napi::Object SoundCaptureUtility::Init(Napi::Env env, Napi::Object exports)
{
    Napi::HandleScope scope(env);
    Napi::Function func = DefineClass(env, "SoundCaptureUtility", {
        InstanceMethod("startListener", &SoundCaptureUtility::StartListener),
        InstanceMethod("stopListener",  &SoundCaptureUtility::StopListener)
    }); 
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("SoundCaptureUtility", func);
    return exports;
}

SoundCaptureUtility::SoundCaptureUtility(const Napi::CallbackInfo& info): Napi::ObjectWrap<SoundCaptureUtility>(info)
{
    _encoder = new AudioEncoder();
    isClosing = false;
}

void SoundCaptureUtility::StartListener(const Napi::CallbackInfo& info)
{
    // Tsfn related
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction())
        throw Napi::TypeError::New( env, "Expects a single function type argument" );

    tsfn = Napi::ThreadSafeFunction::New(env, info[0].As<Napi::Function>(), "SoundCaptureUtilityTsfn", 0, 1);

    _encoder->init();

    // Thread for doing continuous processing
    //nativeThread = std::thread( [this] {
    nativeThread = std::thread( [&] {
        auto callback = [] (Napi::Env env, Napi::Function jsCallback, AVPacket* packet) {
            if (packet != NULL) {
                Napi::Buffer<uint8_t> encoded_audio = Napi::Buffer<uint8_t>::New(env, packet->data, packet->size);
                Napi::Number pts = Napi::Number::New(env, packet->pts);
                jsCallback.Call({Napi::String::New(env, "data"), encoded_audio, pts});
                if (packet)
                    av_packet_free(&packet);        // it calls av_packet_unref(), deallocates memory abd sets packet pointer to null
            } else {
                printf("NULL packet");
            }
            
        };

        while(!isClosing) {
            AVPacket* pkt = _encoder->GetNextFrame();
            napi_status status = this->tsfn.NonBlockingCall(pkt, callback);    
            if (napi_ok != status)
                fprintf(stderr, "Error after calling tsfn at C++: '%d'",status);
            // std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (napi_ok != tsfn.Release())
            fprintf(stderr, "error releasing tsfn for linux audio capturer");
    });
}

void SoundCaptureUtility::StopListener(const Napi::CallbackInfo& info)
{
    isClosing = true;
    nativeThread.join();
    delete _encoder;
    _encoder = NULL;
}

SoundCaptureUtility::~SoundCaptureUtility()
{
    if (_encoder != NULL) {
        isClosing = true;
        nativeThread.join();
        delete _encoder;
        _encoder = NULL;
    }
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports)
{
	return SoundCaptureUtility::Init(env, exports);
}

NODE_API_MODULE(sound_capture_utility, InitAll);
