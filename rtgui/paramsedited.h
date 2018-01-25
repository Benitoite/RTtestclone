/*
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
#ifndef _PARAMEDITED_H_
#define _PARAMEDITED_H_

#include <glibmm.h>
#include <vector>
#include "../rtengine/procparams.h"
#include "../rtengine/rtengine.h"

class GeneralParamsEdited
{

public:
    bool rank;
    bool colorlabel;
    bool intrash;

    operator bool (void) const;
    void set(bool v);
};

class ToneCurveParamsEdited
{

public:
    bool curve;
    bool curve2;
    bool curveMode;
    bool curveMode2;
    bool brightness;
    bool black;
    bool contrast;
    bool saturation;
    bool shcompr;
    bool hlcompr;
    bool hlcomprthresh;
    bool autoexp;
    bool clip;
    bool expcomp;
    bool hrenabled;
    bool method;
    bool histmatching;

    operator bool () const;
};

class RetinexParamsEdited
{
public:
    bool enabled;
    bool str;
    bool scal;
    bool iter;
    bool grad;
    bool grads;
    bool gam;
    bool slope;
    bool neigh;
    bool offs;
    bool retinexMethod;
    bool mapMethod;
    bool viewMethod;
    bool retinexcolorspace;
    bool gammaretinex;
    bool vart;
    bool limd;
    bool highl;
    bool baselog;
    bool skal;
    bool method;
    bool transmissionCurve;
    bool gaintransmissionCurve;
    bool cdcurve;
    bool mapcurve;
    bool cdHcurve;
    bool lhcurve;
    bool retinex;
    bool medianmap;
    bool highlights;
    bool htonalwidth;
    bool shadows;
    bool stonalwidth;
    bool radius;

    operator bool () const;
};


class LCurveParamsEdited
{
public:
    bool enabled;
    bool brightness;
    bool contrast;
    bool chromaticity;
    bool avoidcolorshift;
    bool rstprotection;
    bool lcurve;
    bool acurve;
    bool bcurve;
    bool lcredsk;
    bool cccurve;
    bool chcurve;
    bool lhcurve;
    bool hhcurve;
    bool lccurve;
    bool clcurve;

    operator bool () const;
};


class LocalContrastParamsEdited {
public:
    bool enabled;
    bool radius;
    bool amount;
    bool darkness;
    bool lightness;

    operator bool () const;
};


class RGBCurvesParamsEdited
{

public:
    bool enabled;
    bool lumamode;
    bool rcurve;
    bool gcurve;
    bool bcurve;

    operator bool () const;
};

class ColorToningEdited
{

public:
    bool enabled;
    bool opacityCurve;
    bool colorCurve;
    bool clcurve;
    bool method;
    bool autosat;
    bool satprotectionthreshold;
    bool saturatedopacity;
    bool strength;
    bool shadowsColSat;
    bool hlColSat;
    bool balance;
    bool twocolor;
    bool cl2curve;
    bool redlow;
    bool greenlow;
    bool bluelow;
    bool redmed;
    bool greenmed;
    bool bluemed;
    bool redhigh;
    bool greenhigh;
    bool bluehigh;
    bool satlow;
    bool sathigh;
    bool lumamode;
    bool labgridALow;
    bool labgridBLow;
    bool labgridAHigh;
    bool labgridBHigh;

    operator bool () const;
};

class SharpenEdgeParamsEdited
{

public :
    bool enabled;
    bool passes;
    bool amount;
    bool threechannels;

    operator bool () const;
};

class SharpenMicroParamsEdited
{
public :
    bool enabled;
    bool matrix;
    bool amount;
    bool uniformity;

    operator bool () const;
};

class SharpeningParamsEdited
{

public:
    bool enabled;
    bool radius;
    bool amount;
    bool threshold;
    bool edgesonly;
    bool edges_radius;
    bool edges_tolerance;
    bool halocontrol;
    bool halocontrol_amount;

    bool method;
    bool deconvamount;
    bool deconvradius;
    bool deconviter;
    bool deconvdamping;

    operator bool () const;
};

class VibranceParamsEdited
{

public:
    bool enabled;
    bool pastels;
    bool saturated;
    bool psthreshold;
    bool protectskins;
    bool avoidcolorshift;
    bool pastsattog;
    bool skintonescurve;

    operator bool () const;
};

class WBParamsEdited
{

public:
    bool enabled;
    bool method;
    bool temperature;
    bool green;
    bool equal;
    bool tempBias;

    operator bool () const;
};

class DefringeParamsEdited
{

public:
    bool enabled;
    bool radius;
    bool threshold;
    bool huecurve;

    operator bool () const;
};

class ImpulseDenoiseParamsEdited
{

public:
    bool enabled;
    bool thresh;

    operator bool () const;
};

class ColorAppearanceParamsEdited
{

public:
    bool curve;
    bool curve2;
    bool curve3;
    bool curveMode;
    bool curveMode2;
    bool curveMode3;
    bool enabled;
    bool degree;
    bool autodegree;
    bool degreeout;
    bool autodegreeout;
    bool autoadapscen;
    bool autoybscen;
    bool surround;
    bool surrsrc;
    bool adapscen;
    bool adaplum;
    bool ybscen;
    bool badpixsl;
    bool wbmodel;
    bool algo;
    bool jlight;
    bool qbright;
    bool chroma;
    bool schroma;
    bool mchroma;
    bool contrast;
    bool qcontrast;
    bool colorh;
    bool rstprotection;
    bool surrsource;
    bool gamut;
//  bool badpix;
    bool datacie;
    bool tonecie;
//  bool sharpcie;
    bool tempout;
    bool greenout;
    bool ybout;
    bool tempsc;
    bool greensc;

    operator bool () const;
};

class DirPyrDenoiseParamsEdited
{

public:
    bool enabled;
    bool enhance;
    bool median;
    bool Ldetail;
    bool luma;
    bool chroma;
    bool redchro;
    bool bluechro;
    bool gamma;
    bool lcurve;
    bool cccurve;

//    bool perform;
    bool dmethod;
    bool Lmethod;
    bool Cmethod;
    bool C2method;
    bool smethod;
    bool medmethod;
    bool methodmed;
    bool rgbmethod;
    bool passes;

    operator bool () const;
};

class EPDParamsEdited
{
public:
    bool enabled;
    bool strength;
    bool gamma;
    bool edgeStopping;
    bool scale;
    bool reweightingIterates;

    operator bool () const;
};


class FattalToneMappingParamsEdited {
public:
    bool enabled;
    bool threshold;
    bool amount;
    bool anchor;

    operator bool () const;
};


class SHParamsEdited
{

public:
    bool enabled;
    bool hq;
    bool highlights;
    bool htonalwidth;
    bool shadows;
    bool stonalwidth;
    bool radius;

    operator bool () const;
};

class CropParamsEdited
{

public:
    bool enabled;
    bool x;
    bool y;
    bool w;
    bool h;
    bool fixratio;
    bool ratio;
    bool orientation;
    bool guide;

    operator bool () const;
};

class CoarseTransformParamsEdited
{

public:
    bool rotate;
    bool hflip;
    bool vflip;

    operator bool () const;
};

class CommonTransformParamsEdited
{

public:
    bool autofill;

    operator bool () const;
};

class RotateParamsEdited
{

public:
    bool degree;

    operator bool () const;
};

class DistortionParamsEdited
{

public:
    bool amount;

    operator bool () const;
};

class LensProfParamsEdited
{
public:
    bool lcpFile, useDist, useVign, useCA;
    bool useLensfun, lfAutoMatch, lfCameraMake, lfCameraModel, lfLens;
    bool lcMode;

    operator bool () const;
};

class PerspectiveParamsEdited
{

public:
    bool horizontal;
    bool vertical;

    operator bool () const;
};

class GradientParamsEdited
{

public:
    bool enabled;
    bool degree;
    bool feather;
    bool strength;
    bool centerX;
    bool centerY;

    operator bool () const;
};

class PCVignetteParamsEdited
{

public:
    bool enabled;
    bool strength;
    bool feather;
    bool roundness;

    operator bool () const;
};

class VignettingParamsEdited
{

public:
    bool amount;
    bool radius;
    bool strength;
    bool centerX;
    bool centerY;

    operator bool () const;
};

class ChannelMixerParamsEdited
{

public:
    bool enabled;
    bool red[3];
    bool green[3];
    bool blue[3];

    operator bool () const;
};

class BlackWhiteParamsEdited
{

public:
    bool enabledcc;
    bool enabled;
    bool method;
    bool filter;
    bool setting;
    bool mixerRed;
    bool mixerOrange;
    bool mixerYellow;
    bool mixerGreen;
    bool mixerCyan;
    bool mixerBlue;
    bool mixerMagenta;
    bool mixerPurple;
    bool gammaRed;
    bool gammaGreen;
    bool gammaBlue;
    bool luminanceCurve;
    bool beforeCurve;
    bool beforeCurveMode;
    bool afterCurve;
    bool afterCurveMode;
    bool autoc;
    bool algo;

    operator bool () const;
};

class CACorrParamsEdited
{

public:
    bool red;
    bool blue;

    operator bool () const;
};

/*
class HRecParamsEdited {

    public:
        bool enabled;
        bool method;

    operator bool () const;
};
*/

