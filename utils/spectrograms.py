import glob
import os
import librosa
import librosa.display
import matplotlib.pyplot as plt
import numpy as np
from pathlib import *

if __name__ == "__main__":
    audio_files = glob.glob("**/*.wav", recursive=True)

    if not os.path.exists('spectrograms'):
        os.makedirs('spectrograms')

    for file in audio_files:
        file = Path(file)
        # file processing
        audio, sample_rate = librosa.load(str(file), sr=None)
        # convert to mel spectrogram
        melspec = librosa.feature.melspectrogram(y=audio, sr=sample_rate)
        melspec_db = librosa.power_to_db(melspec, ref=np.max)
        # save figure
        plt.clf()
        librosa.display.specshow(melspec_db, x_axis='time', y_axis='mel', sr=sample_rate)
        plt.colorbar()
        plt.title(file.name)
        plt.savefig(f'spectrograms/{file.name[:file.name.rfind(".")]}.png', format="png")
        # plt.show()


