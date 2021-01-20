{
  "variables": {
    "ffmpeg_root%":"<(module_root_dir)/ffmpeg",
  },
  "targets": [
    {
      "target_name": "sound_capture_utility",
      "cflags!": [ "-fno-exceptions"],
      "cflags_cc!": [ "-fno-exceptions"],
      "include_dirs" : [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<@(ffmpeg_root)/include",
      ],
      "library_dirs" : [
        "/opt/remotepc/ffmpeg"
      ],
      "libraries": [
        "-lasound",
        "-Wl,-rpath=/opt/remotepc/ffmpeg",
        "-lavcodec",
        "-lavutil",
        "-lavformat",
        "-lswresample"
      ],      
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
