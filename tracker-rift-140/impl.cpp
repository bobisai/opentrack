/* Copyright: "i couldn't care less what anyone does with the 5 lines of code i wrote" - mm0zct */
#include "rift-140.hpp"
#include "api/plugin-api.hpp"
#include "compat/util.hpp"
#include <Extras/OVR_Math.h>

#include <QString>

using namespace OVR;

Rift_Tracker::Rift_Tracker() : old_yaw(0), hmd(nullptr)
{
}

Rift_Tracker::~Rift_Tracker()
{
    if (hmd)
    {
        ovr_Destroy(hmd);
        ovr_Shutdown();
    }
}

void Rift_Tracker::start_tracker(QFrame*)
{
    if (OVR_FAILURE(ovr_Initialize(nullptr)))
        goto error;

    if(OVR_FAILURE(ovr_Create(&hmd, &luid)))
        goto error;

    return;
error:
    hmd = nullptr;

    ovrErrorInfo err;
    ovr_GetLastErrorInfo(&err);

    QString strerror(err.ErrorString);
    if (strerror.size() == 0)
        strerror = QStringLiteral("Unknown reason #%1").arg(err.Result);

    ovr_Shutdown();

    QMessageBox::warning(nullptr,
                         "Error",
                         QStringLiteral("Unable to start Rift tracker: %1").arg(strerror),
                         QMessageBox::Ok,
                         QMessageBox::NoButton);
}

void Rift_Tracker::data(double *data)
{
    if (hmd)
    {
        ovrTrackingState ss = ovr_GetTrackingState(hmd, 0, false);
        if (ss.StatusFlags & ovrStatus_OrientationTracked)
        {
            static constexpr float c_mult = 8;
            static constexpr float c_div = 1/c_mult;

            Vector3f axis;
            float angle;

            const Posef pose(ss.HeadPose.ThePose);
            pose.Rotation.GetAxisAngle(&axis, &angle);
            angle *= c_div;

            float yaw, pitch, roll;
            Quatf(axis, angle).GetYawPitchRoll(&yaw, &pitch, &roll);

            yaw *= c_mult;
            pitch *= c_mult;
            roll *= c_mult;

            double yaw_ = double(yaw);
            if (s.useYawSpring)
            {
                yaw_ = old_yaw*s.persistence + (yaw_-old_yaw);
                if(yaw_ > s.deadzone)
                    yaw_ -= s.constant_drift;
                if(yaw_ < -s.deadzone)
                    yaw_ += s.constant_drift;
                old_yaw = yaw_;
            }
            static constexpr double d2r = 180 / M_PI;
            data[Yaw] = yaw_                   * -d2r;
            data[Pitch] = double(pitch)        *  d2r;
            data[Roll] = double(roll)          *  d2r;
            data[TX] = double(pose.Translation.x) * -1e2;
            data[TY] = double(pose.Translation.y) *  1e2;
            data[TZ] = double(pose.Translation.z) *  1e2;
        }
    }
}

OPENTRACK_DECLARE_TRACKER(Rift_Tracker, TrackerControls, FTNoIR_TrackerDll)
