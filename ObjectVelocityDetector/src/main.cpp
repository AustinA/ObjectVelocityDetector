#include "../include/main.h"


using namespace std;
using namespace cv;
using namespace Eigen;
using namespace pcl;


int main(int argc, char *argv[]) {

    // Initialize the Python Interpreter and add all of the local modules
    Py_Initialize();
    PyObject *sysPath = PySys_GetObject((char *) "path");

    PyList_Append(sysPath, PyUnicode_FromString((char *) "."));


    // Create the main module (main.py)
    PyObject *moduleString = PyUnicode_FromString((char *) "main");
    PyObject *mainModule = PyImport_Import(moduleString);


    if (mainModule != nullptr) {
        // Create an object that can invoke the start() function defined in main.py
        PyObject *startFunction = PyObject_GetAttrString(mainModule, (char *) "start");

        if (startFunction && PyCallable_Check(startFunction)) {
            // Create a default blank argument and invoke start() defined in main.py
            PyObject *startArg = PyTuple_New(0);
            PyObject_CallObject(startFunction, startArg);
            Py_DECREF(startArg);
            Py_DECREF(startFunction);

            // Create an object that can invoke execute_session("1") defined in main.py
            PyObject *executeSession1 = PyObject_GetAttrString(mainModule, (char *) "execute_session");
            PyObject *executeArg1 = PyTuple_Pack(1, PyUnicode_FromString((char *) "0"));

            // Create an object that can invoke execute_session("2") defined in main.py
            PyObject *executeSession2 = PyObject_GetAttrString(mainModule, (char *) "execute_session");
            PyObject *executeArg2 = PyTuple_Pack(1, PyUnicode_FromString((char *) "1"));

            // Create an object that can invoke window_terminated_requested() defined in main.py
            PyObject *exitWindowTest = PyObject_GetAttrString(mainModule, (char *) "window_terminated_requested");
            PyObject *exitWindowArg = PyTuple_New(0);
            
            PyObject *get_image = PyObject_GetAttrString(mainModule, (char *) "get_image");
            PyObject *get_boxes = PyObject_GetAttrString(mainModule, (char *) "get_boxes");
            PyObject *get_imageArgs = PyTuple_New(0);
            PyObject *get_boxesArgs = PyTuple_New(0);
            

            // The function result of window_terminated_requested
            PyObject *windowCheck;

            // Converts ndarray of numpy values to a cv::Mat
            NDArrayConverter cvt;
            cv::Mat matImage1;
            cv::Mat matImage2;
            
            PyObject* raw_image;
            PyObject* bounding_boxes;
            
            while (true) {
                if (executeSession1 && PyCallable_Check(executeSession1)
                    && executeSession2 && PyCallable_Check(executeSession2)) {

                    // Call execute_session on camera 1, and convert its result to a cv::Mat
                     PyObject_CallObject(executeSession1, executeArg1);
                     raw_image = PyObject_CallObject(get_image, get_imageArgs);
                     bounding_boxes = PyObject_CallObject(get_boxes, get_boxesArgs);
                
                    
                    if (raw_image != nullptr) {
                        matImage1 = cvt.toMat(raw_image);

                        // Display the image
                        cv::namedWindow("Display window 1", cv::WINDOW_AUTOSIZE);
                        cv::imshow("Display window 1", matImage1);
                    }
                    
                    if (bounding_boxes != nullptr)
                    {
                       // Do something with the bounding boxes
                    }


                    // Call execute_session on camera 2, and convert its result to a cv::Mat
                    PyObject_CallObject(executeSession2, executeArg2);
                    raw_image = PyObject_CallObject(get_image, get_imageArgs);
                     bounding_boxes = PyObject_CallObject(get_boxes, get_boxesArgs);
                    
                    
                    if (raw_image != nullptr) {
                        matImage2 = cvt.toMat(raw_image);
                        // Display the image
                        cv::namedWindow("Display window 2", cv::WINDOW_AUTOSIZE);
                        cv::imshow("Display window 2", matImage2);
                    }
                    
                    if (bounding_boxes != nullptr)
                    {
                        // Do something with the bounding boxes
                    }

                    windowCheck = PyObject_CallObject(exitWindowTest, exitWindowArg);

                    // Terminate the loop if q is pressed, but first clean and dereference
                    if (PyObject_IsTrue(windowCheck) || ((cv::waitKey(1) & 0xFF) == 113)) {
                        cv::destroyAllWindows();
                        Py_DECREF(exitWindowTest);
                        Py_DECREF(exitWindowArg);
                        Py_DECREF(executeSession1);
                        Py_DECREF(executeArg1);
                        Py_DECREF(executeSession2);
                        Py_DECREF(executeArg2);
                        Py_DECREF(windowCheck);
                        Py_DECREF(get_boxes);
                        Py_DECREF(get_boxesArgs);
                        Py_DECREF(get_image);
                        Py_DECREF(get_imageArgs);
                        break;
                    }
                }
            }

            // Invoke the stop() method defined in main.py
            PyObject *stop = PyObject_GetAttrString(mainModule, (char *) "window_terminated_requested");
            PyObject *stopArg = PyTuple_New(0);
            PyObject_CallObject(stop, stopArg);

            Py_DECREF(stop);
            Py_DECREF(stopArg);

        }
    } else {
        std::cout << "main.py could not be found. Be sure to run the program from the same directory as main.py"
                  << std::endl;
    }
    // Terminate the python runtime
    Py_Finalize();
    return 0;
}
