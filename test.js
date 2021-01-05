///const addon = require('bindings')('linux_sound_capture_utility');
//const addon = require('./index/soundCaptureUtility');
//import soundCaptureUtility from './index/soundCaptureUtility';

const { soundCaptureUtility } = require('./index.js');
console.log(soundCaptureUtility);

const eventEmitter = require('events').EventEmitter;

const emitter = new eventEmitter();
emitter.on('data', (encoded_audio, pts) => {
    console.log(`PTS - ${pts} AudioLen - ${encoded_audio.length}`);
});

console.log('starting listener');
soundCaptureUtility.startListener(emitter.emit.bind(emitter));

setTimeout(() => {
    console.log('stopping listener');
    soundCaptureUtility.stopListener();
}, 5000);
