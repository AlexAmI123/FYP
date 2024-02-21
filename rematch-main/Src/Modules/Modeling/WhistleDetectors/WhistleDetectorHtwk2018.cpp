// /*
//  * @file: WhistleDetectorHtwk.cpp
//  *
//  * @author: Rudi Villing
//  */

// #include "WhistleDetectorHtwk.h"

// #include <cmath>

// #include "thirdparty/pffft/pffft.h"

// #include "util/Logger.h"
// #include "util/Stopwatch.h"


// static LoggerPtr logger = Logging::getLogger("WhistleDetectorHtwk");
// static LoggerPtr perfLogger = Logging::getLogger("perf.WhistleDetectorHtwk");


// // =========================================================================

// WhistleDetectorHtwk::Params::Params()
//         : htwkFFTSize(441),
//           htwkSpecStep(10), // corresponds to approx 1000 Hz
//           fftSize(256)
// {
// }

// // =========================================================================

// WhistleDetectorHtwk::WhistleDetectorHtwk(REMF::Memory &memory)
//         : WhistleDetectorHtwkBase(memory),
//           hFFT(NULL),
//           fftInput(NULL),
//           fftOutput(NULL),
//           fftSize(-1),
//           spectrumSize(-1), // htwk spec_size
//           numFeaturesNoise(-1), // htwk num_features_noise
//           lastAudio(60), // num audio windows to average for smoothing
//           lastNoise(1000), // num windows to average for background noise estimation
// //          lastAudio(20), // num audio windows to average for smoothing
// //          lastNoise(100), // num windows to average for background noise estimation
//           numWindowsInit(0)
// {
// }

// void WhistleDetectorHtwk::init()
// {
//     // prepare for FFT

//     fftSize = p.fftSize; // TODO - htwk derive from ALSA period???

//     hFFT = pffft_new_setup(fftSize, PFFFT_REAL);
//     fftInput = (float*) pffft_aligned_malloc(fftSize*sizeof(float));
//     fftOutput = (float*) pffft_aligned_malloc(fftSize*sizeof(float));


//     // ensure input FFT buffer is initially zero'd
//     // (output FFT will be completely overwritten each time so no need to zero it)
//     for (int i=0; i < fftSize; i++)
//         fftInput[i] = 0;

//     spectrumSize = fftSize/2+1;
//     magSpectrum.resize(spectrumSize, 0.0f);

//     LOGI(logger, "init: spectrumSize {}", spectrumSize);

// //    int htwkSpecSize = theAudioInputData.periodFrames/2 + 1;
// //    int htwkNumFeatures = htwkSpecSize/p.coarseSpecStep;
// //
// //    freqStepSize = spectrumSize / htwkNumFeatures;
// //    numFeaturesNoise = spectrumSize / freqStepSize;

//     coarseSpecStep = round(float(p.htwkSpecStep) * p.fftSize / p.htwkFFTSize);
//     numFeaturesNoise = spectrumSize / coarseSpecStep;

//     LOGI(logger, "init: HTWK fft {}, step {} bins ({:.1f} Hz), OUR fft {}, step {} bins ({:.1f} Hz), numFeaturesNoise {}",
//             p.htwkFFTSize, p.htwkSpecStep,
//             float(theAudioInputData.sampleRate)*p.htwkSpecStep/p.htwkFFTSize,
//             fftSize, coarseSpecStep,
//             float(theAudioInputData.sampleRate)*coarseSpecStep/fftSize,
//             numFeaturesNoise);

//     //spectrumSize / freqStepSize
//     audioSum.resize(numFeaturesNoise, 0.0f);

//     lastAudio.assign(lastAudio.capacity(), FeatureVec(numFeaturesNoise, 0));
//     lastNoise.assign(lastNoise.capacity(), 0.0f);

// }

// WhistleDetectorHtwk::~WhistleDetectorHtwk()
// {
//     if (hFFT)
//         pffft_destroy_setup(hFFT);

//     if (fftInput)
//         pffft_aligned_free(fftInput);
//     if (fftOutput)
//         pffft_aligned_free(fftOutput);

// }

// void WhistleDetectorHtwk::processFrame()
// {
//     // ensure whistle detection has been reset if detection not enabled
//     if (!theWhistleDetectionState.detectionEnabled)
//     {
//         numWindowsInit = 0;
//         theWhistleDetectionOutput.filteredDetection = false;
//     }
//     else // whistle detection is enabled (e.g. in RoboCup SET state)
//     {
//         if (theAudioInputData.valid)
//             processWhistleDetection();
//         // else just keep filteredDetection as is
//     }
// }

