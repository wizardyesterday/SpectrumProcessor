//**************************************************************************
// file name: SpectrumProcessor.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as apectrum
// processor.  Given 8-bit IQ samples from an SDR, spectral power can be
// computed over a specfied two-sided bandwidth.
///_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __SPECTRUMPROCESSOR__
#define __SPECTRUMPROCESSOR__

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#include <fftw3.h>

// This is the FFT size.
#define N (8192)

class SpectrumProcessor
{
  //***************************** operations **************************

  public:

  SpectrumProcessor(float sampleRate);
  ~SpectrumProcessor(void);

  float computePower(lowPassBandwidth);

  private:

  //*******************************************************************
  // Utility functions.
  //*******************************************************************
  void initializeFftw(void);
  computePowerSpectrum(int8_t *signalBufferPtr,uint32_t bufferLength);

  //*******************************************************************
  // Attributes.
  //*******************************************************************
  // This will be used to mappiing lowpass bandwidth to FFT bin index.
  float fftBinResolutionInHz;

  // This is used for signal power results.
  float spectralPowerBuffer[N];

  // This will be used to swap the upper and lower halves of an array.
  uint32_t fftShiftTable[N];

  // This will be used for windowing data before the FFT.
  double hanningWindow[N];

  // FFTW3 support.
  fftw_complex *fftInputPtr;
  fftw_complex *fftOutputPtr;
  fftw_plan fftPlan;

#endif // __SPECTRUMPROCESSOR__
