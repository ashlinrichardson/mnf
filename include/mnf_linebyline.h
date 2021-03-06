//=======================================================================================================
// Copyright 2015 Asgeir Bjorgan, Lise Lyngsnes Randeberg, Norwegian University of Science and Technology
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
// http://opensource.org/licenses/MIT)
//=======================================================================================================

#ifndef MNF_LINEBYLINE_H_DEFINED
#define MNF_LINEBYLINE_H_DEFINED



//image statistics, including covariance and means
typedef struct{
	int n; //number of pixels so far summed over
	float *C; 
	double *means; 
} ImageStatistics;

#include "mnf.h"


//run the mnf-lbl method on the entire image
void mnf_linebyline_run_image(MnfWorkspace *workspace, int bands, int numSamples, int numLines, float *data, std::vector<float> wlens);

//run mnf on one line, incrementally updating the covariances by this line and denoising line in place
//MAIN MNF-LBL ALGORITHM.
//se mnf_linebyline_run_image for how it should be called
void mnf_linebyline_run_oneline(MnfWorkspace *workspace, int numBands, int numSamples, float *line, ImageStatistics *imageStats, ImageStatistics *noiseStats);

//explicitly estimate noise by shift difference
//allocates the noise estimate array
void mnf_linebyline_estimate_noise(int bands, int samples, float *line, float **noise_est, int *noise_samples);

//remove/add specified mean from/to data
void mnf_linebyline_remove_mean(const MnfWorkspace *workspace, float *means, int bands, int samples, float *line);
void mnf_linebyline_add_mean(const MnfWorkspace *workspace, float *means, int bands, int samples, float *line);

//(de)initialize covariance arrays
void imagestatistics_initialize(ImageStatistics *stats, int bands);
void imagestatistics_deinitialize(ImageStatistics *stats);

//update covariances and means using a numerically stable algorithm
void imagestatistics_update_with_line(const MnfWorkspace *workspace, int numBands, int numSamples, float *bilData, ImageStatistics *stats);

//get means and covariances from imagestatistics struct
void imagestatistics_get_means(ImageStatistics *stats, int numBands, float *means);
void imagestatistics_get_cov(ImageStatistics *stats, int numBands, float *cov);

//read/write covariances and means to file (covariances: only lower row echelon)
void imagestatistics_write_to_file(MnfWorkspace *workspace, int bands, ImageStatistics *imgStats, ImageStatistics *noiseStats);
void imagestatistics_read_from_file(MnfWorkspace *workspace, int bands, ImageStatistics *imgStats, ImageStatistics *noiseStats);




#endif
