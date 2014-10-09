#include "../include/JPEGEncoder.h"

int YuvToJpegEncoder::PIXEL_FORMAT_NV21 = 0x11;
int YuvToJpegEncoder::PIXEL_FORMAT_YUY2 = 0x14;

YuvToJpegEncoder::YuvToJpegEncoder(const int width, const int height, const int quality)
{
	cinfo_ = jpeg_compress_struct_wrapper_;
	cinfo_->err = jpeg_std_error(&jerr_);

	jpeg_mem_dst(cinfo_, &mgr_);

	cinfo_->image_width = width;
	cinfo_->image_height = height;
	cinfo_->input_components = 3;
	cinfo_->in_color_space = JCS_YCbCr;
	jpeg_set_defaults(cinfo_);

	jpeg_set_quality(cinfo_, quality, TRUE);
	jpeg_set_colorspace(cinfo_, JCS_YCbCr);
	cinfo_->raw_data_in = TRUE;
	cinfo_->dct_method = JDCT_FASTEST;
    cinfo_->write_JFIF_header = FALSE;
    cinfo_->restart_interval = 10;
}

YuvToJpegEncoder::~YuvToJpegEncoder()
{
}

int YuvToJpegEncoder::encode(unsigned char* in_yuv, std::vector<unsigned char> &out_jpeg)
{
	jpeg_start_compress(cinfo_, TRUE);

/*	time_t start, finish;*/
/*	start = clock();*/
	compress(cinfo_, in_yuv);
/*	finish = clock();*/
/*	double totaltime = (double)(finish-start)/CLOCKS_PER_SEC * 1000;*/
/*	LOGD("compress Time = %lfms\n", totaltime);*/

	jpeg_finish_compress(cinfo_);
	out_jpeg = mgr_.data;
	return out_jpeg.size();
}

NV21ToJpegEncoder::NV21ToJpegEncoder(const int width, const int height, const int quality)
	: YuvToJpegEncoder(width, height, quality)
{
	u_rows_ = new unsigned char [8 * (width >> 1)];
	v_rows_ = new unsigned char [8 * (width >> 1)];
	config_sampling_factors(cinfo_);
}

NV21ToJpegEncoder::~NV21ToJpegEncoder() 
{
	delete [] u_rows_;
	delete [] v_rows_;
};

void NV21ToJpegEncoder::config_sampling_factors(jpeg_compress_struct* cinfo)
{
	cinfo->comp_info[0].h_samp_factor = 2;
	cinfo->comp_info[0].v_samp_factor = 2;
	cinfo->comp_info[1].h_samp_factor = 1;
	cinfo->comp_info[1].v_samp_factor = 1;
	cinfo->comp_info[2].h_samp_factor = 1;
	cinfo->comp_info[2].v_samp_factor = 1;
}

void NV21ToJpegEncoder::compress(jpeg_compress_struct* cinfo, unsigned char* yuv)
{
	JSAMPROW y[16];
	JSAMPROW cb[8];
	JSAMPROW cr[8];
	JSAMPARRAY planes[3];
	planes[0] = y;
	planes[1] = cb;
	planes[2] = cr;

	int width = cinfo->image_width;
	int height = cinfo->image_height;
	unsigned char* y_planar = yuv;
	unsigned char* vu_planar = yuv + width * height;

	// process 16 lines of Y and 8 lines of U/V each time.
	while (cinfo->next_scanline < cinfo->image_height) {
		//deitnerleave u and v
		deinterleave(vu_planar, u_rows_, v_rows_, cinfo->next_scanline, width, height);

		// Jpeg library ignores the rows whose indices are greater than height.
		for (int i = 0; i < 16; i++) {
			// y row
			y[i] = y_planar + (cinfo->next_scanline + i) * width;

			// construct u row and v row
			if ((i & 1) == 0) {
				// height and width are both halved because of downsampling
				int offset = (i >> 1) * (width >> 1);
				cb[i/2] = u_rows_ + offset;
				cr[i/2] = v_rows_ + offset;
			}
		}
		jpeg_write_raw_data(cinfo, planes, 16);
	}
}

