// Most of code ripped from https://github.com/stefanhaustein/TerminalImageViewer
// which has an Apache2 license

#include <aalib.h>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

// #include <opencv2/highgui.hpp>
#include <iostream>
#include <stdio.h>

//
// #include "cvMat.h"
#define cimg_plugin1 "cvMat.h"
#include "CImg.h"
#include <map>

#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h>    // for STDOUT_FILENO

// #define cimg_plugin1 "cvMat.h"
// #define cimg_plugin_cvMat

// #define cimg_use_opencv

using namespace cv;
using namespace std;
using namespace cimg_library;

const int FLAG_FG = 1;
const int FLAG_BG = 2;
const int FLAG_MODE_256 = 4;
const int FLAG_24BIT = 8;
const int FLAG_NOOPT = 16;
const int FLAG_TELETEXT = 32;

const int COLOR_STEP_COUNT = 6;
const int COLOR_STEPS[COLOR_STEP_COUNT] = {0, 0x5f, 0x87, 0xaf, 0xd7, 0xff};

const int GRAYSCALE_STEP_COUNT = 24;
const int GRAYSCALE_STEPS[GRAYSCALE_STEP_COUNT] = {
    0x08, 0x12, 0x1c, 0x26, 0x30, 0x3a, 0x44, 0x4e, 0x58, 0x62, 0x6c, 0x76,
    0x80, 0x8a, 0x94, 0x9e, 0xa8, 0xb2, 0xbc, 0xc6, 0xd0, 0xda, 0xe4, 0xee};

