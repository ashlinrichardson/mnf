//=======================================================================================================
// Copyright 2015 Asgeir Bjorgan, Lise Lyngsnes Randeberg, Norwegian University of Science and Technology
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
// http://opensource.org/licenses/MIT)
//=======================================================================================================

/* Author: Asgeir Bjørgan asgeir.bjorgan@iet.ntnu.no NTNU */

#include <iostream>
#include <string>
#include <getopt.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <mnf.h>
#include <readimage.h>
#include <mnf_linebyline.h>
using namespace std;

void showHelp(){
  cerr << "Usage: mnf [OPTION]... [FILE]" << endl
  << "Noise removal using the MNF transform. Requires BIL interleave on images, assumes ENVI format." << endl
  << "--help\t\t\t Show help" << endl
  << "--output=BASEFILENAME \t Output to specified basefilename (FILENAME_transformed.img, FILENAME_inversetransformed.img). Defaults to [input filename]_mnf." << endl
  << endl
  << "Image subset arguments: Defaults to the whole image." << endl
  << "--startpix=START_PIXEL \t Start sample" << endl
  << "--endpix=END_PIXEL \t End sample" << endl
  << "--startline=START_LINE \t Start line" << endl
  << "--endline=END_LINE \t End line" << endl
  << "--startband=START_BAND \t Start band" << endl
  << "--endband=END_BAND \t End band" << endl
  << endl
  << "MNF arguments: " << endl
  << "--line-by-line \t Use the line-by-line modification for denoising." << endl
  << "--forward-only \t Run forward transform only. (Ignored when --line-by-line is set)" << endl
  << "--inverse-only \t Run inverse transform only. Files containing the covariance files are assumed to have filenames [BASEFILENAME]_imgcov.dat and [BASEFILENAME]_noisecov.dat. (Ignored when --line-by-line is set)" << endl
  << "(If run with both --forward-only and --inverse-only, these arguments will be ignored)" << endl
  << "--num-bands-in-inverse=NUM_BANDS \t Specify the number of bands to use in the inverse transform. Default is 10." << endl;

}
void createOptions(option **options){
  int numOptions = 13;
  *options = new option[numOptions];

  (*options)[0].name = "startpix";
  (*options)[0].has_arg = required_argument;
  (*options)[0].flag = NULL;
  (*options)[0].val = 0;

  (*options)[1].name = "endpix";
  (*options)[1].has_arg = required_argument;
  (*options)[1].flag = NULL;
  (*options)[1].val = 1;

  (*options)[2].name = "help";
  (*options)[2].has_arg = no_argument;
  (*options)[2].flag = NULL;
  (*options)[2].val = 2;

  (*options)[3].name = "startline";
  (*options)[3].has_arg = required_argument;
  (*options)[3].flag = NULL;
  (*options)[3].val = 3;

  (*options)[4].name = "endline";
  (*options)[4].has_arg = required_argument;
  (*options)[4].flag = NULL;
  (*options)[4].val = 4;

  (*options)[5].name = "output";
  (*options)[5].has_arg = required_argument;
  (*options)[5].flag = NULL;
  (*options)[5].val = 5;

  (*options)[6].name = "forward-only";
  (*options)[6].has_arg = no_argument;
  (*options)[6].flag = NULL;
  (*options)[6].val = 6;

  (*options)[7].name = "inverse-only";
  (*options)[7].has_arg = no_argument;
  (*options)[7].flag = NULL;
  (*options)[7].val = 7;

  (*options)[8].name = "num-bands-in-inverse";
  (*options)[8].has_arg = required_argument;
  (*options)[8].flag = NULL;
  (*options)[8].val = 8;

  (*options)[9].name = "line-by-line";
  (*options)[9].has_arg = no_argument;
  (*options)[9].flag = NULL;
  (*options)[9].val = 9;

  (*options)[10].name = "startband";
  (*options)[10].has_arg = required_argument;
  (*options)[10].flag = NULL;
  (*options)[10].val = 10;

  (*options)[11].name = "endband";
  (*options)[11].has_arg = required_argument;
  (*options)[11].flag = NULL;
  (*options)[11].val = 11;

  (*options)[12].name = 0;
  (*options)[12].has_arg = 0;
  (*options)[12].flag = 0;
  (*options)[12].val = 0;

}

