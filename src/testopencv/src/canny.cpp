#include <ros/ros.h>
#include <sensor_msgs/image_encodings.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <std_msgs/Empty.h>
#include <std_srvs/Empty.h>
#include "std_msgs/Int32.h"
#include "std_msgs/Float32.h"
#include <geometry_msgs/Twist.h>
#include "ardrone_autonomy/Navdata.h"
#include <geometry_msgs/Vector3.h>
#include <termios.h>
#include <cv.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui_c.h"
#include <opencv2/core/core.hpp>
#include "opencv2/opencv.hpp"

static const char WINDOW[]="tracking";
static const char WINDOW1[]="roiFrame1";
static const char WINDOW2[]="roiFrame2";
static const char WINDOW3[]="tracking";
static const char WINDOW4[]="canny";

using namespace std;
using namespace cv;

    geometry_msgs::Twist twist_msg;
    geometry_msgs::Twist hover;
    std_msgs::Empty msg;

int iLowH = 0,   iHighH = 179,   iLowS = 0,   iHighS = 255,   iLowV = 0,  iHighV = 10;   //hitam

static struct termios initial_settings, new_settings;
static int peek_character = -1;

ardrone_autonomy::Navdata msg_in;
int drone_altd;
float battery;

bool otomatis = false;
bool baterai = false;
unsigned int konturAtas, konturBawah;
unsigned int konturObjek=0;
float konturPersen;
double waktu1, waktu2;
char command =' ';

void chatterCallback( const sensor_msgs::ImageConstPtr & newimage)
{
    using namespace cv;

    Mat frame, frame1, frame2, roiFrame1, roiFrame2;
    Mat hsv, gray,threshold;

    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;


    cv_bridge::CvImagePtr bridge;
    bridge = cv_bridge::toCvCopy(newimage, sensor_msgs::image_encodings::BGR8);
    frame1 = bridge->image;
    resize(frame1,frame,Size(320,240));

    //PPHT
    Mat dst, cdst;
    Canny(frame, dst, 130, 250, 3);
    //cvtColor(dst, cdst, CV_GRAY2BGR);

    vector<Vec4i> lines;
    // detect the lines
    HoughLinesP(dst, lines, 1, CV_PI/180, 130, 150, 80 );
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        // draw the lines
        line( frame, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 2, CV_AA);
        line( cdst, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 2, CV_AA);
    }


    Point CoordA;
    Point CoordB;
    CoordA.x = 0;
    CoordA.y = 0;
    CoordB.x = 0;
    CoordB.y = 0;

    Mat roi1 = frame( Rect(0,0,320,120) );
    Mat roi2 = frame( Rect(0,120,320,120) );

    cvtColor(roi1, roiFrame1, CV_BGR2HSV );
    cvtColor(roi2, roiFrame2, CV_BGR2HSV );

    inRange(roiFrame1, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), roiFrame1);
    inRange(roiFrame2, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), roiFrame2);

    waktu1=0;
    waktu1=ros::Time::now().toSec();

    //--------------------------------ROI ATAS------------------------------------------//
    findContours(roiFrame1.clone(), contours, hierarchy, CV_RETR_TREE,CV_CHAIN_APPROX_SIMPLE, Point(0,0));
    for (unsigned int A = 0; A < contours.size(); A++) {
             float konturGaris = contourArea(contours[A]);
             if (konturGaris > 1000 && konturGaris < 5000) {
                 Moments mm = moments(contours[A], false);
                 double moment10 = mm.m10;
                 double moment01 = mm.m01;
                 konturAtas = mm.m00;
                 CoordA.x = int(moment10 / konturAtas);
                 CoordA.y = int(moment01 / konturAtas);
                 Point2f centerAtas(CoordA.x, CoordA.y);  // lokasi titik centroid Atas
                 circle(frame,centerAtas,5,Scalar(0, 0, 255),-1,8,0);
             }
          }

    //--------------------------------ROI BAWAH------------------------------------------//
    findContours(roiFrame2.clone(), contours, hierarchy, CV_RETR_TREE,CV_CHAIN_APPROX_SIMPLE, Point(0,0));
    for (unsigned int B = 0; B < contours.size(); B++) {
             float konturGaris = contourArea(contours[B]);
             if (konturGaris > 1000 && konturGaris < 5000) {
                 Moments mm = moments(contours[B], false);
                 double moment10 = mm.m10;
                 double moment01 = mm.m01;
                 konturBawah = mm.m00;
                 CoordB.x = int(moment10 / konturBawah);
                 CoordB.y = int(moment01 / konturBawah);
                 Point2f centerBawah(CoordB.x, CoordB.y+120);  // lokasi titik centroid Bawah
                 circle(frame,centerBawah,5,Scalar(0, 0, 255),-1,8,0);
             }
          }

    konturObjek=konturAtas+konturBawah;
    if(CoordA.x!=0 && CoordB.x!=0)
    {
        waktu2=ros::Time::now().toSec();
        waktu2-=waktu1;
        cout<<drone_altd/10<<"cm \t"<<konturObjek<<" px \t"<<waktu2<<" s"<<endl;
    }
	imshow(WINDOW,frame);
    imshow(WINDOW1,roiFrame1);
    imshow(WINDOW2,roiFrame2);
    imshow(WINDOW4,dst);

    int pressedkey;
    pressedkey = waitKey(1);

    
}

