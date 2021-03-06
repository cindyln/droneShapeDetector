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
static const char WINDOW4[]="roiFrame3";
static const char WINDOW3[]="tracking";

using namespace std;
using namespace cv;

    geometry_msgs::Twist twist_msg;
    geometry_msgs::Twist hover;
    geometry_msgs::Twist maju;
    geometry_msgs::Twist mundur;
    geometry_msgs::Twist rotasika;
    geometry_msgs::Twist rotasiki;
    geometry_msgs::Twist geserka;
    geometry_msgs::Twist geserki;
    geometry_msgs::Twist naik;
    geometry_msgs::Twist turun;
    std_msgs::Empty msg;


static struct termios initial_settings, new_settings;
static int peek_character = -1;

ardrone_autonomy::Navdata msg_in;
int drone_altd;
float battery;

bool otomatis = false;
bool baterai = false;
unsigned int konturAtas;
unsigned int konturObjek=0;
float konturPersen;
double waktu1, waktu2;
char command =' ';


void chatterCallback( const sensor_msgs::ImageConstPtr & newimage)
{
    using namespace cv;

    Mat frame, frame1, roiFrame1;
    Mat hsv, gray,threshold;

    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;

    cv_bridge::CvImagePtr bridge;
    bridge = cv_bridge::toCvCopy(newimage, sensor_msgs::image_encodings::BGR8);
    frame1 = bridge->image;
    resize(frame1,frame,Size(320,240));

    Point CoordA;
    CoordA.x = 0;
    CoordA.y = 0;
    Mat roi1 = frame( Rect(107,80,106,80) );
    cvtColor(roi1, roiFrame1, CV_BGR2HSV );


    waktu1=0;
    waktu1=ros::Time::now().toSec();

 
  
  
    
    //----------------------------vertical----------------------------------//
    line( frame, Point( 107,80 ), Point( 107,160), Scalar( 255, 0, 0),  1, 8 );
    line( frame, Point( 213,80 ), Point( 213,160), Scalar( 255, 0, 0),  1, 8 );
    
    //---------------------------horizontal---------------------------------//
    line( frame, Point( 107,80 ), Point( 213,80), Scalar( 255, 0, 0),  1, 8 );
    line( frame, Point( 107,160 ), Point( 213,160), Scalar( 255, 0, 0),  1, 8 );
    //---------------------------------------------------------------------//
   
   Vec3b pix=frame.at<Vec3b>(CoordA.x=160,CoordA.y=120);
     int Red=frame.at<cv::Vec3b>(CoordA.x=160,CoordA.y=120)[2];
     int Green=frame.at<cv::Vec3b>(CoordA.x=160,CoordA.y=120)[1];
     int Blue=frame.at<cv::Vec3b>(CoordA.x=160,CoordA.y=120)[0];
     int maksimal,minimal, max, min;
     char warna[20];
	 
	 
     int r= (Red*100)/255;
    float g=(Green*100)/255;
    float b=(Blue*100)/255;
    float gminb=g-b;
    float bminr=b-r;
    float rming= r-g;
    float h;
   
	if(r>g&&r>b){
		h=gminb/(max-min);
	}else if(g>r&&g>b){
		h=2+ bminr/(max-min);
	}else{
		h=4+rming/(max-min);
	}
	
	
	float v=maksimal*100;
	//float s= ((maksimal-minimal)*100)/maksimal;
    float s= (maksimal-minimal)*100;
     
     
     cout<<"  RED= "<<Red<<"  GREEN="<<Green<<"  BLUE="<<Blue<<"  H="<<h*60 <<" S="<<s/maksimal<<"  V="<<v/255<<" \t\n";
 
	 putText(frame,warna,Point(CoordA.x=160, CoordA.y=120), FONT_HERSHEY_PLAIN, 1, Scalar(0,0,255),2); //

    imshow(WINDOW,frame);
    imshow(WINDOW1,roiFrame1);
   
    

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

/*
    -linear.x: move backward
    +linear.x: move forward
    -linear.y: move right
    +linear.y: move left
    -linear.z: move down
    +linear.z: move up
    -angular.z: turn left
    +angular.z: turn right
*/

    //hover message
                hover.linear.x=0.0;
                hover.linear.y=0.0;
                hover.linear.z=0.0;
                hover.angular.z=0.0;

    //maju message----------------
                maju.linear.x=0.1;
                maju.linear.y=0.0;
                maju.linear.z=0.0;
                maju.angular.z=0.0;

    //mundur message
                mundur.linear.x=-0.1;
                mundur.linear.y=0.0;
                mundur.linear.z=0.0;
                mundur.angular.z=0.0;

    //geser kanan message
                geserka.linear.x=0.0;
                geserka.linear.y=-0.1;
                geserka.linear.z=0.0;
                geserka.angular.z=0.0;

    //geser kiri message
                geserki.linear.x=0.0;
                geserki.linear.y=+0.1;
                geserki.linear.z=0.0;
                geserki.angular.z=0.0;

    //rotasi kanan message
                rotasika.linear.x=0.0;
                rotasika.linear.y=0.0;
                rotasika.linear.z=0.0;
                rotasika.angular.z=-0.2;

    //rotasi kiri message
                rotasiki.linear.x=0.0;
                rotasiki.linear.y=0.0;
                rotasiki.linear.z=0.0;
                rotasiki.angular.z=+0.2;

    //naik message
                naik.linear.x=0.0;
                naik.linear.y=0.0;
                naik.linear.z=+0.2;
                naik.angular.z=0.0;

    //turun message
                turun.linear.x=0.0;
                turun.linear.y=0.0;
                turun.linear.z=-0.2;
                turun.angular.z=0.0;

    pub_twist         = node.advertise<geometry_msgs::Twist>("/cmd_vel", 1);
    pub_empty_takeoff = node.advertise<std_msgs::Empty>("/ardrone/takeoff", 1);
    pub_empty_land    = node.advertise<std_msgs::Empty>("/ardrone/land", 1);
    pub_empty_reset   = node.advertise<std_msgs::Empty>("/ardrone/reset", 1);

    cout<<"Ready to Fly!"<<endl;
    cout<<"Map keyboard:\n"
        <<"w = Maju              t = Take Off\n"
        <<"s = Mundur            l = Landing\n"
        <<"a = Geser Kiri        i = Altitude+\n"
        <<"d = Geser Kanan       k = Altitude-\n"
        <<"q = Rotasi Kiri       h = Hovering\n"
        <<"e = Rotasi Kanan      b = Baterai \n";
    cout<<"Ketinggian(cm)"<<"\t"<<"LuasKontur"<<"\t"<<"LuasKontur(%)"<<"\t"<<"WaktuRespon"<<endl;

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

        case 'w':
            cout<<"Gerak Maju"<<endl;
            pub_twist.publish(maju);
            command=' ';
            break;

        case 's':
            cout<<"Gerak Mundur"<<endl;
            pub_twist.publish(mundur);
            command = ' ';
            break;

        case 'd':
            cout<<"Geser Kanan"<<endl;
            pub_twist.publish(geserka);
            command = ' ';
            break;

        case 'a':
            cout<<"Geser Kiri"<<endl;
            pub_twist.publish(geserki);
            command = ' ';
            break;

        case 'e':
            cout<<"Rotasi Kanan"<<endl;
            pub_twist.publish(rotasika);
            command = ' ';
            break;

        case 'q':
            cout<<"Rotasi Kiri"<<endl;
            pub_twist.publish(rotasiki);
            command = ' ';
            break;

        case 'i':
            cout<<"Altitude+"<<endl;
            pub_twist.publish(naik);
            command = ' ';
            break;

        case 'k':
            cout<<"Altitude-"<<endl;
            pub_twist.publish(turun);
            command = ' ';
            break;

        case 'b':
            if(baterai)baterai=false;
            else if(!baterai)baterai=true;cout<<"Battery: "<<battery<<endl;
            command = ' ';
            break;

        }
    }

    ros::spinOnce();
    loop_rate.sleep();
}//ros::ok

return 0;

}//main

