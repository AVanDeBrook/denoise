from calendar import different_locale
import librosa
import librosa.display
import matplotlib.pyplot as plt
import numpy as np
from pathlib import *

if __name__ == "__main__":
    files = [
        Path("audio_samples/2086-149220-0033-original.wav"),
        Path("audio_samples/2086-149220-0033-denoise.wav")
    ]

    original_audio, original_sr = librosa.load(str(files[0]), sr=None)
    denoised_audio, denoised_sr = librosa.load(str(files[1]), sr=None)

    original_melspec = librosa.feature.melspectrogram(y=original_audio, sr=original_sr)
    denoised_melspec = librosa.feature.melspectrogram(y=denoised_audio, sr=denoised_sr)

    difference = original_melspec - denoised_melspec

    plt.clf()

    librosa.display.specshow(librosa.power_to_db(difference, ref=np.max), x_axis='time', y_axis='mel', sr=original_sr)
    plt.colorbar()
    plt.title("Original - Denoised")
    plt.savefig('spectrograms/comparison.png', format='png')