void NV21ToJpegEncoder::deinterleave(unsigned char* vu_planar, unsigned char* u_row,
									 unsigned char* v_row, int row_index, int width, int height)
{
	int num_rows = (height - row_index) / 2;
	if (num_rows > 8) num_rows = 8;
	for (int row = 0; row < num_rows; ++row) {
		int offset = ((row_index >> 1) + row) * width;
		unsigned char* vu = vu_planar + offset;
		for (int i = 0; i < (width >> 1); ++i) {
			int index = row * (width >> 1) + i;
			u_row[index] = vu[1];
			v_row[index] = vu[0];
			vu += 2;
		}
	}
}

YUY2ToJpegEncoder::YUY2ToJpegEncoder(const int width, const int height, const int quality)
	: YuvToJpegEncoder(width, height, quality)
{
	y_rows_ = new unsigned char [16 * width];
	u_rows_ = new unsigned char [16 * (width >> 1)];
	v_rows_ = new unsigned char [16 * (width >> 1)];
	config_sampling_factors(cinfo_);
}

YUY2ToJpegEncoder::~YUY2ToJpegEncoder() 
{
	delete [] y_rows_;
	delete [] u_rows_;
	delete [] v_rows_;
};

void YUY2ToJpegEncoder::config_sampling_factors(jpeg_compress_struct* cinfo)
{
	cinfo->comp_info[0].h_samp_factor = 2;
	cinfo->comp_info[0].v_samp_factor = 2;
	cinfo->comp_info[1].h_samp_factor = 1;
	cinfo->comp_info[1].v_samp_factor = 2;
	cinfo->comp_info[2].h_samp_factor = 1;
	cinfo->comp_info[2].v_samp_factor = 2;
}

void YUY2ToJpegEncoder::compress(jpeg_compress_struct* cinfo, unsigned char* yuv)
{
	JSAMPROW y[16];
	JSAMPROW cb[16];
	JSAMPROW cr[16];
	JSAMPARRAY planes[3];
	planes[0] = y;
	planes[1] = cb;
	planes[2] = cr;

	int width = cinfo->image_width;
	int height = cinfo->image_height;

	unsigned char* yuv_offset = yuv;

	// process 16 lines of Y and 16 lines of U/V each time.
	while (cinfo->next_scanline < cinfo->image_height) {
		deinterleave(yuv_offset, y_rows_, u_rows_, v_rows_, cinfo->next_scanline, width, height);

		// Jpeg library ignores the rows whose indices are greater than height.
		for (int i = 0; i < 16; i++) {
			// y row
			y[i] = y_rows_ + i * width;

			// construct u row and v row
			// width is halved because of downsampling
			int offset = i * (width >> 1);
			cb[i] = u_rows_ + offset;
			cr[i] = v_rows_ + offset;
		}

		jpeg_write_raw_data(cinfo, planes, 16);
	}
}

void YUY2ToJpegEncoder::deinterleave(unsigned char* yuv, unsigned char* y_row, unsigned char* u_row,
						 unsigned char* v_row, int row_index, int width, int height)
{
	int num_rows = height - row_index;
	if (num_rows > 16) num_rows = 16;
	for (int row = 0; row < num_rows; ++row) {
		unsigned char* yuv_seg = yuv + (row_index + row) * width * 2;
		for (int i = 0; i < (width >> 1); ++i) {
			int y_index = row * width + (i << 1);
			int u_index = row * (width >> 1) + i;
			y_row[y_index] = yuv_seg[0];
			y_row[y_index + 1] = yuv_seg[2];
			u_row[u_index] = yuv_seg[1];
			v_row[u_index] = yuv_seg[3];
			yuv_seg += 4;
		}
	}
}