// void WhistleDetectorHtwk::processWhistleDetection()
// {
//     LOG_STOPWATCH_SCOPE_EVERY_MS(perfLogger, LogLevel::LVL_DEBUG, "processWhistleDetection", 5000); // 5 secs

//     static const int numFeaturesDetection = 6;
//     static const float coefficients[] = { -19.1711084f, -19.1711084f,
//             16.8218568f, 15.6424958f, 1.1391718f, -19.1711084f };

//     // copy from raw audio to format suitable for FFT
//     const int16_t *pSrc = theAudioInputData.interleaved.data();
//     float *pDest = fftInput;
//     int numInput = std::min(fftSize, int(theAudioInputData.interleaved.size()/theAudioInputData.numChannels));
//     for (int i = 0; i < numInput; i++, pSrc += theAudioInputData.numChannels, pDest++)
//         *pDest = s16ToFloat(*pSrc);

//     // FFT the data
//     pffft_transform_ordered(hFFT, fftInput, fftOutput, NULL, PFFFT_FORWARD);

//     // calculate magnitude spectrum
//     magSpectrum[0] = fftOutput[0]; // real value DC and fs/2 components
//     magSpectrum[spectrumSize-1] = fftOutput[1];
//     for (int i=1; i < spectrumSize-1; i++)
//         magSpectrum[i] = calcMagnitude(fftOutput[i*2], fftOutput[i*2+1]);

//     // sum the frequency bins
//     FeatureVec coarseSpec(numFeaturesNoise, 0);

//     sumFreqBins(magSpectrum, coarseSpec);

//     // average audio over several frames
//     pointwise_minuseq(audioSum, lastAudio.front());
//     pointwise_pluseq(audioSum, coarseSpec);
//     lastAudio.push_back(coarseSpec);

//     // average noise over several frames
//     float noise = sum(audioSum);
//     noiseSum -= lastNoise.front();
//     noiseSum += noise;
//     lastNoise.push_back(noise);

//     // only process further if we have processed enough noise averaged to
//     // detect differences from average noise level

//     if (numWindowsInit >= lastNoise.capacity())
//     {
//         float noiseAvg = noiseSum / lastNoise.capacity() / numFeaturesNoise;
//         float sum = 0.f;
//         float thresh = 10.0f;//was 10

//         for (FeatureVec::iterator iter = audioSum.begin();
//                 iter != audioSum.end(); ++iter)
//         {

//             float f = *iter;
//             if (f >= noiseAvg * thresh)
//                 sum += f;
//             //LOGV(logger, "Amplitude = {}, NoiseAvg*Threshold = {}", f, noiseAvg * thresh);
//         }

//         float limit = std::max(sum / numFeaturesNoise, noiseAvg * thresh);
//         float score = 0.6050399f;
//         for (int i = 0; i < numFeaturesDetection; i++)
//         {
//             score += ((audioSum[i] >= limit) ? coefficients[i] : 0.f);
//         }

// //        theWhistleDetectionOutput.filteredDetection =
// //                (std::exp(score)/(1+std::exp(score)) > 0.9f);

//         float logistic = 1.0f / (1.0f + std::exp(-score));
//         theWhistleDetectionOutput.filteredDetection = (logistic > 0.9f);

//         LogLevel::Level level = theWhistleDetectionOutput.filteredDetection ?
//                 LogLevel::LVL_INFO : LogLevel::LVL_VERBOSE;

//         LOG(logger, level, "whistle score {}, logistic {}, filtered detection {}",
//                 score, logistic, theWhistleDetectionOutput.filteredDetection);
//     }
//     else
//     {
//         numWindowsInit++;
//         theWhistleDetectionOutput.filteredDetection = false;

//         if (numWindowsInit < lastNoise.capacity())
//             LOGV(logger, "whistle initializing noise {} / {}",
//                     numWindowsInit, lastNoise.capacity());
//         else
//             LOGI(logger, "whistle finished intializing noise analysis");
//     }
// }

// void WhistleDetectorHtwk::sumFreqBins(const FeatureVec& fullRes, FeatureVec& lowRes)
// {
//     for (size_t i = 0; i < numFeaturesNoise; i++)
//     {
//         for (size_t j = i * coarseSpecStep; j < (i+1) * coarseSpecStep; j++)
//         {
//             lowRes[i] += fullRes[j];
//         }
//     }
// }
