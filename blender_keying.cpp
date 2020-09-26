#include <stdio.h>
#include <math.h>
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

int max_axis_v3(const float vec[3]) {
	const float x = vec[0];
	const float y = vec[1];
	const float z = vec[2];
	return ((x > y) ? ((x > z) ? 0 : 2) : ((y > z) ? 1 : 2));
}

/* The RGB inputs are supposed to be gamma corrected and in the range 0 - 1.0f
 * The YCC outputs are supposed to be in range 0 - 1.0f
 * (Direct implementation from blender)
 * Output YCC have a range of 16-235 and 16-240 except with JFIF_0_255 where the range is 0-255
 * Assumes the colorspace is BLI_YCC_ITU_BT709*/
void rgb_to_ycc_pixel(float r, float g, float b, float& r_y, float& r_cb, float& r_cr)
{
  float sr, sg, sb;
  float y = 128.0f, cr = 128.0f, cb = 128.0f;

  sr = 255.0f * r;
  sg = 255.0f * g;
  sb = 255.0f * b;

  y = (0.183f * sr) + (0.614f * sg) + (0.062f * sb) + 16.0f;
  cb = (-0.101f * sr) - (0.338f * sg) + (0.439f * sb) + 128.0f;
  cr = (0.439f * sr) - (0.399f * sg) - (0.040f * sb) + 128.0f;

  r_y = y / 255.0f;
  r_cb = cb / 255.0f;
  r_cr = cr / 255.0f;
}

float keyingPixel(float inImage[3], float inKey[3])
{
  const float acceptance = 0.523599; /* in radians */
  const float cutoff = 0.18675;     /* in radians */
  // In blender this should be 1. Not sure why the behavior is not the same
  const float gain = 0;

  float x_angle, z_angle, alpha;
  float theta, beta;
  float kfg;

  /* store matte(alpha) value in [0] to go with
   * COM_SetAlphaOperation and the Value output
   */

  /* Algorithm from book "Video Demistified," does not include the spill reduction part */
  /* find theta, the angle that the color space should be rotated based on key */

  /* rescale to -1.0..1.0 */
  // inImage[0] = (inImage[0] * 2.0f) - 1.0f;  // UNUSED
  inImage[1] = (inImage[1] * 2.0f) - 1.0f;
  inImage[2] = (inImage[2] * 2.0f) - 1.0f;

  // inKey[0] = (inKey[0] * 2.0f) - 1.0f;  // UNUSED
  inKey[1] = (inKey[1] * 2.0f) - 1.0f;
  inKey[2] = (inKey[2] * 2.0f) - 1.0f;

  theta = atan2(inKey[2], inKey[1]);

  /*rotate the cb and cr into x/z space */
  x_angle = inImage[1] * cosf(theta) + inImage[2] * sinf(theta);
  z_angle = inImage[2] * cosf(theta) - inImage[1] * sinf(theta);

  /*if within the acceptance angle */
  /* if kfg is <0 then the pixel is outside of the key color */
  kfg = x_angle - (fabsf(z_angle) / tanf(acceptance / 2.0f));

  if (kfg > 0.0f) { /* found a pixel that is within key color */
    alpha = 1.0f - (kfg / gain);

    beta = atan2(z_angle, x_angle);
    /* if beta is within the cutoff angle */
    if (fabsf(beta) < (cutoff / 2.0f)) {
      alpha = 0.0f;
    }
    return alpha;
  }
  else {                    /*pixel is outside key color */
    return 1.0f;
  }
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

    cvtColor(image, image, COLOR_BGR2RGB);

    namedWindow(window_capture_name);
    // Trackbars to set thresholds for HSV values
    createTrackbar("Low H", window_detection_name, &low_H, max_value_H, on_low_H_thresh_trackbar);
    createTrackbar("High H", window_detection_name, &high_H, max_value_H, on_high_H_thresh_trackbar);
    createTrackbar("Low S", window_detection_name, &low_S, max_value, on_low_S_thresh_trackbar);
    createTrackbar("High S", window_detection_name, &high_S, max_value, on_high_S_thresh_trackbar);
    createTrackbar("Low V", window_detection_name, &low_V, max_value, on_low_V_thresh_trackbar);
    createTrackbar("High V", window_detection_name, &high_V, max_value, on_high_V_thresh_trackbar);

    Mat result_image = image.clone();
    //float key_r = 0.0242;
    //float key_g = 0.5;
    //float key_b = 0.08442;

    float key_r = 0.0545252;
    float key_g = 0.5;
    float key_b = 0.20443;
    float key_y, key_cb, key_cr;
    rgb_to_ycc_pixel(key_r, key_g, key_b, key_y, key_cb, key_cr);
    float inKey[3] = {key_y, key_cb, key_cr};
    while (true) {
        for (int row = 0; row < image.rows; row++) {
            for (int col = 0; col < image.cols; col++) {
                Vec3b rgb = image.at<Vec3b>(Point(col, row));
                float r = rgb[0];
                float g = rgb[1];
                float b = rgb[2];
                float y, cb, cr;
                rgb_to_ycc_pixel(r / 255.0f, g / 255.0f, b / 255.0f, y, cb, cr);
                float inImage[3] = {y, cb, cr};
                float alpha = KeyingPixel(inImage, inKey);
                result_image.at<Vec3b>(Point(col, row)) = {r * alpha, g * alpha, b * alpha};
            }
        }
        cvtColor(result_image, result_image, COLOR_RGB2BGR);
        imshow(window_capture_name, result_image);
        char key = (char) waitKey(30);
        if (key == 'q' || key == 27) {
            break;
        }
    }
    return 0;
}
