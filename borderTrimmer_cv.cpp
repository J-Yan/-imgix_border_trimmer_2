#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <magic.h>

using namespace cv;
using namespace std;

#define RGB 3
#define RGBA 4
#define ONE 1

int channel;

string type2str(int type) {
/*
        code modified from https://stackoverflow.com/questions/10167534/how-to-find-out-what-type-of-a-mat-object-is-with-mattype-in-opencv/39780825
        int type: cv::Mat.type()
*/
        string r;
        uchar depth = type & CV_MAT_DEPTH_MASK;
        uchar chans = 1 + (type >> CV_CN_SHIFT);

        switch ( depth ) {
                case CV_8U:  r = "8U"; break;
                case CV_8S:  r = "8S"; break;
                case CV_16U: r = "16U"; break;
                case CV_16S: r = "16S"; break;
                case CV_32S: r = "32S"; break;
                case CV_32F: r = "32F"; break;
                case CV_64F: r = "64F"; break;
                default:     r = "User"; break;
        }

        r += "C";
        r += (chans+'0');
        channel = chans;

        return r;
}

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
        // image/webp
        std::printf("【TYPE】%s\n", type);
        return type;
}

int main( int argc, char** argv ) {

        char* inImage = argv[1];
        char* outImage = argv[2];

        // read image
        Mat inImageMat;

        const char * ext = read_MIME_type(argv[1]);
        if(strstr(ext, "png") || strstr(ext, "webp")) {inImageMat = imread( inImage, -1 ); channel = RGBA;}
        else if(strstr(ext, "jpeg")) {inImageMat = imread( inImage, 1 );}
        else {
                cout << "File format not supported." << endl;
                abort();
        }

        if( !inImageMat.data ) {
                printf( " No image data \n " );
                return -1;
        }

        // trim process

        // check image opencv data type & image size
        string ty =  type2str( inImageMat.type() );
        printf("Input image info: %s %dx%d \n", ty.c_str(), inImageMat.cols, inImageMat.rows );

        int r1, r2, c1, c2;
        // take inImageMat.at<Vec3b>(0,0) as border RGB
        // top border
        bool flag = true;
        for(int y = 0; y < inImageMat.rows; y++) {
                for(int x = 0; x < inImageMat.cols; x++) {
                        bool same = true;
                        switch(channel){
                                case 3: { same = inImageMat.at<Vec3b>(y,x) == inImageMat.at<Vec3b>(0,0); break; }
                                case 4: { same = inImageMat.at<Vec<uint16_t,4>>(y,x) == inImageMat.at<Vec<uint16_t,4>>(0,0); break; }
                        }
                        if(!same){
                                r1 = y;
                                std::printf("top: %d\n", r1);
                                flag = false;
                                break;
                        }
                }
                if(!flag) { flag = true; break; }
        }
        // bottom border
        for(int y = inImageMat.rows-1; y >= 0; y--) {
                for(int x = 0; x < inImageMat.cols; x++) {
                        bool same = true;
                        switch(channel){
                                case 3: { same = inImageMat.at<Vec3b>(y,x) == inImageMat.at<Vec3b>(0,0); break; }
                                case 4:  { same = inImageMat.at<Vec<uint16_t,4>>(y,x) == inImageMat.at<Vec<uint16_t,4>>(0,0); break; }
                        }
                        if(!same){
                                r2 = y;
                                std::printf("bottom: %d\n", r2);
                                flag = false;
                                break;
                        }
                }
                if(!flag) { flag = true; break; }
        }
        // left border
        for(int x = 0; x < inImageMat.cols; x++) {
                for(int y = r1; y <= r2; y++) {
                        bool same = true;
                        switch(channel){
                                case 3: { same = inImageMat.at<Vec3b>(y,x) == inImageMat.at<Vec3b>(0,0); break; }
                                case 4:  { same = inImageMat.at<Vec<uint16_t,4>>(y,x) == inImageMat.at<Vec<uint16_t,4>>(0,0); break; }
                        }
                        if(!same){
                                c1 = x;
                                std::printf("left: %d\n", c1);
                                flag = false;
                                break;
                        }
                }
                if(!flag) { flag = true; break; }
        }
        // right border
        for(int x = inImageMat.cols-1; x >=0 ; x--) {
                for(int y = r1; y <= r2; y++) {
                        bool same = true;
                        switch(channel){
                                case 3: { same = inImageMat.at<Vec3b>(y,x) == inImageMat.at<Vec3b>(0,0); break; }
                                case 4:  { same = inImageMat.at<Vec<uint16_t,4>>(y,x) == inImageMat.at<Vec<uint16_t,4>>(0,0); break; }
                        }
                        if(!same){
                                c2 = x;
                                std::printf("right: %d\n", c2);
                                flag = false;
                                break;
                        }
                }
                if(!flag) { flag = true; break; }
        }

        // initial out image mat
        int h = r2 - r1 + 1, w = c2 - c1 + 1;
        Mat outImageMat;
        switch(channel) {
                case 3: { outImageMat = Mat(Size(w, h), CV_8UC3); break; }
                case 4: { outImageMat = Mat(Size(w, h), CV_16UC4); break; }
        }
        // write out image mat
        for(int y = 0; y < h; y++) {
                for(int x = 0; x < w; x++) {
                        switch(channel) {
                                case 3: { outImageMat.at<Vec3b>(y,x) = inImageMat.at<Vec3b>(y+r1,x+c1); break; }
                                case 4: { outImageMat.at<Vec<uint16_t,4>>(y,x) = inImageMat.at<Vec<uint16_t,4>>(y+r1,x+c1); break; }
                        }
                }
        }

        // write out image
        imwrite( outImage, outImageMat );

        // display in & out image
        namedWindow( inImage, WINDOW_AUTOSIZE );
        namedWindow( "trimmed image", WINDOW_AUTOSIZE );
        imshow( inImage, inImageMat );
        imshow( "trimmed image", outImageMat );
        waitKey(0);
        return 0;
}
