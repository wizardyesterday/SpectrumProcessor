After performing analysis of IQ data captures, from my rtlsdr, I thought it
was cool to load an IQ file, segment at a time, and review the power
spectrum of the data.  This motivated me to create my  original signal
analyzer app so that I could display the log power spectrum of the IQ
data.

This particalar app accepts IQ data from stdin and allows computation of
spectral power over a specified two-idedandwidth about center frequency. This
is useful when computing the power of a narrowband signal. There is no
display of the spectrum since I'm only interested in numerical values of
the power. Using these numerical values,  I can average these mean-squared
values and compute the log-power.

The motivation for creating this app was driven by the need to observe the
spectral line of the ring oscillator output of the R82xx devices that are
used in rtl-sdr radio dongles. so that I can characterize the IF gain
versus control voltage that drives the IF gain analoinput of the R82xx device.

