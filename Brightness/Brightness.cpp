//#include "pch.h"
#include <iostream>
#include <intrin.h>
#include "image.h"
#include "Stopwatch.h"

using namespace std;

int main(int argc, char* argv[])
{
	alignas(16) float scale = atof(argv[2]);
	alignas(16) float scalex[4] = {scale, scale, scale, 0};
	//make sure we can use the scale passed to us
	if (scale < -255 | scale > 255)
	{
		cout << "I cant use this value : " << scale;
		return 1;
	}

	//load in the image
	Image png(argv[1]);

	// get the number of iterations to perform
	unsigned totalPixels = (png.width() * png.height());
	unsigned totalXMMs = totalPixels;

	// set up our scale to be used by simd
	__m128 scaleX = _mm_load_ps(scalex);

	//set up the max and minfloat
	alignas(16)float max = 255;
	alignas(16)float min = 0;

	__m128 maxX = _mm_load1_ps(&max);
	__m128 minX = _mm_load1_ps(&min);

	//grab our pixel data
	unsigned char* pxl = (unsigned char*)png.pixels();
	
	alignas(16) float f[4];
	int swap[4];

	//set up and start our timer
	Stopwatch swatch;
	swatch.start();

	//perform our magic
	int i = 0;
	for (unsigned j = 0; j < totalXMMs; j++)
	{
		i = j * 4;
		//convert to floats so we can do the math
		f[0] = (float)pxl[i]; f[1] = (float)pxl[i + 1]; f[2] = (float)pxl[i + 2]; f[3] = (float)pxl[i + 3];

		__m128 tmp = _mm_load_ps(f); //grab a pixels (r,g,b,a)
		tmp = _mm_add_ps(tmp, scaleX); //add the scale
		tmp = _mm_max_ps(tmp, minX); //make sure it doesn't go above 255
		tmp = _mm_min_ps(tmp, maxX); //make sure it stays above 0
		_mm_store_ps(f, tmp); //store the data back where it belongs

		// Convert our floats back
		swap[0] = (int)f[0]; swap[1] = (int)f[1]; swap[2] = (int)f[2]; swap[3] = (int)f[3];
		pxl[i] = (unsigned char)swap[0]; pxl[i + 1] = (unsigned char)swap[1]; pxl[i + 2] = (unsigned char)swap[2]; pxl[i + 3] = (unsigned char)swap[3];
	}


	//stop the stop watch and tell the user how long this took
	swatch.stop();
	cout << "With SIMD : " << swatch.elapsed_us() << "usec\n";

	//write out the PNG
	png.writePng("out.png");


	// Do it a second time
	Stopwatch swatch2;
	swatch2.start();

	int size = png.width() * png.height() * 4;
	unsigned char* data = (unsigned char*)png.pixels();

	int pix = 0;
	cout << size << "  size\n";
	for (i = 0; i < size; i++)
	{
		if ((i - 1 % 4) != 0) //do not touch the alpha
		{
			pix = (int)data[i];
			pix += (int)scale;
			if (pix > 255)
				pix = 255;
			else if (pix < 0)
				pix = 0;
			data[i] = (unsigned char)pix;

		}
		
	}

	swatch2.stop();
	cout << "With out SIMD : " << swatch2.elapsed_us() << "usec\n";
	png.writePng("out2.png");
	return 0;
}
