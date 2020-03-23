#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#include <magic.h>

#include <png.h>
#include <jpeglib.h>
#include <setjmp.h>

#define RGB 3
#define RGBA 4

int width, height, width_o, height_o, channels;
const char * type;
png_byte color_type;   // int
png_byte bit_depth;
unsigned char **row_pointers = NULL, **out_png = NULL; // png
JSAMPLE * image_buffer;	// Jpeg: Points to large array of R,G,B-order data

const char * read_MIME_type(char *filename) {
/*
        Multipurpose Internet Mail Extensions
*/
        magic_t handle = magic_open(MAGIC_MIME);
        if (handle == NULL) {
                printf("unable to initialize magic library\n");
                abort();
        }
        if (magic_load(handle, NULL) != 0) {
                printf("cannot load magic database - %s\n", magic_error(handle));
                magic_close(handle);
                abort();
        }
        const char * type = magic_file(handle, filename);
        // image/png
        // image/jpeg
        std::printf("【TYPE】%s\n", type);
        return type;
}

void read_png_file(FILE *fp) {

        // ensure that the size of these structures is correct
        // even with a dynamically linked libpng
        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if(!png) abort();

        png_infop info = png_create_info_struct(png);
        if(!info) abort();

        // Save calling environment for long jump
        if(setjmp(png_jmpbuf(png))) abort();

        // initialize input/output for the PNG file
        png_init_io(png, fp);

        // Reads the information before the actual image data from the PNG file.
        // The function allows reading a file that already has the PNG signature
        // bytes read from the stream.
        png_read_info(png, info);

        width      = png_get_image_width(png, info);
        height     = png_get_image_height(png, info);
        color_type = png_get_color_type(png, info); // 6 color type of row
        bit_depth  = png_get_bit_depth(png, info); // 16 bits,  48 bits per pixel
        std::printf("wifth: %d, height: %d, color_type: %d, bit_depth: %d\n",width,height,color_type,bit_depth);

        // Read any color_type into 8bit depth, RGBA format.
        // See http://www.libpng.org/pub/png/libpng-manual.txt
        if(bit_depth == 16) png_set_strip_16(png);

        // The following code transforms grayscale images of less than 8 to 8 bits,
        // changes paletted images to RGB, and adds a full alpha channel
        // if there is transparency information in a tRNS chunk.
        // This is most useful on grayscale images with bit depths of 2 or 4 or
        // if there is a multiple-image viewing application that wishes to treat all images in the same way.
        if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
        // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
        if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
        if(png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);

        // These color_type don't have an alpha channel then fill it with 0xff.
        if(color_type == PNG_COLOR_TYPE_RGB ||
           color_type == PNG_COLOR_TYPE_GRAY ||
           color_type == PNG_COLOR_TYPE_PALETTE) png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
        if(color_type == PNG_COLOR_TYPE_GRAY ||
           color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);
        png_read_update_info(png, info);

        if (row_pointers) abort();
        row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
        for(int y = 0; y < height; y++) {
                // png_byte * aka unsigned char *
                row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
        }
        png_read_image(png, row_pointers);
        png_destroy_read_struct(&png, &info, NULL);
}

void read_jpeg_file(FILE *fp) {

        unsigned long x, y;
        unsigned long data_size;     // length of the file
        int channels;                //  3 =>RGB   4 =>RGBA
        unsigned char * rowptr[1];    // pointer to an array
        unsigned char * jdata;        // data for the image
        struct jpeg_decompress_struct info; //for our jpeg info
        struct jpeg_error_mgr err;          //the error handler

        info.err = jpeg_std_error(& err);
        jpeg_create_decompress(& info);   //fills info structure

        //if the jpeg file doesn't load
        if(!fp) {
                fprintf(stderr, "Error reading JPEG file!");
                return;
        }
        jpeg_stdio_src(&info, fp);
        jpeg_read_header(&info, TRUE);   // read jpeg file header
        jpeg_start_decompress(&info);    // decompress the file

        //set width and height
        width = info.output_width;
        height = info.output_height;
        channels = info.num_components;
        data_size = width * height * channels;

        // read one scanline at a time
        // put bytes in jdata[] array. Assumes an RGB image
        jdata = (unsigned char *)malloc(data_size);
        while (info.output_scanline < info.output_height) {
                // Enable jpeg_read_scanlines() to fill our jdata array
                rowptr[0] = (unsigned char *)jdata + 3 * info.output_width * info.output_scanline;
                jpeg_read_scanlines(&info, rowptr, 1);
        }

        // modify jdata[] to row_pointers[][] shape for process function
        row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
        for (int y = 0; y < height; y++) {
                row_pointers[y] = &(jdata[y * width * channels]);
        }
        jpeg_finish_decompress(&info);   //finish decompressing
}

