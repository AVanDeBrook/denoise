# Denoise
This project uses NVIDIA's Maxine Audio Effects SDK to remove noise from audio data. This is essentially just a demo project at the moment.

Two example audio files are included in this repository (under `audio_samples`) from the [AN4 dataset from CMU](https://dldata-public.s3.us-east-2.amazonaws.com/an4_sphere.tar.gz). The file ending in `noise` has artificial white noise added, the file ending in `denoised` is the result after running the code in this repository.

**At the time of writing this, the code in this repository has been developed and tested on Windows only.**

## Building and Running
Dependencies:
* (Build) [Cmake](https://cmake.org/download/)
* (Build) [Ninja](https://github.com/ninja-build/ninja/releases)
* [NVIDIA Maxine Audio Effects SDK](https://docs.nvidia.com/deeplearning/maxine/audio-effects-sdk/index.html)
* [sndfile](https://github.com/libsndfile/libsndfile/releases/)

**Note**: Ensure all of the above programs and DLLs are on your system path before building and running the project.

Generate build files:
```
cmake --preset default
```
**Note**: To toggle between the chained effect (denoiser, super-resolution) and denoiser effect, change the `SUPERRES` (`1` enables the super-resolution effect, `0` disables it) cache variable in `CMakePresets.json` or use the command `cmake -D SUPERRES=<value> --preset default`.

Build project:
```
ninja -C build
```
Run:
```
build/denoise
```