class ResizeParamsEdited
{

public:
    bool scale;
    bool appliesTo;
    bool method;
    bool dataspec;
    bool width;
    bool height;
    bool enabled;

    operator bool () const;
};

class ColorManagementParamsEdited
{

public:
    bool input;
    bool toneCurve;
    bool applyLookTable;
    bool applyBaselineExposureOffset;
    bool applyHueSatMap;
    bool dcpIlluminant;
    bool working;
    bool output;
    bool outputIntent;
    bool outputBPC;
    bool gamma;
    bool gampos;
    bool slpos;
    bool freegamma;

    operator bool () const;
};

class WaveletParamsEdited
{

public:
    bool enabled;
    bool strength;
    bool balance;
    bool iter;
    bool median;
    bool medianlev;
    bool linkedg;
    bool cbenab;
    bool lipst;
    bool Medgreinf;
    bool avoid;
    bool tmr;
    bool c[9];
    bool ch[9];
    bool Lmethod;
    bool CHmethod;
    bool CHSLmethod;
    bool EDmethod;
    bool BAmethod;
    bool NPmethod;
    bool TMmethod;
    bool HSmethod;
    bool CLmethod;
    bool Backmethod;
    bool Tilesmethod;
    bool daubcoeffmethod;
    bool Dirmethod;
    bool rescon;
    bool resconH;
    bool reschro;
    bool tmrs;
    bool gamma;
    bool sup;
    bool sky;
    bool thres;
    bool threshold;
    bool threshold2;
    bool edgedetect;
    bool edgedetectthr;
    bool edgedetectthr2;
    bool edgesensi;
    bool edgeampli;
    bool chro;
    bool chroma;
    bool contrast;
    bool edgrad;
    bool edgval;
    bool edgthresh;
    bool thr;
    bool thrH;
    bool skinprotect;
    bool hueskin;
    bool hueskin2;
    bool hllev;
    bool bllev;
    bool edgcont;
    bool level0noise;
    bool level1noise;
    bool level2noise;
    bool level3noise;
    bool ccwcurve;
    bool opacityCurveBY;
    bool opacityCurveRG;
    bool opacityCurveW;
    bool opacityCurveWL;
    bool hhcurve;
    bool Chcurve;
    bool pastlev;
    bool satlev;
    bool wavclCurve;
    bool greenlow;
    bool bluelow;
    bool greenmed;
    bool bluemed;
    bool greenhigh;
    bool bluehigh;
    bool expcontrast;
    bool expchroma;
    bool expedge;
    bool expresid;
    bool expfinal;
    bool exptoning;
    bool expnoise;