void init_keyboard()
{
    tcgetattr(0,&initial_settings);
    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
}

int kbhit()
{
    char ch;
    int nread;

    if(peek_character != -1)
        return 1;
    new_settings.c_cc[VMIN]=0;
    tcsetattr(0, TCSANOW, &new_settings);
    nread = read(0,&ch,1);
    new_settings.c_cc[VMIN]=1;
    tcsetattr(0, TCSANOW, &new_settings);

    if(nread == 1) {
        peek_character = ch;
        return 1;
    }
    return 0;
}

int readch()
{
    char ch;

    if(peek_character != -1) {
        ch = peek_character;
        peek_character = -1;
        return ch;
    }
    read(0,&ch,1);
    return ch;
}

void dataNavigasi(const ardrone_autonomy::Navdata& msg_in)
{
    drone_altd = msg_in.altd;
    battery = msg_in.batteryPercent;
}


int main(int argc, char** argv)
{

    ros::init(argc, argv,"lineFollowing");
    ros::NodeHandle node;
    ros::Rate loop_rate(50);
    ros::Subscriber sub_navdata;

    image_transport::ImageTransport it(node);
    image_transport::Subscriber it_sub = it.subscribe("ardrone/image_raw", 1, chatterCallback);

    sub_navdata = node.subscribe("/ardrone/navdata",1,dataNavigasi);

    ros::Publisher pub_empty_takeoff;
    ros::Publisher pub_empty_land;
    ros::Publisher pub_empty_reset;
    ros::Publisher pub_twist;


    //hover message
                hover.linear.x=0.0;
                hover.linear.y=0.0;
                hover.linear.z=0.0;
                hover.angular.z=0.0;


    pub_twist         = node.advertise<geometry_msgs::Twist>("/cmd_vel", 1);
    pub_empty_takeoff = node.advertise<std_msgs::Empty>("/ardrone/takeoff", 1);/* Message queue length is just 1 */
    pub_empty_land    = node.advertise<std_msgs::Empty>("/ardrone/land", 1);
    pub_empty_reset   = node.advertise<std_msgs::Empty>("/ardrone/reset", 1);

    cout<<"Ready to Fly!"<<endl;
    cout<<"Map keyboard:\n"
        <<"t = Take Off\n"
        <<"l = Landing\n"
        <<"h = Hovering\n"
        <<"m = Battery\n";
    cout<<"Ketinggian(cm)"<<"\t"<<"LuasKontur"<<"\t"<<"WaktuRespon"<<endl;

while (ros::ok())
{
    init_keyboard();
    if(kbhit())
        {
        command=readch();
        switch(command)
        {
        case 't':
            pub_empty_takeoff.publish(msg); //launches the drone
            pub_twist.publish(hover); //drone is flat
            cout<<"Taking Off"<<endl;
            command=' ';
            break;

        case 'l':
            pub_twist.publish(hover); //drone is flat
            pub_empty_land.publish(msg); //lands the drone
            cout<<"Landing"<<endl;
            exit(0);
            command = ' ';
            break;

        case 'h':
            cout<<"Hovering"<<endl;
            pub_twist.publish(hover);
            command = ' ';
            break;
            
        case 'm':
            if(baterai)baterai=false;
            else if(!baterai)baterai=true;
            cout<<"Battery: "<<battery<<endl;
            command = ' ';
            break;
        }
    }

    ros::spinOnce();
    loop_rate.sleep();
}//ros::ok

return 0;

}//main

