{
  "variables": {
    # x264_root as relative path to the module root
    "x264_root%": "<(module_root_dir)/x264"
  },
  "targets": [
    { 
      "cflags!": [ "-fno-exceptions", "-std=c++14" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "include_dirs" : [
        "<!@(node -p \"require('node-addon-api').include\")",
        "-I/usr/include/ffmpeg"
      ],
      "link_settings": {
        "libraries": [
          "-lasound", # provided by libx11-dev
          "-lavformat", # provided by libxrandr-dev
          "-lavcodec", # provided by libxtst-dev
          "-lavutil",
          "-lswresample",
          "-I/usr/include/ffmpeg"
          #"<@(x264_root)/lib/libx264.a" # static library exported with the package
        ]
      },
      "target_name": "sound_capture_utility",
      "sources": [ 
        "native_src/sound-capture-utility.cc",
        "native_src/audio-encoder.cc"
      ],
      # To avoid native node modules from throwing cpp exception and raise pending JS exception which can be handled in JS
      # 'dependencies': [ "<!(node -p \"require('node-addon-api').gyp\")" ],
      "defines": [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ]
    }
  ]
}
