#include "Wave.h"
#include <intrin.h>
#include <iostream>
#include "Stopwatch.h"
#include <queue>

using namespace std;

int main(int argc, char* argv[])
{
	//gather all of our data from passed arguments
    string filename = argv[1];
	alignas(16) float decay = stof(argv[2]);
	float delay = stof(argv[3]);
    
	//Check if we can even use this file
	Wave w(filename);
	if (w.format.format != Wave::FormatCode::FLOAT) 
	{
		cout << "Not a float wave\n";
		return 1;
	}

	__m128 decayX = _mm_load1_ps(&decay); // adds 4 floats to this __m128
    
	//set up our stop watch
    Stopwatch swatch;
    swatch.start();

	// find the number of itterations we need to complete
    unsigned totalFloats = w.numFrames * w.format.bytesPerFrame / 4;
    unsigned totalXMMs = totalFloats / 4;

	unsigned int count = 0;
	unsigned int echoStart = delay * w.format.samplesPerSecond * w.format.numChannels;
    float* f = (float*) w.data(); //pull the data from the WAV
	float* f2 = (float*)w.data(); //pull the data from the WAV

	// add the effect
    for(unsigned i=0;i<totalXMMs;i++)
	{
        __m128 tmp = _mm_load_ps(f); // load in the current frame
		if (count > echoStart)
		{
			__m128 tmp2 = _mm_load_ps(f2); // load in the current frame
			tmp2 = _mm_mul_ps(tmp2, decayX); // multiply it by the decay factor
			tmp = _mm_add_ps(tmp, tmp2); // add it to the current frame
			f2 += 4;
		}
        _mm_store_ps(f,tmp);
        f += 4;
		count += 4;
    }

	//tell the user how long this took
    swatch.stop();
    cout << swatch.elapsed_us() << "usec\n";
	//write out the WAV file
    w.write("out.wav");
    return 0;
}