const unsigned int BITMAPS[] = {
    0x00000000, 0x00a0,

    // Block graphics
    // 0xffff0000, 0x2580,  // upper 1/2; redundant with inverse lower 1/2

    0x0000000f, 0x2581, // lower 1/8
    0x000000ff, 0x2582, // lower 1/4
    0x00000fff, 0x2583,
    0x0000ffff, 0x2584, // lower 1/2
    0x000fffff, 0x2585,
    0x00ffffff, 0x2586, // lower 3/4
    0x0fffffff, 0x2587,
    //0xffffffff, 0x2588,  // full; redundant with inverse space

    0xeeeeeeee, 0x258a, // left 3/4
    0xcccccccc, 0x258c, // left 1/2
    0x88888888, 0x258e, // left 1/4

    0x0000cccc, 0x2596, // quadrant lower left
    0x00003333, 0x2597, // quadrant lower right
    0xcccc0000, 0x2598, // quadrant upper left
                        //0xccccffff, 0x2599,  // 3/4 redundant with inverse 1/4
    0xcccc3333, 0x259a, // diagonal 1/2
                        //0xffffcccc, 0x259b,  // 3/4 redundant
                        //0xffff3333, 0x259c,  // 3/4 redundant
    0x33330000, 0x259d, // quadrant upper right
                        //0x3333cccc, 0x259e,  // 3/4 redundant
                        //0x3333ffff, 0x259f,  // 3/4 redundant

    // Line drawing subset: no double lines, no complex light lines

    0x000ff000, 0x2501, // Heavy horizontal
    0x66666666, 0x2503, // Heavy vertical

    0x00077666, 0x250f, // Heavy down and right
    0x000ee666, 0x2513, // Heavy down and left
    0x66677000, 0x2517, // Heavy up and right
    0x666ee000, 0x251b, // Heavy up and left

    0x66677666, 0x2523, // Heavy vertical and right
    0x666ee666, 0x252b, // Heavy vertical and left
    0x000ff666, 0x2533, // Heavy down and horizontal
    0x666ff000, 0x253b, // Heavy up and horizontal
    0x666ff666, 0x254b, // Heavy cross

    0x000cc000, 0x2578, // Bold horizontal left
    0x00066000, 0x2579, // Bold horizontal up
    0x00033000, 0x257a, // Bold horizontal right
    0x00066000, 0x257b, // Bold horizontal down

    0x06600660, 0x254f, // Heavy double dash vertical

    0x000f0000, 0x2500, // Light horizontal
    0x0000f000, 0x2500, //
    0x44444444, 0x2502, // Light vertical
    0x22222222, 0x2502,

    0x000e0000, 0x2574, // light left
    0x0000e000, 0x2574, // light left
    0x44440000, 0x2575, // light up
    0x22220000, 0x2575, // light up
    0x00030000, 0x2576, // light right
    0x00003000, 0x2576, // light right
    0x00004444, 0x2577, // light down
    0x00002222, 0x2577, // light down

    // Misc technical

    0x44444444, 0x23a2, // [ extension
    0x22222222, 0x23a5, // ] extension

    0x0f000000, 0x23ba, // Horizontal scanline 1
    0x00f00000, 0x23bb, // Horizontal scanline 3
    0x00000f00, 0x23bc, // Horizontal scanline 7
    0x000000f0, 0x23bd, // Horizontal scanline 9

    // Geometrical shapes. Tricky because some of them are too wide.

    //0x00ffff00, 0x25fe,  // Black medium small square
    0x00066000, 0x25aa, // Black small square

    //0x11224488, 0x2571,  // diagonals
    //0x88442211, 0x2572,
    //0x99666699, 0x2573,
    //0x000137f0, 0x25e2,  // Triangles
    //0x0008cef0, 0x25e3,
    //0x000fec80, 0x25e4,
    //0x000f7310, 0x25e5,

    0, 0, // End marker for "regular" characters

    // Teletext / legacy graphics 3x2 block character codes.
    // Using a 3-2-3 pattern consistently, perhaps we should create automatic variations....

    0xccc00000, 0xfb00,
    0x33300000, 0xfb01,
    0xfff00000, 0xfb02,
    0x000cc000, 0xfb03,
    0xccccc000, 0xfb04,
    0x333cc000, 0xfb05,
    0xfffcc000, 0xfb06,
    0x00033000, 0xfb07,
    0xccc33000, 0xfb08,
    0x33333000, 0xfb09,
    0xfff33000, 0xfb0a,
    0x000ff000, 0xfb0b,
    0xcccff000, 0xfb0c,
    0x333ff000, 0xfb0d,
    0xfffff000, 0xfb0e,
    0x00000ccc, 0xfb0f,

    0xccc00ccc, 0xfb10,
    0x33300ccc, 0xfb11,
    0xfff00ccc, 0xfb12,
    0x000ccccc, 0xfb13,
    0x333ccccc, 0xfb14,
    0xfffccccc, 0xfb15,
    0x00033ccc, 0xfb16,
    0xccc33ccc, 0xfb17,
    0x33333ccc, 0xfb18,
    0xfff33ccc, 0xfb19,
    0x000ffccc, 0xfb1a,
    0xcccffccc, 0xfb1b,
    0x333ffccc, 0xfb1c,
    0xfffffccc, 0xfb1d,
    0x00000333, 0xfb1e,
    0xccc00333, 0xfb1f,

    0x33300333, 0x1b20,
    0xfff00333, 0x1b21,
    0x000cc333, 0x1b22,
    0xccccc333, 0x1b23,
    0x333cc333, 0x1b24,
    0xfffcc333, 0x1b25,
    0x00033333, 0x1b26,
    0xccc33333, 0x1b27,
    0xfff33333, 0x1b28,
    0x000ff333, 0x1b29,
    0xcccff333, 0x1b2a,
    0x333ff333, 0x1b2b,
    0xfffff333, 0x1b2c,
    0x00000fff, 0x1b2d,
    0xccc00fff, 0x1b2e,
    0x33300fff, 0x1b2f,

    0xfff00fff, 0x1b30,
    0x000ccfff, 0x1b31,
    0xcccccfff, 0x1b32,
    0x333ccfff, 0x1b33,
    0xfffccfff, 0x1b34,
    0x00033fff, 0x1b35,
    0xccc33fff, 0x1b36,
    0x33333fff, 0x1b37,
    0xfff33fff, 0x1b38,
    0x000fffff, 0x1b39,
    0xcccfffff, 0x1b3a,
    0x333fffff, 0x1b3b,

    0, 1 // End marker for extended TELETEXT mode.
};

aa_context *context;
struct aa_savedata ascii_save;

struct CharData
{
  std::array<int, 3> fgColor = std::array<int, 3>{0, 0, 0};
  std::array<int, 3> bgColor = std::array<int, 3>{0, 0, 0};
  int codePoint;
};

