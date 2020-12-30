// Use the newer ALSA API
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
extern "C"
{
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
}
#include <stdio.h>
#include <stdint.h>
#include <iostream>

#define RES_NOT_MUL_OF_TWO 1
#define COULD_NOT_FIND_VID_CODEC 2
#define CONTEXT_CREATION_ERROR 3
#define COULD_NOT_OPEN_VID_CODEC 4
#define COULD_NOT_OPEN_FILE 5
#define COULD_NOT_ALLOCATE_FRAME 6
#define COULD_NOT_ALLOCATE_PIC_BUF 7
#define ERROR_ENCODING_FRAME_SEND 8
#define ERROR_ENCODING_FRAME_RECEIVE 9
#define COULD_NOT_FIND_AUD_CODEC 10
#define COULD_NOT_OPEN_AUD_CODEC 11
#define COULD_NOT_ALL_RESMPL_CONTEXT 12
#define FAILED_TO_INIT_RESMPL_CONTEXT 13
#define COULD_NOT_ALLOC_SAMPLES 14
#define COULD_NOT_CONVERT_AUD 15
#define ERROR_ENCODING_SAMPLES_SEND 16
#define ERROR_ENCODING_SAMPLES_RECEIVE 17

class AudioEncoder
{
    public:
        AudioEncoder();
        ~AudioEncoder();
        void init();
        AVPacket *GetNextFrame();

    private:
        int aud_frame_counter;
        AVCodecContext *aud_codec_context;
        AVFrame *aud_frame;

        // Capturing related
        snd_pcm_t *handle;
        char* buffer;

        // Resampling related
        struct SwrContext *swr_ctx;
        uint8_t **src_data;
        uint8_t **dst_data;

        // Capturing related
        snd_pcm_uframes_t frames = 1024;
        int size;

        // Resampling related
        int src_nb_samples;
        int dst_nb_samples;

        AVPacket *pkt;

        int init_capturer(snd_pcm_t **handle,
                          snd_pcm_uframes_t frames,
                          char **buffer,
                          int *size);
        void close_capturer(snd_pcm_t **handle,
                            char** buffer);
        int init_resampler(struct SwrContext **swr_ctx,
                           int *src_nb_samples, uint8_t ***src_data,
                           int *dst_nb_samples, uint8_t ***dst_data);
        int initialize_encoding_audio(const char *filename);
        AVPacket* encode_audio_samples(uint8_t **aud_samples);
        int finish_audio_encoding();
        void cleanup();
};