void write_png_file(char *filename) {

        FILE *fp = fopen(filename, "wb");
        if(!fp) abort();

        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png) abort();

        png_infop info = png_create_info_struct(png);
        if (!info) abort();
        if (setjmp(png_jmpbuf(png))) abort();

        png_init_io(png, fp);
        // Output is 8bit depth, RGBA format.
        png_set_IHDR(
                png,
                info,
                width_o, height_o,
                8,
                PNG_COLOR_TYPE_RGBA,
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT
        );
        png_write_info(png, info);

        // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
        // Use png_set_filler().
        // png_set_filler(png, 0, PNG_FILLER_AFTER);

        if (!out_png) abort();
        png_write_image(png, out_png);
        png_write_end(png, NULL);
        free(out_png);
        fclose(fp);
        png_destroy_write_struct(&png, &info);
}

void write_jpeg_file(char *filename, int quality = 50) {
/*
        int quality: compression rate
*/
        image_buffer = (unsigned char *)malloc(height_o * width_o * channels);
        // image data from 2D out_png[][] to 1D image_buffer[]
        for(int y = 0; y < height_o; y++) {
                for(int x = 0; x < width_o*channels; x++) {
                        image_buffer[y*width_o*3+x] = out_png[y][x];
                }
        }

        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;
        FILE * outfile;		/* target file */
        JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
        int row_stride;		/* physical row width in image buffer */

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        if ((outfile = fopen(filename, "wb")) == NULL) {
                fprintf(stderr, "can't open %s\n", filename);
                exit(1);
        }
        jpeg_stdio_dest(&cinfo, outfile);

        cinfo.image_width = width_o; 	//in pixels
        cinfo.image_height = height_o;
        cinfo.input_components = RGB;   // # of color components per pixel
        cinfo.in_color_space = JCS_RGB; // colorspace of input image

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
        jpeg_start_compress(&cinfo, TRUE);
        row_stride = width_o * channels;       // JSAMPLEs per row in image_buffer

        while (cinfo.next_scanline < cinfo.image_height) {
                row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
                        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);
        fclose(outfile);
        jpeg_destroy_compress(&cinfo);
}

void read_image(char *filename) {

        // open image file
        FILE *fp = fopen(filename, "rb");

        type = read_MIME_type(filename);

        if(std::strstr(type, "png")) {read_png_file(fp); channels = RGBA;}
        if(std::strstr(type, "jpeg")) {read_jpeg_file(fp); channels = RGB;}

        fclose(fp);
 }

void process_image() {

        // detect border and crop
        // use row_pointers[0][0], row_pointers[0][1], row_pointers[0][2] as border RGB

        // up border
        bool flag = true;
        int r1, r2, c1, c2; // top row, bottom row, left col, right col
        for(int y = 0; y < height; y++) {
                png_bytep row = row_pointers[y];
                for(int x = 0; x < width; x++) {
                        png_bytep px = &(row[x * channels]);  // px[0] -> unsigned char -> d -> use ASCII
                        if(px[0] != row_pointers[0][0] || px[1] != row_pointers[0][1] || px[2] != row_pointers[0][2]){
                                r1 = y;
                                std::printf("top: %d\n", r1);
                                flag = false;
                                break;
                        }
                }
                if(!flag) { flag = true; break; }
        }
        // bottom border
        for(int y = height - 1; y >= 0; y--) {
                png_bytep row = row_pointers[y];
                for(int x = 0; x < width; x++) {
                        png_bytep px = &(row[x * channels]);
                        if(px[0] != row_pointers[0][0] || px[1] != row_pointers[0][1] || px[2] != row_pointers[0][2]){
                                r2 = y;
                                std::printf("bottom: %d\n", r2);
                                flag = false;
                                break;
                        }
                }
                if(!flag) { flag = true; break; }
        }
        // left border
        for(int x = 0; x < width; x++) {
                for(int y = r1; y <= r2; y++) {
                        png_bytep row = row_pointers[y];
                        png_bytep px = &(row[x * channels]);  // px[0] -> unsigned char -> d -> use ASCII
                        if(px[0] != row_pointers[0][0] || px[1] != row_pointers[0][1] || px[2] != row_pointers[0][2]){
                                c1 = x;
                                std::printf("left: %d\n", c1);
                                flag = false;
                                break;
                        }
                }
                if(!flag) { flag = true; break; }
        }
        // right border
        for(int x = width-1; x >= 0; x--) {
                for(int y = r1; y <= r2; y++) {
                        png_bytep row = row_pointers[y];
                        png_bytep px = &(row[x * channels]);  // px[0] -> unsigned char -> d -> use ASCII
                        if(px[0] != row_pointers[0][0] || px[1] != row_pointers[0][1] || px[2] != row_pointers[0][2]){
                                c2 = x;
                                std::printf("right: %d\n", c2);
                                flag = false;
                                break;
                        }
                }
                if(!flag) { flag = true; break; }
        }

        // save inner image data
        height_o = r2 - r1 + 1; width_o = c2 - c1 + 1;
        std::printf("height_o: %d, width_o: %d\n", height_o, width_o);
        out_png = (png_bytep*)malloc(sizeof(png_bytep) * height_o);
        for(int y = 0; y < height_o; y++) out_png[y] = row_pointers[y+r1]+c1*channels;

}

int main(int argc, char *argv[]) {

        if(argc != 3) abort();

        read_image(argv[1]);
        process_image();
        if(std::strstr(type, "png")) write_png_file(argv[2]);
        if(std::strstr(type, "jpeg")) write_jpeg_file(argv[2],50);

        return 0;
}
