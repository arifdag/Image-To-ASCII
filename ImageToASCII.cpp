#include <filesystem>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>
#include <regex>

using namespace std;
using namespace cv;
using namespace filesystem;

// ASCII character set for intensity mapping
const char ASCIICharacters[] = {
    '@', '&', '%', 'Q', 'W', 'N', 'M', '0', 'g', 'B', '$', '#', 'D', 'R', '8', 'm',
    'H', 'X', 'K', 'A', 'U', 'b', 'G', 'O', 'p', 'V', '4', 'd', '9', 'h', '6', 'P',
    'k', 'q', 'w', 'S', 'E', '2', ']', 'a', 'y', 'j', 'x', 'Y', '5', 'Z', 'o', 'e',
    'n', '[', 'u', 'l', 't', '1', '3', 'I', 'f', '}', 'C', '{', 'i', 'F', '|', '(',
    '7', 'J', ')', 'v', 'T', 'L', 's', '?', 'z', '/', '*', 'c', 'r', '!', '+', '<',
    '>', ';', '=', '^', ',', ':', '_', '\'', '-', '.', ' '
};
char intensityToAscii[256]; // Global lookup table

map<char, Mat> preRenderedChars;
int charWidth = 0;
int charHeight = 0;

constexpr int ASCII_SIZE = sizeof(ASCIICharacters) / sizeof(ASCIICharacters[0]);
constexpr bool QUALITY_MODE = true;

