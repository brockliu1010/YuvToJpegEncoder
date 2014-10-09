#include <iostream>
#include <chrono>

#include "../include/JPEGEncoder.h"

using namespace std;
const int WIDTH = 1280;
const int HEIGHT = 720;
static unsigned char img_data[WIDTH*HEIGHT*3] = {0};

int main()
{
	
 	GenerateYUY2Data(img_data, WIDTH, HEIGHT);
    //GenerateNV21Data(img_data, WIDTH, HEIGHT);
	for(int i = 0; i < 5; i++)
	{
		YuvToJpegEncoder *encoder = new YUY2ToJpegEncoder(WIDTH, HEIGHT, 80);
		printf("%d\n", i);
		std::vector<unsigned char> jpeg;
 		jpeg.clear();
        auto t0 = std::chrono::high_resolution_clock::now();    // 获取一个时间点 time_point
 		int encode_len = encoder->encode(img_data, jpeg);
        auto t1 = std::chrono::high_resolution_clock::now();    // 获取另外一个时间点 time_point
        cout << std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count() << endl; 
 		char name[64];
 		sprintf(name, "img/%d.jpg", i);
 		FILE *pjpeg = fopen(name, "w");
 		fwrite(jpeg.data(), sizeof(unsigned char), encode_len, pjpeg);
 		fclose(pjpeg);
 		delete encoder;
 	}
 	
 	return 0;
}
