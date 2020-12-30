///const addon = require('bindings')('linux_sound_capture_utility');
//const addon = require('./index/soundCaptureUtility');
//import soundCaptureUtility from './index/soundCaptureUtility';

const soundCaptureUtility = require('bindings')('sound_capture_utility');
console.log(soundCaptureUtility);

const addon = new soundCaptureUtility.SoundCaptureUtility();
const eventEmitter = require('events').EventEmitter;

const emitter = new eventEmitter();
emitter.on('data', (encoded_audio, pts) => {
    console.log(`PTS - ${pts} AudioLen - ${encoded_audio.length}`);
});

console.log('starting listener');
addon.startListener(emitter.emit.bind(emitter));

setTimeout(() => {
    console.log('stopping listener');
    addon.stopListener();
}, 5000);
