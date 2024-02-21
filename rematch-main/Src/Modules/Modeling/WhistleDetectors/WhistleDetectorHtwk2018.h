// /*
//  * This is a port and adaptation of the HTWK open source whistle detection code
//  * to our framework and pffft instead of fftw3
//  *
//  * @file: WhistleDetectorHtwk.h
//  *
//  * @author: Rudi Villing
//  */

// #ifndef AUDIOINPUT_MODULES_WHISTLEDETECTORHTWK_H_
// #define AUDIOINPUT_MODULES_WHISTLEDETECTORHTWK_H_

// #include <vector>
// #include <numeric>

// #include <boost/circular_buffer.hpp>

// #include "util/REModuleFramework/module.h"
// //#include "util/RingBuffer.h"

// #include "memoryUnits/audioInput/AudioInputData.h"
// #include "memoryUnits/audioInput/WhistleDetectionOutput.h"
// #include "memoryUnits/audioInput/WhistleDetectionState.h"



// class PFFFT_Setup;

// class WhistleDetectorHtwkBase : public REMF::ModuleBase
// {
// protected:
//     // uses/requires
//     const WhistleDetectionState &theWhistleDetectionState;
//     const AudioInputData &theAudioInputData;
//     // provides
//     WhistleDetectionOutput &theWhistleDetectionOutput;

// public:
//     MODULE(WhistleDetectorHtwkBase)
//         USES(WhistleDetectionState),
//         REQUIRES(AudioInputData),
//         PROVIDES(WhistleDetectionOutput)
//     END_MODULE()
// };

// class WhistleDetectorHtwk : public WhistleDetectorHtwkBase
// {
// public:
//     class Params
//     {
//     public:
//         Params();

//         int htwkFFTSize; // must relate to periodFrames in audioInput - htwk calls frame size
//         int htwkSpecStep; // htwk call block size
//         int fftSize; // htwk calls frame_size but derives it from value returned by alsa and reqFrameSize=441
//     };

//     WhistleDetectorHtwk(REMF::Memory &memory);
//     virtual ~WhistleDetectorHtwk();

//     void init();
//     void processFrame();

// private:
//     // params
//     Params p;

//     PFFFT_Setup *hFFT;
//     float *fftInput;
//     float *fftOutput;

//     int fftSize;
//     int spectrumSize;
//     size_t numFeaturesNoise;
//     size_t coarseSpecStep;
//     std::vector<float> magSpectrum;

//     typedef std::vector<float> FeatureVec;

//     boost::circular_buffer<FeatureVec> lastAudio;
//     FeatureVec audioSum;
//     float noiseSum;;
//     boost::circular_buffer<float> lastNoise;
//     unsigned int numWindowsInit;



//     void processWhistleDetection();
//     void sumFreqBins(const FeatureVec& fullRes, FeatureVec& decimated);


//     inline float s16ToFloat(int16_t inVal)
//     {
//         return (float) inVal / SHRT_MAX;
//     }

//     inline float calcMagnitude(float xReal, float xImag)
//     {
//         return sqrt(xReal*xReal + xImag*xImag);
//     }

//     template <class Container1, class Container2>
//     void pointwise_pluseq(Container1& in1, const Container2& in2)
//     {
//         std::transform(in1.begin(), in1.end(), in2.begin(), in1.begin(),
//                        std::plus<typename Container1::value_type>());
//     }

//     template <class Container1, class Container2>
//     void pointwise_minuseq(Container1& in1, const Container2& in2)
//     {
//         std::transform(in1.begin(), in1.end(), in2.begin(), in1.begin(),
//                        std::minus<typename Container1::value_type>());
//     }

//     template <class Container>
//     typename Container::value_type sum(const Container& in)
//     {
//         return std::accumulate(in.begin(), in.end(), 0);
//     }

// };

// #endif /* AUDIOINPUT_MODULES_WHISTLEDETECTORHTWK_H_ */
