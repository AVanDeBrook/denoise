#include <stdio.h>
#include <stdlib.h>
#include "denoise.h"
#include "sndfile.h"
#include "nvAudioEffects.h"

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
#define EFFECT_INTENSITY	1.0f
#define DENOISE_MODEL_PATH  "C:\\Program Files\\NVIDIA Corporation\\NVIDIA Audio Effects SDK\\models\\denoiser_16k.trtpkg"
#define SUPERRES_MODEL_PATH "C:\\Program Files\\NVIDIA Corporation\\NVIDIA Audio Effects SDK\\models\\superres_16kto48k.trtpkg"

/**
 * @return int status code as defined by status_t enumerator.
 */
int main(int argc, char *argv[])
{
	status_t status = STATUS_OK;
	NvAFX_Status err = NVAFX_STATUS_SUCCESS;
	unsigned int input_samples_per_frame = SAMPLES_PER_FRAME;
	unsigned int input_channels = MAX_CHANNELS;
	unsigned int output_samples_per_frame, output_channels, output_sample_rate;

	// handle for the denoise effect (created below)
	NvAFX_Handle handle = NULL;
#ifdef SUPERRES
	err = NvAFX_CreateChainedEffect(NVAFX_CHAINED_EFFECT_DENOISER_16k_SUPERRES_16k_TO_48k, &handle);
#else
	err = NvAFX_CreateEffect(NVAFX_EFFECT_DENOISER, &handle);
#endif

	if (err != NVAFX_STATUS_SUCCESS) {
		fprintf(stderr, "[Error] Error creating effect: %d\n", err);
		status = STATUS_NVAFX_ERROR;
		return status;
	} else {
#ifdef SUPERRES
		printf("[Info] Chained effect (dnoiser, super-resolution) created\n");
#else
		printf("[Info] Denoiser effect created\n");
#endif
	}

	// set up sample rate, denoiser effect intensity, and model path
	// (required parameters for model to work, though the API docs do not explicity state this)
	err = NvAFX_SetU32(handle, NVAFX_PARAM_INPUT_SAMPLE_RATE, SAMPLE_RATE);
	if (err != NVAFX_STATUS_SUCCESS) {
		fprintf(stderr, "[Error] Error setting denoiser sample rate: %d\n", err);
		status = STATUS_NVAFX_ERROR;
		goto nvafx_failure;
	}

#ifdef SUPERRES
	float effect_intensities[2] = { EFFECT_INTENSITY, EFFECT_INTENSITY };
	err = NvAFX_SetFloatList(handle, NVAFX_PARAM_INTENSITY_RATIO, effect_intensities, 2);
#else
	// intensity ratio can be in range 0.0 to 1.0, 0 being no effect, 1 being maximum effect
	err = NvAFX_SetFloat(handle, NVAFX_PARAM_DENOISER_INTENSITY_RATIO, EFFECT_INTENSITY);
#endif

	if (err != NVAFX_STATUS_SUCCESS) {
		fprintf(stderr, "[Error] Error setting denoiser intensity ratio: %d\n", err);
		status = STATUS_NVAFX_ERROR;
		goto nvafx_failure;
	}

#ifdef SUPERRES
	const char *model_paths[2] = { DENOISE_MODEL_PATH, SUPERRES_MODEL_PATH };
	err = NvAFX_SetStringList(handle, NVAFX_PARAM_MODEL_PATH, model_paths, 2);
#else
	// 16k sample rate model (just because AN4 dataset is all 16k float audio)
	// TODO: move model path into a config file or command line argument (?)
	err = NvAFX_SetString(handle, NVAFX_PARAM_MODEL_PATH, DENOISE_MODEL_PATH);
#endif

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
	NvAFX_GetU32(handle, NVAFX_PARAM_NUM_INPUT_SAMPLES_PER_FRAME, &input_samples_per_frame);
	NvAFX_GetU32(handle, NVAFX_PARAM_NUM_INPUT_CHANNELS, &input_channels);
	printf("[Info] (input) Running with %u samples per audio frame\n", input_samples_per_frame);
	printf("[Info] (input) Running with %u audio channels\n", input_channels);

	// output info (samples per frame, channels, sample rate) needed for writing the resulting sound files
	// and setting up the datastructures for the audio info
	NvAFX_GetU32(handle, NVAFX_PARAM_NUM_OUTPUT_SAMPLES_PER_FRAME, &output_samples_per_frame);
	NvAFX_GetU32(handle, NVAFX_PARAM_NUM_OUTPUT_CHANNELS, &output_channels);
	NvAFX_GetU32(handle, NVAFX_PARAM_OUTPUT_SAMPLE_RATE, &output_sample_rate);
	printf("[Info] (output) Running with %u samples per audio frame\n", output_samples_per_frame);
	printf("[Info] (output) Running with %u audio channels\n", output_channels);
	printf("[Info] (output) Running with sample rate of %u\n", output_sample_rate);

	// info structure (format field must be set to 0 when opening a file in read mode)
	// https://libsndfile.github.io/libsndfile/api.html#open
	SF_INFO input_info = {
		.format = 0
	};
	// TODO: replace with info from command line OR separate into functions for an API/library
	SNDFILE *input_waveform = sf_open("audio_samples/an251-fash-b-noise.wav", SFM_READ, &input_info);

	SF_INFO output_info = {
		.frames 	= input_info.frames,
		.samplerate = output_sample_rate,
		.channels 	= output_channels,
		.format 	= SF_FORMAT_WAV|SF_FORMAT_FLOAT,
	};

	SNDFILE *output_waveform = sf_open("audio_samples/an251-fash-b-denoised.wav", SFM_WRITE, &output_info);

	if (input_waveform == NULL || output_waveform == NULL) {
		fprintf(stderr, "[Error] File not found\n");
		status = STATUS_FILENOTEFOUND_ERROR;
		goto nvafx_failure;
	}

	// each channel has one array attached to it (i.e. one pointer)
	float **input  = (float **)malloc(input_info.channels * sizeof(float *));
	float **output = (float **)malloc(input_info.channels * sizeof(float *));

	if (input == NULL || output == NULL) {
		fprintf(stderr, "[Error] Out of memory\n");
		status = STATUS_OUTOFMEMORY_ERROR;
		goto nvafx_failure;
	}

	// allocate array for each channel (reference stored in pointer allocated above)
	for (int i = 0; i < input_info.channels; i++) {
		*(input + i)  = (float *)malloc(input_samples_per_frame * sizeof(float));
		*(output + i) = (float *)malloc(output_samples_per_frame * sizeof(float));

		if (*(input + i) == NULL || *(output + i) == NULL) {
			fprintf(stderr, "[Error] Out of memory\n");
			status = STATUS_OUTOFMEMORY_ERROR;
			// if the program got to this point, the malloc
			// above passed so those pointers need to be freed
			goto indirect_free_return;
		} else {
			printf("[Info] Allocated %u bytes for channel %u (input)\n", input_samples_per_frame * sizeof(float), i);
			printf("[Info] Allocated %u bytes for channel %u (output)\n", output_samples_per_frame * sizeof(float), i);
		}
	}

	// run the model over the entire waveform
	printf("[Info] Running model...\n");
	for (int i = 0; i < input_info.frames / input_samples_per_frame; i++) {
		// read waveform data into memory
		sf_read_float(input_waveform, *input, input_samples_per_frame);
		// run model on audio chunk (individual frame)
		err = NvAFX_Run(handle, (const float **)input, output, input_samples_per_frame, input_info.channels);

		if (err != NVAFX_STATUS_SUCCESS) {
			fprintf(stderr, "[Error] Error running audio effect: %d\n", err);
			status = STATUS_NVAFX_ERROR;
			// pointer positions need to be reset to free all the allocated memory
			break;
		}

		// write model output to output file
		sf_write_float(output_waveform, *output, output_samples_per_frame);
	}

	sf_close(input_waveform);
	sf_close(output_waveform);

free_return:
	printf("[Info] Freeing memory...\n");
	// free direct pointers
	for (unsigned int i = 0; i < input_channels; i++) {
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
		fprintf(stderr, "[Error] Error destorying effect handle\n");

	if (status == STATUS_NVAFX_ERROR || status == STATUS_SNDFILE_ERROR)
		fprintf(stderr, "[Error] Library error: make sure the NVIDIA Audio Effects and sndfile\
						 libraries are installed and their DLLs are on the system path.\n");


	return status;
}
