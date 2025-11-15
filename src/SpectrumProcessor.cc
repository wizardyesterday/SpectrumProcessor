//************************************************************************
// file name: SpectrumProcessor.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "SpectrumProcessor.h"

using namespace std;

/*****************************************************************************

  Name: SpectrumProcessor

  Purpose: The purpose of this function is to serve as the constructor for
  an instance of an SpectrumProcessor.

  Calling Sequence: SpectrumProcessor(sampleRate)
 
  Inputs:

    sampleRate - The sample rate in S/s.

 Outputs:

    None.

*****************************************************************************/
SpectrumProcessor::SpectrumProcessor(float sampleRate)
{
  uint32_t i;

  if (sampleRate <= 0)
  {
    // Keep it sane.
    sampleRate = 256000;
  } // if

  // Save for later use.
  this->sampleRate = sampleRate;

  // We need to know the FFT bin resolution.
  fftBinResolutionInHz = sampleRate / N;

  // Construct the Hanning window array.
  for (i = 0; i < N; i++)
  {
    hanningWindow[i] = 0.5 - 0.5 * cos((2 * M_PI * i)/N);
  } // for

  // Set up the FFT stuff.
  initializeFftw();

  return;

} // SpectrumProcessor

/*****************************************************************************

  Name: ~SpectrumProcessor

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an SpectrumProcessor.

  Calling Sequence: ~SpectrumProcessor()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
SpectrumProcessor::~SpectrumProcessor(void)
{

  // Release FFT resources.
  fftw_destroy_plan(fftPlan);
  fftw_free(fftInputPtr);
  fftw_free(fftOutputPtr);

  return;

} // ~SpectrumProcessor

/*****************************************************************************

  Name: initializeFftw

  Purpose: The purpose of this function is to initialize FFTW so that
  FFT's can be performed.

  Calling Sequence: initializeFftw()

  Inputs:

    None.

 Outputs:

    None.

*****************************************************************************/
void SpectrumProcessor::initializeFftw(void)
{
  uint32_t i;

  // Construct the permuted indices.
  for (i = 0; i < N/2; i++)
  {
    fftShiftTable[i] = i + N/2;
    fftShiftTable[i + N/2] = i;
  } // for

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This block of code sets up FFTW for a size of 8192 points.  This
  // is the result of N being defined as 8192.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  fftInputPtr = (fftw_complex *)fftw_malloc(sizeof(fftw_complex)*N);
  fftOutputPtr = (fftw_complex *)fftw_malloc(sizeof(fftw_complex)*N);

  fftPlan = fftw_plan_dft_1d(N,fftInputPtr,fftOutputPtr,
                             FFTW_FORWARD,FFTW_ESTIMATE);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return;

} // initializeFftw

/*****************************************************************************

  Name: computeSpectralPower

  Purpose: The purpose of this function is to compute the total power
  within the specified bandwidth about a signal of interest.

  Calling Sequence: power = computeSpectralPower(lowpassBandwidth)
                                                 signalufferPtr,
                                                 bufferLength)

  Inputs:

    lowpassBandwidthInHz - The single-sided bandwidth of interest.

    signalBufferPtr - A pointer to a buffer of IQ data.  The buffer is
    formatted with interleaved data as: I1,Q1,I2,Q2,...

    bufferLength - The number of values in the signal buffer.  This
    represents the total number of items in the buffer, rather than
    the number of IQ sample pairs in the buffer.

 Outputs:

    power - The power within the double-sided bandwidth about center
    frequency.

*****************************************************************************/
float SpectrumProcessor::computeSpectralPower(
  float lowpassBandwidthInHz,
  int8_t *signalBufferPtr,
  uint32_t bufferLength)
{
  uint32_t spectrumLength;
  float power;
  uint32_t i;
  uint32_t lowerBinIndex;
  uint32_t upperBinIndex;
  uint32_t lowpassSpan;

  if (lowpassBandwidthInHz > (sampleRate/2))
  {
    // Clip it.
    lowpassBandwidthInHz = sampleRate / 2;
  } // if

  // Initial value;
  power = 0;

  // Compute the number of FFT bins for the lowpass span.
  lowpassSpan = (uint32_t)(lowpassBandwidthInHz / fftBinResolutionInHz);

  // Compute lower FFT bin index.
  lowerBinIndex = (N/2) - lowpassSpan;

  // Compute upper FFT bin index.
  upperBinIndex = (N/2) + lowpassSpan;

  // Run the FFT.
  spectrumLength =computePowerSpectrum(signalBufferPtr,bufferLength);

  // Compute power within the specified bandwidth.
  for (i = lowerBinIndex; i <= upperBinIndex; i++)
  {
    power += spectralPowerBuffer[i];
  } // for

  return (power);

} // computeSpectralPower

/*****************************************************************************

  Name: computePowerSpectrum

  Purpose: The purpose of this function is to compute the power spectrum
  of IQ data.

  Calling Sequence: computePowerSpectrum(signalBufferPtr,bufferLength)

  Inputs:

    signalBufferPtr - A pointer to a buffer of IQ data.  The buffer is
    formatted with interleaved data as: I1,Q1,I2,Q2,...

    bufferLength - The number of values in the signal buffer.  This
    represents the total number of items in the buffer, rather than
    the number of IQ sample pairs in the buffer.

 Outputs:

    None.

*****************************************************************************/
uint32_t SpectrumProcessor::computePowerSpectrum(
  int8_t *signalBufferPtr,
  uint32_t bufferLength)
{
  uint32_t i;
  uint32_t j;
  int16_t iMagnitude, qMagnitude;
  double power;
  double iK, qK;

  // Reference the beginning of the FFT buffer.
  j = 0;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Fill up the input array.  The second index of the
  // array is used as follows: a value of 0 references
  // the real component of the signal, and a value of 1
  // references the imaginary component of the signal.
  // Each component is windowed so that sidelobes are
  // reduced.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  for (i = 0; i < bufferLength; i += 2)
  {
    // Store the real value.
    fftInputPtr[j][0] = (double)signalBufferPtr[i] * hanningWindow[j];

    // Store the imaginary value.
    fftInputPtr[j][1] = (double)signalBufferPtr[i+1] * hanningWindow[j];

    // Reference the next storage location.
    j += 1; 
  } // for
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // Compute the DFT.
  fftw_execute(fftPlan);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Compute the magnitude-squared value of the spectrum.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  for (i = 0; i < N; i++)
  {
    // Retrive the in-phase and quadrature parts.
    iK = fftOutputPtr[i][0];
    qK = fftOutputPtr[i][1];

    // Compute signal power, |I + jQ|^2.
    power = (iK * iK) + (qK * qK);
    
    // Scale for a normalized output.
    power /= N;

    //--------------------------------------------
    // The fftShiftTable[] allows us to store
    // the FFT output values such that the
    // center frequency bin is in the center
    // of the output array.  This results in a
    // display that looks like that of a spectrum
    // analyzer.
    //--------------------------------------------
    j = fftShiftTable[i];
    //--------------------------------------------

    // Save the values in the proper order.
    spectralPowerBuffer[j] = power;
  } // for
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return (bufferLength / 2);

} // computePowerSpectrum

