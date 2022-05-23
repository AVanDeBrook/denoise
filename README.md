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
cmake -G Ninja -B build .
```
**Note**: To use the chained denoiser, superresolution effect use `cmake -D SUPERRES=1 -G Ninja -B build .` in place of the command above.

Build project:
```
ninja -C build
```
Run:
```
build/denoise
```