#if (!defined T_VI_CAMERA_BASLER_USB_H && defined USE_USB)
#define T_VI_CAMERA_BASLER_USB_H

#include "../i_camera_base.h"

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

// Namespace for using pylon objects.
using namespace Pylon;

#if defined( USE_1394 )
// Setting for using  Basler IEEE 1394 cameras.
#include <pylon/1394/Basler1394InstantCamera.h>
typedef Pylon::CBasler1394InstantCamera Camera_t;
using namespace Basler_IIDC1394CameraParams;
#elif defined ( USE_GIGE )
// Setting for using Basler GigE cameras.
#include <pylon/gige/BaslerGigEInstantCamera.h>
typedef Pylon::CBaslerGigEInstantCamera Camera_t;
using namespace Basler_GigECameraParams;
#elif defined ( USE_CAMERALINK )
// Setting for using Basler Camera Link cameras.
#include <pylon/cameralink/BaslerCameraLinkInstantCamera.h>
typedef Pylon::CBaslerCameraLinkInstantCamera Camera_t;
using namespace Basler_CLCameraParams;
#elif defined ( USE_USB )
// Setting for using Basler USB cameras.
#include <pylon/usb/BaslerUsbInstantCamera.h>
using namespace Basler_UsbCameraParams;
#else
#error Camera type is not specified. For example, define USE_GIGE for using GigE cameras.
#endif

#include <QElapsedTimer>

class t_vi_camera_basler_usb : public i_vi_camera_base
{
private:
    // Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
    // is initialized during the lifetime of this object.
    Pylon::PylonAutoInitTerm autoInitTerm;
    CBaslerUsbInstantCamera camera;

public:

    int init(){

        DeviceInfoList_t list;
        CTlFactory::GetInstance().EnumerateDevices(list);

        if(list.size() == 0){

            sta = CAMSTA_ERROR;
            return -1;
        }

        camera.Attach(CTlFactory::GetInstance().CreateFirstDevice());

        if(!camera.IsPylonDeviceAttached()){

            sta = CAMSTA_ERROR;
            return -2;
        }

        camera.Open();

        QString mode = par["General"].get().toString();
        if(mode.compare("MANUAL")){

            /*! \todo imprint setup to camera
                par["Width"]
                par["Height"]
                par["Gain"]
                par["Exposition"]
            */
        }

        sta = CAMSTA_PREPARED;
        return 0;
    }

    /*! \brief nastavuje a cte expozisni cas
     * \param[in] time - zalezi na parametru act
     * \param[in] act
     *  CAMVAL_ABS - nastavi absolutni hodnotu expozie na time
     *  CAMVAL_TIMEOUT - nastavi automatiku a cekame time na ustaleni; pokud time == 0 necha ji zapnutou na porad
     *  CAMVAL_TOLERANCE - nastavi automatiku a cekame na ustaleni v toleranci -/+ time
     *  CAMVAL_UNDEF - nenastavi nic, jen vycte
     * */


