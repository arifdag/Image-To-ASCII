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
int rangePerCharacter = 256 / size(ASCIICharacters);


char mapIntensityToAscii(int intensity)
{
    intensity /= rangePerCharacter;
    return ASCIICharacters[intensity];
}

void imageToASCII(Mat image)
{
    string ASCIIStr = "";
    int width = image.rows;
    int height = image.cols;
    for (int y = 0; y < width; y++)
    {
        for (int x = 0; x < height; x++)
        {
            int intensity = image.at<uchar>(y, x);
            ASCIIStr += mapIntensityToAscii(intensity);
        }
        ASCIIStr += "\n";
    }
    cout << ASCIIStr;
}

int main()
{
    string path = "Path/To/Image";
    Mat image = imread(path,IMREAD_GRAYSCALE);
    

    // Check if image is loaded successfully
    if (image.empty())
    {
        cout << "Error: Could not load the image!" << endl;
        return -1;
    }

    // Adjust the size as needed
    Mat resizedImage;
    int newWidth = image.cols / 2; 
    int newHeight = image.rows / 4;
    resize(image, resizedImage, Size(newWidth, newHeight));
    

    imageToASCII(resizedImage);
    return 0;
}
