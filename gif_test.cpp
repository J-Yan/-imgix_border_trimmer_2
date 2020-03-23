#include <FreeImage.h>
#include <FreeImagePlus.h>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>

using namespace cv;
using namespace std;

int main( int argc, char** argv ){


        // char * GifImagePath = argv[1];
        // FREE_IMAGE_FORMAT format = FreeImage_GetFileType(argv[1], 0);
        // FIBITMAP *image = FreeImage_Load(FIF_GIF,format, argv[1]);
        FIBITMAP *image = FreeImage_Load(FIF_ICO, argv[1]);
        RGBQUAD *pal = FreeImage_GetPalette(image);
        unsigned width = FreeImage_GetWidth(image);
        unsigned height = FreeImage_GetHeight(image);

        FIBITMAP *tmp = FreeImage_Allocate(width, height, 32);
        BYTE *tmp_bits = FreeImage_GetBits(tmp);

        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                BYTE t;

                FreeImage_GetPixelIndex(image,x, y, &t);//safer, in animated gif
                // t = image[x+y*height];//Probably faster

                RGBQUAD color = pal[t];


                //FreeImage_SetPixelColor(tmp, x, y, &color);//safer, in animated
                //Probably faster
                tmp_bits[(x+y*height)*4 +FI_RGBA_RED] = color.rgbRed;
                tmp_bits[(x+y*height)*4+FI_RGBA_GREEN] = color.rgbGreen;
                tmp_bits[(x+y*height)*4+FI_RGBA_BLUE] = color.rgbBlue;
                tmp_bits[(x+y*height)*4+FI_RGBA_ALPHA] = color.rgbReserved;

            }
        }


        Mat img(FreeImage_GetHeight(tmp), FreeImage_GetWidth(tmp), CV_8UC3, FreeImage_GetBits(tmp), FreeImage_GetPitch(tmp));
        cout<< FreeImage_GetPitch(tmp)<< endl;
        namedWindow("Display window", WINDOW_AUTOSIZE);
        imshow("Display window", img);

        waitKey(0);
        return 0;
}