    /*! \todo - merime ne podle celkove expozicni doby ale podle jasu na ROI
     */
    int64_t exposure(int64_t time, e_camvalue act = CAMVAL_UNDEF){

        if(act == CAMVAL_ABS){

            /*! fixed exposition time */
            /*! \todo use setup property */
            camera.ExposureTime.SetValue(time);
        } else if(act == CAMVAL_AUTO_TIMEOUT){

            camera.ExposureAuto.SetValue(ExposureAuto_Continuous);

            QElapsedTimer etimer;
            int prev_exp = 1, exp = 0;
            for (etimer.start(); etimer.elapsed() < time; ) {

                if(prev_exp != exp){

                    CBaslerUsbInstantCamera::GrabResultPtr_t ptrGrabResultA;
                    camera.GrabOne(5000, ptrGrabResultA);
#if defined PYLON_WIN_BUILD && defined xQT_DEBUG
                    Pylon::DisplayImage(1, ptrGrabResultA);
#endif //QT_DEBUG
                    prev_exp = exp;
                    exp = camera.ExposureTime.GetValue();
                } else {

                    if(time) camera.ExposureAuto.SetValue(ExposureAuto_Off);
                    break;
                }
            }

        } else if(act == CAMVAL_AUTO_TOLERANCE) {

            camera.ExposureAuto.SetValue(ExposureAuto_Continuous);

            // When the "once" mode of operation is selected,
            // the parameter values are automatically adjusted until the related image property
            // reaches the target value. After the automatic parameter value adjustment is complete, the auto
            // function will automatically be set to "off", and the new parameter value will be applied to the
            // subsequently grabbed images.
            int n = 0, prev_exp = 2*time, exp = 0;
            while (abs(exp - prev_exp) > time)
            {
                CBaslerUsbInstantCamera::GrabResultPtr_t ptrGrabResultA;
                camera.GrabOne( 5000, ptrGrabResultA);
#if defined PYLON_WIN_BUILD && defined xQT_DEBUG
                    Pylon::DisplayImage(1, ptrGrabResultA);
#endif //QT_DEBUG
                ++n;

                prev_exp = exp;
                exp = camera.ExposureTime.GetValue();

                //For demonstration purposes only. Wait until the image is shown.
                ::Sleep(100);

                qDebug() << "ExposureAuto " << n << " frames.";
                qDebug() << "Final exposure time = ";
                qDebug() << exp << " us";

                //Make sure the loop is exited.
                if (n > 100)
                {
                    throw RUNTIME_EXCEPTION( "The adjustment of auto exposure did not finish.");
                    break;
                }
            }

            camera.ExposureAuto.SetValue(ExposureAuto_Off);
            qDebug() << "ExposureAuto done!";
        }

        return camera.ExposureTime.GetValue();
    }

    int isnap(void *img, unsigned free, t_campic_info *info = NULL){

        if(sta != CAMSTA_PREPARED)
            return -1;

        try
        {

            //Loading user set 1 settings - already dony in BaslerViewer
            //camera.UserSetSelector.SetValue(UserSetSelector_UserSet1);
            //camera.UserSetLoad.Execute();

            // This smart pointer will receive the grab result data.
            CGrabResultPtr ptrGrabResult;
            camera.StartGrabbing(1);
            while ( camera.IsGrabbing())
            {
                // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
                camera.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException);

                // Image grabbed successfully?
                if (ptrGrabResult->GrabSucceeded())
                {
                    // Access the image data.
                    cout << "basler-pic-x: " << ptrGrabResult->GetWidth() << endl;
                    cout << "basler-pic-y: " << ptrGrabResult->GetHeight() << endl;
                    const uint8_t *pImageBuffer = (uint8_t *) ptrGrabResult->GetBuffer();

#if defined PYLON_WIN_BUILD && defined xQT_DEBUG
                    // Display the grabbed image.
                    Pylon::DisplayImage(1, ptrGrabResult);
#endif
                    t_campic src, out;

                    out.p = img;
                    out.size = free;

                    src.p = (void *)pImageBuffer;
                    src.size = ptrGrabResult->GetImageSize();
                    src.t.w = ptrGrabResult->GetWidth();
                    src.t.h = ptrGrabResult->GetHeight();

                    switch(ptrGrabResult->GetPixelType()){

                        case PixelType_Mono8:
                            src.t.format = CAMF_8bMONO;
                        break;
                        default:   //dodelat podporu rgb pokud bude potreba, bayerovych masek respektive
                            return -101;
                        break;
                    }

                    //vystup muze byt pozadvan v jime formatu - dano konfiguraci
                    int ret = convertf(src, out);
                    if(info) *info = out.t;
                    return ret;
                }
                else
                {
                    cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
                    return 0;
                }
            }
        }
        catch (GenICam::GenericException &e)
        {
            // Error handling.
            cerr << "An exception occurred." << endl
                    << e.GetDescription() << endl;
            return -102;
        }

        return 0;
    }

    t_vi_camera_basler_usb(const QString &path):
        i_vi_camera_base(path),
        camera()
    {

    }

    t_vi_camera_basler_usb():
        camera()
    {

    }

    virtual ~t_vi_camera_basler_usb(){;}
};

#endif // T_VI_CAMERA_BASLER_USB_H
