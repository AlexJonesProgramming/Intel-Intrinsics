//#include "pch.h"
#include "ImageReaders.h"
#include "Stopwatch.h"
#include <iostream>
#include <intrin.h>
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
	using std::cout;

	Targa img(argv[1]);
	const TargaHeader& H = img.header();
	uint8_t* p = (uint8_t*)img.data();
	unsigned numPixels = H.width*H.height;
	unsigned bytesPerPixel = H.bitsPerPixel / 8;
	alignas(16) vector<uint8_t> outPix;
	outPix.reserve(H.width*H.height * 3 + 50);

	uint8_t*pixleBuf = outPix.data();

	uint8_t* rowStart;
	int rowDelta;

	if ((H.descriptor == 0) || (H.descriptor == 8)) {
		//lower left origin BRG
		rowStart = (uint8_t*)img.data();
		rowDelta = H.width*bytesPerPixel;
	}
	else {
		//upper left origin
		rowStart = (uint8_t*)img.data() + (H.height - 1)*H.width*bytesPerPixel;
		rowDelta = -H.width*bytesPerPixel;
	}
	Stopwatch sw;
	sw.start();

	if ((H.descriptor == 0) || (H.descriptor == 32)) // works for type one
	{
		for (unsigned y = 0; y < H.height; ++y) //move thorugh the height
		{
			p = rowStart; // sets the row
			rowStart += rowDelta; //moves to next row
			for (unsigned x = 0; x < H.width; x += 4) //move through the width
			{
				__m128i v = _mm_load_si128((__m128i*)p); //grab the RGB data from P
				p += bytesPerPixel * 4; // move P over
				_mm_store_si128((__m128i*)pixleBuf, v); //store the pixel data at PixelBuf
				pixleBuf += 16; //move outPix over to the next RGB
			}
		}
	}


	else if ((H.descriptor == 8) || (H.descriptor == 40)) // BGRA we want BGR
	{
		char malt[16] = { 0,1,2,4,5,6,8,9,10,12,13,14,0x80,0x80,0x80,0x80 };
		__m128i m = _mm_load_si128((__m128i*) malt);

		for (unsigned y = 0; y < H.height; ++y) //move thorugh the height
		{
			p = rowStart; // sets the row
			rowStart += rowDelta; //moves to next row
			for (unsigned x = 0; x < H.width; x += 4) //move through the width
			{
				__m128i v = _mm_load_si128((__m128i*)p); //grab the RGBA data from P

				v = _mm_shuffle_epi8(v, m); // moves all the BGR values to the front of the buffer

				p += bytesPerPixel * 4; // move P over
				_mm_store_si128((__m128i*)pixleBuf, v); //store the pixel data at PixelBuf
				pixleBuf += 12; //move outPix over to the next RGBA
			}
		}
	}

	sw.stop();
	cout << "Time: " << sw.elapsed_us() << " microseconds\n";
	Bitmap outbmp(H.width, H.height, outPix.data());
	outbmp.write("out.bmp");
	return 0;
}
