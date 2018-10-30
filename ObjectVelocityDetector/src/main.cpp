#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <Eigen/Dense>
#include <Eigen/SVD>
#include <Eigen/Geometry>
#include <unsupported/Eigen/MatrixFunctions>
#include <pcl/common/eigen.h>
#include <pcl/common/transforms.h>
#include "../include/pythoncodecontroller.h"

#include <math.h>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/vlp_grabber.h>
#include <pcl/console/parse.h>
#include <pcl/visualization/pcl_visualizer.h>

#include "../include/utilities.h"

#include <chrono>

using namespace cv;
using namespace Eigen;
using namespace std;

constexpr float Rad2Deg = 180.0 / M_PI;

Matrix4f left_rigid_body_transformation;
Matrix4f right_rigid_body_transformation;

cv::Mat left_projection_matrix;

void InitXForms()
{
//    left_rigid_body_transformation <<
//            1.0f,       -0.0f,          -0.0f,      0.4019f,
//            0.0f,       1.0f,           0.291714f,  -0.31337f,
//            0.0f,       -0.291852f,     1.0f,       -0.0f,
//            0,          0,              0,          1.0f;


    //left_rigid_body_transformation <<
      //                             1.0f,       -0.0f,          -0.0f,      0.31337f,
        //                                       0.0f,       1.0f,           0.0,        -0.0f,
          //                                     0.0f,       -0.0,           1.0f,       -0.0f,
            //                                   0,          0,              0,          1.0f;
    left_rigid_body_transformation <<
            0.999814f, -0.00518012f, -0.0185683f, 0.4019f,
            0.0103731f, 0.956449f, 0.291714f, -0.31337f,
            0.0162485f, -0.291852f, 0.956325f, -0.0502383f,
            0, 0, 0, 1;

    right_rigid_body_transformation <<
                                    0.998874f, -0.0219703f, -0.0420543f, -0.23583f,
                                               0.0293173f, 0.982682f, 0.182967, -0.301624f,
                                               0.0373061, -0.183993f, 0.982219f, 0.0268978f,
                                               0, 0, 0, 1;

    float left_raw_projection[12] = {942.129700f, 0.0, 985.634114f, 0.0,
                                     0.0, 1060.674438f, 600.441036f, 0.0,
                                     0.0, 0.0, 1.0f, 0.0
                                    };
                                    
    float left_raw_projection_example[12] = {611.651245f, 0.0f, 642.388357f, 0.0f,
0.0f, 688.443726f, 365.971718f, 0.0f,
0.0f, 0.0f, 1.0f, 0.0f};

    cv::Mat(3, 4, CV_32FC1, &left_raw_projection).copyTo(left_projection_matrix);
}


struct mtypes
{
    float v4;
    float v3;
    float a4;
    float a3;
};

struct xyzw
{
    mtypes x, y, z, w;
};


// Point Type
// pcl::PointXYZ, pcl::PointXYZI, pcl::PointXYZRGBA


void parseInitialArgs(int argc, char *argv[], std::string& ipaddress, std::string& port, std::string& pcap)
{
    // Command-Line Argument Parsing
    if( pcl::console::find_switch( argc, argv, "-help" ) ) {
        std::cout << "usage: " << argv[0]
                  << " [-ipaddress <192.168.1.70>]"
                  << " [-port <2368>]"
                  << " [-pcap <*.pcap>]"
                  << " [-help]"
                  << std::endl;
        exit(1);
    }

    pcl::console::parse_argument( argc, argv, "-ipaddress", ipaddress );
    pcl::console::parse_argument( argc, argv, "-port", port );
    pcl::console::parse_argument( argc, argv, "-pcap", pcap );

    std::cout << "-ipadress : " << ipaddress << std::endl;
    std::cout << "-port : " << port << std::endl;
    std::cout << "-pcap : " << pcap << std::endl;
}

void initializePCLViewer(boost::shared_ptr<pcl::visualization::PCLVisualizer>& viewer,  pcl::visualization::PointCloudColorHandler<PointType>::Ptr& handler)
{
    viewer->addCoordinateSystem( 3.0, "coordinate" );
    viewer->setBackgroundColor( 0.0, 0.0, 0.0, 0 );
    viewer->initCameraParameters();
    viewer->setCameraPosition( 0.0, 0.0, 30.0, 0.0, 1.0, 0.0, 0 );

    const std::type_info& type = typeid( PointType );
    if( type == typeid( pcl::PointXYZ ) ) {
        std::vector<double> color = { 255.0, 255.0, 255.0 };
        boost::shared_ptr<pcl::visualization::PointCloudColorHandlerCustom<PointType>> color_handler( new pcl::visualization::PointCloudColorHandlerCustom<PointType>( color[0], color[1], color[2] ) );
        handler = color_handler;
    }
    else if( type == typeid( pcl::PointXYZI ) ) {
        boost::shared_ptr<pcl::visualization::PointCloudColorHandlerGenericField<PointType>> color_handler( new pcl::visualization::PointCloudColorHandlerGenericField<PointType>( "intensity" ) );
        handler = color_handler;
    }
    else if( type == typeid( pcl::PointXYZRGBA ) ) {
        boost::shared_ptr<pcl::visualization::PointCloudColorHandlerRGBField<PointType>> color_handler( new pcl::visualization::PointCloudColorHandlerRGBField<PointType>() );
        handler = color_handler;
    }
    else {
        throw std::runtime_error( "This PointType is unsupported." );
    }
}

