/*  -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
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
 */

#include "cat02adaptation.h"
#include "iccstore.h"
#include "color.h"
#include "ciecam02.h"
#include "improcfun.h"
#include "StopWatch.h"

namespace rtengine
{

extern const Settings* settings;

namespace
{

void ciecamcat02loc_float(LabImage* lab, LabImage* dest, int tempa, double gree, int cat_02, const ColorManagementParams &cmp, const ColorAppearanceParams &cap)
{
    BENCHFUN
#ifdef _DEBUG
    MyTime t1e, t2e;
    t1e.set();
#endif
    printf("Call ciecamcat02\n");
    int width = lab->W, height = lab->H;
    float Yw;
    Yw = 1.0f;
    double Xw, Zw;
    float f = 0.f, nc = 0.f, la, c = 0.f, xw, yw, zw, f2 = 1.f, c2 = 1.f, nc2 = 1.f, yb2;
    float fl, n, nbb, ncb, aw; //d
    float xwd, ywd, zwd, xws, yws, zws;
    //  int alg = 0;
    double Xwout, Zwout;
    double Xwsc, Zwsc;

    int tempo;
    tempo = tempa;
    ColorTemp::temp2mulxyz(tempa, "Custom", Xw, Zw); //compute white Xw Yw Zw  : white current WB
    ColorTemp::temp2mulxyz(tempo, "Custom", Xwout, Zwout);
    ColorTemp::temp2mulxyz(5000, "Custom", Xwsc, Zwsc);

    //viewing condition for surrsrc
    f  = 1.00f;
    c  = 0.69f;
    nc = 1.00f;
    //viewing condition for surround
    f2 = 1.0f, c2 = 0.69f, nc2 = 1.0f;
    //with which algorithm
    //  alg = 0;


    xwd = 100.f * Xwout;
    zwd = 100.f * Zwout;
    ywd = 100.f / (float) gree;

    xws = 100.f * Xwsc;
    zws = 100.f * Zwsc;
    yws = 100.f;


    yb2 = 18;
    //La and la2 = ambiant luminosity scene and viewing
    la = 400.f;
    const float la2 = 400.f;
    float pilotout;
    const float pilot = (float)(cat_02) / 100.0f;
    pilotout = pilot;
    //  printf("pilot=%f temp=%i\n", pilot, tempo);
    LUTu hist16J;
    LUTu hist16Q;
    float yb = 18.f;
    float d, dj;

    const int gamu = 0; //(params->colorappearance.gamut) ? 1 : 0;
    xw = 100.0f * Xw;
    yw = 100.0f * Yw;
    zw = 100.0f * Zw;
    float xw1 = 0.f, yw1 = 0.f, zw1 = 0.f, xw2 = 0.f, yw2 = 0.f, zw2 = 0.f;
// free temp and green
    xw1 = xws;
    yw1 = yws;
    zw1 = zws;
    xw2 = xwd;
    yw2 = ywd;
    zw2 = zwd;

    float cz, wh, pfl;
    Ciecam02::initcam1float(gamu, yb, pilot, f, la, xw, yw, zw, n, d, nbb, ncb, cz, aw, wh, pfl, fl, c);
//   const float chr = 0.f;
    const float pow1 = pow_F(1.64f - pow_F(0.29f, n), 0.73f);
    float nj, nbbj, ncbj, czj, awj, flj;
    Ciecam02::initcam2float(gamu, yb2, pilotout, f2,  la2,  xw2,  yw2,  zw2, nj, dj, nbbj, ncbj, czj, awj, flj);
    const float reccmcz = 1.f / (c2 * czj);
    const float pow1n = pow_F(1.64f - pow_F(0.29f, nj), 0.73f);
//    const float QproFactor = (0.4f / c) * (aw + 4.0f) ;
    const bool LabPassOne = true;

#ifdef __SSE2__
    int bufferLength = ((width + 3) / 4) * 4; // bufferLength has to be a multiple of 4
#endif
#ifndef _DEBUG
    #pragma omp parallel
#endif
    {
#ifdef __SSE2__
        // one line buffer per channel and thread
        //we can suppress thes buffer if no threatment between => and <=
        float Jbuffer[bufferLength] ALIGNED16;
        float Cbuffer[bufferLength] ALIGNED16;
        float hbuffer[bufferLength] ALIGNED16;
        float Qbuffer[bufferLength] ALIGNED16;
        float Mbuffer[bufferLength] ALIGNED16;
        float sbuffer[bufferLength] ALIGNED16;
#endif
#ifndef _DEBUG
        #pragma omp for schedule(dynamic, 16)
#endif

        for (int i = 0; i < height; i++) {
#ifdef __SSE2__
            // vectorized conversion from Lab to jchqms
            int k;
            vfloat x, y, z;
            vfloat J, C, h, Q, M, s;

            vfloat c655d35 = F2V(655.35f);

            for (k = 0; k < width - 3; k += 4) {
                Color::Lab2XYZ(LVFU(lab->L[i][k]), LVFU(lab->a[i][k]), LVFU(lab->b[i][k]), x, y, z);
                x = x / c655d35;
                y = y / c655d35;
                z = z / c655d35;
                Ciecam02::xyz2jchqms_ciecam02float(J, C,  h,
                                                   Q,  M,  s, F2V(aw), F2V(fl), F2V(wh),
                                                   x,  y,  z,
                                                   F2V(xw1), F2V(yw1),  F2V(zw1),
                                                   F2V(c),  F2V(nc), F2V(pow1), F2V(nbb), F2V(ncb), F2V(pfl), F2V(cz), F2V(d));
                STVF(Jbuffer[k], J);
                STVF(Cbuffer[k], C);
                STVF(hbuffer[k], h);
                STVF(Qbuffer[k], Q);
                STVF(Mbuffer[k], M);
                STVF(sbuffer[k], s);
            }

            for (; k < width; k++) {
                float L = lab->L[i][k];
                float a = lab->a[i][k];
                float b = lab->b[i][k];
                float x, y, z;
                //convert Lab => XYZ
                Color::Lab2XYZ(L, a, b, x, y, z);
                x = x / 655.35f;
                y = y / 655.35f;
                z = z / 655.35f;
                float J, C, h, Q, M, s;
                Ciecam02::xyz2jchqms_ciecam02float(J, C,  h,
                                                   Q,  M,  s, aw, fl, wh,
                                                   x,  y,  z,
                                                   xw1, yw1,  zw1,
                                                   c,  nc, gamu, pow1, nbb, ncb, pfl, cz, d);
                Jbuffer[k] = J;
                Cbuffer[k] = C;
                hbuffer[k] = h;
                Qbuffer[k] = Q;
                Mbuffer[k] = M;
                sbuffer[k] = s;
            }

#endif // __SSE2__

            for (int j = 0; j < width; j++) {
                float J, C, h, Q, M, s;

#ifdef __SSE2__
                // use precomputed values from above
                J = Jbuffer[j];
                C = Cbuffer[j];
                h = hbuffer[j];
                Q = Qbuffer[j];
                M = Mbuffer[j];
                s = sbuffer[j];
#else
                float x, y, z;
                float L = lab->L[i][j];
                float a = lab->a[i][j];
                float b = lab->b[i][j];
                float x1, y1, z1;
                //convert Lab => XYZ
                Color::Lab2XYZ(L, a, b, x1, y1, z1);
                x = (float)x1 / 655.35f;
                y = (float)y1 / 655.35f;
                z = (float)z1 / 655.35f;
                //process source==> normal
                Ciecam02::xyz2jchqms_ciecam02float(J, C,  h,
                                                   Q,  M,  s, aw, fl, wh,
                                                   x,  y,  z,
                                                   xw1, yw1,  zw1,
                                                   c,  nc, gamu, pow1, nbb, ncb, pfl, cz, d);
#endif
                float Jpro, Cpro, hpro, Qpro, Mpro, spro;
                Jpro = J;
                Cpro = C;
                hpro = h;
                Qpro = Q;
                Mpro = M;
                spro = s;
                /*
                //we can here make some adjustements if necessary
                //On J or C

                */


                //retrieve values C,J...s
                C = Cpro;
                J = Jpro;
                Q = Qpro;
                M = Mpro;
                h = hpro;
                s = spro;

                if (LabPassOne) {//always true, but I keep
#ifdef __SSE2__
                    // write to line buffers
                    Jbuffer[j] = J;
                    Cbuffer[j] = C;
                    hbuffer[j] = h;
#else
                    float xx, yy, zz;
                    //process normal==> viewing

                    Ciecam02::jch2xyz_ciecam02float(xx, yy, zz,
                                                    J,  C, h,
                                                    xw2, yw2,  zw2,
                                                    f2,  c2, nc2, gamu, pow1n, nbbj, ncbj, flj, czj, dj, awj);
                    float x, y, z;
                    x = xx * 655.35f;
                    y = yy * 655.35f;
                    z = zz * 655.35f;
                    float Ll, aa, bb;
                    //convert xyz=>lab
                    Color::XYZ2Lab(x,  y,  z, Ll, aa, bb);
                    dest->L[i][j] = Ll;
                    dest->a[i][j] = aa;
                    dest->b[i][j] = bb;

#endif
                }

                //    }
            }

#ifdef __SSE2__
            // process line buffers
            float *xbuffer = Qbuffer;
            float *ybuffer = Mbuffer;
            float *zbuffer = sbuffer;

            for (k = 0; k < bufferLength; k += 4) {
                Ciecam02::jch2xyz_ciecam02float(x, y, z,
                                                LVF(Jbuffer[k]), LVF(Cbuffer[k]), LVF(hbuffer[k]),
                                                F2V(xw2), F2V(yw2), F2V(zw2),
                                                F2V(nc2), F2V(pow1n), F2V(nbbj), F2V(ncbj), F2V(flj), F2V(dj), F2V(awj), F2V(reccmcz));
                STVF(xbuffer[k], x * c655d35);
                STVF(ybuffer[k], y * c655d35);
                STVF(zbuffer[k], z * c655d35);
            }

            // XYZ2Lab uses a lookup table. The function behind that lut is a cube root.
            // SSE can't beat the speed of that lut, so it doesn't make sense to use SSE
            for (int j = 0; j < width; j++) {
                float Ll, aa, bb;
                //convert xyz=>lab
                Color::XYZ2Lab(xbuffer[j], ybuffer[j], zbuffer[j], Ll, aa, bb);

                dest->L[i][j] = Ll;
                dest->a[i][j] = aa;
                dest->b[i][j] = bb;
            }

#endif
        }

    }
#ifdef _DEBUG

    if (settings->verbose) {
        t2e.set();
        printf("CAT02 performed in %d usec:\n", t2e.etime(t1e));
    }

#endif
}

} // namespace


void cat02adaptationAutoComputeloc(ImageSource *imgsrc, ProcParams &params)
{
    if (!params.wb.enabled) {
        params.localwb.enabled = false;
    }

    if (!params.localwb.enabled) {
        return;
    }

    const FramesMetaData* metaDatac = imgsrc->getMetaData();
    int imgNumc = 0;

    if (imgsrc->isRAW()) {
        if (imgsrc->getSensorType() == ST_BAYER) {
            imgNumc = rtengine::LIM<unsigned int>(params.raw.bayersensor.imageNum, 0, metaDatac->getFrameCount() - 1);
        } else if (imgsrc->getSensorType() == ST_FUJI_XTRANS) {
            //imgNum = rtengine::LIM<unsigned int>(params.raw.xtranssensor.imageNum, 0, metaData->getFrameCount() - 1);
        }
    }

    float fnumc = metaDatac->getFNumber(imgNumc);          // F number
    float fisoc = metaDatac->getISOSpeed(imgNumc) ;        // ISO
    float fspeedc = metaDatac->getShutterSpeed(imgNumc) ;  // Speed
    double fcompc = metaDatac->getExpComp(imgNumc);        // Compensation +/-
    float ada = 1.f;

    if (fnumc < 0.3f || fisoc < 5.f || fspeedc < 0.00001f) { //if no exif data or wrong
        ada = 2000.;
    } else {
        double E_V = fcompc + log2(double ((fnumc * fnumc) / fspeedc / (fisoc / 100.f)));
        E_V += params.toneCurve.expcomp;// exposure compensation in tonecurve ==> direct EV
        E_V += log2(params.raw.expos);  // exposure raw white point ; log2 ==> linear to EV
        ada = powf(2.f, E_V - 3.f);  // cd / m2
        // end calculation adaptation scene luminosity
    }

    int cat0 = 100;

    //printf("temp=%i \n", params.wb.temperature);
    if (params.localwb.temp < 4000  || params.localwb.temp > 15000) { //15000 arbitrary value
        float kunder = 1.f;

        if (params.localwb.temp > 20000) {
            kunder = 0.05f;    //underwater photos
        }

        if (ada < 5.f) {
            cat0 = 1;
        } else if (ada < 10.f) {
            cat0 = 2 * kunder;
        } else if (ada < 15.f) {
            cat0 = 3 * kunder;
        } else if (ada < 30.f) {
            cat0 = 5 * kunder;
        } else if (ada < 100.f) {
            cat0 = 50 * kunder;
        } else if (ada < 300.f) {
            cat0 = 80 * kunder;
        } else if (ada < 500.f) {
            cat0 = 90 * kunder;
        } else if (ada < 3000.f) {
            cat0 = 95 * kunder;
        }
    } else {
        if (ada < 5.f) {
            cat0 = 30;
        } else if (ada < 10.f) {
            cat0 = 50;
        } else if (ada < 30.f) {
            cat0 = 60;
        } else if (ada < 100.f) {
            cat0 = 70;
        } else if (ada < 300.f) {
            cat0 = 80;
        } else if (ada < 500.f) {
            cat0 = 90;
        } else if (ada < 1000.f) {
            cat0 = 95;
        }
    }

    if (params.localwb.autoamount) {
        params.localwb.amount = cat0;
    }

    double gree0 = 1.0;
    float Tref = (float) params.localwb.temp;

    if (Tref > 8000.f) {
        Tref = 8000.f;
    }

    if (Tref < 4000.f) {
        Tref = 4000.f;
    }

    float dT = fabs((Tref - 5000.) / 1000.f);
    float dG = params.localwb.green - 1.;
    gree0 = 1.f - 0.00055f * dT * dG * params.localwb.amount;//empirical formula

    if (params.localwb.autoluminanceScaling) {
        params.localwb.luminanceScaling = gree0;
    }
}


void cat02adaptationloc(Imagefloat *image, float gain, const ProcParams &params)
{
    const ColorAppearanceParams &cap = params.colorappearance;
//    const CAT02AdaptationParams &cat = params.cat02adap;
    const ColorManagementParams &cmp = params.icm;
//    const WBParams &wbp = params.wb;
	const LocWBParams &wbl = params.localwb;

    if (wbl.amount > 1  && !cap.enabled  && wbl.enabled) { //
        //    printf("OK cat02 CAT\n");
        LabImage *bufcat02 = nullptr;
        bufcat02 = new LabImage(image->getWidth(), image->getHeight());
        LabImage *bufcat02fin = nullptr;
        bufcat02fin = new LabImage(image->getWidth(), image->getHeight());
        TMatrix wiprof = ICCStore::getInstance()->workingSpaceInverseMatrix("WideGamut");//Widegamut gives generaly best results than Prophoto (blue) or sRGBD65
        TMatrix wprof = ICCStore::getInstance()->workingSpaceMatrix("WideGamut");

        double wip[3][3] = {
            {wiprof[0][0], wiprof[0][1], wiprof[0][2]},
            {wiprof[1][0], wiprof[1][1], wiprof[1][2]},
            {wiprof[2][0], wiprof[2][1], wiprof[2][2]}
        };

        double wp[3][3] = {
            {wprof[0][0], wprof[0][1], wprof[0][2]},
            {wprof[1][0], wprof[1][1], wprof[1][2]},
            {wprof[2][0], wprof[2][1], wprof[2][2]}
        };


#ifdef _OPENMP
        #pragma omp parallel for schedule(dynamic,16)
#endif

        for (int y = 0; y <  image->getHeight() ; y++) //{
            for (int x = 0; x < image->getWidth(); x++) {
                bufcat02->L[y][x] = 0.f;
                bufcat02->a[y][x] = 0.f;
                bufcat02->b[y][x] = 0.f;
                bufcat02fin->L[y][x] = 0.f;
                bufcat02fin->a[y][x] = 0.f;
                bufcat02fin->b[y][x] = 0.f;

            }

#ifdef _OPENMP
        #pragma omp parallel for schedule(dynamic,16)
#endif

        for (int y = 0; y <  image->getHeight() ; y++) //{
            for (int x = 0; x < image->getWidth(); x++) {
                float X, Y, Z;
                float LR, aR, bR;
                //Color::gammatab_srgb[ try with but not good results
                // I add gain to best results
                Color::rgbxyz(gain * image->r(y, x), gain * image->g(y, x), gain * image->b(y, x), X, Y, Z, wp);
                Color::XYZ2Lab(X, Y, Z, LR, aR, bR);
                bufcat02->L[y][x] = LR;
                bufcat02->a[y][x] = aR;
                bufcat02->b[y][x] = bR;
            }

        ciecamcat02loc_float(bufcat02, bufcat02fin, wbl.temp, wbl.luminanceScaling, wbl.amount, cmp, cap);
#ifdef _OPENMP
        #pragma omp parallel for schedule(dynamic,16)
#endif

        for (int y = 0; y <  image->getHeight() ; y++) //{
            for (int x = 0; x < image->getWidth(); x++) {
                float XR, YR, ZR;
                float LL, aa, bb;
                LL = bufcat02fin->L[y][x];
                aa = bufcat02fin->a[y][x];
                bb = bufcat02fin->b[y][x];

                Color::Lab2XYZ(LL, aa, bb, XR, YR, ZR);
                Color::xyz2rgb(XR, YR, ZR, image->r(y, x), image->g(y, x), image->b(y, x), wip);
                //image->r(y, x) = Color::igammatab_srgb[image->r(y, x)];
                //image->g(y, x) = Color::igammatab_srgb[image->g(y, x)];
                //image->b(y, x) = Color::igammatab_srgb[image->b(y, x)];
                image->r(y, x) /= gain;
                image->g(y, x) /= gain;
                image->b(y, x) /= gain;

            }

        delete bufcat02;
        delete bufcat02fin;
    }
}

} // namespace rtengine
