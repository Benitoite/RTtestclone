/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modifyf
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 *  2016 Jacques Desmis <jdesmis@gmail.com>
 *  2016 Ingo Weyrich <heckflosse@i-weyrich.de>

 */
#include <cmath>
#include <glib.h>
#include <glibmm.h>

#include "rtengine.h"
#include "improcfun.h"
#include "rawimagesource.h"
#include "../rtgui/thresholdselector.h"
#include "curves.h"
#include "gauss.h"
#include "iccstore.h"
#include "iccmatrices.h"
#include "color.h"
#include "rt_math.h"
#include "jaggedarray.h"
#ifdef _DEBUG
#include "mytime.h"
#endif
#ifdef _OPENMP
#include <omp.h>
#endif

#include "cplx_wavelet_dec.h"

#define BENCHMARK
#include "StopWatch.h"

#define cliploc( val, minv, maxv )    (( val = (val < minv ? minv : val ) ) > maxv ? maxv : val )

#define CLIPC(a) ((a)>-42000?((a)<42000?(a):42000):-42000)  // limit a and b  to 130 probably enough ?
#define CLIPL(x) LIM(x,0.f,40000.f) // limit L to about L=120 probably enough ?
#define CLIPLOC(x) LIM(x,0.f,32767.f)
#define CLIPLIG(x) LIM(x,0.f, 99.5f)
#define CLIPCHRO(x) LIM(x,0.f, 140.f)
#define CLIPRET(x) LIM(x,-99.5f, 99.5f)

namespace
{

float calcLocalFactor(const float lox, const float loy, const float lcx, const float dx, const float lcy, const float dy, const float ach)
{
//elipse x2/a2 + y2/b2=1
//transition elipsoidal
//x==>lox y==>loy
// a==> dx  b==>dy

    float kelip = dx / dy;
    float belip = sqrt((rtengine::SQR((lox - lcx) / kelip) + rtengine::SQR(loy - lcy)));    //determine position ellipse ==> a and b
    float aelip = belip * kelip;
    float degrad = aelip / dx;
    float ap = rtengine::RT_PI / (1.f - ach);
    float bp = rtengine::RT_PI - ap;
    return 0.5f * (1.f + xcosf(degrad * ap + bp));  //trigo cos transition

}

float calcLocalFactorrect(const float lox, const float loy, const float lcx, const float dx, const float lcy, const float dy, const float ach)
{
    float eps = 0.0001f;
    float krap = fabs(dx / dy);
    float kx = (lox - lcx);
    float ky = (loy - lcy);
    float ref = 0.f;

    if (fabs(kx / (ky + eps)) < krap) {
        ref = sqrt(rtengine::SQR(dy) * (1.f + rtengine::SQR(kx / (ky + eps))));
    } else {
        ref = sqrt(rtengine::SQR(dx) * (1.f + rtengine::SQR(ky / (kx + eps))));
    }

    float rad = sqrt(rtengine::SQR(kx) + rtengine::SQR(ky));
    float coef = rad / ref;
    float ac = 1.f / (ach - 1.f);
    float fact = ac * (coef - 1.f);
    return fact;

}


}
using namespace std;

