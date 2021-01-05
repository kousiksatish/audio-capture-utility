#include <iostream>
#include "../headers/audio-encoder.h"
#include <unistd.h>


AudioEncoder::AudioEncoder()
{
    // Encoding related
    aud_codec_context = NULL;

    // Resampling related
    swr_ctx = NULL;
    src_data = NULL;
    dst_data = NULL;

    // Capturing related
    frames = 1024;

    // Resampling related
    src_nb_samples = frames;
}

void AudioEncoder::init()
{
    // Encoding related
    float_t** aud_samples;
    int src_samples_linesize;
    int src_channels = 2;

    // Initialization
    init_capturer(&handle, frames, &buffer, &size);
    init_resampler(&swr_ctx, &src_nb_samples, &src_data, &dst_nb_samples, &dst_data);
    initialize_encoding_audio("result.mp4");
    av_samples_alloc_array_and_samples((uint8_t***)&aud_samples, &src_samples_linesize, src_channels,
                                             src_nb_samples,           AV_SAMPLE_FMT_FLTP,    0);
}

int AudioEncoder::init_resampler(struct SwrContext **swr_ctx, 
                    int *src_nb_samples, uint8_t ***src_data,
                    int *dst_nb_samples, uint8_t ***dst_data) {
    int64_t src_ch_layout = AV_CH_LAYOUT_STEREO;
    int src_rate = 44100;
    enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16;
    
    int64_t dst_ch_layout = AV_CH_LAYOUT_STEREO;
    int dst_rate = 44100;
    enum AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_FLTP;

    int ret;

    int src_nb_channels = 0;
    int src_linesize;

    int dst_nb_channels = 0;
    int dst_linesize;

    /* create resampler context */
    *swr_ctx = swr_alloc();
    if (!*swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        return -1;
    }

    /* set options */
    av_opt_set_int(*swr_ctx, "in_channel_layout",    src_ch_layout, 0);
    av_opt_set_int(*swr_ctx, "in_sample_rate",       src_rate, 0);
    av_opt_set_sample_fmt(*swr_ctx, "in_sample_fmt", src_sample_fmt, 0);

    av_opt_set_int(*swr_ctx, "out_channel_layout",    dst_ch_layout, 0);
    av_opt_set_int(*swr_ctx, "out_sample_rate",       dst_rate, 0);
    av_opt_set_sample_fmt(*swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

    /* initialize the resampling context */
    if ((ret = swr_init(*swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        return -1;
    }

    /* allocate source and destination samples buffers 
        - For S16, 2 channels 16 bits per sample. So, each sample takes 4 bytes.
        - Linesize is the total bytes allocated in multiples of 128. 
        - Based on src_nb_samples, src_linesize is decided and space allocated in src_data
    */

    src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
    ret = av_samples_alloc_array_and_samples(src_data, &src_linesize, src_nb_channels,
                                             *src_nb_samples, src_sample_fmt, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate source samples\n");
        return -1;
    }

    printf("Line size : %d\n", src_linesize);

    /* compute the number of converted samples: buffering is avoided
     * ensuring that the output buffer will contain at least all the
     * converted input samples */
    //max_dst_nb_samples = *dst_nb_samples =
    *dst_nb_samples = av_rescale_rnd(*src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

    printf("Number of samples in dest: %d\n", *dst_nb_samples);

    /* buffer is going to be directly written to a rawaudio file, no alignment */
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
    ret = av_samples_alloc_array_and_samples(dst_data, &dst_linesize, dst_nb_channels,
                                             *dst_nb_samples, dst_sample_fmt, 0);
    
    return 0;
}

int AudioEncoder::initialize_encoding_audio(const char *filename)
{
    int ret;
    enum AVCodecID aud_codec_id = AV_CODEC_ID_AAC;
    enum AVSampleFormat sample_fmt = AV_SAMPLE_FMT_FLTP;

    avcodec_register_all();
    av_register_all();

    AVCodec *aud_codec = avcodec_find_encoder(aud_codec_id);
    avcodec_register(aud_codec);

    if (!aud_codec)
        return COULD_NOT_FIND_AUD_CODEC;

    aud_codec_context = avcodec_alloc_context3(aud_codec);
    if (!aud_codec_context)
        return CONTEXT_CREATION_ERROR;

    aud_codec_context->bit_rate = 64000;
    aud_codec_context->sample_rate = 44100;
    printf("Sample rate selected : %d\n", aud_codec_context->sample_rate);
    aud_codec_context->sample_fmt = sample_fmt;
    aud_codec_context->channel_layout = AV_CH_LAYOUT_STEREO;
    aud_codec_context->channels = av_get_channel_layout_nb_channels(aud_codec_context->channel_layout);

    aud_codec_context->codec = aud_codec;
    aud_codec_context->codec_id = aud_codec_id;

    ret = avcodec_open2(aud_codec_context, aud_codec, NULL);

    if (ret < 0)
        return COULD_NOT_OPEN_AUD_CODEC;

    aud_frame = av_frame_alloc();
    aud_frame->nb_samples = aud_codec_context->frame_size;
    aud_frame->format = aud_codec_context->sample_fmt;
    aud_frame->channel_layout = aud_codec_context->channel_layout;

    int buffer_size = av_samples_get_buffer_size(NULL, aud_codec_context->channels, aud_codec_context->frame_size,
        aud_codec_context->sample_fmt, 0);

    av_frame_get_buffer(aud_frame, buffer_size / aud_codec_context->channels);

    if (!aud_frame)
        return COULD_NOT_ALLOCATE_FRAME;

    aud_frame_counter = 0;

    return 0;
}

int AudioEncoder::init_capturer(snd_pcm_t **handle, snd_pcm_uframes_t frames, char **buffer, int *size)
{
    int err, dir;
    snd_pcm_hw_params_t *params;
    char *device = (char*) "default";

    // Settings
    unsigned int sample_rate = 44100; // CD Quality
    unsigned int bits_per_sample = 16; // As we are using S16_LE forma
    unsigned int number_of_channels = 2; // stereo

    printf("Capture device is %s\n", device);

    /* Open PCM device for recording (capture). */
    err = snd_pcm_open(handle, device, SND_PCM_STREAM_CAPTURE, 0);
    if (err) {
        fprintf(stderr, "Unable to open PCM device: %s\n", snd_strerror(err));
        return err;
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(*handle, params);

    /* ### Set the desired hardware parameters. ### */

    /* Interleaved mode */
    err = snd_pcm_hw_params_set_access(*handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err) {
        fprintf(stderr, "Error setting interleaved mode: %s\n", snd_strerror(err));
        snd_pcm_close(*handle);
        return err;
    }

    /* Signed capture format (16-bit little-endian format) */
    if (bits_per_sample == 16) err = snd_pcm_hw_params_set_format(*handle, params, SND_PCM_FORMAT_S16_LE);
    else err = snd_pcm_hw_params_set_format(*handle, params, SND_PCM_FORMAT_U8);
    if (err) {
        fprintf(stderr, "Error setting format: %s\n", snd_strerror(err));
        snd_pcm_close(*handle);
        return err;
    }

    /* Setting number of channels */
    err = snd_pcm_hw_params_set_channels(*handle, params, number_of_channels);
    if (err) {
        fprintf(stderr, "Error setting channels: %s\n", snd_strerror(err));
        snd_pcm_close(*handle);
        return err;
    }

    /* Setting sampling rate */
    err = snd_pcm_hw_params_set_rate_near(*handle, params, &sample_rate, &dir);
    if (err) {
        fprintf(stderr, "Error setting sampling rate (%d): %s\n", sample_rate, snd_strerror(err));
        snd_pcm_close(*handle);
        return err;
    }

    /* Set period size*/
    err = snd_pcm_hw_params_set_period_size_near(*handle, params, &frames, &dir);
    if (err) {
        fprintf(stderr, "Error setting period size: %s\n", snd_strerror(err));
        snd_pcm_close(*handle);
        return err;
    }

    /* Write the parameters to the driver */
    err = snd_pcm_hw_params(*handle, params);
    if (err < 0) {
        fprintf(stderr, "Unable to set HW parameters: %s\n", snd_strerror(err));
        snd_pcm_close(*handle);
        return err;
    }

    /* Use a buffer large enough to hold one period (Find number of frames in one period) */
    err = snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    if (err) {
        fprintf(stderr, "Error retrieving period size: %s\n", snd_strerror(err));
        snd_pcm_close(*handle);
        return err;
    }

    /* Allocating buffer in number of bytes per period (2 bytes/sample, 2 channels) */
    *size = frames * bits_per_sample / 8 * number_of_channels; 
    *buffer = (char *) malloc(*size);
    if (!buffer) {
        fprintf(stdout, "Buffer error.\n");
        snd_pcm_close(*handle);
        return -1;
    }

    unsigned int period_time;
    err = snd_pcm_hw_params_get_period_time(params, &period_time, &dir);
    printf("Period time : %d\n", period_time);
    if (err) {
        fprintf(stderr, "Error retrieving period time: %s\n", snd_strerror(err));
        snd_pcm_close(*handle);
        free(buffer);
        return err;
    }

    printf("Sample rate: %d Hz\n", sample_rate);
    printf("Channels: %d\n", number_of_channels);
    printf("Number of frames: %lu", frames);
    return 0;
}

void AudioEncoder::close_capturer(snd_pcm_t **handle, char** buffer)
{
    //snd_pcm_drain(*handle);
    snd_pcm_drop(*handle);
    snd_pcm_close(*handle);
    free(*buffer);
}

AVPacket* AudioEncoder::encode_audio_samples(uint8_t **aud_samples)
{
    int ret;

    int buffer_size = av_samples_get_buffer_size(NULL, aud_codec_context->channels, aud_codec_context->frame_size,
        aud_codec_context->sample_fmt, 0);

    //for (size_t i = 0; i < buffer_size / aud_codec_context->channels; i++) {
    for (int i = 0; i < buffer_size / aud_codec_context->channels; i++) {
        aud_frame->data[0][i] = aud_samples[0][i];
        aud_frame->data[1][i] = aud_samples[1][i];
    }

    aud_frame->pts = aud_frame_counter++;

    ret = avcodec_send_frame(aud_codec_context, aud_frame);
    if (ret < 0) {
        fprintf(stderr, "ERROR_ENCODING_SAMPLES_SEND: '%d'\n", ret);
        return NULL;
    }

    pkt = av_packet_alloc();      // it calls av_init_packet(), and calling av_packet_unref is hadled by avcodec_receive_packet
    
    fflush(stdout);

    ret = avcodec_receive_packet(aud_codec_context, pkt);
    if (!ret) {
        // av_packet_rescale_ts(pkt, aud_codec_context->time_base, audio_st->time_base);
        // pkt->stream_index = audio_st->index;
        // printf("Duration : %d Timebase : %d %d\n", pkt->duration, aud_codec_context->time_base.den, audio_st->time_base);
        return pkt;
    }

    fprintf(stderr, "error in receiving encoded packet: '%d'\n", ret);  //ERROR_ENCODING_SAMPLES_RECEIVE);
    return NULL;
}

int AudioEncoder::finish_audio_encoding()
{
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    fflush(stdout);

    int ret = avcodec_send_frame(aud_codec_context, NULL);
    if (ret < 0)
        return ERROR_ENCODING_FRAME_SEND;

    while (1) {
        ret = avcodec_receive_packet(aud_codec_context, &pkt);
        if (!ret) {
            // if (pkt.pts != AV_NOPTS_VALUE)
            //     pkt.pts = av_rescale_q(pkt.pts, aud_codec_context->time_base, audio_st->time_base);
            // if (pkt.dts != AV_NOPTS_VALUE)
            //     pkt.dts = av_rescale_q(pkt.dts, aud_codec_context->time_base, audio_st->time_base);

            av_packet_unref(&pkt);
        }
        if (ret == -AVERROR(AVERROR_EOF))
            break;
        else if (ret < 0)
            return ERROR_ENCODING_FRAME_RECEIVE;
    }    
    return 0;
}

void AudioEncoder::cleanup()
{
    if (aud_frame)
        av_frame_free(&aud_frame);

    if (aud_codec_context) {
        avcodec_close(aud_codec_context);
        av_free(aud_codec_context);
    }
}
AudioEncoder::~AudioEncoder()
{
    this->cleanup();
}
AVPacket *AudioEncoder::GetNextFrame()
{
    int err = snd_pcm_readi(handle, buffer, frames);
    if (err < 0) {
        fprintf(stderr, "Error occured while recording: '%s'\n", snd_strerror(err));
        snd_pcm_recover(handle, err, 0);
    } else {
        memcpy(src_data[0], buffer, size);
        int ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)src_data, src_nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting: '%d, %d'\n", ret, AVERROR(ret));
        } else {
            AVPacket* pkt = encode_audio_samples((uint8_t **)dst_data);
            if (pkt != NULL) {
                return pkt;
            }
            
        }
    }
    return NULL;
}

int main() {
    AudioEncoder* _encoder = new AudioEncoder();
    _encoder->init();

    for(int i=0; i<100; i++) {
        _encoder->GetNextFrame();
    }
}