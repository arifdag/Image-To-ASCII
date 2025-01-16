#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>

using namespace std;
using namespace cv;

// ASCII character set for intensity mapping
const char ASCIICharacters[] = {
    '@', '&', '%', 'Q', 'W', 'N', 'M', '0', 'g', 'B', '$', '#', 'D', 'R', '8', 'm',
    'H', 'X', 'K', 'A', 'U', 'b', 'G', 'O', 'p', 'V', '4', 'd', '9', 'h', '6', 'P',
    'k', 'q', 'w', 'S', 'E', '2', ']', 'a', 'y', 'j', 'x', 'Y', '5', 'Z', 'o', 'e',
    'n', '[', 'u', 'l', 't', '1', '3', 'I', 'f', '}', 'C', '{', 'i', 'F', '|', '(',
    '7', 'J', ')', 'v', 'T', 'L', 's', '?', 'z', '/', '*', 'c', 'r', '!', '+', '<',
    '>', ';', '=', '^', ',', ':', '_', '\'', '-', '.', ' '
};
constexpr int ASCII_SIZE = sizeof(ASCIICharacters) / sizeof(ASCIICharacters[0]);
constexpr bool QUALITY_MODE = true;

// Mutex for thread-safe concatenation of ASCII art
mutex asciiMutex;

// Function to map pixel intensity to ASCII character
char mapIntensityToAscii(int intensity)
{
    return ASCIICharacters[intensity * ASCII_SIZE / 256];
}

// Function to resize an image for ASCII conversion
Mat resizeImage(const Mat& image, int targetWidth)
{
    int targetHeight = static_cast<int>(image.rows * (static_cast<float>(targetWidth) / image.cols));
    Mat resizedImage;
    resize(image, resizedImage, Size(targetWidth, targetHeight), 0, 0, INTER_LINEAR);
    return resizedImage;
}

// Function to convert a chunk of an image to ASCII art
void convertChunkToASCII(const Mat& image, string& asciiArt, int startRow, int endRow)
{
    string localAsciiArt;
    for (int y = startRow; y < endRow; y++)
    {
        for (int x = 0; x < image.cols; x++)
        {
            int intensity = image.at<uchar>(y, x);
            localAsciiArt += mapIntensityToAscii(intensity);
        }
        localAsciiArt += '\n';
    }

    // Thread-safe concatenation
    lock_guard<mutex> lock(asciiMutex);
    asciiArt += localAsciiArt;
}

// Function to convert an image to an ASCII art string using threads
string convertImageToASCII(const Mat& image)
{
    int numThreads = thread::hardware_concurrency();
    vector<thread> threads;
    vector<string> results(numThreads); // To store ASCII art chunks

    int rowsPerThread = image.rows / numThreads;

    for (int i = 0; i < numThreads; ++i)
    {
        int startRow = i * rowsPerThread;
        int endRow = (i == numThreads - 1) ? image.rows : startRow + rowsPerThread;

        threads.emplace_back(convertChunkToASCII, cref(image), ref(results[i]), startRow, endRow);
    }

    for (auto& t : threads) t.join();

    // Combine results
    string asciiArt;
    for (const auto& result : results)
    {
        asciiArt += result;
    }
    return asciiArt;
}

// Function to render a chunk of ASCII art lines to an image
void renderChunkToImage(const vector<string>& lines, Mat& asciiImage, int startLine, int endLine, int lineHeight,
                        int fontSize)
{
    Scalar textColor(0); // Black text
    for (int i = startLine; i < endLine; i++)
    {
        Point textOrg(5, (i + 1) * lineHeight);
        putText(asciiImage, lines[i], textOrg, FONT_HERSHEY_SIMPLEX,
                fontSize / 24.0, textColor, 1, LINE_AA);
    }
}

// Function to render ASCII art to an image using threads
Mat renderAsciiToImage(const string& asciiArt, int fontSize = 12)
{
    vector<string> lines;
    stringstream ss(asciiArt);
    string line;

    while (getline(ss, line))
    {
        lines.push_back(line);
    }

    int lineHeight = fontSize + 2;
    int maxLineWidth = 0;
    for (const auto& l : lines)
    {
        maxLineWidth = max(maxLineWidth, static_cast<int>(l.length()));
    }

    int imageWidth = static_cast<int>(maxLineWidth * (fontSize / 1.3));
    int imageHeight = static_cast<int>(lines.size() * lineHeight);

    Mat asciiImage = Mat::zeros(imageHeight, imageWidth, CV_8UC3);
    asciiImage.setTo(Scalar(255, 255, 255)); // White background

    int numThreads = std::thread::hardware_concurrency();
    vector<thread> threads;
    int linesPerThread = lines.size() / numThreads;

    for (int i = 0; i < numThreads; ++i)
    {
        int startLine = i * linesPerThread;
        int endLine = (i == numThreads - 1) ? lines.size() : startLine + linesPerThread;

        threads.emplace_back(renderChunkToImage, cref(lines), ref(asciiImage), startLine, endLine, lineHeight,
                             fontSize);
    }

    for (auto& t : threads) t.join();

    return asciiImage;
}

// Function to save an image to file
void saveImage(const Mat& image, const string& outputPath)
{
    vector<int> compressionParams;
    if (QUALITY_MODE)
    {
        compressionParams = {IMWRITE_JPEG_QUALITY, 100, IMWRITE_JPEG_OPTIMIZE, 0};
    }
    else
    {
        compressionParams = {IMWRITE_JPEG_QUALITY, 0, IMWRITE_JPEG_OPTIMIZE, 1};
    }
    imwrite(outputPath, image, compressionParams);
}

void extractFrames(string videoFilePath)
{
    VideoCapture video(videoFilePath);
    if (!video.isOpened()) {
        cerr << "Error: Could not load the video!" << endl;
        return;
    }

    bool success = true;
    Mat image;
    int count = 0;
    while (success)
    {
        success = video.read(image);
        string filename = "frame " + to_string(count) + ".jpeg";
        saveImage(image, filename);
        count++;
    }
    
}

void processImage(Mat& image, string outputPath)
{
    if (image.empty())
    {
        cerr << "Error: Could not load the image!" << '\n';
        return;
    }

    int targetWidth = 350;
    Mat resizedImage = resizeImage(image, targetWidth);
    string asciiArt = convertImageToASCII(resizedImage);

    int fontSize = 6;
    Mat asciiImage = renderAsciiToImage(asciiArt, fontSize);

    saveImage(asciiImage, outputPath);

    cout << "ASCII art image saved to " << outputPath << endl;
}

int main()
{
    const string path = "Path/To/File";

    
    extractFrames(path);
    /*processImage(image,outputPath)
     * Mat image = imread(path, IMREAD_GRAYSCALE);
     ;*/
    return 0;
}

struct imageSettings
{
    int targetWidth;
    int fontSize;
};