    operator bool () const;
};

class DirPyrEqualizerParamsEdited
{

public:
    bool enabled;
    bool gamutlab;
    bool mult[6];
    bool cbdlMethod;
    bool threshold;
    bool skinprotect;
    bool hueskin;
    //     bool algo;

    operator bool () const;
};

class HSVEqualizerParamsEdited
{

public:
    bool enabled;
    bool hcurve;
    bool scurve;
    bool vcurve;

    operator bool () const;
};

class FilmSimulationParamsEdited
{
public:
    bool enabled;
    bool clutFilename;
    bool strength;

    operator bool () const;
};

class RAWParamsEdited
{

public:
    class BayerSensor
    {

    public:
        bool method;
        bool imageNum;
        bool ccSteps;
        bool exBlack0;
        bool exBlack1;
        bool exBlack2;
        bool exBlack3;
        bool exTwoGreen;
        bool dcbIterations;
        bool dcbEnhance;
        bool lmmseIterations;
        bool pixelShiftMotion;
        bool pixelShiftMotionCorrection;
        bool pixelShiftMotionCorrectionMethod;
        bool pixelShiftStddevFactorGreen;
        bool pixelShiftStddevFactorRed;
        bool pixelShiftStddevFactorBlue;
        bool pixelShiftEperIso;
        bool pixelShiftNreadIso;
        bool pixelShiftPrnu;
        bool pixelShiftSigma;
        bool pixelShiftSum;
        bool pixelShiftRedBlueWeight;
        bool pixelShiftShowMotion;
        bool pixelShiftShowMotionMaskOnly;
        bool pixelShiftAutomatic;
        bool pixelShiftNonGreenHorizontal;
        bool pixelShiftNonGreenVertical;
        bool pixelShiftHoleFill;
        bool pixelShiftMedian;
        bool pixelShiftMedian3;
        bool pixelShiftGreen;
        bool pixelShiftBlur;
        bool pixelShiftSmooth;
        bool pixelShiftExp0;
        bool pixelShiftLmmse;
        bool pixelShiftOneGreen;
        bool pixelShiftEqualBright;
        bool pixelShiftEqualBrightChannel;
        bool pixelShiftNonGreenCross;
        bool pixelShiftNonGreenCross2;
        bool pixelShiftNonGreenAmaze;

