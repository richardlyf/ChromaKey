#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace cv;

const int max_value_H = 360/2;
const int max_value = 255;
const String window_capture_name = "Video Capture";
const String window_detection_name = "Object Detection";
int low_H = 0, low_S = 0, low_V = 0;
int high_H = max_value_H, high_S = max_value, high_V = max_value;

static void on_low_H_thresh_trackbar(int, void *)
{
    low_H = min(high_H-1, low_H);
    setTrackbarPos("Low H", window_detection_name, low_H);
}

static void on_high_H_thresh_trackbar(int, void *)
{
    high_H = max(high_H, low_H+1);
    setTrackbarPos("High H", window_detection_name, high_H);
}

static void on_low_S_thresh_trackbar(int, void *)
{
    low_S = min(high_S-1, low_S);
    setTrackbarPos("Low S", window_detection_name, low_S);
}

static void on_high_S_thresh_trackbar(int, void *)
{
    high_S = max(high_S, low_S+1);
    setTrackbarPos("High S", window_detection_name, high_S);
}

static void on_low_V_thresh_trackbar(int, void *)
{
    low_V = min(high_V-1, low_V);
    setTrackbarPos("Low V", window_detection_name, low_V);
}

static void on_high_V_thresh_trackbar(int, void *)
{
    high_V = max(high_V, low_V+1);
    setTrackbarPos("High V", window_detection_name, high_V);
}

int main(int argc, char** argv )
{
    if ( argc != 2 )
    {
        printf("usage: DisplayImage.out <Image_Path>\n");
        return -1;
    }
    Mat image;
    image = imread( argv[1], 1 );
    if ( !image.data )
    {
        printf("No image data \n");
        return -1;
    }

    namedWindow("Before");
    namedWindow(window_capture_name);
    namedWindow(window_detection_name);
    // Trackbars to set thresholds for HSV values
    createTrackbar("Low H", window_detection_name, &low_H, max_value_H, on_low_H_thresh_trackbar);
    createTrackbar("High H", window_detection_name, &high_H, max_value_H, on_high_H_thresh_trackbar);
    createTrackbar("Low S", window_detection_name, &low_S, max_value, on_low_S_thresh_trackbar);
    createTrackbar("High S", window_detection_name, &high_S, max_value, on_high_S_thresh_trackbar);
    createTrackbar("Low V", window_detection_name, &low_V, max_value, on_low_V_thresh_trackbar);
    createTrackbar("High V", window_detection_name, &high_V, max_value, on_high_V_thresh_trackbar);

    Mat frame, frame_HSV, green_mask;
    while (true) {
	    frame = image.clone();
	    // Convert from BGR to HSV colorspace
	    cvtColor(frame, frame_HSV, COLOR_BGR2HSV);
	    // Detect the object based on HSV Range Values
        // low_H = 60;
        // low_S = 102;
        // low_V = 45;
        // high_H = 130;
        // high_S = 255;
        // high_V = 255;
	    inRange(frame_HSV, Scalar(low_H, low_S, low_V), Scalar(high_H, high_S, high_V), green_mask);

	    Mat mask;
	    Mat masked_frame = Mat::zeros(frame.size(), frame.type());
	    bitwise_not (green_mask, mask);
        dilate(mask, mask, Mat());
        erode(mask, mask, Mat());
	    frame.copyTo(masked_frame, mask);

        imshow("Before", masked_frame);

        // reduce green
        Mat masked_frame_HSV;
        cvtColor(masked_frame, masked_frame_HSV, COLOR_BGR2HSV);
        for (int row = 0; row < masked_frame_HSV.rows; row++) {
            for (int col = 0; col < masked_frame_HSV.cols; col++) {
                Vec3b hsv = masked_frame_HSV.at<Vec3b>(Point(col, row));
                if (hsv[0] >= 35 && hsv[0] <= 130 && hsv[1] >= 0.15 * 255 && hsv[2] >= 0.15 * 255) {
                    Vec3b rgb = masked_frame.at<Vec3b>(Point(col, row));
                    int r = rgb[0];
                    int g = rgb[1];
                    int b = rgb[2];
                    
                    if (r * b != 0 && (g * g) / (r * b) > 1.5) {
                        masked_frame.at<Vec3b>(Point(col, row)) = Vec3b((int)r * 1.7, g, (int)b * 1.7);
                    } else {
                        masked_frame.at<Vec3b>(Point(col, row)) = Vec3b((int)r * 1.5, g, (int)b * 1.5);
                    }
                }
            }
        }
        

	    imshow(window_detection_name, mask);
	    imshow(window_capture_name, masked_frame);

	    
	    char key = (char) waitKey(30);
	    if (key == 'q' || key == 27)
	    {
		    break;
	    }
    }
    return 0;
}