int main(int argc, char ** argv){
  //process options
  char shortopts[] = "";
  option *longopts;
  createOptions(&longopts);

  //properties to be extracted
  int startline = 0;
  int endline = 0;
  int startpix = 0;
  int endpix = 0;
  int startband = 0;
  int endband = 0;
  string mnfOutFilename;
  bool mnfOutFilenameWasSet = false;

  int index;

  int numBandsToKeepInInverse = 10; //number of bands to use in the inverse transformation
  bool shouldOnlyInverse = false;
  bool shouldOnlyForward = false;
  bool shouldLineByLine = false;

  while (true){
    int flag = getopt_long(argc, argv, shortopts, longopts, &index);
    switch (flag){
      case 0:
      startpix = strtod(optarg, NULL);
      break;

      case 1:
      endpix = strtod(optarg, NULL);
      break;

      case 2: //help
      showHelp();
      exit(0);
      break;

      case 3:
      startline = strtod(optarg, NULL);
      break;

      case 4:
      endline = strtod(optarg, NULL);
      break;
      case 5:
      mnfOutFilename = string(optarg);
      mnfOutFilenameWasSet = true;
      break;
      case 6:
      shouldOnlyForward = true;
      break;
      case 7:
      shouldOnlyInverse = true;
      break;
      case 8:
      numBandsToKeepInInverse = strtod(optarg, NULL);
      break;
      case 9:
      shouldLineByLine = true;
      break;
      case 10:
      startband = strtod(optarg, NULL);
      break;
      case 11:
      endband = strtod(optarg, NULL);
      break;
    }
    if (flag == -1){
      break;
    }
  }
  char *filename = argv[optind];
  if (filename == NULL){
    cerr << "Missing filename!" << endl;
    exit(1);
  }

  if (!mnfOutFilenameWasSet){
    mnfOutFilename = string(filename) + "_mnf";
  }
  delete [] longopts;

  //read hyperspectral image header
  size_t offset;
  vector<float> wlens;
  HyspexHeader header;
  hyperspectral_read_header(filename, &header);

  //set default startline and the like
  if (!startline){
    startline = 0;
  }
  if (!endline){
    endline = header.lines;
  }
  if (!startpix){
    startpix = 0;
  }
  if (!endpix){
    endpix = header.samples;
  }
  if (!startband){
    startband = 0;
  }
  if (!endband){
    endband = header.bands;
  }

  //new hyperspectral data cube sizes after subsetting
  int newNumLines = endline - startline;
  int newNumSamples = endpix - startpix;
  int newNumBands = endband - startband;

  ImageSubset subset;
  subset.startSamp = startpix;
  subset.endSamp = endpix;
  subset.startLine = startline;
  subset.endLine = endline;
  subset.startBand = startband;
  subset.endBand = endband;

  //read hyperspectral image
  float *data = new float[newNumLines*newNumSamples*newNumBands];
  hyperspectral_read_image(filename, &header, subset, data);

  for (int i=subset.startBand; i < subset.endBand; i++){
    wlens.push_back(header.wlens[i]);
  }

  //run MNF
  TransformDirection dir;
  //do some parsing on the booleans specifying whether to run inverse or forward
  if (shouldOnlyForward && shouldOnlyInverse){
    dir = RUN_BOTH;
  }
  else if (!shouldOnlyForward && !shouldOnlyInverse){
    dir = RUN_BOTH;
  }
  else if (shouldOnlyForward){
    dir = RUN_FORWARD;
  }
  else{
    dir = RUN_INVERSE;
  }

  MnfWorkspace workspace;
  mnf_initialize(dir, newNumBands, newNumSamples, numBandsToKeepInInverse, &workspace, mnfOutFilename);

  if (shouldLineByLine){
    mnf_linebyline_run_image(&workspace, newNumBands, newNumSamples, newNumLines, data, wlens);
  }
  else {
    mnf_run(&workspace, newNumBands, newNumSamples, newNumLines, data, wlens);
  }

  mnf_deinitialize(&workspace);
  delete [] data;
}