        //bool allEnhance;
        bool greenEq;
        bool linenoise;

        operator bool () const;
    };

    class XTransSensor
    {

    public:
        bool method;
        bool ccSteps;
        bool exBlackRed;
        bool exBlackGreen;
        bool exBlackBlue;

        operator bool () const;
    };

    BayerSensor bayersensor;
    XTransSensor xtranssensor;

    bool ca_autocorrect;
    bool cared;
    bool cablue;
    bool hotPixelFilter;
    bool deadPixelFilter;
    bool hotdeadpix_thresh;
    bool darkFrame;
    bool df_autoselect;
    bool ff_file;
    bool ff_AutoSelect;
    bool ff_BlurRadius;
    bool ff_BlurType;
    bool ff_AutoClipControl;
    bool ff_clipControl;
    bool exPos;
    bool exPreser;

    operator bool () const;
};


class MetaDataParamsEdited {
public:
    bool mode;

    operator bool () const;
};


class ParamsEdited
{

public:
    GeneralParamsEdited           general;
    ToneCurveParamsEdited         toneCurve;
    LCurveParamsEdited            labCurve;
    LocalContrastParamsEdited     localContrast;
    RGBCurvesParamsEdited         rgbCurves;
    ColorToningEdited             colorToning;
    RetinexParamsEdited           retinex;
    SharpeningParamsEdited        sharpening;
    SharpeningParamsEdited        prsharpening;
    SharpenEdgeParamsEdited       sharpenEdge;
    SharpenMicroParamsEdited      sharpenMicro;
    VibranceParamsEdited          vibrance;
    ColorAppearanceParamsEdited   colorappearance;
    WBParamsEdited                wb;
    DefringeParamsEdited          defringe;
    DirPyrDenoiseParamsEdited     dirpyrDenoise;
    EPDParamsEdited               epd;
    FattalToneMappingParamsEdited fattal;
    ImpulseDenoiseParamsEdited    impulseDenoise;
    SHParamsEdited                sh;
    CropParamsEdited              crop;
    CoarseTransformParamsEdited   coarse;
    CommonTransformParamsEdited   commonTrans;
    RotateParamsEdited            rotate;
    DistortionParamsEdited        distortion;
    LensProfParamsEdited          lensProf;
    PerspectiveParamsEdited       perspective;
    GradientParamsEdited          gradient;
    PCVignetteParamsEdited        pcvignette;
    CACorrParamsEdited            cacorrection;
    VignettingParamsEdited        vignetting;
    ChannelMixerParamsEdited      chmixer;
    BlackWhiteParamsEdited        blackwhite;
    ResizeParamsEdited            resize;
    ColorManagementParamsEdited   icm;
    RAWParamsEdited               raw;
    DirPyrEqualizerParamsEdited   dirpyrequalizer;
    WaveletParamsEdited           wavelet;
    HSVEqualizerParamsEdited      hsvequalizer;
    FilmSimulationParamsEdited    filmSimulation;
    MetaDataParamsEdited          metadata;
    bool                          exif;
    bool                          iptc;

    explicit ParamsEdited (bool value = false);

    void set   (bool v, int subPart = rtengine::procparams::ProcParams::SP_FLAGS
                                     |rtengine::procparams::ProcParams::SP_EXIF
                                     |rtengine::procparams::ProcParams::SP_IPTC
                                     |rtengine::procparams::ProcParams::SP_TOOL);

    void initFrom (const std::vector<rtengine::procparams::ProcParams>& src);
    void combine (rtengine::procparams::ProcParams& toEdit, const rtengine::procparams::ProcParams& mods, bool forceSet);

    bool isTagsSet();
    bool isToolSet();
    bool isExifSet();
    bool isIptcSet();
};
#endif
