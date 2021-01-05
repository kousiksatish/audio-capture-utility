export declare const soundCaptureUtility: INativeSoundCaptureUtility;

interface INativeSoundCaptureUtility {
    startListener(func: Function): void;

    stopListener(): void;
}