void initializeGrabber(boost::shared_ptr<pcl::VLPGrabber>& grabber, std::string& ipaddress, std::string& port, std::string& pcap)
{
    if( !pcap.empty() ) {
        std::cout << "Capture from PCAP..." << std::endl;
        grabber = boost::shared_ptr<pcl::VLPGrabber>( new pcl::VLPGrabber( pcap ) );
    }
    else if( !ipaddress.empty() && !port.empty() ) {
        std::cout << "Capture from Sensor..." << std::endl;
        grabber = boost::shared_ptr<pcl::VLPGrabber>( new pcl::VLPGrabber( boost::asio::ip::address::from_string( ipaddress ), boost::lexical_cast<unsigned short>( port ) ) );
    }

}

int main( int argc, char *argv[] )
{
    std::string ipaddress( "192.168.1.70" );
    std::string port( "2368" );
    std::string pcap;

    parseInitialArgs(argc, argv, ipaddress, port, pcap);

    // Point Clouds initialization
    pcl::PointCloud<PointType>::ConstPtr cloud;
    pcl::PointCloud<PointType>::ConstPtr xformedCloud = cloud;


    // Point Cloud Color Handler and viewer initialization
    pcl::visualization::PointCloudColorHandler<PointType>::Ptr handler;
    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer( new pcl::visualization::PCLVisualizer( "Velodyne Viewer" ) );

    initializePCLViewer(viewer, handler);


    // Retrieved Point Cloud Callback Function
    boost::mutex mutex;
    boost::function<void( const pcl::PointCloud<PointType>::ConstPtr& )> function =
    [ &cloud, &mutex ]( const pcl::PointCloud<PointType>::ConstPtr& ptr ) {
        boost::mutex::scoped_lock lock( mutex );

        /* Point Cloud Processing */
        performTransform(*ptr, *boost::const_pointer_cast<pcl::PointCloud<PointType> >(ptr), 0, 0, 0, M_PI / 2, 0, 0);
        pcl::transformPointCloud(*ptr, *boost::const_pointer_cast<pcl::PointCloud<PointType> >(ptr), left_rigid_body_transformation);
        cloud = ptr;
    };

    // VLP Grabber
    boost::shared_ptr<pcl::VLPGrabber> grabber;
    initializeGrabber(grabber, ipaddress, port, pcap);


    // Register Callback Function
    boost::signals2::connection connection = grabber->registerCallback( function );

    // Initialize transforms
    InitXForms();

    // Initialize Python script controllers
    Py_Initialize();
    PythonCodeController* pcc = new PythonCodeController();
    pcc->start();

    // Start Grabber
    grabber->start();

    // The loop where the magic happens
    while( !viewer->wasStopped() )
    {
        // Update Viewer
        viewer->spinOnce();

        boost::mutex::scoped_try_lock lock( mutex );
        if( lock.owns_lock())
        {
            if (cloud)
            {
                xformedCloud = cloud;

                handler->setInputCloud( xformedCloud );
                if( !viewer->updatePointCloud( xformedCloud, *handler, "cloud" ) )
                {
                    viewer->addPointCloud( xformedCloud, *handler, "cloud" );
                }

                pcc->spinOnCamera1();
                cv::Mat image = pcc->imageFromLastSpin();
                cv::Mat boxes = pcc->boxesFromLastSpin();

                const pcl::PointCloud<PointType> *raw_foo = xformedCloud.get();

                cv::Rect frame(0, 0, 1280, 720);

                pcl::PointCloud<PointType>* newCloud = new pcl::PointCloud<PointType>();
                
                cv::Mat points_projected = project(left_projection_matrix, frame, *raw_foo, newCloud);
                cv::threshold(points_projected, points_projected, 10, 255, 0);
                
                
                cv::Mat combined_rgb_laser;
                std::vector<cv::Mat> rgb_laser_channels;
                
                cv:cvtColor(image, image, CV_BGR2GRAY);

                rgb_laser_channels.push_back(points_projected);
                rgb_laser_channels.push_back(cv::Mat::zeros(points_projected.size(), CV_8UC1));
                rgb_laser_channels.push_back(image);

                cv::merge(rgb_laser_channels, combined_rgb_laser);

                cv::namedWindow("Display window 1", cv::WINDOW_AUTOSIZE);
                cv::imshow("Display window 1", combined_rgb_laser);
            }

            if (((cv::waitKey(1) & 0xFF) == 113))
            {
                cv::destroyAllWindows();
                break;
            }
        }
    }

    // Stop Python script controller
    pcc->terminate();
    Py_Finalize();

    // Stop Grabber
    grabber->stop();

    // Disconnect Callback Function
    if( connection.connected() ) {
        connection.disconnect();
    }

    return 0;
}
