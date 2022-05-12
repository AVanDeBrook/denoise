#include <stdio.h>
#include <stdlib.h>
#include "denoise.h"
#include "lib/sndfile.h"
#include "lib/nvAudioEffects.h"

/**
 * @brief Maximum of 2 channel audio
 *
 * It is possible to have audio with more than two channels, but for the purposes
 * of this program, multi-channel audio would be surprising.
 */
#define MAX_CHANNELS		2U

/**
 * @brief NVAFX constants used for model and data initialization.
 *
 * Not explicitly stated by the NVAFX API, but if these constants aren't
 * defined, the program devolves into lots of magic numbers and trial and error.
 */
#define SAMPLE_RATE			16000U
#define SAMPLES_PER_FRAME	160U

/**
 * @return int status code as defined by status_t enumerator.
 */
int main(int argc, char *argv[])
{
	status_t status = STATUS_OK;
	NvAFX_Status err = NVAFX_STATUS_SUCCESS;
	unsigned int num_samples_per_frame = SAMPLES_PER_FRAME;
	unsigned int num_channels = MAX_CHANNELS;

	// handle for the denoise effect (created below)
	NvAFX_Handle handle = NULL;
	err = NvAFX_CreateEffect(NVAFX_EFFECT_DENOISER, &handle);

	if (err != NVAFX_STATUS_SUCCESS) {
		fprintf(stderr, "[Error] Error creating denoiser effect: %d\n", err);
		status = STATUS_NVAFX_ERROR;
		return status;
	} else {
		printf("[Info] Denoiser effect loaded\n");
	}

	// set up sample rate, denoiser effect intensity, and model path
	// (required parameters for model to work, though the API docs do not explicity state this)
	err = NvAFX_SetU32(handle, NVAFX_PARAM_DENOISER_SAMPLE_RATE, SAMPLE_RATE);
	if (err != NVAFX_STATUS_SUCCESS) {
		fprintf(stderr, "[Error] Error setting denoiser sample rate: %d\n", err);
		status = STATUS_NVAFX_ERROR;
		goto nvafx_failure;
	}

	// intensity ratio can be in range 0.0 to 1.0, 0 being no effect, 1 being maximum effect
	err = NvAFX_SetFloat(handle, NVAFX_PARAM_DENOISER_INTENSITY_RATIO, 1.0f);
	if (err != NVAFX_STATUS_SUCCESS) {
		fprintf(stderr, "[Error] Error setting denoiser intensity ratio: %d\n", err);
		status = STATUS_NVAFX_ERROR;
		goto nvafx_failure;
	}

	// 16k sample rate model (just because AN4 dataset is all 16k float audio)
	// TODO: move model path into a config file or command line argument (?)
	err = NvAFX_SetString(handle, NVAFX_PARAM_MODEL_PATH, "C:\\Program Files\\NVIDIA Corporation\\NVIDIA Audio Effects SDK\\models\\denoiser_16k.trtpkg");
	if (err != NVAFX_STATUS_SUCCESS) {
		fprintf(stderr, "[Error] Error setting model path: %d\n", err);
		status = STATUS_NVAFX_ERROR;
		goto nvafx_failure;
	}

	// load model (verifies parameters, moves model into GPU memory)
	err = NvAFX_Load(handle);
	if (err != NVAFX_STATUS_SUCCESS) {
		fprintf(stderr, "[Error] Error loading model: %d\n", err);
		status = STATUS_NVAFX_ERROR;
		goto nvafx_failure;
	} else {
		printf("[Info] Model configured and loaded successfully\n");
	}

	// need these constants when running the model and setting up data
	// (again API does not explicity say this, although it somewhat implies it)
	NvAFX_GetU32(handle, NVAFX_PARAM_NUM_SAMPLES_PER_FRAME, &num_samples_per_frame);
	NvAFX_GetU32(handle, NVAFX_PARAM_DENOISER_NUM_CHANNELS, &num_channels);
	printf("[Info] Running with %u samples per audio frame\n", num_samples_per_frame);
	printf("[Info] Running with %u audio channels\n", num_channels);

	// info structure (format field must be set to 0 when opening a file in read mode)
	// https://libsndfile.github.io/libsndfile/api.html#open
	SF_INFO info = {
		.format = 0
	};
	// TODO: replace with info from command line OR separate into functions for an API/library
	SNDFILE* waveform = sf_open("audio_samples/an251-fash-b-noise.wav", SFM_READ, &info);

	if (waveform == NULL) {
		fprintf(stderr, "[Error] File not found\n");
		status = STATUS_FILENOTEFOUND_ERROR;
		goto nvafx_failure;
	}

	const sf_count_t frames = info.frames;

	// each channel has one array attached to it (i.e. one pointer)
	float** input  = (float **)malloc(info.channels * sizeof(float *));
	float** output = (float **)malloc(info.channels * sizeof(float *));

	if (input == NULL || output == NULL) {
		fprintf(stderr, "[Error] Out of memory\n");
		status = STATUS_OUTOFMEMORY_ERROR;
		goto nvafx_failure;
	}

	// allocate array for each channel (reference stored in pointer allocated above)
	for (int i = 0; i < info.channels; i++) {
		*(input + i)  = (float *)malloc(info.frames * sizeof(float));
		*(output + i) = (float *)malloc(info.frames * sizeof(float));

		if (*(input + i) == NULL || *(output + i) == NULL) {
			fprintf(stderr, "[Error] Out of memory\n");
			status = STATUS_OUTOFMEMORY_ERROR;
			// if the program got to this point, the malloc
			// above passed so those pointers need to be freed
			goto indirect_free_return;
		}
	}

	if (status == STATUS_OK) {
		printf("[Info] Allocated %llu bytes of memory (%llu frames)\n", info.frames * info.channels * sizeof(float), info.frames);
	}

	// read waveform data into memory
	sf_readf_float(waveform, input[0], info.frames);
	sf_close(waveform);

	// save position of the beginning of the input and output arrays
	float *input_top  = *input;
	float *output_top = *output;

	// run the model over the entire waveform
	printf("[Info] Running model...\n");
	for (int i = 0; i < info.frames / num_samples_per_frame; i++) {
		// run model on audio chunk (individual frame)
		err = NvAFX_Run(handle, (const float **)input, output, num_samples_per_frame, info.channels);

		if (err != NVAFX_STATUS_SUCCESS) {
			fprintf(stderr, "[Error] Error running audio effect: %d\n", err);
			status = STATUS_NVAFX_ERROR;
			// pointer positions need to be reset to free all the allocated memory
			break;
		}

		*input  += num_samples_per_frame;
		*output += num_samples_per_frame;
	}

	// reset pointer positions
	*input  = input_top;
	*output = output_top;

	// free memory and return after the pointers have been reset
	if (status == STATUS_NVAFX_ERROR)
		goto free_return;

	// sound file info (sample rate, channels, format required to be set before opening a file in write mode)
	// https://libsndfile.github.io/libsndfile/api.html#open
	info.samplerate = SAMPLE_RATE;
	info.channels   = num_channels;
	info.format     = SF_FORMAT_WAV|SF_FORMAT_FLOAT;

	// TODO: replace with info from command line OR separate into functions for an API
	waveform = sf_open("audio_samples/an251-fash-b-denoised.wav", SFM_WRITE, &info);

	if (waveform == NULL) {
		fprintf(stderr, "[Error] Error creating output file\n");
		status = STATUS_FILENOTEFOUND_ERROR;
		goto free_return;
	} else {
		printf("[Info] Writing %llu bytes (%llu frames)\n", frames * sizeof(float), frames);
	}

	sf_writef_float(waveform, *output, frames);
	sf_close(waveform);

free_return:
	printf("[Info] Freeing memory...\n");
	// free direct pointers
	for (unsigned int i = 0; i < num_channels; i++) {
		free(*input + i);
		free(*output + i);
	}

indirect_free_return:
	// free indirect pointers
	free(input);
	free(output);

nvafx_failure:
	// clean up NVAFX API handles
	printf("[Info] Freeing NVAFX instances...\n");
	if (NvAFX_DestroyEffect(handle) != NVAFX_STATUS_SUCCESS)
		fprintf(stderr, "Error destorying effect handle\n");

	if (status == STATUS_NVAFX_ERROR || status == STATUS_SNDFILE_ERROR)
		fprintf(stderr, "[Error] Library error: make sure the NVIDIA Audio Effects and sndfile\
						 libraries are installed and their DLLs are on the system path.\n");


	return status;
}
