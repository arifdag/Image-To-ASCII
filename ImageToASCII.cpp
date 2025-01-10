#include <iostream>
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;

char ASCIICharacters[] = {
    '@', '&', '%', 'Q', 'W', 'N', 'M', '0', 'g', 'B', '$', '#', 'D', 'R', '8', 'm',
    'H', 'X', 'K', 'A', 'U', 'b', 'G', 'O', 'p', 'V', '4', 'd', '9', 'h', '6', 'P',
    'k', 'q', 'w', 'S', 'E', '2', ']', 'a', 'y', 'j', 'x', 'Y', '5', 'Z', 'o', 'e',
    'n', '[', 'u', 'l', 't', '1', '3', 'I', 'f', '}', 'C', '{', 'i', 'F', '|', '(',
    '7', 'J', ')', 'v', 'T', 'L', 's', '?', 'z', '/', '*', 'c', 'r', '!', '+', '<',
    '>', ';', '=', '^', ',', ':', '_', '\'', '-', '.', ' '
};
constexpr int ascii_size = std::size(ASCIICharacters);
int rangePerCharacter = 256 / ascii_size;


char mapIntensityToAscii(int intensity)
{
    return ASCIICharacters[intensity * ascii_size / 256];
}

string imageToASCII(Mat image)
{
    string ascii_str;
    const int width = image.rows;
    const int height = image.cols;
    for (int y = 0; y < width; y++)
    {
        for (int x = 0; x < height; x++)
        {
            const int intensity = image.at<uchar>(y, x);
            ascii_str += mapIntensityToAscii(intensity);
        }
        ascii_str += "\n";
    }
    return ascii_str;
}

Mat renderAsciiArtToImage(const string& asciiArt, int fontSize = 12)
{
    // Split the ASCII art into lines
    vector<string> lines;
    stringstream ss(asciiArt);
    string line;
    while (getline(ss, line))
    {
        lines.push_back(line);
    }

    // Calculate image dimensions
    const int lineHeight = fontSize + 4;
    int maxLineWidth = 0;
    for (const auto& l : lines)
    {
        maxLineWidth = max(maxLineWidth, static_cast<int>(l.length()));
    }

    // Create a white image with proper dimensions
    const int imageWidth = maxLineWidth * (fontSize / 1.3); // Adjust for character width
    const int imageHeight = lines.size() * lineHeight;
    Mat image = Mat::zeros(imageHeight, imageWidth, CV_8UC3);
    image.setTo(Scalar(255, 255, 255)); // White background

    // Set up text parameters
    Point textOrg;
    const Scalar textColor(0, 0, 0); // Black text

    // Write each line of ASCII art
    for (size_t i = 0; i < lines.size(); i++)
    {
        textOrg = Point(5, (i + 1) * lineHeight);
        putText(image, lines[i], textOrg, FONT_HERSHEY_SIMPLEX,
                fontSize / 24.0, textColor, 1, LINE_AA);
    }

    return image;
}

Mat resizeForAscii(const Mat& image, float aspectRatio = 2.0)
{
    const int new_width = static_cast<int>(image.cols * aspectRatio);
    Mat resizedImage;
    resize(image, resizedImage, Size(new_width, image.rows), 0, 0, INTER_LINEAR);
    return resizedImage;
}


int main()
{
    const string path = "Path/to/Images";
    const Mat image = imread(path, IMREAD_GRAYSCALE);

    // Check if image is loaded successfully
    if (image.empty())
    {
        cout << "Error: Could not load the image!" << '\n';
        return -1;
    }

    // Adjust image for ASCII aspect ratio
    const Mat adjustedImage = resizeForAscii(image);

    // Convert to ASCII art
    const string asciiArt = imageToASCII(adjustedImage);

    // Render ASCII art back to an image
    const Mat asciiImage = renderAsciiArtToImage(asciiArt, 12);


    imwrite("ascii_art_output.png", asciiImage);
    return 0;
}
