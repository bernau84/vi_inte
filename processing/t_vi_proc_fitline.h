#ifndef T_VI_PROC_FIT_LINE
#define T_VI_PROC_FIT_LINE

#include "i_proc_stage.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

static const QString proc_fitline_defconfigpath(":/js_config_fitline.txt");

class t_vi_proc_fitline : public i_proc_stage
{
private:
    Mat *src;
    Mat loc;
    Vec4f line;
    float error;
    cv::Rect roi;
    int weight;
    int dir;

public:
    t_vi_proc_fitline(const QString &path = proc_fitline_defconfigpath):
        i_proc_stage(path)
    {
        fancy_name = "fitline (" + fancy_name + ")";
        reload(0);
        qDebug() << "Filine setup (1-4 roi), weight, direction:" <<
                    roi.x << roi.y << roi.width << roi.height <<
                    weight << dir;
    }

    virtual ~t_vi_proc_fitline(){;}

private:

    //puvodne prevzate z roll_approx
    Vec4f linear_approx(float *err = NULL){

        vector<Point> locations; Vec4f line;   // output, locations of non-zero pixels; vx, vy, x0, y0

        switch(dir){
            default:
            case 0: //from left
                for(int y = roi.y; y < roi.y + roi.height; y++)
                    for(int x = roi.x; x < roi.x + roi.width; x++)
                        if(src->at<uchar>(Point(x, y))){

                            locations.push_back(Point(x, y));
                            break;
                        }
            break;
            case 1: //from top
                for(int x = roi.x; x < roi.x + roi.width; x++)
                    for(int y = roi.y; y < roi.y + roi.height; y++)
                        if(src->at<uchar>(Point(x, y))){

                            locations.push_back(Point(x, y));
                            break;
                        }
            break;
            case 2: //from right
                for(int y = roi.y + roi.height - 1; y >= roi.y; y--)
                    for(int x = roi.x; x < roi.x + roi.width; x++)
                        if(src->at<uchar>(Point(x, y))){

                            locations.push_back(Point(x, y));
                            break;
                        }
            break;
            case 3: //from botom
                for(int x = roi.x; x < roi.x + roi.width; x++)
                    for(int y = roi.y + roi.height - 1; y >= roi.y; y--)
                        if(src->at<uchar>(Point(x, y))){

                            locations.push_back(Point(x, y));
                            break;
                        }
            break;
        }

        for(unsigned i=0; i<locations.size(); i++){

            Point p(locations[i].x, locations[i].y);
            cv::line(loc, p, p, Scalar(64, 64, 64), 3, 8);
        }


        if(locations.size())
            cv::fitLine(locations, line, weight, 0, 0.01, 0.01);

        qDebug() << "line" <<
                    "vx" << QString::number(line[0]) <<
                    "vy" << QString::number(line[1]) <<
                    "x0" << QString::number(line[2]) <<
                    "y0" << QString::number(line[3]);

        if(err){

            float a = line[1];
            float b = -line[0];
            float c = line[0]*line[3] - line[1]*line[2]; //algebraicky tvar primky
            float d = sqrt(a*a + b*b);
            float cumsum = 0;

            for(unsigned i=0; i<locations.size(); i++){

                float dist = fabs(a*locations[i].x + b*locations[i].y + c) / d;
                cumsum += dist;
            }

            *err = cumsum / locations.size();
        }

        return line;
    }

public slots:

    int reload(int p){

        p = p;

        roi.x = roi.y = roi.width = roi.height = 0;

        if(par.ask("fitline-roi-center-x"))
            roi.x = par["fitline-roi-center-x"].get().toInt();

        if(par.ask("fitline-roi-center-y"))
            roi.y = par["fitline-roi-center-y"].get().toInt();

        if(par.ask("fitline-roi-center-width"))
            roi.width = par["fitline-roi-width"].get().toInt();

        if(par.ask("fitline-roi-center-height"))
            roi.height = par["fitline-roi-height"].get().toInt();

        weight = 0;
        if(par.ask("fitline-weight")) //L1, L2, nebo nejakou jinou vahu
            weight = par["fitline-weight"].get().toInt();

        dir = 0;
        if(par.ask("search-from"))  //"0-left, 1-top, 2-right, 3-bottom",
            dir = par["search-from"].get().toInt();

        return 1;
    }

private:
    int iproc(int p1, void *p2){

        p1 = p1;
        Mat *src = (Mat *)p2;

        loc = *src;

        line.zeros();
        error = 0;

        if(roi.width == 0) roi.width = src->cols;
        if(roi.height == 0) roi.height = src->rows;

        line = linear_approx(&error);

        //vizualizace
        Mat resized;
        resize(loc, resized, Size((loc.cols/2) & ~0x3, (loc.rows/2) & ~0x3));
        loc = resized;
        cv::namedWindow("Roll-approximation", CV_WINDOW_AUTOSIZE);
        cv::imshow("Roll-approximation", resized);
        cv::resizeWindow("Roll-approximation", resized.cols, resized.rows);

        elapsed = etimer.elapsed();
        emit next(1, src); //pousti beze zmeny dal
        return 1;
    }

};
#endif // T_VI_PROC_FIT_LINE