void initializeLookupTable()
{
    for (int i = 0; i < 256; ++i)
    {
        intensityToAscii[i] = ASCIICharacters[i * ASCII_SIZE / 256];
    }
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
    for (int y = startRow; y < endRow; y++)
    {
        const uchar* row = image.ptr<uchar>(y);
        for (int x = 0; x < image.cols; x++)
        {
            asciiArt += intensityToAscii[row[x]];
        }
        asciiArt += '\n';
    }
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

void preRenderCharacters(int fontSize)
{
    preRenderedChars.clear();
    int baseline;
    Scalar textColor(0);
    double fontScale = fontSize / 24.0;

    // First pass: calculate max character dimensions
    for (char c : ASCIICharacters)
    {
        string s(1, c);
        Size textSize = getTextSize(s, FONT_HERSHEY_SIMPLEX, fontScale, 1, &baseline);
        charWidth = max(charWidth, textSize.width);
        charHeight = max(charHeight, textSize.height + baseline);
    }

    // Second pass: render characters centered in fixed-size cells
    for (char c : ASCIICharacters)
    {
        Mat charImg(charHeight, charWidth, CV_8UC3, Scalar(255, 255, 255));
        string s(1, c);
        Size textSize = getTextSize(s, FONT_HERSHEY_SIMPLEX, fontScale, 1, &baseline);

        // Center character in cell
        Point org(
            (charWidth - textSize.width) / 2,
            (charHeight + textSize.height - baseline) / 2
        );

        putText(charImg, s, org, FONT_HERSHEY_SIMPLEX, fontScale, textColor, 1, LINE_AA);
        preRenderedChars[c] = charImg;
    }
}

// Function to render a chunk of ASCII art lines to an image in a dedicated sub-region
void renderChunkToImage(const vector<string>& lines, Mat& asciiImage,
                        int startLine, int endLine, int lineSpacing)
{
    for (int i = startLine; i < endLine; i++)
    {
        int yPos = i * (charHeight + lineSpacing);
        for (int x = 0; x < lines[i].size(); x++)
        {
            char c = lines[i][x];
            Mat& charImg = preRenderedChars[c];

            // Calculate position for this character
            Rect destROI(
                x * charWidth, // X position
                yPos, // Y position
                charWidth, // Cell width
                charHeight // Cell height
            );

            // Ensure ROI stays within image bounds
            if (destROI.x + destROI.width > asciiImage.cols ||
                destROI.y + destROI.height > asciiImage.rows)
            {
                continue;
            }

            charImg.copyTo(asciiImage(destROI));
        }
    }
}

// Function to render ASCII art to an image using threads
Mat renderAsciiToImage(const string& asciiArt, int fontSize = 12)
{
    static bool initialized = false;
    if (!initialized)
    {
        preRenderCharacters(fontSize);
        initialized = true;
    }

    vector<string> lines;
    stringstream ss(asciiArt);
    string line;

    while (getline(ss, line))
    {
        lines.push_back(line);
    }

    if (lines.empty())
    {
        return Mat::zeros(100, 100, CV_8UC3); // Return a small blank image if no lines
    }

    // Calculate image dimensions
    int maxLineLength = 0;
    for (const auto& l : lines)
    {
        maxLineLength = max(maxLineLength, (int)l.size());
    }

    Mat asciiImage(
        lines.size() * charHeight, // Height
        maxLineLength * charWidth, // Width
        CV_8UC3, // Type
        Scalar(255, 255, 255) // White background
    );

    // Render with threads
    const int numThreads = thread::hardware_concurrency();
    vector<thread> threads;
    const int linesPerThread = lines.size() / numThreads;

    for (int i = 0; i < numThreads; i++)
    {
        int startLine = i * linesPerThread;
        int endLine = (i == numThreads - 1) ? lines.size() : startLine + linesPerThread;

        threads.emplace_back(renderChunkToImage, cref(lines), ref(asciiImage), startLine, endLine, 2);
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

void processImage(Mat& image, string outputPath)
{
    if (image.empty())
    {
        cerr << "Error: Could not load the image!" << '\n';
        return;
    }

    int targetWidth = 350;
    Mat resizedImage = resizeImage(image, targetWidth);
    Mat grayImage;
    cvtColor(resizedImage, grayImage, COLOR_BGR2GRAY);
    string asciiArt = convertImageToASCII(grayImage);

    int fontSize = 6;
    Mat asciiImage = renderAsciiToImage(asciiArt, fontSize);
    saveImage(asciiImage, outputPath);

    cout << "ASCII art image saved to " << outputPath << endl;
}

// Function to extract the numeric part from a filename
int extractFrameNumber(const string& filename)
{
    regex re("frame (\\d+)\\.jpeg");
    smatch match;
    if (regex_search(filename, match, re))
    {
        return stoi(match[1]);
    }
    return -1; // Return -1 for filenames that don't match
}

void extractFrames(string videoFilePath)
{
    VideoCapture video(videoFilePath);
    if (!video.isOpened())
    {
        cerr << "Error: Could not load the video!" << endl;
        return;
    }

    bool success = true;
    Mat image;
    int count = 0;

    while (success)
    {
        success = video.read(image);
        string filename = "video frames/bad apple ascii/frame " + to_string(count) + ".jpeg";
        processImage(image, filename);
        count++;
    }
}

void generateVideo(string fileName, int fps, int width, int height, string framePath, string outputPath)
{
    VideoWriter video(outputPath, VideoWriter::fourcc('D', 'I', 'V', 'X'), fps, Size(width, height));

    if (!video.isOpened())
    {
        cerr << "Error: Could not open the video writer!" << '\n';
        return;
    }

    // Collect all file paths
    vector<string> filePaths;
    for (const auto& entry : directory_iterator(framePath))
    {
        if (entry.is_regular_file())
        {
            filePaths.push_back(entry.path().string());
        }
    }

    // Sort the file paths numerically by extracting frame numbers
    sort(filePaths.begin(), filePaths.end(), [](const string& a, const string& b)
    {
        return extractFrameNumber(a) < extractFrameNumber(b);
    });

    int count = 0;
    for (const auto& filePath : filePaths)
    {
        Mat frame = imread(filePath);
        if (frame.empty())
        {
            cerr << "Warning: Could not read frame " << filePath << endl;
            continue;
        }

        // Resize the frame to the target size
        Mat resizedFrame;
        resize(frame, resizedFrame, Size(width, height));

        video.write(resizedFrame);
        cout << "Processed frame: " << filePath << endl;
        count++;
    }

    video.release();
    cout << "Video generated successfully with " << count << " frames at " << outputPath << endl;
}

int main()
{
    initializeLookupTable();
    string framePath = "video/path";
    extractFrames(framePath);

    // Generate the video from the ASCII frames
    string asciiFramesPath = "frame/path";
    string outputVideoPath = "file_name.avi";

    generateVideo("File Name", 30, 1280, 720, asciiFramesPath, outputVideoPath);

    return 0;
}

struct imageSettings
{
    int targetWidth;
    int fontSize;
};