CharData createCharData(const cimg_library::CImg<unsigned char> &image, int x0, int y0, int codepoint, int pattern)
{
  CharData result;
  result.codePoint = codepoint;
  int fg_count = 0;
  int bg_count = 0;
  unsigned int mask = 0x80000000;

  for (int y = 0; y < 8; y++)
  {
    for (int x = 0; x < 4; x++)
    {
      int *avg;
      if (pattern & mask)
      {
        avg = result.fgColor.data();
        fg_count++;
      }
      else
      {
        avg = result.bgColor.data();
        bg_count++;
      }
      for (int i = 0; i < 3; i++)
      {
        avg[i] += image(x0 + x, y0 + y, 0, i);
      }
      mask = mask >> 1;
    }
  }

  // Calculate the average color value for each bucket
  for (int i = 0; i < 3; i++)
  {
    if (bg_count != 0)
    {
      result.bgColor[i] /= bg_count;
    }
    if (fg_count != 0)
    {
      result.fgColor[i] /= fg_count;
    }
  }
  return result;
}

CharData findCharData(const cimg_library::CImg<unsigned char> &image, int x0, int y0, int flags)
{
  int min[3] = {255, 255, 255};
  int max[3] = {0};
  std::map<long, int> count_per_color;

  // Determine the minimum and maximum value for each color channel
  for (int y = 0; y < 8; y++)
  {
    for (int x = 0; x < 4; x++)
    {
      long color = 0;
      for (int i = 0; i < 3; i++)
      {
        int d = image(x0 + x, y0 + y, 0, i);
        min[i] = std::min(min[i], d);
        max[i] = std::max(max[i], d);
        color = (color << 8) | d;
      }
      count_per_color[color]++;
    }
  }

  std::multimap<int, long> color_per_count;
  for (auto i = count_per_color.begin(); i != count_per_color.end(); ++i)
  {
    color_per_count.insert(std::pair<int, long>(i->second, i->first));
  }

  auto iter = color_per_count.rbegin();
  int count2 = iter->first;
  long max_count_color_1 = iter->second;
  long max_count_color_2 = max_count_color_1;
  if ((++iter) != color_per_count.rend())
  {
    count2 += iter->first;
    max_count_color_2 = iter->second;
  }

  unsigned int bits = 0;
  bool direct = count2 > (8 * 4) / 2;

  if (direct)
  {
    for (int y = 0; y < 8; y++)
    {
      for (int x = 0; x < 4; x++)
      {
        bits = bits << 1;
        int d1 = 0;
        int d2 = 0;
        for (int i = 0; i < 3; i++)
        {
          int shift = 16 - 8 * i;
          int c1 = (max_count_color_1 >> shift) & 255;
          int c2 = (max_count_color_2 >> shift) & 255;
          int c = image(x0 + x, y0 + y, 0, i);
          d1 += (c1 - c) * (c1 - c);
          d2 += (c2 - c) * (c2 - c);
        }
        if (d1 > d2)
        {
          bits |= 1;
        }
      }
    }
  }
  else
  {
    // Determine the color channel with the greatest range.
    int splitIndex = 0;
    int bestSplit = 0;
    for (int i = 0; i < 3; i++)
    {
      if (max[i] - min[i] > bestSplit)
      {
        bestSplit = max[i] - min[i];
        splitIndex = i;
      }
    }

    // We just split at the middle of the interval instead of computing the median.
    int splitValue = min[splitIndex] + bestSplit / 2;

    // Compute a bitmap using the given split and sum the color values for both buckets.
    for (int y = 0; y < 8; y++)
    {
      for (int x = 0; x < 4; x++)
      {
        bits = bits << 1;
        if (image(x0 + x, y0 + y, 0, splitIndex) > splitValue)
        {
          bits |= 1;
        }
      }
    }
  }

  // Find the best bitmap match by counting the bits that don't match,
  // including the inverted bitmaps.
  int best_diff = 8;
  unsigned int best_pattern = 0x0000ffff;
  int codepoint = 0x2584;
  bool inverted = false;
  unsigned int end_marker = flags & FLAG_TELETEXT ? 1 : 0;
  for (int i = 0; BITMAPS[i + 1] != end_marker; i += 2)
  {
    // Skip all end markers
    if (BITMAPS[i + 1] < 32)
    {
      continue;
    }
    unsigned int pattern = BITMAPS[i];
    for (int j = 0; j < 2; j++)
    {
      int diff = (std::bitset<32>(pattern ^ bits)).count();
      if (diff < best_diff)
      {
        best_pattern = BITMAPS[i]; // pattern might be inverted.
        codepoint = BITMAPS[i + 1];
        best_diff = diff;
        inverted = best_pattern != pattern;
      }
      pattern = ~pattern;
    }
  }

  if (direct)
  {
    CharData result;
    if (inverted)
    {
      long tmp = max_count_color_1;
      max_count_color_1 = max_count_color_2;
      max_count_color_2 = tmp;
    }
    for (int i = 0; i < 3; i++)
    {
      int shift = 16 - 8 * i;
      result.fgColor[i] = (max_count_color_2 >> shift) & 255;
      result.bgColor[i] = (max_count_color_1 >> shift) & 255;
      result.codePoint = codepoint;
    }
    return result;
  }
  return createCharData(image, x0, y0, codepoint, best_pattern);
}

