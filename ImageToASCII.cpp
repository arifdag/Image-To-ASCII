#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <sstream>
#include <string>

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

// Function to map pixel intensity to ASCII character
char mapIntensityToAscii(int intensity) {
    return ASCIICharacters[intensity * ASCII_SIZE / 256];
}

// Function to resize an image for ASCII conversion
Mat resizeImage(const Mat& image, int targetWidth) {
    int targetHeight = static_cast<int>(image.rows * (static_cast<float>(targetWidth) / image.cols));
    Mat resizedImage;
    resize(image, resizedImage, Size(targetWidth, targetHeight), 0, 0, INTER_LINEAR);
    return resizedImage;
}

// Function to convert an image to an ASCII art string
string convertImageToASCII(const Mat& image) {
    string asciiArt;
    for (int y = 0; y < image.rows; y++) {
        for (int x = 0; x < image.cols; x++) {
            int intensity = image.at<uchar>(y, x);
            asciiArt += mapIntensityToAscii(intensity);
        }
        asciiArt += '\n';
    }
    return asciiArt;
}

// Function to render ASCII art to an image
Mat renderAsciiToImage(const string& asciiArt, int fontSize = 12) {
    vector<string> lines;
    stringstream ss(asciiArt);
    string line;

    while (getline(ss, line)) {
        lines.push_back(line);
    }

    int lineHeight = fontSize + 2;
    int maxLineWidth = 0;
    for (const auto& l : lines) {
        maxLineWidth = max(maxLineWidth, static_cast<int>(l.length()));
    }

    int imageWidth = static_cast<int>(maxLineWidth * (fontSize / 1.3));
    int imageHeight = static_cast<int>(lines.size() * lineHeight);

    Mat asciiImage = Mat::zeros(imageHeight, imageWidth, CV_8UC3);
    asciiImage.setTo(Scalar(255, 255, 255)); // White background

    Point textOrg;
    Scalar textColor(0); // Black text

    for (size_t i = 0; i < lines.size(); i++) {
        textOrg = Point(5, (i + 1) * lineHeight);
        putText(asciiImage, lines[i], textOrg, FONT_HERSHEY_SIMPLEX,
                fontSize / 24.0, textColor, 1, LINE_AA);
    }

    return asciiImage;
}

// Function to save an image to file
void saveImage(const Mat& image, const string& outputPath) {
    vector<int> compressionParams;
    if (QUALITY_MODE) {
        compressionParams = {IMWRITE_JPEG_QUALITY, 100, IMWRITE_JPEG_OPTIMIZE, 0};
    } else {
        compressionParams = {IMWRITE_JPEG_QUALITY, 0, IMWRITE_JPEG_OPTIMIZE, 1};
    }
    imwrite(outputPath, image, compressionParams);
}

int main() {
    const string imagePath = "Path/to/Image";
    Mat image = imread(imagePath, IMREAD_GRAYSCALE);

    if (image.empty()) {
        cerr << "Error: Could not load the image!" << endl;
        return -1;
    }

    int targetWidth = 350;
    Mat resizedImage = resizeImage(image, targetWidth);
    string asciiArt = convertImageToASCII(resizedImage);

    int fontSize = 6;
    Mat asciiImage = renderAsciiToImage(asciiArt, fontSize);

    const string outputPath = "ascii_art_output.jpeg";
    saveImage(asciiImage, outputPath);

    cout << "ASCII art image saved to " << outputPath << endl;
    return 0;
}