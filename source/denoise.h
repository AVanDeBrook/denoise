#ifndef __DENOISELIB_H
#define __DENOISELIB_H

typedef enum tag_status_t {
	STATUS_OK,
	// error when calling NVAFX API functions
	STATUS_NVAFX_ERROR,
	// error when calling sndfile API functions
	STATUS_SNDFILE_ERROR,
	// error when opening a file
	STATUS_FILENOTEFOUND_ERROR,
	// suprisingly need this because the program can take up quite a bit of memory (2-3 GB in testing)
	STATUS_OUTOFMEMORY_ERROR,
} status_t;

#endif