int clamp_byte(int value)
{
  return value < 0 ? 0 : (value > 255 ? 255 : value);
}

int best_index(int value, const int data[], int count)
{
  int best_diff = std::abs(data[0] - value);
  int result = 0;
  for (int i = 1; i < count; i++)
  {
    int diff = std::abs(data[i] - value);
    if (diff < best_diff)
    {
      result = i;
      best_diff = diff;
    }
  }
  return result;
}

double sqr(double n)
{
  return n * n;
}

void emitCodepoint(int codepoint)
{
  if (codepoint < 128)
  {
    std::cout << (char)codepoint;
  }
  else if (codepoint < 0x7ff)
  {
    std::cout << (char)(0xc0 | (codepoint >> 6));
    std::cout << (char)(0x80 | (codepoint & 0x3f));
  }
  else if (codepoint < 0xffff)
  {
    std::cout << (char)(0xe0 | (codepoint >> 12));
    std::cout << (char)(0x80 | ((codepoint >> 6) & 0x3f));
    std::cout << (char)(0x80 | (codepoint & 0x3f));
  }
  else if (codepoint < 0x10ffff)
  {
    std::cout << (char)(0xf0 | (codepoint >> 18));
    std::cout << (char)(0x80 | ((codepoint >> 12) & 0x3f));
    std::cout << (char)(0x80 | ((codepoint >> 6) & 0x3f));
    std::cout << (char)(0x80 | (codepoint & 0x3f));
  }
  else
  {
    std::cerr << "ERROR";
  }
}

void emit_color(int flags, int r, int g, int b)
{
  r = clamp_byte(r);
  g = clamp_byte(g);
  b = clamp_byte(b);

  bool bg = (flags & FLAG_BG) != 0;

  if ((flags & FLAG_MODE_256) == 0)
  {
    std::cout << (bg ? "\x1b[48;2;" : "\x1b[38;2;") << r << ';' << g << ';' << b << 'm';
    return;
  }

  int ri = best_index(r, COLOR_STEPS, COLOR_STEP_COUNT);
  int gi = best_index(g, COLOR_STEPS, COLOR_STEP_COUNT);
  int bi = best_index(b, COLOR_STEPS, COLOR_STEP_COUNT);

  int rq = COLOR_STEPS[ri];
  int gq = COLOR_STEPS[gi];
  int bq = COLOR_STEPS[bi];

  int gray = static_cast<int>(std::round(r * 0.2989f + g * 0.5870f + b * 0.1140f));

  int gri = best_index(gray, GRAYSCALE_STEPS, GRAYSCALE_STEP_COUNT);
  int grq = GRAYSCALE_STEPS[gri];

  int color_index;
  if (0.3 * sqr(rq - r) + 0.59 * sqr(gq - g) + 0.11 * sqr(bq - b) <
      0.3 * sqr(grq - r) + 0.59 * sqr(grq - g) + 0.11 * sqr(grq - b))
  {
    color_index = 16 + 36 * ri + 6 * gi + bi;
  }
  else
  {
    color_index = 232 + gri; // 1..24 -> 232..255
  }
  std::cout << (bg ? "\x1B[48;5;" : "\u001B[38;5;") << color_index << "m";
}

