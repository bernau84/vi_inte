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
    RotatedRect selContRect;  //expotni aby sem si mohl zkontrolovat vysledky

private:
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    Mat out;
    Mat loc;

    enum {
       ALL,
       BIGGEST,
       DARKEST,
       BRIGHTEST
    } which_contours;

    int adaptive;
    int type;
    int thresh;
    int max_thresh;
    int min_contour_area;

    virtual double __count_light(Mat& img, Point2f *rect_points){

        double sum = 0;
        double total = 0;

        for(Point2f i=rect_points[0]; norm(i-rect_points[1])>1; i+=(rect_points[1]-i)/norm((rect_points[1]-i))){

            Point2f destination = i+rect_points[2]-rect_points[1];
            for(Point2f j=i; norm(j-destination)>1; j+=(destination-j)/norm((destination-j))){

                if(j.x > 0 && j.x < (img.cols-1))
                    if(j.y > 0 && j.y < (img.rows-1)){

                        sum += img.at<uchar>(j);
                        total += 1;
                    }
            }
        }

        return sum/total;
    }

    virtual void __preproc_source(){

        Mat pre;  //uzavreni kvuli omezeni poctu objektu
        Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));

        cv::morphologyEx(loc, pre, MORPH_CLOSE, kernel);
        loc = pre;

        cv::morphologyEx(loc, pre, MORPH_OPEN, kernel);
        loc = pre;

        qDebug() << "t_vi_proc_threshold::morphology-close";
    }

    virtual double __weight_contour(RotatedRect &crect){

        double weight = 1.0;

        //position weight
        if(0){ //todo - do konfigurace
            //k okrajum penalizujeme
            weight += 0.25 * fabs(crect.center.x/loc.cols - 0.5);
            weight -= 0.25 * fabs(crect.center.y/loc.rows - 0.5);
        }

        double w = crect.boundingRect().width;
        double h = crect.boundingRect().height;

        //orientation weight
        if(1){

//            if(fabs(crect.angle) > 45){
//                //pokud je to pres 45 tak prohodime strany
//                        //a otocime o 90st
//                if(crect.angle > 0) crect.angle -= 90;
//                else crect.angle += 90;

//                int tw = w, th = h;
//                h = tw; w = th;
//            }

            if(fabs(crect.angle) > 15)
                weight = 0;
        }

        //shape weight
        if(1){
            //vybirame jen ty kde je vyska vetsi nez sirka

            if(h > w) weight *= (h - w) / h;
            else weight = 0.0;
        }



        return weight;
    }

    virtual void __select_contours(Mat *src){

        int maxarea = 0, maxindex = 0;
        int lightarea = 0, lightindex = 0;
        int darkarea = 255, darkindex = 0;

        selContRect.size.width = selContRect.size.height = 0;

        ///++ Draw contours
        unsigned n_cont = contours.size();
        if(!n_cont)
            return;

        for(unsigned i = 0; i < n_cont; i++){

            int area = contourArea(contours[i]);
            if(area > min_contour_area){

                RotatedRect crect = minAreaRect(Mat(contours[i]));
                drawContours(loc, contours, i, Scalar(255, 0, 0), 2, 8, hierarchy, 0, Point());
                if(which_contours == ALL) drawContours(out, contours, i, Scalar(255, 0, 0), 2, 8, hierarchy, 0, Point());

                Point2f rect_points[4];
                crect.points(rect_points);

                for(int j = 0; j < 4; j++)
                   line(loc, rect_points[j], rect_points[(j+1)%4], Scalar(128, 0, 0), 1, 8);

                //vazeni?
                double weight = __weight_contour(crect);
                double light = __count_light(*src, rect_points);

                qDebug() << QString("t_vi_proc_threshold::iproc count#%1 an=%2,w=%3,h=%4,br=%5,wg=%6").
                            arg(i).
                            arg(crect.angle).
                            arg(crect.size.width).arg(crect.size.height).
                            arg(light).
                            arg(weight);

                //ruzne sortovani z ktereho si pak vyberem tu pravou konturu
                if(maxarea < area){

                    maxarea = area;
                    maxindex = i;
                }

                double wlight = light * weight;
                if(lightarea < wlight){

                    lightarea = wlight;
                    lightindex = i;
                }

                wlight = (light * (1 - weight));
                if(darkarea > wlight){

                    darkarea = wlight;
                    darkindex = i;
                }
            }
        }

        if(which_contours == ALL){

            //TODO - selContRect = region of all contours
        } else if(which_contours == BIGGEST) {

            selContRect = minAreaRect(Mat(contours[maxindex]));
            drawContours(out, contours, maxindex, Scalar(255, 255, 255), 1, 8, hierarchy, 0, Point());
            qDebug() << "t_vi_proc_threshold::iproc biggest contour drawn";
        } else if(which_contours == DARKEST){

            selContRect = minAreaRect(Mat(contours[darkindex]));
            drawContours(out, contours, darkindex, Scalar(255, 255, 255), 1, 8, hierarchy, 0, Point());
            qDebug() << "t_vi_proc_threshold::iproc darkest contours drawn";
        } else if(which_contours == BRIGHTEST){

            selContRect = minAreaRect(Mat(contours[lightindex]));
            drawContours(out, contours, lightindex, Scalar(255, 255, 255), 1, 8, hierarchy, 0, Point());
            qDebug() << "t_vi_proc_threshold::iproc brightest contours drawn";
        }

        Point2f rect_points[4];
        selContRect.points(rect_points);
        for(int j = 0; j < 4; j++)
           line(loc, rect_points[j], rect_points[(j+1)%4], Scalar(0, 0, 0), 3, 8);
    }

