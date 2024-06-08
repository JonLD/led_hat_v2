# Speeding up the Fast Fourier Transform
Problem statement: the FFT is slow. Let's see if we can make it faster.

## Baseline
The baseline method is:
* Run `computeFFT()` in `loop()`
* Generate some non-trivial (i.e. not all zeros) data to pass in
* Use [`esp_timer_get_time`][timer_get_time] to get the elapsed number of microseconds using the
  hardware timer.
* Print the number of micros taken over serial-via-USB, captured to a log file
* Post-process and plot

This method gave the following performance:
![Performance graph for the unmodified code](/initial_timings.png "Performance graph for the unmodified code")

The obvious observation is that the time taken to compute is trimodal; it takes about 16ms, then
14ms, then settles at a reliable 10ms. The reason for this is not immediately obvious, but could be
related to processor initialisation, something FreeRTOS or Arduino are doing, or a specialising
optimization in some bit of the assembly/silicon. Because all of these are very difficult to
diagnose, and in a much longer run the 10ms seems constant, it is presumed to be an initialisation
quirk and ignored. The average duration for the baseline is taken to be 10ms.

## Double Trouble
The first improvement is to upgrade ArduinoFFT from its current v1.6 to >v2.0, because it converts
the `ArduinoFFT` class into a [C++ template][cpp_template]. This allows us to initialise the class
with `float` instead of `double` arguments, for which the ESP32 is much better prepared - its
onboard FPU can [only support single precision][esp32_perf_guide], so all double precision
floating-point maths is emulated in software. Incidentally, that webpage describes several other
techniques that we'll try in this analysis.

Swapping the code to use single- instead of double-precision gives a roughly 3x improvement in
performance.
![Single precision performance](/single_precision_timings.png "Single precision performance")

## Optimise ArduinoFFT Library Calls
The ArduinoFFT library has a [set of performance recommendations][arduinofft_optimization], of which
2 are easily applicable to us. Defining `FFT_SPEED_OVER_PRECISION` and `FFT_SQRT_APPROXIMATION`
before importing `ArduinoFFT.h` will use optimisations in the (slow) square root operation.

Further, the newer version of ArduinoFFT has an extra argument `windowingFactors`, an optional bool
which defaults to `false`. When set to `true`, it stores the factors computed in `windowing()`
rather than recalculating them every time. Because we're using a single instance and a fixed sample
size, we can use this optimisation.

This all results in more performance improvements, down to sub-millisecond compute times.
![Performance graph for optimised ArduinoFFT](/arduinofft_optimisations_timings.png "Optimised ArduinoFFT")

## Compiler Optimisations
PlatformIO compiles by default with the GCC compiler flag `-Os` ("optimise for size, then speed")
which turns on most of `-O2` but leaves out things that involve extra static data. GCC's
most-optimised option is `-O3`, which increases the binary size but increases the speed. In this
case, that is an acceptable trade off because it isn't (yet) limiting.

This further speeds up the FFT algorithm.
![Performance with O3](/compiler_optimisation_timings.png "Performance with O3")

It is notable that this has also increased the jitter in the timing. The reason for this is unclear.

## IRAM
Forcing functions into IRAM, Instruction RAM, can sometimes improve performance further. Espressif
[suggest it for "hot" code][esp_iram] where it _might_ improve performance, by mitigating the
penalty of loading the function from ROM. However, in testing it did not have a meaningful effect on
`computeFFT` or `ArduinoFFT::Compute`.
![Performance with functions in IRAM](/compute_iram_timings.png "Performance with functions in IRAM")

## Fixed Point
The ideal optimisation is to avoid floating point maths altogether and do the computations with
integers. However, this requires a bigger rewrite of the code.

`i2s_read` already reads 32-bit numbers into the buffer, but we need functions that can work with
integers - ArduinoFFT can only work with floating point numbers. The [esp_dsp][esp_dsp] module comes
from Espressif and has not only a fixed-point implementation, it is written in assembly and
optimised for the ESP32.

Implementing a rough implementation (just enough to get an idea of performance, not validated) based
on reading the source and looking at [this StackOverflow][fft_so] answer, the performance comes out
as taking around 530us. Some of the C could probably be made even faster with some clever
algorithmic thinking and by specialising for this problem.

![Performance with fixed point](/fixed_point_timings.png "Performance with fixed point")

However, the code for this is much more complicated than the equivalent floating point, and the
performance gain is small compared to the original 10ms. This optimisation is only worthwhile if the
performance is actually needed.

# Conclusion
The following optimisations are worthwhile and can, together, reduce the FFT compute time from ~10ms
to ~800us:
* Upgrade `ArduinoFFT` and use `float` instead of `double`
* Optimise calls and instantiation of `ArduinoFFT`
* Change the compiler optimiser flags to `-O3`

The following optimisations were not worth implementing:
* Explicitly move hot functions to IRAM (no discernable performance improvement)
* Fixed point arithmetic (very complex for relatively small improvement)

[timer_get_time]: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html#obtaining-current-time
[cpp_template]: https://en.cppreference.com/w/cpp/language/templates
[esp32_perf_guide]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/performance/speed.html#improving-overall-speed
[arduinofft_optimization]: https://github.com/kosme/arduinoFFT/wiki/optimizations
[esp_iram]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/memory-types.html#iram-instruction-ram
[esp_dsp]: https://docs.espressif.com/projects/esp-dsp/en/latest/esp32/esp-dsp-library.html
[fft_so]: https://stackoverflow.com/questions/78490683/esp32-fft-im-not-getting-the-correct-max-frequency-of-the-input-signal