void emit_image(const cimg_library::CImg<unsigned char> &image, int flags)
{
  CharData lastCharData;
  for (int y = 0; y <= image.height() - 8; y += 8)
  {
    for (int x = 0; x <= image.width() - 4; x += 4)
    {
      CharData charData = flags & FLAG_NOOPT
                              ? createCharData(image, x, y, 0x2584, 0x0000ffff)
                              : findCharData(image, x, y, flags);
      if (x == 0 || charData.bgColor != lastCharData.bgColor)
        emit_color(flags | FLAG_BG, charData.bgColor[0], charData.bgColor[1], charData.bgColor[2]);
      if (x == 0 || charData.fgColor != lastCharData.fgColor)
        emit_color(flags | FLAG_FG, charData.fgColor[0], charData.fgColor[1], charData.fgColor[2]);
      emitCodepoint(charData.codePoint);
      lastCharData = charData;
    }
    std::cout << "\x1b[0m" << std::endl;
  }
}

int main(int, char **)
{
  Mat frame;
  //--- INITIALIZE VIDEOCAPTURE
  VideoCapture capture;

  // capture.set(cv::CAP_PROP_FRAME_WIDTH, 0x7FFFFFFF);          // working
  // capture.set(cv::CAP_PROP_FRAME_HEIGHT, 0x7FFFFFFF);         // working
  // open the default camera using default API
  // cap.open(0);
  // OR advance usage: select any API backend
  int deviceID = 0;        // 0 = open default camera
  int apiID = cv::CAP_ANY; // 0 = autodetect default API
  // open selected camera using selected API
  capture.open(deviceID, apiID);
  // check if we succeeded
  if (!capture.isOpened())
  {
    cerr << "ERROR! Unable to open camera\n";
    return -1;
  }
  //--- GRAB AND WRITE LOOP
  cout << "Start grabbing" << endl
       << "Press any key to terminate" << endl;

  // aa_context *aa_init(struct aa_driver *driver,  struct aa_hardware_params *defparams,  void *driverdata)
  // aa_context *aa_autoinit(struct aa_hardware_params *params);

  // aa_context *aa_init(struct aa_driver *driver,
  //                     struct aa_hardware_params *defparams,
  //                     void *driverdata)

  // aa_context context = aa_autoinit(&aa_defparams);

  context = aa_autoinit(&aa_defparams);
  // context = aa_autoinit(&ascii_hwparms);

  if (context == NULL)
  {
    fprintf(stderr, "Cannot initialize AA-lib. Sorry\n");
    exit(1);
  }

  // IplImage *img, *img_dest;

  // const int width=3;
  // const int height=2;

  Mat dst;
  // int rows = dst.rows;
  // int cols = dst.cols;

  struct winsize size;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

  /* size.ws_row is the number of rows, size.ws_col is the number of columns. */

  // dst.rows = size.ws_row;
  // dst.cols = size.ws_col;

  // dst.rows = frame.rows/2;
  // dst.cols = frame.cols/2;

  // dst.rows = 40;
  // dst.cols = 80;

  dst.rows = 80;
  dst.cols = 160;

  for (;;)
  {
    // cimg_library::CImg<unsigned char> image = load_rgb_CImg(file_names[i].c_str());
    capture.read(frame);

    // cv::resize(frame, dst, dst.size(), 0, 0, cv::INTER_CUBIC);
    cv::resize(frame, dst, dst.size(), 0, 0, cv::INTER_LINEAR);
    // cv::resize(frame, dst, dst.size(), 0, 0, cv::INTER_LANCZOS4);

    // cout << frame;
    // cimg_library::CImg<unsigned char>* img = new cimg_library::CImg<unsigned char>(200, 200, 0, 3, 1);
    // cimg_library::CImg<unsigned char>* img = new cimg_library::CImg<unsigned char>(200, 200, 0, 3, 1);
    // cimg_library::CImg<array>* theimg = frame;

    // CImg cimg2(frame);

    CImg<> cimg2(dst);

    // CImg<unsigned char> image((unsigned char*)&w[0],width,height,1,3);

    // void emit_image(const cimg_library::CImg<unsigned char> & image, int flags)
    emit_image(cimg2, 1);

    // CImg<unsigned char> image(frame);

    // cout << typeid(frame).name() << endl;
  }
  // the camera will be deinitialized automatically in VideoCapture destructor
  return 0;
}