public:
    t_vi_proc_threshold(const QString &path = proc_threshold_defconfigpath):
        i_proc_stage(path)
    {
        qDebug() << "t_vi_proc_threshold::t_vi_proc_threshold()";
        fancy_name = "treshold-contours(" + fancy_name + ")";
        qDebug() << "t_vi_proc_threshold::t_vi_proc_threshold" << fancy_name;

        reload(0);

        qDebug() << "t_vi_proc_threshold::t_vi_proc_threshold setup-thresh" << thresh;
        qDebug() << "t_vi_proc_threshold::t_vi_proc_threshold setup-max_thresh" << max_thresh;
        qDebug() << "t_vi_proc_threshold::t_vi_proc_threshold setup-min_contour_area" <<  min_contour_area;
        qDebug() << "t_vi_proc_threshold::t_vi_proc_threshold setup-type" << type;
        qDebug() << "t_vi_proc_threshold::t_vi_proc_threshold setup-adaptive" << adaptive;
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


        t_setup_entry e;
        which_contours = ALL;
        if(par.ask("contour_sel", &e)){

            QString cont_str = e.get().toString();
            if(cont_str.contains("BIGGEST")) which_contours = BIGGEST;
            else if(cont_str.contains("DARKEST")) which_contours = DARKEST;
            else if(cont_str.contains("BRIGHTEST")) which_contours = BRIGHTEST;
        }

        return 1;
    }

private:
    int iproc(int p1, void *p2){

        qDebug() << "t_vi_proc_threshold::iproc()";

        p1 = p1;

        Mat *src = (Mat *)p2;

        selContRect = RotatedRect(Point2f(0, 0), Size2f(0, 0), 0.0);

        loc.release();
        out.release();

        qDebug() << "t_vi_proc_threshold::iproc released previous";

        /// Show in a window
        cv::imwrite("threshold-before.bmp", *src);
        qDebug() << "t_vi_proc_threshold::iproc imwrite threshold-before.bmp";

        /// Hard limit - convert to binary
        if(!adaptive){
            /// for otzu vraci skutecny prah
            double rthresh = cv::threshold(*src, loc, thresh, max_thresh, type);
            qDebug() << "t_vi_proc_threshold::iproc threshol given" << thresh << "and used" << rthresh;
        } else {
            cv::adaptiveThreshold(*src, loc, max_thresh, ADAPTIVE_THRESH_GAUSSIAN_C, type, 3, 5);
            qDebug() << "t_vi_proc_threshold::iproc adaptive-threshol";
        }

        //reduce noise, etc.
        __preproc_source();

//        Mat resized; resize(loc, resized, Size(), 0.5, 0.5);
//        cv::namedWindow("Threshold-preproc", CV_WINDOW_AUTOSIZE);
//        cv::imshow("Threshold-preproc", resized);
//        cv::resizeWindow("Threshold-preproc", resized.cols, resized.rows);

        /// Find contours
        findContours(loc, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
        qDebug() << "t_vi_proc_threshold::iproc contour number" << contours.size();

        //init
        loc = src->clone();
        out = Mat::zeros(src->size(), CV_8UC1);

        //select valid contours
        __select_contours(src);

        //nejvetsi objekt je nula - konec
        if(selContRect.center.x * selContRect.center.y == 0){

            qDebug() << "t_vi_proc_threshold::iproc error - zero maxContRect";
            return 0;  //zadny emit - koncime
        }

        /// Show in a window
        Mat resized2; resize(loc, resized2, Size(), 0.5, 0.5);
        cv::namedWindow("Contoures all", CV_WINDOW_AUTOSIZE);
        cv::imshow("Contoures all", resized2);
        cv::resizeWindow("Contoures all", resized2.cols, resized2.rows);

        if(1){  //uz nepokracujeme

            elapsed = etimer.elapsed();
            emit next(1, &out);
            return 1;
        }


        qDebug() << "t_vi_proc_threshold::iproc pre_rows" << QString::number(src->rows);
        qDebug() << "t_vi_proc_threshold::iproc pre_clms" << QString::number(src->cols);

        // matrices we'll use
        Mat M, rotated, cropped;
        Size rect_size = selContRect.size;

        // thanks to http://felix.abecassis.me/2011/10/opencv-rotation-deskewing/
        if (selContRect.angle < -45.) {

            selContRect.angle += 90.0;
            swap(rect_size.width, rect_size.height);
        }

        // get the rotation matrix
        M = cv::getRotationMatrix2D(selContRect.center,
                                    selContRect.angle, 1.0);
        qDebug() << "t_vi_proc_threshold::iproc getRotationMatrix";

        // perform the affine transformation
        cv::warpAffine(out, rotated, M, out.size(), INTER_NEAREST);
        qDebug() << "t_vi_proc_threshold::iproc warpAffine";

        // crop the resulting image
        cv::getRectSubPix(rotated, rect_size, selContRect.center, cropped);
        qDebug() << "t_vi_proc_threshold::iproc getRectSubPix";

        qDebug() << "t_vi_proc_threshold::iproc cropped_rows" << QString::number(cropped.rows);
        qDebug() << "t_vi_proc_threshold::iproc cropped_clms" << QString::number(cropped.cols);

        out = cropped.clone();

//        /// Show in a window
//        cv::namedWindow("Orto/Croped", CV_WINDOW_AUTOSIZE);
//        cv::imshow("Orto/Croped", out);
//        cv::resizeWindow("Orto/Croped", out.cols, out.rows);

        elapsed = etimer.elapsed();
        emit next(1, &out);
        return 1;
    }

};

#endif // T_VI_PROC_THRESHOLD_H
