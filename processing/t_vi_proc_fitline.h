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
public:
    Mat *src;
    Mat loc;
    Vec4f line;
    float error;
    int ofs[4];
    int weight;
    int dir;

public:
    t_vi_proc_fitline(const QString &path = proc_fitline_defconfigpath):
        i_proc_stage(path)
    {
        qDebug() << "t_vi_proc_fitline::t_vi_proc_fitline()";
        fancy_name = "fitline (" + fancy_name + ")";
        qDebug() << "t_vi_proc_fitline::t_vi_proc_fitline" << fancy_name;

        reload(0);
        qDebug() << "t_vi_proc_fitline::t_vi_proc_fitline setup (1-4 roi), weight, direction:" <<
                    ofs[0] << ofs[1] << ofs[2] << ofs[3] <<
                    weight << dir;
    }

    virtual ~t_vi_proc_fitline(){;}

private:

    //puvodne prevzate z roll_approx
    Vec4f linear_approx(float *err = NULL){

        qDebug() << "t_vi_proc_fitline::linear_approx()";

        vector<Point> locations; Vec4f line;   // output, locations of non-zero pixels; vx, vy, x0, y0

        int from_x = ofs[0];
        int to_x = src->cols - ofs[2];
        int from_y = ofs[1];
        int to_y = src->rows - ofs[3];

        switch(dir){
            default:
            case 0: //from left
                for(int y = from_y; y < to_y; y++)
                    for(int x = from_x; x < to_x; x++)
                        if(src->at<uchar>(Point(x, y))){

                            locations.push_back(Point(x, y));
                            break;
                        }
            break;
            case 1: //from top
                for(int x = from_x; x < to_x; x++)
                    for(int y = from_y; y < to_y; y++)
                        if(src->at<uchar>(Point(x, y))){

                            locations.push_back(Point(x, y));
                            break;
                        }
            break;
            case 2: //from right
                for(int y = from_y; y < to_y; y++)
                    for(int x = to_x - 1; x >= from_x; x--)
                        if(src->at<uchar>(Point(x, y))){

                            locations.push_back(Point(x, y));
                            break;
                        }
            break;
            case 3: //from botom
                for(int x = from_x; x < to_x; x++)
                    for(int y = to_y - 1; y >= from_y; y--)
                        if(src->at<uchar>(Point(x, y))){

                            locations.push_back(Point(x, y));
                            break;
                        }
            break;
        }

        qDebug() << "t_vi_proc_fitline::linear_approx location size" << locations.size();

        for(unsigned i=0; i<locations.size(); i++){

            Point p(locations[i].x, locations[i].y);
            cv::line(loc, p, p, Scalar(64, 64, 64), 3, 8);
        }


        if(locations.size()){

            qDebug() << "t_vi_proc_fitline::linear_approx before fitLine";
            cv::fitLine(locations, line, weight, 0, 0.01, 0.01);
        }

        qDebug() << "t_vi_proc_fitline::linear_approx line approx" <<
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
            qDebug() << "t_vi_proc_fitline::linear_approx error eval" << *err;
        }

        return line;
    }

public slots:

    int reload(int p){

        qDebug() << "t_vi_proc_fitline::reload()";
        p = p;

        ofs[0] = ofs[1] = ofs[2] = ofs[3] = 0;

        t_setup_entry tmp;
        if(par.ask("fitline-offs-left", &tmp))
            ofs[0] = tmp.get().toInt();

        if(par.ask("fitline-offs-top", &tmp))
            ofs[1] = tmp.get().toInt();

        if(par.ask("fitline-offs-right", &tmp))
            ofs[2] = tmp.get().toInt();

        if(par.ask("fitline-offs-bottom", &tmp))
            ofs[3] = tmp.get().toInt();

        weight = 1;
        if(par.ask("fitline-weight", &tmp)) //L1, L2, nebo nejakou jinou vahu
            weight = tmp.get().toInt();

        dir = 0;
        if(par.ask("search-from", &tmp)){  //"0-left, 1-top, 2-right, 3-bottom",

            //qDebug() << "search-from " << tmp.get();
            dir = tmp.get().toInt();
        }

        return 1;
    }

private:
    int iproc(int p1, void *p2){

        qDebug() << "t_vi_proc_fitline::iproc()";

        p1 = p1;
        src = (Mat *)p2;

        loc = *src;

        error = 0;
        line.zeros();
        line = linear_approx(&error);

        cv::line(loc,
                 Point(line[2]-line[0]*300, line[3]-line[1]*300),
                 Point(line[2]+line[0]*300, line[3]+line[1]*300),
                 Scalar(160, 160, 160),
                 5, CV_AA);

        //vizualizace
        Mat resized;
        resize(loc, resized, Size(loc.cols/2, loc.rows/2));
        qDebug() << "t_vi_proc_fitline::visual resize by 2";

        loc = resized;
//        cv::namedWindow("Fitted-Line", CV_WINDOW_AUTOSIZE);
//        cv::imshow("Fitted-Line", resized);
//        cv::resizeWindow("Fitted-Line", resized.cols, resized.rows);

        elapsed = etimer.elapsed();
        emit next(1, src); //pousti beze zmeny dal
        return 1;
    }

};
#endif // T_VI_PROC_FIT_LINE


