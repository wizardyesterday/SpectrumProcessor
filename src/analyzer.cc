//*************************************************************************
// File name: analyzer.cc
//*************************************************************************

//*************************************************************************
// This program provides the ability to compute the average spectral
// power of a signal of interest.
// IQ (In-phase or Quadrature) signals that are provided by stdin.  The
// data is 8-bit signed 2's complement, and is formatted as
// I1,Q1; I2,Q2; ...
//
// To run this program type,
// 
//    ./analyzer -t <tag> -n <numberToAverage> -r <sampleRate>
//              -B <BandwidthInHz> -U < inputFile
//
//
// where,
//
//    tag - A user-supplied integer tag that will be written to stdout
//    along with other data. For example, it might be a gain setting
//    on a signal source such as a radio.
//
//    numberOfAverages - The number of power levels to average before
//    outputing a mean value of accumulated spectral power levels.
//
//    sampleRate - The sample rate of the IQ data in S/s.
//
//    bandwidthInHz - The one-sided bandwidth about the signal of
//    of interest. Because complex samples are being used, ultimately,
//    this will be mmapped to a two-sided bandwidth.
//
//    The U flag indicates that the IQ samples are unsigned 8-bit
//    quantities rather than the default signed values.  This allows
//    this program to work with the standard rtl-sdr tools such as
//    rtl_sdr.
//
///*************************************************************************
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "SpectrumProcessor.h"

// This structure is used to consolidate user parameters.
struct MyParameters
{
  int *tagPtr;
  uint32_t *numberToAveragePtr;
  float *sampleRatePtr;
  float *bandwidthInHzPtr;
  bool *unsignedSamplesPtr;
};

/*****************************************************************************

  Name: getUserArguments

  Purpose: The purpose of this function is to retrieve the user arguments
  that were passed to the program.  Any arguments that are specified are
  set to reasonable default values.

  Calling Sequence: exitProgram = getUserArguments(parameters)

  Inputs:

    parameters - A structure that contains pointers to the user parameters.

  Outputs:

    exitProgram - A flag that indicates whether or not the program should
    be exited.  A value of true indicates to exit the program, and a value
    of false indicates that the program should not be exited..

*****************************************************************************/
bool getUserArguments(int argc,char **argv,struct MyParameters parameters)
{
  bool exitProgram;
  bool done;
  int opt;

  // Default not to exit program.
  exitProgram = false;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Default parameters.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // This is a reasonable value.
  *parameters.tagPtr = 0;

  // Default to no averaging.
  *parameters.numberToAveragePtr = 1;

  // Default to 256000S/s.
  *parameters.sampleRatePtr = 256000;

  // Default to 1000Hz.
  *parameters.bandwidthInHzPtr = 1000;

  // Default to signed IQ samples.
  *parameters.unsignedSamplesPtr = false;
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // Set up for loop entry.
  done = false;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Retrieve the command line arguments.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  while (!done)
  {
    // Retrieve the next option.
    opt = getopt(argc,argv,"t:n:r:B:Uh");

    switch (opt)
    {
      case 't':
      {
        *parameters.tagPtr = atoi(optarg);
        break;
      } // case

      case 'n':
      {
        *parameters.numberToAveragePtr = atol(optarg);
        break;
      } // case

      case 'r':
      {
        *parameters.sampleRatePtr = atof(optarg);
        break;
      } // case

      case 'B':
      {
        *parameters.bandwidthInHzPtr = atof(optarg);
        break;
      } // case

      case 'U':
      {
        *parameters.unsignedSamplesPtr = true;
        break;
      } // case

      case 'h':
      {
        // Display usage.
        fprintf(stderr,"./analyzer -t tag\n"
                "           -n numbertoaverage\n"
                "           -r samplerate (S/s) \n"
                "           -B bandwidthInHz (Hz)\n"
                "           -U (unsigned samples)\n");

        // Indicate that program must be exited.
        exitProgram = true;
        break;
      } // case

      case -1:
      {
        // All options consumed, so bail out.
        done = true;
      } // case
    } // switch

  } // while
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return (exitProgram);

} // getUserArguments

//*************************************************************************
// Mainline code.
//*************************************************************************
int main(int argc,char **argv)
{
  bool done;
  int8_t *signedBufferPtr;
  bool exitProgram;
  uint32_t i, j;
  uint32_t count;
  uint8_t inputBuffer[16384];
  SpectrumProcessor *analyzerPtr;
  int tag;
  uint32_t numberToAverage;
  float sampleRate;
  bool unsignedSamples;
  float bandwidthInHz;
  float power, logPower;
  struct MyParameters parameters;

  // Set up for parameter transmission.
  parameters.tagPtr = &tag;
  parameters.numberToAveragePtr = &numberToAverage;
  parameters.sampleRatePtr = &sampleRate;
  parameters.bandwidthInHzPtr = &bandwidthInHz;
  parameters.unsignedSamplesPtr = &unsignedSamples;

  // Retrieve the system parameters.
  exitProgram = getUserArguments(argc,argv,parameters);

  if (exitProgram)
  {
    // Bail out.
    return (0);
  } // if

  // Instantiate signal analyzer.
  analyzerPtr = new SpectrumProcessor(sampleRate);

  // Reference the input buffer in 8-bit signed context.
  signedBufferPtr = (int8_t *)inputBuffer;

  // Set up for loop entry.
  done = false;

  while (!done)
  {
    // Read a block of input samples (2 * complex FFT length).
    count = fread(inputBuffer,sizeof(int8_t),(2 * N),stdin);

    if (count == 0)
    {
      // We're done.
      done = true;
    } // if
    else
    {
      if (unsignedSamples)
      {
        for (i = 0; i < count; i++)
        {
          // Convert unsigned samples to signed quantities.
          signedBufferPtr[i] -= 128;
        } // for
      } // if

      // Initialize sum.
      power = 0;

      for (j = 0; j < numberToAverage; j++)
      {
        // Compute the power within the specified bandwidth.
        power += analyzerPtr->computeSpectralPower(bandwidthInHz,
                                                   signedBufferPtr,
                                                   count);
      } // for

      // Normalize to mean value.
      power /= numberToAverage;

      logPower = 10*log10(power);
      fprintf(stdout,"%d    %0.2f    %0.2f\n",tag,power,logPower);

      // Bail out.
      done = true;

    } // else
  } // while

  // Release resources.
  delete analyzerPtr;

  return (0);

} // main

