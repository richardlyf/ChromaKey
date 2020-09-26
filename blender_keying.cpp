#include <stdio.h>
#include <math.h>
#include <vector>
#include <chrono>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

const String window_capture_name = "Video Capture";
const String window_detection_name = "Object Detection";

int max_axis_v3(const vector<double>& vec3) {
	const double x = vec3[0];
	const double y = vec3[1];
	const double z = vec3[2];
	return ((x > y) ? ((x > z) ? 0 : 2) : ((y > z) ? 1 : 2));
}

double get_pixel_saturation(const vector<double>& pixel_color, double screen_balance, int primary_channel) {
  const int other_1 = (primary_channel + 1) % 3;
  const int other_2 = (primary_channel + 2) % 3;

  const int min_channel = min(other_1, other_2);
  const int max_channel = max(other_1, other_2);

  const double val = screen_balance * pixel_color[min_channel] + (1.0 - screen_balance) * pixel_color[max_channel];

  return (pixel_color[primary_channel] - val) * abs(1.0f - val);
}

/* The RGB inputs are supposed to be gamma corrected and in the range 0 - 1.0f
 * The YCC outputs are supposed to be in range 0 - 1.0f
 * (Direct implementation from blender)
 * Output YCC have a range of 16-235 and 16-240 except with JFIF_0_255 where the range is 0-255
 * Assumes the colorspace is BLI_YCC_ITU_BT709*/
void rgb_to_ycc_pixel(double r, double g, double b, double& r_y, double& r_cb, double& r_cr) {
  double y = 128.0f, cr = 128.0f, cb = 128.0f;
  double sr = 255.0f * r;
  double sg = 255.0f * g;
  double sb = 255.0f * b;

  y = (0.183f * sr) + (0.614f * sg) + (0.062f * sb) + 16.0f;
  cb = (-0.101f * sr) - (0.338f * sg) + (0.439f * sb) + 128.0f;
  cr = (0.439f * sr) - (0.399f * sg) - (0.040f * sb) + 128.0f;

  r_y = y / 255.0f;
  r_cb = cb / 255.0f;
  r_cr = cr / 255.0f;
}

double keyingOperation(const vector<double>& pixel_color, const vector<double>& key_color) {
    // Inputs that should be made into private vars
    double screen_balance = 0.5;

    double output = 0;
    const int primary_channel = max_axis_v3(key_color);
    const double min_pixel_color = min(min(pixel_color[0], pixel_color[1]), pixel_color[2]);

    if (min_pixel_color > 1.0) {
        /* Overexposure doesn't happen on screen itself and usually happens
        * on light sources in the shot, this need to be checked separately
        * because saturation and falloff calculation is based on the fact
        * that pixels are not overexposed
        */
        output = 1.0;
    } else {
        double saturation = get_pixel_saturation(pixel_color, screen_balance, primary_channel);
        double screen_saturation = get_pixel_saturation(key_color, screen_balance, primary_channel);

        if (saturation < 0) {
            // Means main channel of pixel is different from screen, assume this is completely a foreground
            output = 1.0;
        } else if (saturation >= screen_saturation) {
            // Matched main channels and higher saturation on pixel is treated as completely background
            output = 0.0;
        } else {
            // Nice alpha falloff on edges
            float distance = 1.0 - saturation / screen_saturation;
            output = distance;
        }
    }
    return output;
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

    namedWindow(window_capture_name);
    
    cvtColor(image, image, COLOR_BGR2RGB);
    Mat result_image = image.clone();
    // Image 2
    // double key_r = 0.0162343;
    // double key_g = 0.5;
    // double key_b = 0.0658958;

    // Image 1
    double key_r = 0.133293;
    double key_g = 0.178868;
    double key_b = 0.133967;

    const vector<double> key_color = {key_r, key_g, key_b};
    while (true) {
        auto start = chrono::high_resolution_clock::now();
        for (int row = 0; row < image.rows; row++) {
            for (int col = 0; col < image.cols; col++) {
                Vec3b rgb = image.at<Vec3b>(Point(col, row));
                double r = rgb[0];
                double g = rgb[1];
                double b = rgb[2];
                const vector<double> pixel_color = {r / 255, g / 255 , b / 255};
                double alpha = keyingOperation(pixel_color, key_color);
                result_image.at<Vec3b>(Point(col, row)) = {r * alpha, g * alpha, b * alpha};
            }
        }
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
        cout << "Time taken: " << duration.count() << " ms" << endl;
        cvtColor(result_image, result_image, COLOR_RGB2BGR);
        imshow(window_capture_name, result_image);
        char key = (char) waitKey(30);
        if (key == 'q' || key == 27) {
            break;
        }
    }
    return 0;
}
