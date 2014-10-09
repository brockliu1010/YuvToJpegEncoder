#ifndef __JPEGIMAGEENCODER_H__
#define __JPEGIMAGEENCODER_H__

#include <iostream>
#include <string>
#include <vector>

#include <jpeglib.h>
#include <jerror.h>

#undef ENCODE_TMP_BUFFER_LEN
#define ENCODE_TMP_BUFFER_LEN 131072L

typedef struct _jpeg_destination_mem_mgr
{
	jpeg_destination_mgr mgr;
	std::vector<unsigned char> data;
} jpeg_destination_mem_mgr;

static void mem_init_destination( j_compress_ptr cinfo )
{
	jpeg_destination_mem_mgr* dst = (jpeg_destination_mem_mgr*)cinfo->dest;
	dst->data.resize( ENCODE_TMP_BUFFER_LEN );
	cinfo->dest->next_output_byte = dst->data.data();
	cinfo->dest->free_in_buffer = dst->data.size();
}

static void mem_term_destination( j_compress_ptr cinfo )
{
	jpeg_destination_mem_mgr* dst = (jpeg_destination_mem_mgr*)cinfo->dest;
	dst->data.resize( dst->data.size() - cinfo->dest->free_in_buffer );
}

static boolean mem_empty_output_buffer( j_compress_ptr cinfo )
{
	jpeg_destination_mem_mgr* dst = (jpeg_destination_mem_mgr*)cinfo->dest;
	size_t oldsize = dst->data.size();
	dst->data.resize( oldsize + ENCODE_TMP_BUFFER_LEN );
	cinfo->dest->next_output_byte = dst->data.data() + oldsize;
	cinfo->dest->free_in_buffer = ENCODE_TMP_BUFFER_LEN;
	return true;
}

static void jpeg_mem_dst( j_compress_ptr cinfo, jpeg_destination_mem_mgr * dst )
{
	cinfo->dest = (jpeg_destination_mgr*)dst;
	cinfo->dest->init_destination = mem_init_destination;
	cinfo->dest->term_destination = mem_term_destination;
	cinfo->dest->empty_output_buffer = mem_empty_output_buffer;
}

class JpegCompressStructWrapper
{
public:
	JpegCompressStructWrapper()
	{
		jpeg_create_compress(&this->cinfo_);
	}

	~JpegCompressStructWrapper()
	{
		jpeg_destroy_compress(&this->cinfo_);
	}

	operator jpeg_compress_struct*()
	{
		return &this->cinfo_;
	}
private:
	jpeg_compress_struct cinfo_;
};

class YuvToJpegEncoder 
{
public:
	static int PIXEL_FORMAT_NV21; // YYYY U V
	static int PIXEL_FORMAT_YUY2; // YUYV YUYV

	YuvToJpegEncoder(const int width, const int height, const int quality);
	virtual ~YuvToJpegEncoder();

	int encode(unsigned char* in_yuv, std::vector<unsigned char> &out_jpeg);

	// libjpeg stuff
	jpeg_compress_struct *cinfo_;
	jpeg_error_mgr jerr_;
	jpeg_destination_mem_mgr mgr_;
	JpegCompressStructWrapper jpeg_compress_struct_wrapper_;

private:
	virtual void config_sampling_factors(jpeg_compress_struct* cinfo) = 0;
	virtual void compress(jpeg_compress_struct* cinfo, unsigned char* yuv) = 0;
};

// YUV420SP NV21 YYYY... U... V...
class NV21ToJpegEncoder : public YuvToJpegEncoder
{
public:
	NV21ToJpegEncoder(const int width, const int height, const int quality);
	virtual ~NV21ToJpegEncoder();
private:
	virtual void config_sampling_factors(jpeg_compress_struct* cinfo);
	virtual void compress(jpeg_compress_struct* cinfo, unsigned char* yuv);
	void deinterleave(unsigned char* vu_planar, unsigned char* u_rows,
		unsigned char* v_row, int row_index, int width, int height);
	unsigned char* u_rows_;
	unsigned char* v_rows_;
};

// YUY2 YUYV Yuv422I YUYVYUYV...
class YUY2ToJpegEncoder : public YuvToJpegEncoder
{
public:
	YUY2ToJpegEncoder(const int width, const int height, const int quality);
	virtual ~YUY2ToJpegEncoder();
private:
	virtual void config_sampling_factors(jpeg_compress_struct* cinfo);
	virtual void compress(jpeg_compress_struct* cinfo, unsigned char* yuv);
	void deinterleave(unsigned char* yuv, unsigned char* y_row, unsigned char* u_rows,
		unsigned char* v_row, int row_index, int width, int height);
	unsigned char* y_rows_;
	unsigned char* u_rows_;
	unsigned char* v_rows_;
};

static const unsigned int yuyv_pattern[] = {
	0xFF4B544C,	// 2 red pixels		255 0	0
	0xB6B21BB2, // 2 orange pixels	255	175	0
	0x15942B95, // 2 green pixels	0	255 0
	0x07A983A9, // 2 blue-green pix	0	255	175
	0x6B1CFF1D, // 2 blue pixels	0	0	255
	0xC552E152, // 2 purple pixels	180	0	255
	0x80FE80FF, // 2 white pixels	255 255 255
	0x80508050, // 2 grey pixels	80	80	80
};

static void GenerateNV21Data(unsigned char *data, const int width, const int height)
{
	unsigned char *py = (unsigned char *)data;
	unsigned char *pu = (unsigned char *)py + width * height;
	unsigned char *pv = (unsigned char *)pu + width * height / 4;
	unsigned char *src;

	int x, y;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width ; x+=2) {
			// select the right pattern in the array
			int *pattern = (int *)&yuyv_pattern[sizeof(yuyv_pattern) / sizeof(yuyv_pattern[0]) * x / width];
			src = (unsigned char *)pattern;

			// copy first and third bytes (Y1 Y2)
			*py++ = *src;
			*py++ = *(src + 2);

			// copy second and fourth byte (U & V) if line number is even
			if ((y & 0x1) == 0) {
				*pu++ = *(src + 1);
				*pv++ = *(src + 3);
			}
		}
	}
}

static void GenerateYUY2Data(unsigned char *data, const int width, const int height)
{
 	int num_patterns = sizeof(yuyv_pattern) / sizeof(yuyv_pattern[0]);
 	int num_pix_per_pattern = 2;
 	int x, y;
 	unsigned char *ptr = data;
 	for (y = 0; y < height; y++) {
 		for (x = 0; x < width ; x += num_pix_per_pattern) {
 			*ptr++ = yuyv_pattern[num_patterns * x / width];
 		}
 	}
/*	FILE *p = fopen("../data/test.yuv", "rb");
	fread(data, sizeof(unsigned char), width * height * 2, p);
	fclose(p);*/
}

static void NV21ToYUY2(unsigned char *nv21, unsigned char *yuy2, const int width, const int height)
{
    unsigned char *y = nv21;
    unsigned char *u = nv21 + width * height;
    unsigned char *v = nv21 + width * height + width * height / 4;
    for(int r = 0; r < height; r++) {
        for(int c = 0; c < width; c+=2) {
           yuy2[r*width*2+c*2] = y[r*width+c];
           yuy2[r*width*2+c*2+2] = y[r*width+c+1];
           yuy2[r*width*2+c*2+1] = u[r/2*width+c];
           yuy2[r*width*2+c*2+3] = v[r/2*width+c];
        }
    }
}
#endif