namespace rtengine
{
using namespace procparams;

#define SAT(a,b,c) ((float)max(a,b,c)-(float)min(a,b,c))/(float)max(a,b,c)


extern const Settings* settings;

struct local_params {
    float yc, xc;
    float lx, ly;
    float lxL, lyT;
    float dxx, dyy;
    float iterat;
    int cir;
    float thr;
    int prox;
    int chro, cont, sens, sensv;
    float ligh;
    float threshol;
    double temp;
    double  green;
    double equal;
    int trans;
    int shapmet;


};

void fillCurveArrayVibloc(DiagonalCurve* diagCurve, LUTf &outCurve)
{

    if (diagCurve) {
#ifdef _OPENMP
        #pragma omp parallel for
#endif

        for (int i = 0; i <= 0xffff; i++) {
            // change to [0,1] range
            // apply custom/parametric/NURBS curve, if any
            // and store result in a temporary array
            outCurve[i] = 65535.f * diagCurve->getVal(double (i) / 65535.0);
        }
    } else {
        outCurve.makeIdentity();
    }
}



static void calcLocalParams(int oW, int oH, const LocrgbParams& localwb, struct local_params& lp)
{
    int w = oW;
    int h = oH;
    int circr = localwb.circrad;

    float thre = localwb.thres / 100.f;
    double local_x = localwb.locX / 2000.0;
    double local_y = localwb.locY / 2000.0;
    double local_xL = localwb.locXL / 2000.0;
    double local_yT = localwb.locYT / 2000.0;
    double local_center_x = localwb.centerX / 2000.0 + 0.5;
    double local_center_y = localwb.centerY / 2000.0 + 0.5;
    double local_dxx = localwb.proxi / 8000.0;//for proxi = 2==> # 1 pixel
    double local_dyy = localwb.proxi / 8000.0;
    float iterati = (float) localwb.proxi;


    if (localwb.wbshaMethod == "eli") {
        lp.shapmet = 0;
    } else if (localwb.wbshaMethod == "rec") {
        lp.shapmet = 1;
    }

    int local_sensi = localwb.sensi;
    int local_transit = localwb.transit;

    lp.cir = circr;
    //  lp.actsp = acti;
    lp.xc = w * local_center_x;
    lp.yc = h * local_center_y;
    lp.lx = w * local_x;
    lp.ly = h * local_y;
    lp.lxL = w * local_xL;
    lp.lyT = h * local_yT;
    lp.sens = local_sensi;


    lp.trans = local_transit;
    lp.iterat = iterati;
    lp.dxx = w * local_dxx;
    lp.dyy = h * local_dyy;
    lp.thr = thre;

    lp.temp = localwb.temp;
    lp.green = localwb.green;
    lp.equal = localwb.equal;
}


static void calcTransitionrect(const float lox, const float loy, const float ach, const local_params& lp, int &zone, float &localFactor)
{
    zone = 0;

    if (lox >= lp.xc && lox < (lp.xc + lp.lx) && loy >= lp.yc && loy < lp.yc + lp.ly) {
        if (lox < (lp.xc + lp.lx * ach)  && loy < (lp.yc + lp.ly * ach)) {
            zone = 2;
        } else {
            zone = 1;
            localFactor = calcLocalFactorrect(lox, loy, lp.xc, lp.lx, lp.yc, lp.ly, ach);
        }

    } else if (lox >= lp.xc && lox < lp.xc + lp.lx && loy < lp.yc && loy > lp.yc - lp.lyT) {
        if (lox < (lp.xc + lp.lx * ach) && loy > (lp.yc - lp.lyT * ach)) {
            zone = 2;
        } else {
            zone = 1;
            localFactor = calcLocalFactorrect(lox, loy, lp.xc, lp.lx, lp.yc, lp.lyT, ach);
        }


    } else if (lox < lp.xc && lox > lp.xc - lp.lxL && loy <= lp.yc && loy > lp.yc - lp.lyT) {
        if (lox > (lp.xc - lp.lxL * ach) && loy > (lp.yc - lp.lyT * ach)) {
            zone = 2;
        } else {
            zone = 1;
            localFactor = calcLocalFactorrect(lox, loy, lp.xc, lp.lxL, lp.yc, lp.lyT, ach);
        }

    } else if (lox < lp.xc && lox > lp.xc - lp.lxL && loy > lp.yc && loy < lp.yc + lp.ly) {
        if (lox > (lp.xc - lp.lxL * ach) && loy < (lp.yc + lp.ly * ach)) {
            zone = 2;
        } else {
            zone = 1;
            localFactor = calcLocalFactorrect(lox, loy, lp.xc, lp.lxL, lp.yc, lp.ly, ach);
        }

    }

}




static void calcTransition(const float lox, const float loy, const float ach, const local_params& lp, int &zone, float &localFactor)
{
    // returns the zone (0 = outside selection, 1 = transition zone between outside and inside selection, 2 = inside selection)
    // and a factor to calculate the transition in case zone == 1

    zone = 0;

    if (lox >= lp.xc && lox < (lp.xc + lp.lx) && loy >= lp.yc && loy < lp.yc + lp.ly) {
        float zoneVal = SQR((lox - lp.xc) / (ach * lp.lx)) + SQR((loy - lp.yc) / (ach * lp.ly));
        zone = zoneVal < 1.f ? 2 : 0;

        if (!zone) {
            zone = (zoneVal > 1.f && ((SQR((lox - lp.xc) / (lp.lx)) + SQR((loy - lp.yc) / (lp.ly))) < 1.f)) ? 1 : 0;

            if (zone) {
                localFactor = calcLocalFactor(lox, loy, lp.xc, lp.lx, lp.yc, lp.ly, ach);
            }
        }
    } else if (lox >= lp.xc && lox < lp.xc + lp.lx && loy < lp.yc && loy > lp.yc - lp.lyT) {
        float zoneVal = SQR((lox - lp.xc) / (ach * lp.lx)) + SQR((loy - lp.yc) / (ach * lp.lyT));
        zone = zoneVal < 1.f ? 2 : 0;

        if (!zone) {
            zone = (zoneVal > 1.f && ((SQR((lox - lp.xc) / (lp.lx)) + SQR((loy - lp.yc) / (lp.lyT))) < 1.f)) ? 1 : 0;

            if (zone) {
                localFactor = calcLocalFactor(lox, loy, lp.xc, lp.lx, lp.yc, lp.lyT, ach);
            }
        }
    } else if (lox < lp.xc && lox > lp.xc - lp.lxL && loy <= lp.yc && loy > lp.yc - lp.lyT) {
        float zoneVal = SQR((lox - lp.xc) / (ach * lp.lxL)) + SQR((loy - lp.yc) / (ach * lp.lyT));
        zone = zoneVal < 1.f ? 2 : 0;

        if (!zone) {
            zone = (zoneVal > 1.f && ((SQR((lox - lp.xc) / (lp.lxL)) + SQR((loy - lp.yc) / (lp.lyT))) < 1.f)) ? 1 : 0;

            if (zone) {
                localFactor = calcLocalFactor(lox, loy, lp.xc, lp.lxL, lp.yc, lp.lyT, ach);
            }
        }
    } else if (lox < lp.xc && lox > lp.xc - lp.lxL && loy > lp.yc && loy < lp.yc + lp.ly) {
        float zoneVal = SQR((lox - lp.xc) / (ach * lp.lxL)) + SQR((loy - lp.yc) / (ach * lp.ly));
        zone = zoneVal < 1.f ? 2 : 0;

        if (!zone) {
            zone = (zoneVal > 1.f && ((SQR((lox - lp.xc) / (lp.lxL)) + SQR((loy - lp.yc) / (lp.ly))) < 1.f)) ? 1 : 0;

            if (zone) {
                localFactor = calcLocalFactor(lox, loy, lp.xc, lp.lxL, lp.yc, lp.ly, ach);
            }
        }
    }
}
void ImProcFunctions::calcrgb_ref(LabImage * original, LabImage * transformed, int sk, int sx, int sy, int cx, int cy, int oW, int oH,  int fw, int fh, double & hueref, double & chromaref, double & lumaref)
{
    if (params->localwb.enabled) {
        //always calculate hueref, chromaref, lumaref  before others operations use in normal mode for all modules exceprt denoise
        struct local_params lp;
        calcLocalParams(oW, oH, params->localwb, lp);
        //  printf("OK calcrgb\n");
        //int sk = 1;
// double precision for large summations
        double aveA = 0.;
        double aveB = 0.;
        double aveL = 0.;
        double aveChro = 0.;
// int precision for the counters
        int nab = 0;
// single precision for the result
        float avA, avB, avL;
        int spotSize = 0.88623f * max(1,  lp.cir / sk);  //18

        //O.88623 = sqrt(PI / 4) ==> sqare equal to circle
        //  printf("Spots=%i  xc=%i\n", spotSize, (int) lp.xc);
        // very small region, don't use omp here
        for (int y = max(cy, (int)(lp.yc - spotSize)); y < min(transformed->H + cy, (int)(lp.yc + spotSize + 1)); y++) {
            for (int x = max(cx, (int)(lp.xc - spotSize)); x < min(transformed->W + cx, (int)(lp.xc + spotSize + 1)); x++) {
                aveL += original->L[y - cy][x - cx];
                aveA += original->a[y - cy][x - cx];
                aveB += original->b[y - cy][x - cx];
                aveChro += sqrtf(SQR(original->b[y - cy][x - cx]) + SQR(original->a[y - cy][x - cx]));

                nab++;
            }
        }

        aveL = aveL / nab;
        aveA = aveA / nab;
        aveB = aveB / nab;
        aveChro = aveChro / nab;
        aveChro /= 327.68f;
        avA = aveA / 327.68f;
        avB = aveB / 327.68f;
        avL = aveL / 327.68f;
        hueref = xatan2f(avB, avA);    //mean hue
        chromaref = aveChro;
        lumaref = avL;
    }
}





void ImProcFunctions::WB_Local(ImageSource* imgsrc, int call, int sp, int sx, int sy, int cx, int cy, int oW, int oH,  int fw, int fh, Imagefloat* improv, Imagefloat* imagetransformed, const ColorTemp &ctemploc, int tran, Imagefloat* imageoriginal, const PreviewProps &pp, const ToneCurveParams &hrp, const ColorManagementParams &cmp, const RAWParams &raw, const LocrgbParams &wbl, const ColorAppearanceParams &cap, double &ptemp, double &pgreen)
{
    if (params->localwb.enabled) {
        // BENCHFUN
#ifdef _DEBUG
        MyTime t1e, t2e;
        t1e.set();
// init variables to display Munsell corrections
        //    MunsellDebugInfo* MunsDebugInfo = new MunsellDebugInfo();
#endif



        struct local_params lp;
        calcLocalParams(oW, oH, params->localwb, lp);


        Imagefloat* bufimage = nullptr;

        int bfh = 0.f, bfw = 0.f;
        int del = 3; // to avoid crash

        int begy = lp.yc - lp.lyT;
        int begx = lp.xc - lp.lxL;
        int yEn = lp.yc + lp.ly;
        int xEn = lp.xc + lp.lx;
        //  printf ("by=%i bx=%i y=%i x=%i\n", begx, begy, yEn, xEn);

        if (call <= 3) { //simpleprocess, dcrop, improccoordinator
            bfh = int (lp.ly + lp.lyT) + del; //bfw bfh real size of square zone
            bfw = int (lp.lx + lp.lxL) + del;

            bufimage = new Imagefloat(bfw, bfh);
#ifdef _OPENMP
            #pragma omp parallel for
#endif

            for (int ir = 0; ir < bfh; ir++) //fill with 0
                for (int jr = 0; jr < bfw; jr++) {
                    bufimage->r(ir, jr) = 0.f;
                    bufimage->g(ir, jr) = 0.f;
                    bufimage->b(ir, jr) = 0.f;
                }

            bool tjvr = true;

            if (tjvr) {

                imgsrc->getImage_local(begx, begy, yEn, xEn, cx, cy, ctemploc, tran, improv, bufimage, pp, hrp, cmp, raw, wbl, cap);

                Whitebalance_Local(call, sp, bufimage, lp, imageoriginal, imagetransformed, cx, cy);

            }

            delete bufimage;

        }
    }
}



void ImProcFunctions::Whitebalance_Local(int call, int sp, Imagefloat* bufimage, const struct local_params & lp, Imagefloat* imageoriginal, Imagefloat* imagetransformed, int cx, int cy)
{
    BENCHFUN

    const float ach = (float)lp.trans / 100.f;

#ifdef _OPENMP
    #pragma omp parallel if (multiThread)
#endif
    {
#ifdef __SSE2__
        //  float atan2Buffer[transformed->W] ALIGNED16;
        //  float sqrtBuffer[transformed->W] ALIGNED16;
        //  vfloat c327d68v = F2V (327.68f);
#endif

#ifdef _OPENMP
        #pragma omp for schedule(dynamic,16)
#endif

        for (int y = 0; y < imagetransformed->getHeight(); y++) {
            const int loy = cy + y;

            const bool isZone0 = loy > lp.yc + lp.ly || loy < lp.yc - lp.lyT; // whole line is zone 0 => we can skip a lot of processing

            if (isZone0) { // outside selection and outside transition zone => no effect, keep original values
                for (int x = 0; x < imagetransformed->getWidth(); x++) {
                    imagetransformed->r(y, x) = imageoriginal->r(y, x);
                    imagetransformed->g(y, x) = imageoriginal->g(y, x);
                    imagetransformed->b(y, x) = imageoriginal->b(y, x);

                }


                continue;
            }

#ifdef __SSE2__
            int i = 0;

            for (; i < imagetransformed->getWidth() - 3; i += 4) {
                //    vfloat av = LVFU (original->a[y][i]);
                //    vfloat bv = LVFU (original->b[y][i]);
                //    STVF (atan2Buffer[i], xatan2f (bv, av));
                //    STVF (sqrtBuffer[i], _mm_sqrt_ps (SQRV (bv) + SQRV (av)) / c327d68v);
            }

            for (; i < imagetransformed->getWidth(); i++) {
                //    atan2Buffer[i] = xatan2f (original->b[y][i], original->a[y][i]);
                //    sqrtBuffer[i] = sqrt (SQR (original->b[y][i]) + SQR (original->a[y][i])) / 327.68f;
            }

#endif



            for (int x = 0, lox = cx + x; x < imagetransformed->getWidth(); x++, lox++) {
                int zone = 0;
                float localFactor = 1.f;

                if (lp.shapmet == 0) {
                    calcTransition(lox, loy, ach, lp, zone, localFactor);
                } else if (lp.shapmet == 1) {
                    calcTransitionrect(lox, loy, ach, lp, zone, localFactor);
                }

                // calcTransition (lox, loy, ach, lp, zone, localFactor);

                if (zone == 0) { // outside selection and outside transition zone => no effect, keep original values
                    imagetransformed->r(y, x) = imageoriginal->r(y, x);
                    imagetransformed->g(y, x) = imageoriginal->g(y, x);
                    imagetransformed->b(y, x) = imageoriginal->b(y, x);

                    continue;
                }

                int begx = lp.xc - lp.lxL;
                int begy = lp.yc - lp.lyT;

                switch (zone) {

                    case 1: { // inside transition zone
                        float difr = 0.f, difg = 0.f, difb = 0.f;

                        if (call <= 3) {

                            difr = bufimage->r(loy - begy, lox - begx) - imageoriginal->r(y, x);
                            difg = bufimage->g(loy - begy, lox - begx) - imageoriginal->g(y, x);
                            difb = bufimage->b(loy - begy, lox - begx) - imageoriginal->b(y, x);
                            /*
                                                        difL = tmp1->L[loy - begy][lox - begx] - original->L[y][x];
                                                        difa = tmp1->a[loy - begy][lox - begx] - original->a[y][x];
                                                        difb = tmp1->b[loy - begy][lox - begx] - original->b[y][x];
                            */
                        }

                        difr *= localFactor;
                        difg *= localFactor;
                        difb *= localFactor;

                        imagetransformed->r(y, x) = imageoriginal->r(y, x) + difr;
                        imagetransformed->g(y, x) = imageoriginal->g(y, x) + difg;
                        imagetransformed->b(y, x) = imageoriginal->b(y, x) + difb;

                        // transformed->L[y][x] = original->L[y][x] + difL * kch * fach;


                        break;
                    }

                    case 2: { // inside selection => full effect, no transition
                        float difr = 0.f, difg = 0.f, difb = 0.f;

                        if (call <= 3) {
                            difr = bufimage->r(loy - begy, lox - begx) - imageoriginal->r(y, x);
                            difg = bufimage->g(loy - begy, lox - begx) - imageoriginal->g(y, x);
                            difb = bufimage->b(loy - begy, lox - begx) - imageoriginal->b(y, x);

                        }

                        imagetransformed->r(y, x) = imageoriginal->r(y, x) + difr;
                        imagetransformed->g(y, x) = imageoriginal->g(y, x) + difg;
                        imagetransformed->b(y, x) = imageoriginal->b(y, x) + difb;

                        //    transformed->L[y][x] = original->L[y][x] + difL * kch * fach;

                    }
                }
            }
        }
    }
}





}