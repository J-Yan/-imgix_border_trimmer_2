#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h> // strtok

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <magic.h>

std::string type2str(int type, int * channel) {
/*
        code modified from https://stackoverflow.com/questions/10167534/how-to-find-out-what-type-of-a-mat-object-is-with-mattype-in-opencv/39780825
        int type: cv::Mat.type()
*/
        std::string r;
        uchar depth = type & CV_MAT_DEPTH_MASK; //
        uchar chans = 1 + (type >> CV_CN_SHIFT);  // CV_CN_SHIFT 3

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
        *channel = chans;

        return r;
}

const char * read_MIME_type(char * filename) {
/*
        Multipurpose Internet Mail Extensions
*/
        magic_t handle { magic_open(MAGIC_MIME) };
        if (handle == NULL) {
                printf("unable to initialize magic library\n");
                abort();
        }
        if (magic_load(handle, NULL) != 0) {
                printf("cannot load magic database - %s\n", magic_error(handle));
                magic_close(handle);
                abort();
        }
        const char * type { magic_file(handle, filename) };
        // image/png
        // image/jpeg
        // image/webp
        printf("【MIME】%s\n", type);
        return type;
}

int main( int argc, char ** argv ) {

        char * inImage { argv[1] };
        char * outImage { argv[2] };
        int channel ;

        // read image
        cv::Mat inImageMat;

        const char * ext { read_MIME_type(argv[1]) };
        char * ext2 { strtok((char *)ext, "/;") };
        ext2 = strtok(NULL, "/;");
        printf("【Image type】%s\n", ext2);
        if((std::string)ext2 == "png" || (std::string)ext2 == "webp" || (std::string)ext2 == "jpeg") {inImageMat = cv::imread( inImage, 1 ); /* -1: cv::IMREAD_UNCHANGED*/}
        else {
                std::cout << "File format not supported." << std::endl;
                abort();
        }

        if( !inImageMat.data ) {
                printf( " No image data \n " );
                return -1;
        }

        // trim process

        // check image opencv data type & image size
        std::string ty { type2str( inImageMat.type(), &channel) };
        printf("【OpenCV data type】%s\n【size】%dx%d \n-------------------\n【ROI info】\n", ty.c_str(), inImageMat.cols, inImageMat.rows );

        int r1 {0}, r2 {0}, c1 {0}, c2 {0};
        // take inImageMat.at<Vec3b>(0,0) as border RGB
        // top border
        bool flag {true};
        for(int y = 0; y < inImageMat.rows; y++) {
                for(int x = 0; x < inImageMat.cols; x++) {
                        bool same {true};
                        switch(channel){
                                case 3: { same = inImageMat.at<cv::Vec3b>(y,x) == inImageMat.at<cv::Vec3b>(0,0); break; }
                                case 4: { same = inImageMat.at<cv::Vec<uint16_t,4>>(y,x) == inImageMat.at<cv::Vec<uint16_t,4>>(0,0); break; }
                        }
                        if(!same){
                                r1 = y;
                                printf("top: %d\n", r1);
                                std::cout<<x<<"   "<<y<<"   "<<inImageMat.at<cv::Vec3b>(y,x)<<"  "<<inImageMat.at<cv::Vec3b>(0,0);
                                flag = false;
                                break;
                        }
                        if(y == inImageMat.rows - 1 && x == inImageMat.cols - 1) {
                                printf("Input image is a pure color image, NOT qualified for this trimming process!");
                                abort();
                        }
                }
                if(!flag) { flag = true; break; }

        }
        // bottom border
        for(int y = inImageMat.rows-1; y >= 0; y--) {
                for(int x = 0; x < inImageMat.cols; x++) {
                        bool same {true};
                        switch(channel){
                                case 3: { same = inImageMat.at<cv::Vec3b>(y,x) == inImageMat.at<cv::Vec3b>(0,0); break; }
                                case 4:  { same = inImageMat.at<cv::Vec<uint16_t,4>>(y,x) == inImageMat.at<cv::Vec<uint16_t,4>>(0,0); break; }
                        }
                        if(!same){
                                r2 = y;
                                printf("bottom: %d\n", r2);
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
                                case 3: { same = inImageMat.at<cv::Vec3b>(y,x) == inImageMat.at<cv::Vec3b>(0,0); break; }
                                case 4:  { same = inImageMat.at<cv::Vec<uint16_t,4>>(y,x) == inImageMat.at<cv::Vec<uint16_t,4>>(0,0); break; }
                        }
                        if(!same){
                                c1 = x;
                                printf("left: %d\n", c1);
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
                                case 3: { same = inImageMat.at<cv::Vec3b>(y,x) == inImageMat.at<cv::Vec3b>(0,0); break; }
                                case 4:  { same = inImageMat.at<cv::Vec<uint16_t,4>>(y,x) == inImageMat.at<cv::Vec<uint16_t,4>>(0,0); break; }
                        }
                        if(!same){
                                c2 = x;
                                printf("right: %d\n", c2);
                                flag = false;
                                break;
                        }
                }
                if(!flag) { flag = true; break; }
        }

        // initial out image mat
        int h = r2 - r1 + 1, w = c2 - c1 + 1;
        cv::Mat outImageMat;
        switch(channel) {
                case 3: { outImageMat = cv::Mat(cv::Size(w, h), CV_8UC3); break; }
                case 4: { outImageMat = cv::Mat(cv::Size(w, h), CV_16UC4); break; }
        }
        // write out image mat
        for(int y = 0; y < h; y++) {
                for(int x = 0; x < w; x++) {
                        switch(channel) {
                                case 3: { outImageMat.at<cv::Vec3b>(y,x) = inImageMat.at<cv::Vec3b>(y+r1,x+c1); break; }
                                case 4: { outImageMat.at<cv::Vec<uint16_t,4>>(y,x) = inImageMat.at<cv::Vec<uint16_t,4>>(y+r1,x+c1); break; }
                        }
                }
        }

        // write out image
        cv::imwrite( strcat(strcat(outImage, "."),ext2), outImageMat );

        // display in & out image
        namedWindow( inImage, cv::WINDOW_AUTOSIZE );
        namedWindow( "trimmed image", cv::WINDOW_AUTOSIZE );
        imshow( inImage, inImageMat );
        imshow( "trimmed image", outImageMat );
        cv::waitKey(0);
        return 0;
}
