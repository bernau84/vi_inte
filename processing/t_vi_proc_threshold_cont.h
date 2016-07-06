#ifndef T_VI_PROC_THRESHOLD_H
#define T_VI_PROC_THRESHOLD_H


#include "i_proc_stage.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

static const QString proc_threshold_defconfigpath(":/js_config_threshold_cont.txt");

class t_vi_proc_threshold : public i_proc_stage
{
public:
    RotatedRect maxContRect;  //expotni aby sem si mohl zkontrolovat vysledky

private:
    vector<vector<Point> > contours;
    Mat out;
    Mat loc;

    bool all_contours;
    int adaptive;
    int type;
    int thresh;
    int max_thresh;
    int min_contour_area;

public:
    t_vi_proc_threshold(const QString &path = proc_threshold_defconfigpath):
        i_proc_stage(path)
    {
        fancy_name = "treshold-contours(" + fancy_name + ")";
        reload(0);
        qDebug() << "Threshold & contours setup:" << thresh << max_thresh << min_contour_area << type << adaptive;
    }


    virtual ~t_vi_proc_threshold(){;}

public slots:

    int reload(int p){

        p = p;

        if(0 > (thresh = par["threshold_positive"].get().toInt()))
            thresh = 90;

        if(0 > (max_thresh = par["threshold_binaryval"].get().toInt()))
            max_thresh = 255;

        if(0 > (min_contour_area = par["contour_minimal"].get().toInt()))
            min_contour_area = 100;

        QString type_str = "BINARY";
        if(par.ask("threshold_method"))
            type_str = par["threshold_method"].get().toString();

        if(type_str.contains("BINARY")){
            type = THRESH_BINARY;
        } else if(type_str.contains("INV")){
            type = THRESH_BINARY_INV;
        }

        adaptive = false;
        if(type_str.contains("+OTZU")){
            type |= THRESH_OTSU;
        } else if(type_str.contains("+ADAPTIVE")){
            adaptive = true;
        }

        all_contours = false;
        if(par.ask("contour_sel"))
            all_contours = par["contour_sel"].get().toString().contains("ALL");

        return 1;
    }

private:
    int iproc(int p1, void *p2){

        p1 = p1;

        Mat *src = (Mat *)p2;

        maxContRect = RotatedRect(Point2f(0, 0), Size2f(0, 0), 0.0);

        /// Show in a window
        //cv::imwrite("threshold-before.bmp", *src);

        /// Hard limit - convert to binary
        if(!adaptive){
            /// for otzu vraci skutecny prah
            double rthresh = cv::threshold(*src, loc, thresh, max_thresh, type);
            qDebug() << "threshol given" << thresh << "and used" << rthresh;
        } else {
            cv::adaptiveThreshold(*src, loc, max_thresh, ADAPTIVE_THRESH_GAUSSIAN_C, type, 3, 5);
        }

//        /// Show in a window
//        cv::namedWindow("Threshold", CV_WINDOW_AUTOSIZE);
//        cv::imshow("Threshold", out);
//        cv::resizeWindow("Threshold", out.cols, out.rows);

        /// Find contours
        vector<Vec4i> hierarchy;
        findContours(loc, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

        loc = *src;

        /// Draw contours
        int maxarea = 0, maxindex = 0;    ///
        RotatedRect crect;
        out = Mat::zeros(src->size(), CV_8UC1);


        unsigned n_cont = contours.size();
        if(n_cont > 100){

            qDebug() << "contour number" << n_cont;
            //qDebug() << "limited to 100" << n_cont;
            //n_cont = 100;
        }

        for(unsigned i = 0; i < n_cont; i++){

            int area = contourArea(contours[i]);
            if(area > min_contour_area){

                crect = minAreaRect(Mat(contours[i]));
                drawContours(loc, contours, i, Scalar(255, 0, 0), 2, 8, hierarchy, 0, Point());
                if(all_contours) drawContours(out, contours, i, Scalar(255, 0, 0), 2, 8, hierarchy, 0, Point());

                Point2f rect_points[4]; crect.points(rect_points);
                for(int j = 0; j < 4; j++){

                   line(loc, rect_points[j], rect_points[(j+1)%4], Scalar(128, 0, 0), 1, 8);
                }

                if(maxarea < area){

                    maxarea = area; maxindex = i;
                    maxContRect = crect;
                }
            }
        }

        /// Show in a window
        Mat resized;
        resize(loc, resized, Size(), 0.5, 0.5);
        cv::namedWindow("Contoures all", CV_WINDOW_AUTOSIZE);
        cv::imshow("Contoures all", resized);
        cv::resizeWindow("Contoures all", resized.cols, resized.rows);

        if(all_contours){

            elapsed = etimer.elapsed();
            emit next(1, &out);
            return 1;
        } else {
            //BIGGEST is assumed
            drawContours(out, contours, maxindex, Scalar(255, 255, 255), 1, 8, hierarchy, 0, Point());
        }

        if(maxContRect.center.x * maxContRect.center.y == 0)
            return 0;  //zadny emit - koncime

        qDebug() << "pre_rows" << QString::number(src->rows);
        qDebug() << "pre_clms" << QString::number(src->cols);

        // matrices we'll use
        Mat M, rotated, cropped;
        Size rect_size = maxContRect.size;

        // thanks to http://felix.abecassis.me/2011/10/opencv-rotation-deskewing/
        if (maxContRect.angle < -45.) {

            maxContRect.angle += 90.0;
            swap(rect_size.width, rect_size.height);
        }

        // get the rotation matrix
        M = cv::getRotationMatrix2D(maxContRect.center,
                                    maxContRect.angle, 1.0);

        // perform the affine transformation
        cv::warpAffine(out, rotated, M, out.size(), INTER_NEAREST);

        // crop the resulting image
        cv::getRectSubPix(rotated, rect_size,
                          maxContRect.center, cropped);

        qDebug() << "cropped_rows" << QString::number(cropped.rows);
        qDebug() << "cropped_clms" << QString::number(cropped.cols);

        out = cropped.clone();

        /// Show in a window
        cv::namedWindow("Orto/Croped", CV_WINDOW_AUTOSIZE);
        cv::imshow("Orto/Croped", out);
        cv::resizeWindow("Orto/Croped", out.cols, out.rows);

        elapsed = etimer.elapsed();
        emit next(1, &out);
        return 1;
    }

};

#endif // T_VI_PROC_THRESHOLD_H
