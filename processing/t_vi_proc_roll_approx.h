#ifndef T_VI_PROC_ROLL_APPROX
#define T_VI_PROC_ROLL_APPROX

#include "i_proc_stage.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

static const QString proc_roll_approx_defconfigpath(":/js_config_cylinder_approx.txt");

class t_vi_proc_roll_approx : public i_proc_stage
{

public:
    //output result
    typedef struct {
        float length;       //delka
        float diameter;     //vyska/prumer valce
        float left_corr;    //leva korekce roi
        float right_corr;   //prava korekce roi
        float left_err;  //chyba mereni stredu leve strany
        float right_err;    //chyba mereni stredu prave strany
    } t_vi_proc_roll_ind_res;

    t_vi_proc_roll_ind_res midprof;   //mereni nejdelsi stredni cary
    t_vi_proc_roll_ind_res eliptic;   //mereni stredu aproximovanych eliptickych cel
    float eliptic_left_radius;  //doplnkova informace
    float eliptic_right_radius;

    Mat out;
    Mat loc;  //pro vizualizaci na cmarani

private:

    Vec4f linear_approx(int from, int to, float *err = NULL){

        vector<Point> locations; Vec4f line;   // output, locations of non-zero pixels; vx, vy, x0, y0

        for(int y = from; y < to; y++)
            for(int x = 0; x < out.cols/4; x++)
                if(out.at<uchar>(Point(x, y))){

                    locations.push_back(Point(x, y));
                    break;
                }

#ifdef QT_DEBUG
                        for(unsigned i=0; i<locations.size(); i++){

                            Point p(locations[i].x, locations[i].y);
                            cv::line(loc, p, p, Scalar(64, 64, 64), 3, 8);
                        }
#endif //xQT_DEBUG

        if(locations.size())
            cv::fitLine(locations, line, CV_DIST_L1, 0, 0.01, 0.01);

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

    Vec4f eliptic_approx(int from, int to, float *err = NULL){

        vector<Point> locations; Vec4f line;   // output, locations of non-zero pixels; vx, vy, x0, y0

        int lheight = to - from;

        for(int y = from; y < to; y++)
            for(int x = eliptic.diameter/4; x >= 0; x--)  //D/4 je omezeni na maximalni moznou delku kratsi poloosy elipsy
                if(out.at<uchar>(Point(x, y))){

                    locations.push_back(Point(x, y - from));  //from 0 to lheight
                    break;
                }
#ifdef xQT_DEBUG
                            for(unsigned i=0; i<locations.size(); i++){

                                Point p(locations[i].x, locations[i].y + from);
                                cv::line(loc, p, p, Scalar(192, 64, 64), 3, 8);
                            }
#endif //QT_DEBUG

        Mat trans = Mat::zeros(out.size(), CV_8UC1);
        vector<Point> locations_t;
        for(unsigned i=0; i<locations.size(); i++){  //little magic - transform eliptic(quadratic) eq to linear
                                                    //with knowledge of bigger radius (height) we get

            float y_t = (1.0 * (locations[i].y - lheight/2)) / (lheight/2); //now in range <-1, +1>
            y_t = lheight/2 * sqrt(1 - pow(y_t, 2));
            Point p(locations[i].x, y_t);  //in picture coords <0, lheight>
            locations_t.push_back(p);

#ifdef xQT_DEBUG
            cv::line(trans, p, p, Scalar(255, 255, 255), 2, 8);
#endif //QT_DEBUG
        }

        if(locations_t.size())
            cv::fitLine(locations_t, line, CV_DIST_L1, 0, 0.01, 0.01);

        qDebug() << "line" <<
                    "vx" << QString::number(line[0]) <<
                    "vy" << QString::number(line[1]) <<
                    "x0" << QString::number(line[2]) <<
                    "y0" << QString::number(line[3]);

#ifdef xQT_DEBUG
                cv::line(trans,
                         Point(line[2]-line[0]*200, line[3]-line[1]*200),
                         Point(line[2]+line[0]*200, line[3]+line[1]*200),
                         Scalar(128, 128 ,128),
                         3, CV_AA);

                cv::line(trans,
                         Point(line[2], from),
                         Point(line[2], to),
                         Scalar(64, 64 ,64),
                         1, CV_AA);

                imshow("Trans", trans);
#endif //QT_DEBUG

        if(err){

            //chyba jako cumsum rozilu radiusu
            double B = fabs(line[3] / (line[1] + 1e-6) * line[0]);  //druha poloosa elipsy
            double A = lheight / 2;  //prvni poloosa
            double X = line[2] - ((line[3] - A) / (line[1] + 1e-6) * line[0]);  //posun v xove ose
            double Y = lheight / 2;  //posun v yove ose

            float cumsum = 0;

            for(unsigned i=0; i<locations.size(); i++){

                float distx = fabs(locations[i].x - X - B);
                float disty = Y - locations[i].y;
                float r = sqrt(distx*distx + disty*disty);

                //! tady pozor - vychazi to z rovnosti ze
                //! B * cos(gama) / A * sin(gama) == distx / disty
                float gama = atan((disty * B) / (distx * A + 1e-6));
                float elipx = cos(gama) * B;
                float elipy = sin(gama) * A;
                float R = sqrt(elipx*elipx + elipy*elipy);

                cumsum += fabs(R - r);
            }

            *err = cumsum / locations.size();
        }
        return line;
    }


    Vec4f eliptic_approx_hough(int from, int to, float *err = NULL){

        vector<Point> locations; Vec4f line;   // output, locations of non-zero pixels; vx, vy, x0, y0
        err = err;

        int A = (to - from) / 2;  //hlavni poloosa
        int Z = (to + from) / 2;  //umela nula

        for(int y = from; y < to; y++)
            for(int x = eliptic.diameter/4; x >= 0; x--)  //D/4 je omezeni na maximalni moznou delku kratsi poloosy elipsy
                if(out.at<uchar>(Point(x, y))){

                    locations.push_back(Point(x, y));  //umela nula na stredu
                    break;
                }

#ifdef xQT_DEBUG
                            for(unsigned i=0; i<locations.size(); i++){

                                Point p(locations[i].x, locations[i].y);
                                cv::line(loc, p, p, Scalar(192, 64, 64), 3, 8);
                            }
#endif //QT_DEBUG

        Mat trans = Mat::zeros(out.size(), CV_8UC1);
        vector<Point> locations_t;
        for(unsigned i=0; i<locations.size(); i++){  //little magic - transform eliptic(quadratic) eq to linear
                                                    //with knowledge of bigger radius (height) we get

            float y_t = locations[i].y - Z; //now in range <-A, A>
            y_t = A * sqrt(1 - pow(y_t/A, 2)); //<0, A>
            Point p(locations[i].x, y_t);
            locations_t.push_back(p);

            cv::line(trans, p, p, Scalar(255, 255, 255), 2, 8);
        }

        if(locations_t.size()){

            cv::fitLine(locations_t, line, CV_DIST_L1, 0, 0.01, 0.01);

            vector<Vec4i> lines;
            HoughLinesP(trans, lines, 1, CV_PI/180, 50, 0, 0 );

            for( size_t i = 0; i < lines.size(); i++ )
            {
              Vec4i l = lines[i];
              cv::line(trans, Point(l[0] + (i+1)*50, l[1]), Point(l[2] + (i+1)*50, l[3]), Scalar(255,255,255), 1, CV_AA);
            }

            if(lines.size()){

                double x0 = lines[0][0], y0 = lines[0][1];
                line[0] = lines[0][2] - lines[0][0];
                line[1] = lines[0][3] - lines[0][1];
                line[2] = x0;
                line[3] = y0;
            }
        }

        qDebug() << "line" <<
                    "vx" << QString::number(line[0]) <<
                    "vy" << QString::number(line[1]) <<
                    "x0" << QString::number(line[2]) <<
                    "y0" << QString::number(line[3]);

#ifdef xQT_DEBUG
                cv::line(trans,
                         Point(line[2]-line[0]*200, line[3]-line[1]*200),
                         Point(line[2]+line[0]*200, line[3]+line[1]*200),
                         Scalar(128, 128 ,128),
                         3, CV_AA);

                cv::line(trans,
                         Point(line[2], from),
                         Point(line[2], to),
                         Scalar(64, 64 ,64),
                         1, CV_AA);

                imshow("Trans", trans);
#endif //QT_DEBUG

        return line;
    }

public:
    t_vi_proc_roll_approx(const QString &path = proc_roll_approx_defconfigpath):
        i_proc_stage(path)
    {
        fancy_name = "roll-approximation(" + fancy_name + ")";
        reload(0);
    }


    virtual ~t_vi_proc_roll_approx(){;}

public slots:
    int reload(int p){

        p = p;
        return 1;
    }

private:
    int iproc(int p1, void *p2){

        memset(&eliptic, 0, sizeof(t_vi_proc_roll_ind_res));
        memset(&midprof, 0, sizeof(t_vi_proc_roll_ind_res));

        eliptic_left_radius = -1;  //jako non-init
        eliptic_right_radius = -1;

        p1 = p1;
        cv::Mat *src = (Mat *)p2;
        cv::Mat tmp;

        ///horni strana
        cv::transpose(*src, out);
        cv::transpose(*src, loc);
        Vec4f line1 = linear_approx(out.cols/3, out.rows - out.cols/3);

                cv::line(loc,
                         Point(line1[2]-line1[0]*100, line1[3]-line1[1]*100),
                         Point(line1[2]+line1[0]*100, line1[3]+line1[1]*100),
                         Scalar(128, 128 ,128),
                         2, CV_AA);

        ///spodni strana
        tmp = out; cv::flip(tmp, out, 1);
        tmp = loc; cv::flip(tmp, loc, 1);
        Vec4f line2 = linear_approx(out.cols/3, out.rows - out.cols/3);

                cv::line(loc,
                         Point(line2[2]-line2[0]*100, line2[3]-line2[1]*100),
                         Point(line2[2]+line2[0]*100, line2[3]+line2[1]*100),
                         Scalar(128, 128 ,128),
                         2, CV_AA);

        eliptic.diameter = midprof.diameter = out.cols - line2[2] - line1[2];
        qDebug() << "Me-Diameter:" << QString::number(midprof.diameter);

        //najdem pruseciky horni a spodni aproximace s levym a pravym celem
        //to definuje mez pro hledani elipticke aproximace cela
        float left_s1 = line1[2] - (line1[0]/(line1[1] + 1e-6))*line1[3];
        float left_s2 = line2[2] - (line2[0]/(line2[1] + 1e-6))*line2[3];
        qDebug() << "Left-Corr:" << QString::number(left_s1) << QString::number(left_s2);

        float right_s1 = line1[2] + (line1[0]/(line1[1] + 1e-6))*(out.rows - line1[3]);
        float right_s2 = line2[2] + (line2[0]/(line2[1] + 1e-6))*(out.rows - line2[3]);
        qDebug() << "Right-Corr:" << QString::number(right_s1) << QString::number(right_s2);

        if(left_s1 < line1[2]) left_s1 = line1[2];  //nejdem pod zapor ani pod stredni hodnotu aproximace
        if(left_s2 < line2[2]) left_s2 = line2[2];
        if(right_s1 < line1[2]) right_s1 = line1[2];
        if(right_s2 < line2[2]) right_s2 = line2[2];

        ///leva
        tmp = out.t(); cv::flip(tmp, out, 0); //*src;  //original
        tmp = loc.t(); cv::flip(tmp, loc, 0); //*src;  //original

        //apriximace cela elipsou
        //Vec4f line3 = eliptic_approx_hough(left_s1, out.rows - left_s2);
        Vec4f line3 = eliptic_approx(left_s1, out.rows - left_s2, &eliptic.left_err);

        double laxis = fabs(line3[3] / (line3[1] + 1e-6) * line3[0]);  //druha poloosa elipsy
        double ldia = out.rows - left_s2 - left_s1;  //prvni poloosa
        double loffs = line3[2] - ((line3[3] - ldia/2) / (line3[1] + 1e-6) * line3[0]);  //posun v xove ose

        eliptic.left_corr = loffs + laxis;  //shift calc - ie. transform from parametric to y=f(x) eq.
        eliptic_left_radius = laxis;

        qDebug() << "Correction-Left-eliptic:" << QString::number(eliptic.left_corr) <<
                    ", radius" << QString::number(eliptic_left_radius);

        if(eliptic.left_corr <= 1e-6 && eliptic_left_radius <= 1e-6){
            //emit next(1, src);
            return 0;//koncime zadny emit - nepovedlo se; zbuchlo byt o pri pokusu vykreslit elipsu
        }

                cv::ellipse(loc, Point(eliptic.left_corr, line1[2] + eliptic.diameter/2), Size(abs(laxis), abs(ldia/2)),
                        0.0, 0.0, 360, Scalar(128, 128 ,128), 2, 4);

        //zmereni leveho stredu (nejdelsiho lineprofilu)
        float ylmid = (left_s1 + out.rows - left_s2) / 2;
        Vec4f line3_mid = linear_approx(ylmid - midprof.diameter/10, ylmid + midprof.diameter/10, &midprof.left_err);

        midprof.left_corr = line3_mid[2];// + (line3_mid[0]/(line3_mid[1] + 1e-6))*(ylmid - line3_mid[3]);
        qDebug() << "Correction-Left-midline:" << QString::number(midprof.left_corr);

                cv::line(loc,
                         Point(midprof.left_corr, ylmid - midprof.diameter/2),
                         Point(midprof.left_corr, ylmid + midprof.diameter/2),
                         Scalar(55, 55, 55),
                         3, CV_AA);
        ///prava
        tmp = out; cv::flip(tmp, out, 1);
        tmp = loc; cv::flip(tmp, loc, 1);

        //Vec4f line4 = eliptic_approx_hough(right_s1, out.rows - right_s2);
        Vec4f line4 = eliptic_approx(right_s1, out.rows - right_s2, &eliptic.right_err);

        double raxis = fabs((line4[3] / (line4[1] + 1e-6) * line4[0]));
        double rdia = out.rows - right_s2 - right_s1;
        double roffs = line4[2] - ((line4[3] - ldia/2) / (line4[1] + 1e-6) * line4[0]);

        eliptic.right_corr = roffs + raxis;
        eliptic_right_radius = raxis;
        qDebug() << "Correction-Right-eliptic:" << QString::number(eliptic.right_corr) <<
                    ", radius" << QString::number(eliptic_right_radius);

        if(eliptic.right_corr <= 1e-6 && eliptic_right_radius <= 1e-6){
            //emit next(1, src);            
            return 0;//koncime zadny emit - nepovedlo se; zbuchlo byt o pri pokusu vykreslit elipsu
        }

                cv::ellipse(loc, Point(eliptic.right_corr, line1[2] + eliptic.diameter/2), Size(abs(raxis), abs(rdia/2)),
                        0.0, 0.0, 360, Scalar(128, 128, 128), 2, 4);

        //zmereni praveho stredu (nejdelsiho lineprofilu)
        float yrmid = (right_s1 + out.rows - right_s2) / 2;
        Vec4f line4_mid = linear_approx(yrmid - midprof.diameter/10, yrmid + midprof.diameter/10, &midprof.right_err);

        midprof.right_corr = line4_mid[2]; // + (line4_mid[0]/(line4_mid[1] + 1e-6))*(yrmid - line4_mid[3]);
        qDebug() << "Correction-Right-midline:" << QString::number(midprof.right_corr);

                cv::line(loc,
                         Point(midprof.right_corr, yrmid - midprof.diameter/2),
                         Point(midprof.right_corr, yrmid + midprof.diameter/2),
                         Scalar(55, 55, 55),
                         3, CV_AA);

        midprof.length = out.cols - midprof.right_corr - midprof.left_corr;
        qDebug() << "Mid-profile Length:" << QString::number(midprof.length) <<
            QString("(+/-%1 on left, +/-%2 on right)").arg(midprof.left_err).arg(midprof.right_err);

        eliptic.length = out.cols - eliptic.left_corr - eliptic.right_corr;
        qDebug() << "Eliptic Length:" << QString::number(eliptic.length) <<
            QString("(+/-%1 on left, +/-%2 on right)").arg(eliptic.left_err).arg(eliptic.right_err);

        //vizualizace
        Mat resized;
        resize(loc, resized, Size((loc.cols/2) & ~0x3, (loc.rows/2) & ~0x3));
        loc = resized;

//        cv::namedWindow("Roll-approximation", CV_WINDOW_AUTOSIZE);
//        cv::imshow("Roll-approximation", resized);
//        cv::resizeWindow("Roll-approximation", resized.cols, resized.rows);

//        cv::namedWindow("Roll-original", CV_WINDOW_AUTOSIZE);
//        cv::imshow("Roll-original", out);

        elapsed = etimer.elapsed();
        emit next(1, src);
        return 1;
    }
};

#endif // T_VI_PROC_ROLL_APPROX

