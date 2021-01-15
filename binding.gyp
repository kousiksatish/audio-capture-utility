{
  "variables": {
    "ffmpeg_root%":"<(module_root_dir)/ffmpeg",
  },
  "targets": [
    { 
      "cflags!": [ "-fno-exceptions"],
      "cflags_cc!": [ "-fno-exceptions"],
      "include_dirs" : [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "link_settings": {
        "libraries": [
          "-lasound", # provided by libx11-dev
          "-lavcodec",
          "<@(ffmpeg_root)/lib/libavutil.a",
          "<@(ffmpeg_root)/lib/libavformat.a",
          "<@(ffmpeg_root)/lib/libswresample.a",
        ]
      },
      "target_name": "sound_capture_utility",
      "sources": [ 
        "native_src/sound-capture-utility.cc",
        "native_src/audio-encoder.cc"
      ],
      # To avoid native node modules from throwing cpp exception and raise pending JS exception which can be handled in JS
      'dependencies': [ "<!(node -p \"require('node-addon-api').gyp\")" ],
      "defines": [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ]
    }
  ]
}
