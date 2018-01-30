#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <sensor_msgs/image_encodings.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

static const char WINDOW[]="SRC Image";
static const char WINDOW2[]="DST Image";
static const char WINDOW3[]="Black White Image";


/**
 * Helper function to find a cosine of angle between vectors
 * from pt0->pt1 and pt0->pt2
 */
static double angle(cv::Point pt1, cv::Point pt2, cv::Point pt0)
{
	double dx1 = pt1.x - pt0.x;
	double dy1 = pt1.y - pt0.y;
	double dx2 = pt2.x - pt0.x;
	double dy2 = pt2.y - pt0.y;
	return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

/**
 * Helper function to display text in the center of a contour
 */
void setLabel(cv::Mat& im, const std::string label, std::vector<cv::Point>& contour)
{
	int fontface = cv::FONT_HERSHEY_SIMPLEX;
	double scale = 0.4;
	int thickness = 1;
	int baseline = 0;

	cv::Size text = cv::getTextSize(label, fontface, scale, thickness, &baseline);
	cv::Rect r = cv::boundingRect(contour);

	cv::Point pt(r.x + ((r.width - text.width) / 2), r.y + ((r.height + text.height) / 2));
	cv::rectangle(im, pt + cv::Point(0, baseline), pt + cv::Point(text.width, -text.height), CV_RGB(255,255,255), CV_FILLED);
	cv::putText(im, label, pt, fontface, scale, CV_RGB(0,0,0), thickness, 8);
}


void process(const sensor_msgs::ImageConstPtr& cam_image){
	

	//cv::Mat src = cv::imread("polygon.png");
	cv::Mat src;
	cv::Mat gray;
	cv::Mat bw;
	cv::Mat dst;
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Point> approx;
	cv_bridge::CvImagePtr bridge;
	bridge = cv_bridge::toCvCopy(cam_image, sensor_msgs::image_encodings::BGR8);
	gray = bridge->image;
	resize(gray,src,Size(1080,720));
	

	cv::cvtColor(src, gray, CV_BGR2GRAY);

			// Use Canny instead of threshold to catch squares with gradient shading
blur( gray, bw, Size(3,3) );
cv::Canny(gray, bw, 80, 240, 3);
cv::imshow("bw", bw);
			//cv::bitwise_not(bw, bw);

			// Find contours
cv::findContours(bw.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

src.copyTo(dst);

for (int i = 0; i < contours.size(); i++)
{
				// Approximate contour with accuracy proportional
				// to the contour perimeter
	cv::approxPolyDP(cv::Mat(contours[i]), approx, cv::arcLength(cv::Mat(contours[i]), true)*0.02, true);

				// Skip small or non-convex objects 
	if (std::fabs(cv::contourArea(contours[i])) < 100 || !cv::isContourConvex(approx))
		continue;

	if (approx.size() == 3)
	{
					setLabel(dst, "TRI", contours[i]);    // Triangles
				}
				else if (approx.size() >= 4 && approx.size() <= 6)
				{
					// Number of vertices of polygonal curve
					int vtc = approx.size();

					// Get the cosines of all corners
					std::vector<double> cos;
					for (int j = 2; j < vtc+1; j++)
						cos.push_back(angle(approx[j%vtc], approx[j-2], approx[j-1]));

					// Sort ascending the cosine values
					std::sort(cos.begin(), cos.end());

					// Get the lowest and the highest cosine
					double mincos = cos.front();
					double maxcos = cos.back();

					// Use the degrees obtained above and the number of vertices
					// to determine the shape of the contour
					if (vtc == 4 )
						setLabel(dst, "RECT", contours[i]);
					else if (vtc == 5 )
						setLabel(dst, "PENTA", contours[i]);
					else if (vtc == 6 )
						setLabel(dst, "HEXA", contours[i]);
				}
				else
				{
					// Detect and label circles
					double area = cv::contourArea(contours[i]);
					cv::Rect r = cv::boundingRect(contours[i]);
					int radius = r.width / 2;

					if (std::abs(1 - ((double)r.width / r.height)) <= 0.2 &&
						std::abs(1 - (area / (CV_PI * (radius*radius)))) <= 0.2)
						setLabel(dst, "CIR", contours[i]);
				}
			}


	cv::imshow(WINDOW,src);
	cv::imshow(WINDOW2,dst);
	cvWaitKey(1);
}

// Convert to grayscale




int main(int argc, char **argv){
	ros::init(argc,argv,"Display_Images");
	ros::NodeHandle n;
	image_transport::ImageTransport it(n);
	image_transport::Subscriber image_sub = it.subscribe("/ardrone/image_raw",1,process);

	cv::namedWindow(WINDOW);
	cv::namedWindow(WINDOW2);
	cv::namedWindow(WINDOW3);

	cv::destroyWindow(WINDOW);
	cv::destroyWindow(WINDOW2);
	cv::destroyWindow(WINDOW3);

	ros::spin();
	return 0;
}
