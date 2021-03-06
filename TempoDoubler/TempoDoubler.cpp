#include "Wave.h"
#include <intrin.h>
#include <iostream>
#include "Stopwatch.h"
#include <queue>
#include <fstream>
#include <string>
#include <mmintrin.h> //mmx
#include <xmmintrin.h> //sse
#include <emmintrin.h> //sse2
#include <pmmintrin.h> //sse3
#include <tmmintrin.h> //ssse3
#include <smmintrin.h> //sse4.1
#include <nmmintrin.h> //sse4.2
#include <immintrin.h> //avx, avx2

using namespace std;


int main(int argc, char* argv[])
{
	string filename = argv[1];

	//Check if we can even use this file
	Wave w(filename);
	if (w.format.format != Wave::FormatCode::FLOAT)
	{
		cout << "Not a float wave\n";
		return 1;
	}

	// find the number of itterations we need to complete
	unsigned totalFloats = w.numFrames * w.format.bytesPerFrame / sizeof(float);
	unsigned totalXMMs = totalFloats / 16;

	//pull the data from the WAV 
	float* f = (float*)w.data();
	vector<float> outArray;
	outArray.resize(totalFloats/2);
	float* outPtr = (float*)outArray.data();

	//set up our stop watch
	Stopwatch swatch;
	swatch.start();

	int BotMask[] = {0,2,4,6,2,6,3,7}; //move the frames we want to the bottom
	int TopMask[] = {0,0,0,0,0,2,4,6}; //move the frames we want to the top

	__m256i idx1 = _mm256_loadu_si256((__m256i*)BotMask); //set the mask
	__m256i idx2 = _mm256_loadu_si256((__m256i*)TopMask); // set the mask
	// add the effect
	for (unsigned i = 0; i < totalXMMs; i++)
	{
		//grab 16 frames
		__m256 tmp1 = _mm256_loadu_ps(f);
		f += 8;
		__m256 tmp2 = _mm256_loadu_ps(f);
		f += 8;

		//shuffel the frames we want to the bottom
		tmp1 = _mm256_permutevar8x32_ps(tmp1, idx1); //move the frames forward
		//shuffel the frames we want to the Top
		tmp2 = _mm256_permutevar8x32_ps(tmp2, idx2); //move the frames to the back

		//Combine the bottom of tmp1, and top of tmp 2
		__m256 out = _mm256_blend_ps(tmp1, tmp2, 240); //240 is binary 11110000

		//store the data
		_mm256_store_ps(outPtr, out);
		outPtr += 8;
	}

	//tell the user how long this took
	swatch.stop();
	cout << swatch.elapsed_us() << "usec\n";
	//write out the WAV file
	f = (float*)w.data();
	outPtr = (float*)outArray.data();
	memcpy(f, outPtr, outArray.size() * sizeof(float));
	w.numFrames = w.numFrames / 2;
	w.write("out.wav");
	return 0;
}