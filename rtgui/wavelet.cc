/*
 *  This file is part of RawTherapee.
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
 *
 *  2014 Jacques Desmis <jdesmis@gmail.com>
 */

#include "wavelet.h"
#include <cmath>
#include "edit.h"
#include "guiutils.h"
#include "options.h"
#include "rtimage.h"
#include "eventmapper.h"

using namespace rtengine;
using namespace rtengine::procparams;
extern Options options;

namespace
{

GradientMilestone makeHsvGm (double position, float h, float s, float v)
{
    float r;
    float g;
    float b;
    Color::hsv2rgb01 (h, s, v, r, g, b);
    return GradientMilestone (position, r, g, b);
}

std::vector<GradientMilestone> makeWholeHueRange()
{
    std::vector<GradientMilestone> res;
    res.reserve (7);

    for (int i = 0; i < 7; ++i) {
        const float x = static_cast<float> (i) / 6.0f;
        res.push_back (makeHsvGm (x, x, 0.5f, 0.5f));
    }

    return res;
}

}



Wavelet::Wavelet() :
    FoldableToolPanel (this, "wavelet", M ("TP_WAVELET_LABEL"), true, true),
    lastlabFilename (""),
    walistener (NULL),
    curveEditorG (new CurveEditorGroup (options.lastWaveletCurvesDir, M ("TP_WAVELET_CONTEDIT"))),
    CCWcurveEditorG (new CurveEditorGroup (options.lastWaveletCurvesDir, M ("TP_WAVELET_CCURVE"))),
    curveEditorRES (new CurveEditorGroup (options.lastWaveletCurvesDir)),
    curveEditorGAM (new CurveEditorGroup (options.lastWaveletCurvesDir)),
    CCWcurveEditorT (new CurveEditorGroup (options.lastWaveletCurvesDir, M ("TP_RETINEX_TRANSMISSION"))),
    CCWcurveEditorgainT (new CurveEditorGroup (options.lastWaveletCurvesDir, M ("TP_WAVELET_TRANSMISSIONGAIN"))),
    CCWcurveEditormerg (new CurveEditorGroup (options.lastWaveletCurvesDir, M ("TP_WAVELET_MCURVE"))),
    CCWcurveEditormerg2 (new CurveEditorGroup (options.lastWaveletCurvesDir, M ("TP_WAVELET_MCURVE2"))),
    CCWcurveEditorsty (new CurveEditorGroup (options.lastWaveletCurvesDir, M ("TP_WAVELET_ST2CURVE"))),
    CCWcurveEditorsty2 (new CurveEditorGroup (options.lastWaveletCurvesDir, M ("TP_WAVELET_ST2CURVE2"))),
    curveEditorsty (new CurveEditorGroup (options.lastWaveletCurvesDir)),
    opaCurveEditorG (new CurveEditorGroup (options.lastWaveletCurvesDir, M ("TP_WAVELET_COLORT"))),
    opacityCurveEditorG (new CurveEditorGroup (options.lastWaveletCurvesDir, M ("TP_WAVELET_OPACITY"))),
    opacityCurveEditorW (new CurveEditorGroup (options.lastWaveletCurvesDir, M ("TP_WAVELET_OPACITYW"))),
    opacityCurveEditorWL (new CurveEditorGroup (options.lastWaveletCurvesDir, M ("TP_WAVELET_OPACITYWL"))),
    separatorCB (Gtk::manage (new Gtk::HSeparator())),
    separatorNeutral (Gtk::manage (new Gtk::HSeparator())),
    separatoredge (Gtk::manage (new Gtk::HSeparator())),
    separatorRT (Gtk::manage (new Gtk::HSeparator())),
    separatorsty (Gtk::manage (new Gtk::HSeparator())),
    separatorsty2 (Gtk::manage (new Gtk::HSeparator())),
    separatorsty3 (Gtk::manage (new Gtk::HSeparator())),
    median (Gtk::manage (new Gtk::CheckButton (M ("TP_WAVELET_MEDI")))),
    medianlev (Gtk::manage (new Gtk::CheckButton (M ("TP_WAVELET_MEDILEV")))),
    linkedg (Gtk::manage (new Gtk::CheckButton (M ("TP_WAVELET_LINKEDG")))),
    cbenab (Gtk::manage (new Gtk::CheckButton (M ("TP_WAVELET_CBENAB")))),
    lipst (Gtk::manage (new Gtk::CheckButton (M ("TP_WAVELET_LIPST")))),
    avoid (Gtk::manage (new Gtk::CheckButton (M ("TP_WAVELET_AVOID")))),
    tmr (Gtk::manage (new Gtk::CheckButton (M ("TP_WAVELET_BALCHRO")))),
    neutralchButton (Gtk::manage (new Gtk::Button (M ("TP_WAVELET_NEUTRAL")))),
    neutral2 (Gtk::manage (new Gtk::Button (M ("TP_WAVELET_NEUTRAL2")))),
    rescon (Gtk::manage (new Adjuster (M ("TP_WAVELET_RESCON"), -100, 100, 1, 0))),
    resconH (Gtk::manage (new Adjuster (M ("TP_WAVELET_RESCONH"), -100, 100, 1, 0))),
    reschro (Gtk::manage (new Adjuster (M ("TP_WAVELET_RESCHRO"), -100, 100, 1, 0))),
    tmrs (Gtk::manage (new Adjuster (M ("TP_WAVELET_TMSTRENGTH"), -1.0, 2.0, 0.01, 0.0))),
    gamma (Gtk::manage (new Adjuster (M ("TP_WAVELET_COMPGAMMA"), 0.4, 2.0, 0.01, 1.0))),
    sup (Gtk::manage (new Adjuster (M ("TP_WAVELET_SUPE"), -100, 350, 1, 0))),
    sky (Gtk::manage (new Adjuster (M ("TP_WAVELET_SKY"), -100., 100.0, 1., 0.))),
    thres (Gtk::manage (new Adjuster (M ("TP_WAVELET_LEVELS"), 4, 9, 1, 7))),
    chroma (Gtk::manage (new Adjuster (M ("TP_WAVELET_CHRO"), 1, 9, 1, 5))),
    chro (Gtk::manage (new Adjuster (M ("TP_WAVELET_CHR"), 0., 100., 1., 0.))),
    contrast (Gtk::manage (new Adjuster (M ("TP_WAVELET_CONTRA"), -100, 100, 1, 0))),
    thr (Gtk::manage (new Adjuster (M ("TP_WAVELET_THR"), 0, 100, 1, 35))),
    thrH (Gtk::manage (new Adjuster (M ("TP_WAVELET_THRH"), 0, 100, 1, 65))),
    skinprotect (Gtk::manage ( new Adjuster (M ("TP_WAVELET_SKIN"), -100, 100, 1, 0.) )),
    edgrad (Gtk::manage ( new Adjuster (M ("TP_WAVELET_EDRAD"), 0, 100, 1, 15) )),
    edgval (Gtk::manage ( new Adjuster (M ("TP_WAVELET_EDVAL"), 0, 100, 1, 0) )),
    edgthresh (Gtk::manage (new Adjuster (M ("TP_WAVELET_EDGTHRESH"), -50, 100, 1, 10 ))),
    strength (Gtk::manage (new Adjuster (M ("TP_WAVELET_STRENGTH"), 0, 100, 1, 100))),
    balance (Gtk::manage (new Adjuster (M ("TP_WAVELET_BALANCE"), -30, 100, 1, 0))),
    iter (Gtk::manage (new Adjuster (M ("TP_WAVELET_ITER"), -3, 3, 1, 0))),
    mergeL (Gtk::manage (new Adjuster (M ("TP_WAVELET_MERGEL"), -50, 100, 1, 40))),
    mergeC (Gtk::manage (new Adjuster (M ("TP_WAVELET_MERGEC"), -50, 100, 1, 20))),
    balanleft (Gtk::manage (new Adjuster (M ("TP_WAVELET_HORIZSH"), 0, 100, 1, 50))),
    balanhig (Gtk::manage (new Adjuster (M ("TP_WAVELET_VERTSH"), 0, 100, 1, 50))),
    sizelab (Gtk::manage (new Adjuster (M ("TP_WAVELET_SIZELAB"), 1.0, 4.0, 0.1, 1))),
    dirV (Gtk::manage (new Adjuster (M ("TP_WAVELET_DIRV"), -100, 100, 1, 60))),
    dirH (Gtk::manage (new Adjuster (M ("TP_WAVELET_DIRH"), -100, 100, 1, 0))),
    dirD (Gtk::manage (new Adjuster (M ("TP_WAVELET_DIRD"), -100, 100, 1, -30))),
    balmerch (Gtk::manage (new Adjuster (M ("TP_WAVELET_BALMERCH"), 0, 300, 1, 180))),
    shapedetcolor (Gtk::manage (new Adjuster (M ("TP_WAVELET_SHAPEDETCOLOR"), -100, 100, 1, 0))),
    balmerres (Gtk::manage (new Adjuster (M ("TP_WAVELET_BALMERRES"), 0, 150, 1, 25))),
    balmerres2 (Gtk::manage (new Adjuster (M ("TP_WAVELET_BALMERRES2"), -20, 120, 1, 70))),
    blend (Gtk::manage (new Adjuster (M ("TP_WAVELET_BLEND"), 0, 100, 1, 50))),
    blendc (Gtk::manage (new Adjuster (M ("TP_WAVELET_BLENDC"), -100, 100, 1, 30))),
    grad (Gtk::manage (new Adjuster (M ("TP_WAVELET_GRAD"), -20, 100, 1, 20))),
    offs (Gtk::manage (new Adjuster (M ("TP_WAVELET_OFFS"), -1000, 5000, 1, 0))),
    str (Gtk::manage (new Adjuster (M ("TP_WAVELET_STR"), 0, 100, 1, 20))),
    neigh (Gtk::manage (new Adjuster (M ("TP_WAVELET_NEIGH"), 14, 150, 1, 50))),
    vart (Gtk::manage (new Adjuster (M ("TP_WAVELET_VART"), 50, 500, 1, 200))),
    limd (Gtk::manage (new Adjuster (M ("TP_WAVELET_LIMD"), 2, 100, 1, 8))),
    chrrt (Gtk::manage (new Adjuster (M ("TP_WAVELET_CHRRT"), 0, 100, 1, 0))),
    scale (Gtk::manage (new Adjuster (M ("TP_WAVELET_SCA"), 1, 8, 1, 3))),
    gain (Gtk::manage (new Adjuster (M ("TP_WAVELET_GAIN"), 0, 200, 1, 50))),
    hueskin (Gtk::manage (new ThresholdAdjuster (M ("TP_WAVELET_HUESKIN"), -314., 314., -5., 25., 170., 120., 0, false))),
    hueskin2 (Gtk::manage (new ThresholdAdjuster (M ("TP_WAVELET_HUESKY"), -314., 314., -260., -250, -130., -140., 0, false))),
    hueskinsty (Gtk::manage (new ThresholdAdjuster (M ("TP_WAVELET_HUESKINSTY"), -314., 314., -5., 25., 170., 120., 0, false))),
    hllev (Gtk::manage (new ThresholdAdjuster (M ("TP_WAVELET_HIGHLIGHT"), 0., 100., 50., 75., 100., 98., 0, false))),
    bllev (Gtk::manage (new ThresholdAdjuster (M ("TP_WAVELET_LOWLIGHT"), 0., 100., 0., 2., 50., 25., 0, false))),
    pastlev (Gtk::manage (new ThresholdAdjuster (M ("TP_WAVELET_PASTEL"), 0., 70., 0., 2., 30., 20., 0, false))),
    satlev (Gtk::manage (new ThresholdAdjuster (M ("TP_WAVELET_SAT"), 0., 130., 30., 45., 130., 100., 0, false))),
    edgcont (Gtk::manage (new ThresholdAdjuster (M ("TP_WAVELET_EDGCONT"), 0., 100., 0, 10, 75, 40, 0., false))),
    level0noise (Gtk::manage (new ThresholdAdjuster (M ("TP_WAVELET_LEVZERO"), -30., 100., 0., M ("TP_WAVELET_STREN"), 1., 0., 100., 0., M ("TP_WAVELET_NOIS"), 1., nullptr, false))),
    level1noise (Gtk::manage (new ThresholdAdjuster (M ("TP_WAVELET_LEVONE"), -30., 100., 0., M ("TP_WAVELET_STREN"), 1., 0., 100., 0., M ("TP_WAVELET_NOIS"), 1., nullptr, false))),
    level2noise (Gtk::manage (new ThresholdAdjuster (M ("TP_WAVELET_LEVTWO"), -30., 100., 0., M ("TP_WAVELET_STREN"), 1., 0., 100., 0., M ("TP_WAVELET_NOIS"), 1., nullptr, false))),
    level3noise (Gtk::manage (new ThresholdAdjuster (M ("TP_WAVELET_LEVTHRE"), -30., 100., 0., M ("TP_WAVELET_STREN"), 1., 0., 100., 0., M ("TP_WAVELET_NOIS"), 1., nullptr, false))),
    threshold (Gtk::manage (new Adjuster (M ("TP_WAVELET_THRESHOLD"), 1, 9, 1, 5))),
    threshold2 (Gtk::manage (new Adjuster (M ("TP_WAVELET_THRESHOLD2"), 1, 9, 1, 4))),
    edgedetect (Gtk::manage (new Adjuster (M ("TP_WAVELET_EDGEDETECT"), 0, 100, 1, 90))),
    edgedetectthr (Gtk::manage (new Adjuster (M ("TP_WAVELET_EDGEDETECTTHR"), 0, 100, 1, 20))),
    edgedetectthr2 (Gtk::manage (new Adjuster (M ("TP_WAVELET_EDGEDETECTTHR2"), -10, 100, 1, 0))),
    edgesensi (Gtk::manage (new Adjuster (M ("TP_WAVELET_EDGESENSI"), 0, 100, 1, 60))),
    edgeampli (Gtk::manage (new Adjuster (M ("TP_WAVELET_EDGEAMPLI"), 0, 100, 1, 10))),
    highlights (Gtk::manage (new Adjuster (M ("TP_SHADOWSHLIGHTS_HIGHLIGHTS"), 0, 100, 1, 0))),
    h_tonalwidth (Gtk::manage (new Adjuster (M ("TP_SHADOWSHLIGHTS_HLTONALW"), 10, 100, 1, 80))),
    shadows (Gtk::manage (new Adjuster (M ("TP_SHADOWSHLIGHTS_SHADOWS"), 0, 100, 1, 0))),
    s_tonalwidth (Gtk::manage (new Adjuster (M ("TP_SHADOWSHLIGHTS_SHTONALW"), 10, 100, 1, 80))),
    radius (Gtk::manage (new Adjuster (M ("TP_SHADOWSHLIGHTS_RADIUS"), 5, 100, 1, 40))),
    shapind (Gtk::manage (new Adjuster (M ("TP_WAVELET_SHAPIND"), 0, 10, 1, 8))),

    Lmethod (Gtk::manage (new MyComboBoxText())),
    CHmethod (Gtk::manage (new MyComboBoxText())),
    CHSLmethod (Gtk::manage (new MyComboBoxText())),
    EDmethod (Gtk::manage (new MyComboBoxText())),
    BAmethod (Gtk::manage (new MyComboBoxText())),
    NPmethod (Gtk::manage (new MyComboBoxText())),
    TMmethod (Gtk::manage (new MyComboBoxText())),
    HSmethod (Gtk::manage (new MyComboBoxText())),
    CLmethod (Gtk::manage (new MyComboBoxText())),
    Backmethod (Gtk::manage (new MyComboBoxText())),
    Tilesmethod (Gtk::manage (new MyComboBoxText())),
    daubcoeffmethod (Gtk::manage (new MyComboBoxText())),
    Dirmethod (Gtk::manage (new MyComboBoxText())),
    Medgreinf (Gtk::manage (new MyComboBoxText())),
    usharpmethod (Gtk::manage (new MyComboBoxText ())),
    ushamethod (Gtk::manage (new MyComboBoxText ())),
    shapMethod (Gtk::manage (new MyComboBoxText ())),
    retinexMethod (Gtk::manage (new MyComboBoxText ())),
    mergMethod (Gtk::manage (new MyComboBoxText ())),
    mergMethod2 (Gtk::manage (new MyComboBoxText ())),
    mergevMethod (Gtk::manage (new MyComboBoxText ())),
    mergBMethod (Gtk::manage (new MyComboBoxText ())),

    chanMixerHLFrame (Gtk::manage (new Gtk::Frame (M ("TP_COLORTONING_HIGHLIGHT")))),
    chanMixerMidFrame (Gtk::manage (new Gtk::Frame (M ("TP_COLORTONING_MIDTONES")))),
    chanMixerShadowsFrame (Gtk::manage (new Gtk::Frame (M ("TP_COLORTONING_SHADOWS")))),
    gainFrame (Gtk::manage (new Gtk::Frame (M ("TP_WAVELET_GAINF")))),
    tranFrame (Gtk::manage (new Gtk::Frame (M ("TP_WAVELET_TRANF")))),
    gaussFrame (Gtk::manage (new Gtk::Frame (M ("TP_WAVELET_GAUSF")))),
    balMFrame (Gtk::manage (new Gtk::Frame (M ("TP_WAVELET_BALM")))),
    shaFrame (Gtk::manage (new Gtk::Frame (M ("TP_WAVELET_SHAF")))),
    wavLabels (Gtk::manage (new Gtk::Label ("---", Gtk::ALIGN_CENTER))),
    labmC (Gtk::manage (new Gtk::Label (M ("TP_WAVELET_CTYPE") + ":"))),
    labmNP (Gtk::manage (new Gtk::Label (M ("TP_WAVELET_NPTYPE") + ":"))),
    labmdh (Gtk::manage (new Gtk::Label (M ("TP_WAVRETI_METHOD") + ":"))),
    labmdhpro (Gtk::manage (new Gtk::Label (M ("TP_WAVRETI_METHODPRO") + ":"))),
    labretifin (Gtk::manage (new Gtk::Label (M ("TP_WAVRETI_FIN")))),
    labmmg1 (Gtk::manage (new Gtk::Label (M ("TP_WAVELET_MERG_METHOD") + ":"))),
    labmmg2 (Gtk::manage (new Gtk::Label (M ("TP_WAVELET_MERG_METHOD2") + ":"))),
    labmmg (Gtk::manage (new Gtk::Label (M ("TP_WAVELET_VIEW_METHOD") + ":"))),
    labmmgB (Gtk::manage (new Gtk::Label (M ("TP_WAVELET_MERGEB_METHOD") + ":"))),
    mMLabels (Gtk::manage (new Gtk::Label ("---", Gtk::ALIGN_CENTER))),
    transLabels (Gtk::manage (new Gtk::Label ("---", Gtk::ALIGN_CENTER))),
    transLabels2 (Gtk::manage (new Gtk::Label ("---", Gtk::ALIGN_CENTER))),
    usharpLabel (Gtk::manage (new Gtk::Label (M ("TP_WAVELET_USHARP") + ":"))),
    inLabel (Gtk::manage (new Gtk::Label (M ("GENERAL_FILE")))),

    expsettings (new MyExpander (false, M ("TP_WAVELET_SETTINGS"))),
    expcontrast (new MyExpander (true, M ("TP_WAVELET_LEVF"))),
    expchroma (new MyExpander (true, M ("TP_WAVELET_LEVCH"))),
    exptoning (new MyExpander (true, M ("TP_WAVELET_TON"))),
    expnoise (new MyExpander (true, M ("TP_WAVELET_NOISE"))),
    expedge (new MyExpander (true, M ("TP_WAVELET_EDGE"))),
    expreti (new MyExpander (true, M ("TP_WAVELET_RETI"))),
    expgamut (new MyExpander (false, M ("TP_WAVELET_CONTR"))),
    expresid (new MyExpander (true, M ("TP_WAVELET_RESID"))),
    expmerge (new MyExpander (true, M ("TP_WAVELET_MERGE"))),
    expfinal (new MyExpander (true, M ("TP_WAVELET_FINAL"))),
    expsettingreti (new MyExpander (false, M ("TP_RETINEX_SETTINGS"))),
    expedg1 (new MyExpander (false, M ("TP_WAVELET_EDG1"))),





    neutrHBox (Gtk::manage (new Gtk::HBox())),
    usharpHBox (Gtk::manage (new Gtk::HBox())),
    dhbox (Gtk::manage (new Gtk::HBox ())),
    dhboxpro (Gtk::manage (new Gtk::HBox ())),
    mg1box (Gtk::manage (new Gtk::HBox ())),
    mg2box (Gtk::manage (new Gtk::HBox ())),
    hbin (Gtk::manage (new Gtk::HBox ())),
    mgbox (Gtk::manage (new Gtk::HBox ())),
    neutrHBox2 (Gtk::manage (new Gtk::HBox ())),
    labretifinbox (Gtk::manage (new Gtk::HBox ())),
    mgBbox (Gtk::manage (new Gtk::HBox ())),
    mgVBox (Gtk::manage (new Gtk::VBox()))


{
    CurveListener::setMulti (true);

    nextmin = 0.;
    nextmax = 0.;
    nextminiT = 0.;
    nextmaxiT = 0.;
    nextmeanT = 0.;
    nextsigma = 0.;
    nextminT = 0.;
    nextmaxT = 0.;

    auto m = ProcEventMapper::getInstance();
    EvWavmergeC = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVMERGEC");//494
    EvWavgain = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVGAIN");//= 495,
    EvWavoffs = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVOFFS");//= 496,
    EvWavstr = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVSTR");//= 497,
    EvWavneigh = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVNEIGH");//= 498,
    EvWavvart = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVVART");//= 499,
    EvWavlimd = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVLIMD");//= 500,
    EvWavretinexMethod = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVRETIMETH");//= 501,
    EvWavCTCurve = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVCTCURV");//= 502,
    EvWavchrrt = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVCHRRT");//= 503,
    EvWavretinexMethodpro = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVRETIMETPRO");//= 504,
    EvWavenamerge = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVENAMERG");//= 505,
    EvWaveinput = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVINPUT");//= 506,
    EvWavmergevMethod = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVMERVGEMET");//= 507,
    EvWavmergMethod = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVMERGMET");//= 508,
    EvWavbalanlow = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVBALANLOW");//= 509,
    EvWavbalanhigh = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVBALANHIGH");//= 510,
    EvWavblen = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVBLEN");//= 511,
    EvWavblenc = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVBLENC");//= 512,
    EvmFile = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVVMFILE");//= 513,
    EvWavushamet = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVUSHAMET");//= 514,
    EvWavenareti = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVENARET");//= 515,
    EvWavmergBMethod = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVMERGEBMET");//= 516,
    EvWavradius = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVRADIUS");//= 517,
    EvWavhighlights = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVHIGHLIGH");//= 518,
    EWavh_tonalwidth = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVHTONALW");//= 519,
    EvWavshadows = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVSHADOWS");//= 520,
    EvWavs_tonalwidth = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVTONALWI");// = 521,
    EvWavscale = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVSCALE");// = 522,
    EvWavgrad = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVGRAD");// = 523,
    EvWavmergCurve = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVMERGCURV");// = 524,
    EvWavCTgainCurve = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVCTGAINCURV");// = 525,
    EvWavsizelab = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVSIZELAB");// = 527,
    EvWaveletbalmer = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVBALMER");// = 528,
    EvWavbalmerch = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVBALMERCH");// = 529,
    EvWavbalmerres = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVBALMERRES");// = 530,
    EvWavshapedet = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVSHAPDET");// = 531,
    EvWavstyCurve = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVSTYCURV");// = 532,
    EvWavHueskinsty = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVHUESKIN");// = 533,
    EvWavdirV = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVDIRV");// = 534,
    EvWavdirH = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVDIRD");// = 535,
    EvWavdirD = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVSHADOWS2");// = 536,
    EvWavshstyCurve = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVSTYCURV2");// = 537,
    EvWavmerg2Curve = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVMERG2CURV");// = 538,
    EvWaveletbalmer2 = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVBAMER2");// = 539,
    EvWavbalmerres2 = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVBALMERRES2");// = 540,
    EvWavmergMethod2 = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVMERGMET2");// = 541,
    EvWavshapMethod = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVSHAPMETH");// = 542,
    EvWavshapind = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVSHAPIND");// = 543,
    EvWavsty2Curve = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVSTY2CURV");// = 544,
    EvWavusharpmet = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVSHARPMET");// = 545,
    EvWavmergeL = m->newEvent(DIRPYREQUALIZER, "HISTORY_MSG_WAVMERGEL");// = 546,
	
    nextnlevel = 7.;

    std::vector<double> defaultCurve;

    expsettings->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), expsettings) );

    expcontrast->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), expcontrast) );
    enableContrastConn = expcontrast->signal_enabled_toggled().connect ( sigc::bind ( sigc::mem_fun (this, &Wavelet::enableToggled), expcontrast) );

    expchroma->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), expchroma) );
    enableChromaConn = expchroma->signal_enabled_toggled().connect ( sigc::bind ( sigc::mem_fun (this, &Wavelet::enableToggled), expchroma) );

    exptoning->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), exptoning) );
    enableToningConn = exptoning->signal_enabled_toggled().connect ( sigc::bind ( sigc::mem_fun (this, &Wavelet::enableToggled), exptoning) );

    expnoise->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), expnoise) );
    enableNoiseConn = expnoise->signal_enabled_toggled().connect ( sigc::bind ( sigc::mem_fun (this, &Wavelet::enableToggled), expnoise) );

    expedge->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), expedge) );
    enableEdgeConn = expedge->signal_enabled_toggled().connect ( sigc::bind ( sigc::mem_fun (this, &Wavelet::enableToggled), expedge) );

    expedg1->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), expedg1) );

    expreti->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), expreti) );
    enableretiConn = expreti->signal_enabled_toggled().connect ( sigc::bind ( sigc::mem_fun (this, &Wavelet::enableToggled), expreti) );
    expreti->set_tooltip_text (M ("TP_WAVELET_RETI_TOOLTIP"));

    expgamut->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), expgamut) );

    expresid->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), expresid) );
    enableResidConn = expresid->signal_enabled_toggled().connect ( sigc::bind ( sigc::mem_fun (this, &Wavelet::enableToggled), expresid) );

    expmerge->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), expmerge) );
    enableMergeConn = expmerge->signal_enabled_toggled().connect ( sigc::bind ( sigc::mem_fun (this, &Wavelet::enableToggled), expmerge) );
    expmerge->set_tooltip_text (M ("TP_WAVELET_MERGE_TOOLTIP"));

    expfinal->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), expfinal) );
    enableFinalConn = expfinal->signal_enabled_toggled().connect ( sigc::bind ( sigc::mem_fun (this, &Wavelet::enableToggled), expfinal) );

    expsettingreti->signal_button_release_event().connect_notify ( sigc::bind ( sigc::mem_fun (this, &Wavelet::foldAllButMe), expsettingreti) );


// Wavelet Settings
    ToolParamBlock* const settingsBox = Gtk::manage (new ToolParamBlock());

    strength->setAdjusterListener (this);

    thres->set_tooltip_text (M ("TP_WAVELET_LEVELS_TOOLTIP"));
    thres->setAdjusterListener (this);

    Tilesmethod->append (M ("TP_WAVELET_TILESFULL"));
    Tilesmethod->append (M ("TP_WAVELET_TILESBIG"));
    Tilesmethod->append (M ("TP_WAVELET_TILESLIT"));
    Tilesmethodconn = Tilesmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::TilesmethodChanged) );
    Tilesmethod->set_tooltip_text (M ("TP_WAVELET_TILES_TOOLTIP"));
    Gtk::HBox* const tilesizeHBox = Gtk::manage (new Gtk::HBox());
    Gtk::Label* const tilesizeLabel = Gtk::manage (new Gtk::Label (M ("TP_WAVELET_TILESIZE") + ":"));
    tilesizeHBox->pack_start (*tilesizeLabel, Gtk::PACK_SHRINK, 4);
    tilesizeHBox->pack_start (*Tilesmethod);

    daubcoeffmethod->set_sensitive (true);
    daubcoeffmethod->append (M ("TP_WAVELET_DAUB2"));
    daubcoeffmethod->append (M ("TP_WAVELET_DAUB4"));
    daubcoeffmethod->append (M ("TP_WAVELET_DAUB6"));
    daubcoeffmethod->append (M ("TP_WAVELET_DAUB10"));
    daubcoeffmethod->append (M ("TP_WAVELET_DAUB14"));
    daubcoeffmethodconn = daubcoeffmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::daubcoeffmethodChanged) );
    daubcoeffmethod->set_tooltip_text (M ("TP_WAVELET_DAUB_TOOLTIP"));
    Gtk::Label* const daubcoeffLabel = Gtk::manage (new Gtk::Label (M ("TP_WAVELET_DAUB") + ":"));
    Gtk::HBox* const daubcoeffHBox = Gtk::manage (new Gtk::HBox());
    daubcoeffHBox->pack_start (*daubcoeffLabel, Gtk::PACK_SHRINK, 4);
    daubcoeffHBox->pack_start (*daubcoeffmethod);

    Backmethod->append (M ("TP_WAVELET_B0"));
    Backmethod->append (M ("TP_WAVELET_B1"));
    Backmethod->append (M ("TP_WAVELET_B2"));
    Backmethodconn = Backmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::BackmethodChanged) );
    Gtk::HBox* const backgroundHBox = Gtk::manage (new Gtk::HBox());
    Gtk::Label* const backgroundLabel = Gtk::manage (new Gtk::Label (M ("TP_WAVELET_BACKGROUND") + ":"));
    backgroundHBox->pack_start (*backgroundLabel, Gtk::PACK_SHRINK, 4);
    backgroundHBox->pack_start (*Backmethod);

    CLmethod->append (M ("TP_WAVELET_LEVDIR_ONE"));
    CLmethod->append (M ("TP_WAVELET_LEVDIR_INF"));
    CLmethod->append (M ("TP_WAVELET_LEVDIR_SUP"));
    CLmethod->append (M ("TP_WAVELET_LEVDIR_ALL"));
    CLmethodconn = CLmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::CLmethodChanged) );
    Gtk::HBox* const levdirMainHBox = Gtk::manage (new Gtk::HBox());
    Gtk::Label* const levdirMainLabel = Gtk::manage (new Gtk::Label (M ("TP_WAVELET_PROC") + ":"));
    levdirMainHBox->pack_start (*levdirMainLabel, Gtk::PACK_SHRINK, 4);
    levdirMainHBox->pack_start (*CLmethod); //same

    Lmethod->set_sensitive (false);
    Lmethod->set_sensitive (false);
    Lmethod->append (M ("TP_WAVELET_1"));
    Lmethod->append (M ("TP_WAVELET_2"));
    Lmethod->append (M ("TP_WAVELET_3"));
    Lmethod->append (M ("TP_WAVELET_4"));
    Lmethod->append (M ("TP_WAVELET_5"));
    Lmethod->append (M ("TP_WAVELET_6"));
    Lmethod->append (M ("TP_WAVELET_7"));
    Lmethod->append (M ("TP_WAVELET_8"));
    Lmethod->append (M ("TP_WAVELET_9"));
    Lmethod->append (M ("TP_WAVELET_SUPE"));
    Lmethod->append (M ("TP_WAVELET_RESID"));
    Lmethod->set_active (0);
    Dirmethod->set_sensitive (false);
    Dirmethod->append (M ("TP_WAVELET_DONE"));
    Dirmethod->append (M ("TP_WAVELET_DTWO"));
    Dirmethod->append (M ("TP_WAVELET_DTHR"));
    Dirmethod->append (M ("TP_WAVELET_DALL"));
    Lmethodconn = Lmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::LmethodChanged) );
    Dirmethodconn = Dirmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::DirmethodChanged) );
    Gtk::HBox* const levdirSubHBox = Gtk::manage (new Gtk::HBox());
    levdirSubHBox->pack_start (*Lmethod);
    levdirSubHBox->pack_start (*Dirmethod, Gtk::PACK_EXPAND_WIDGET, 2); // same, but 2 not 4?

    settingsBox->pack_start (*strength);
    settingsBox->pack_start (*thres);
    settingsBox->pack_start (*tilesizeHBox);
    settingsBox->pack_start (*daubcoeffHBox);
    settingsBox->pack_start (*backgroundHBox);
    settingsBox->pack_start (*levdirMainHBox);
    settingsBox->pack_start (*levdirSubHBox);

// Contrast
    ToolParamBlock* const levBox = Gtk::manage (new ToolParamBlock());

    Gtk::HBox* const buttonBox = Gtk::manage (new Gtk::HBox (true, 10));
    levBox->pack_start (*buttonBox, Gtk::PACK_SHRINK, 2);

    Gtk::Button* const contrastMinusButton = Gtk::manage (new Gtk::Button (M ("TP_WAVELET_CONTRAST_MINUS")));
    buttonBox->pack_start (*contrastMinusButton);
    contrastMinusPressedConn = contrastMinusButton->signal_pressed().connect ( sigc::mem_fun (*this, &Wavelet::contrastMinusPressed));

    Gtk::Button* const neutralButton = Gtk::manage (new Gtk::Button (M ("TP_WAVELET_NEUTRAL")));
    buttonBox->pack_start (*neutralButton);
    neutralPressedConn = neutralButton->signal_pressed().connect ( sigc::mem_fun (*this, &Wavelet::neutralPressed));

    Gtk::Button* const contrastPlusButton = Gtk::manage (new Gtk::Button (M ("TP_WAVELET_CONTRAST_PLUS")));
    buttonBox->pack_start (*contrastPlusButton);
    contrastPlusPressedConn = contrastPlusButton->signal_pressed().connect ( sigc::mem_fun (*this, &Wavelet::contrastPlusPressed));

    buttonBox->show_all_children();

    for (int i = 0; i < 9; i++) {
        Glib::ustring ss;

        switch ( i ) {
            case 0:
                ss = Glib::ustring::compose ( "%1 (%2)", (i + 1), M ("TP_WAVELET_FINEST"));
                break;

            case 8:
                ss = Glib::ustring::compose ( "%1 (%2)", (i + 1), M ("TP_WAVELET_LARGEST"));
                break;

            default:
                ss = Glib::ustring::compose ( "%1", (i + 1));
        }

        correction[i] = Gtk::manage ( new Adjuster (ss, -100, 350, 1, 0) );
        correction[i]->setAdjusterListener (this);
        levBox->pack_start (*correction[i]);
    }

    levBox->pack_start (*sup);
    sup->setAdjusterListener (this);
    wavLabels->show();
    levBox->pack_start (*wavLabels);

    Gtk::VBox* const contrastSHVBox = Gtk::manage (new Gtk::VBox);
    contrastSHVBox->set_spacing (2);

    HSmethod->append (M ("TP_WAVELET_HS1"));
    HSmethod->append (M ("TP_WAVELET_HS2"));
    HSmethodconn = HSmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::HSmethodChanged) );

    const std::vector<GradientMilestone> milestones2 = {
        GradientMilestone (0.0, 0.0, 0.0, 0.0),
        GradientMilestone (1.0, 1.0, 1.0, 1.0)
    };

    hllev->setAdjusterListener (this);
    hllev->setBgGradient (milestones2);

    threshold->setAdjusterListener (this);
    threshold->set_tooltip_text (M ("TP_WAVELET_THRESHOLD_TOOLTIP"));

    bllev->setAdjusterListener (this);
    bllev->setBgGradient (milestones2);

    threshold2->setAdjusterListener (this);
    threshold2->set_tooltip_text (M ("TP_WAVELET_THRESHOLD2_TOOLTIP"));

    contrastSHVBox->pack_start (*HSmethod);
    contrastSHVBox->pack_start (*hllev);
    contrastSHVBox->pack_start (*threshold);
    contrastSHVBox->pack_start (*bllev);
    contrastSHVBox->pack_start (*threshold2);
    Gtk::Frame* const contrastSHFrame = Gtk::manage (new Gtk::Frame (M ("TP_WAVELET_APPLYTO")));
    contrastSHFrame->add (*contrastSHVBox);
    levBox->pack_start (*contrastSHFrame);

// Chromaticity
    ToolParamBlock* const chBox = Gtk::manage (new ToolParamBlock());

    Gtk::Label* const labmch = Gtk::manage (new Gtk::Label (M ("TP_WAVELET_CHTYPE") + ":"));
    Gtk::HBox* const ctboxch = Gtk::manage (new Gtk::HBox());
    ctboxch->pack_start (*labmch, Gtk::PACK_SHRINK, 1);

    CHmethod->append (M ("TP_WAVELET_CH1"));
    CHmethod->append (M ("TP_WAVELET_CH2"));
    CHmethod->append (M ("TP_WAVELET_CH3"));
    CHmethodconn = CHmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::CHmethodChanged) );
    ctboxch->pack_start (*CHmethod);
    chBox->pack_start (*ctboxch);

    Gtk::HBox* const ctboxCH = Gtk::manage (new Gtk::HBox());
    ctboxCH->pack_start (*labmC, Gtk::PACK_SHRINK, 1);

    CHSLmethod->append (M ("TP_WAVELET_CHSL"));
    CHSLmethod->append (M ("TP_WAVELET_CHCU"));
    CHSLmethodconn = CHSLmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::CHSLmethodChanged) );
    ctboxCH->pack_start (*CHSLmethod);

    Gtk::HSeparator* const separatorChromaMethod = Gtk::manage (new Gtk::HSeparator());
    chBox->pack_start (*separatorChromaMethod, Gtk::PACK_SHRINK, 2);

    chroma->set_tooltip_text (M ("TP_WAVELET_CHRO_TOOLTIP"));
    chBox->pack_start (*chroma);
    chroma->setAdjusterListener (this);

    satlev->setAdjusterListener (this);
    satlev->setBgGradient (milestones2);

    pastlev->setAdjusterListener (this);
    pastlev->setBgGradient (milestones2);

    chBox->pack_start (*pastlev);
    chBox->pack_start (*satlev);

    chro->set_tooltip_text (M ("TP_WAVELET_CHR_TOOLTIP"));
    chBox->pack_start (*chro);
    chro->setAdjusterListener (this);

    Gtk::HBox* const buttonchBox = Gtk::manage (new Gtk::HBox (true, 10));
    neutralchPressedConn = neutralchButton->signal_pressed().connect ( sigc::mem_fun (*this, &Wavelet::neutralchPressed));
    chBox->pack_start (*separatorNeutral, Gtk::PACK_SHRINK, 2);
    buttonchBox->pack_start (*neutralchButton);
    buttonchBox->show_all_children();
    chBox->pack_start (*buttonchBox, Gtk::PACK_SHRINK, 2);

    for (int i = 0; i < 9; i++) {
        Glib::ustring ss;

        switch ( i ) {
            case 0:
                ss = Glib::ustring::compose ( "%1 (%2)", (i + 1), M ("TP_WAVELET_FINEST"));
                break;

            case 8:
                ss = Glib::ustring::compose ( "%1 (%2)", (i + 1), M ("TP_WAVELET_LARGEST"));
                break;

            default:
                ss = Glib::ustring::compose ( "%1", (i + 1));
        }

        correctionch[i] = Gtk::manage ( new Adjuster (ss, -100, 100, 1, 0) );
        correctionch[i]->setAdjusterListener (this);
        chBox->pack_start (*correctionch[i]);
    }

// Toning
    ToolParamBlock* const tonBox = Gtk::manage (new ToolParamBlock());

    opaCurveEditorG->setCurveListener (this);

    // std::vector<double> defaultCurve;

    rtengine::WaveletParams::getDefaultOpacityCurveRG (defaultCurve);
    opacityShapeRG = static_cast<FlatCurveEditor*> (opaCurveEditorG->addCurve (CT_Flat, "", nullptr, false, false));
    opacityShapeRG->setIdentityValue (0.);
    opacityShapeRG->setResetCurve (FlatCurveType (defaultCurve.at (0)), defaultCurve);

    opaCurveEditorG->curveListComplete();
    opaCurveEditorG->show();

    tonBox->pack_start ( *opaCurveEditorG, Gtk::PACK_SHRINK, 2);

    opacityCurveEditorG->setCurveListener (this);

    rtengine::WaveletParams::getDefaultOpacityCurveBY (defaultCurve);
    opacityShapeBY = static_cast<FlatCurveEditor*> (opacityCurveEditorG->addCurve (CT_Flat, "", nullptr, false, false));
    opacityShapeBY->setIdentityValue (0.);
    opacityShapeBY->setResetCurve (FlatCurveType (defaultCurve.at (0)), defaultCurve);

    opacityCurveEditorG->curveListComplete();
    opacityCurveEditorG->show();

    tonBox->pack_start ( *opacityCurveEditorG, Gtk::PACK_SHRINK, 2);

// Denoise and Refine
    ToolParamBlock* const noiseBox = Gtk::manage (new ToolParamBlock());

    linkedg->set_active (true);
    linkedgConn = linkedg->signal_toggled().connect ( sigc::mem_fun (*this, &Wavelet::linkedgToggled) );
    noiseBox->pack_start (*linkedg);

    level0noise->setAdjusterListener (this);
    level0noise->setUpdatePolicy (RTUP_DYNAMIC);

    level1noise->setAdjusterListener (this);
    level1noise->setUpdatePolicy (RTUP_DYNAMIC);

    level2noise->setAdjusterListener (this);
    level2noise->setUpdatePolicy (RTUP_DYNAMIC);

    level3noise->setAdjusterListener (this);
    level3noise->setUpdatePolicy (RTUP_DYNAMIC);

    noiseBox->pack_start ( *level0noise, Gtk::PACK_SHRINK, 0);
    noiseBox->pack_start ( *level1noise, Gtk::PACK_SHRINK, 0);
    noiseBox->pack_start ( *level2noise, Gtk::PACK_SHRINK, 0);
    noiseBox->pack_start ( *level3noise, Gtk::PACK_SHRINK, 0);

// Edge Sharpness
    ToolParamBlock* const edgBox = Gtk::manage (new ToolParamBlock());
//    ToolParamBlock* const edgBoxM = Gtk::manage (new ToolParamBlock());

    edgval->setAdjusterListener (this);
    edgBox->pack_start (*edgval);

    edgrad->setAdjusterListener (this);
    edgBox->pack_start (*edgrad);
    edgrad->set_tooltip_markup (M ("TP_WAVELET_EDRAD_TOOLTIP"));

    edgthresh->setAdjusterListener (this);
    edgthresh->set_tooltip_markup (M ("TP_WAVELET_EDGTHRESH_TOOLTIP"));
    edgBox->pack_start (*edgthresh);

    Gtk::Label* const labmedgr = Gtk::manage (new Gtk::Label (M ("TP_WAVELET_MEDGREINF") + ":"));
    Gtk::HBox* const edbox = Gtk::manage (new Gtk::HBox());
    edbox->pack_start (*labmedgr, Gtk::PACK_SHRINK, 1);

    Medgreinf->append (M ("TP_WAVELET_RE1"));
    Medgreinf->append (M ("TP_WAVELET_RE2"));
    Medgreinf->append (M ("TP_WAVELET_RE3"));
    MedgreinfConn = Medgreinf->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::MedgreinfChanged) );
    Medgreinf->set_tooltip_markup (M ("TP_WAVELET_EDGREINF_TOOLTIP"));
    edbox->pack_start (*Medgreinf);
    edgBox->pack_start (*edbox);

    Gtk::HSeparator* const separatorlc = Gtk::manage (new  Gtk::HSeparator());
    edgBox->pack_start (*separatorlc, Gtk::PACK_SHRINK, 2);

    Gtk::Label* const labmED = Gtk::manage (new Gtk::Label (M ("TP_WAVELET_EDTYPE") + ":"));
    Gtk::HBox* const ctboxED = Gtk::manage (new Gtk::HBox());
    ctboxED->pack_start (*labmED, Gtk::PACK_SHRINK, 1);

    EDmethod->append (M ("TP_WAVELET_EDSL"));
    EDmethod->append (M ("TP_WAVELET_EDCU"));
    EDmethodconn = EDmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::EDmethodChanged) );
    ctboxED->pack_start (*EDmethod);
    edgBox->pack_start (*ctboxED);

    edgcont->setAdjusterListener (this);
    edgcont->setBgGradient (milestones2);
    edgcont->set_tooltip_markup (M ("TP_WAVELET_EDGCONT_TOOLTIP"));


    // <-- Edge Sharpness  Local Contrast curve
    CCWcurveEditorG->setCurveListener (this);

    rtengine::WaveletParams::getDefaultCCWCurve (defaultCurve);
    ccshape = static_cast<FlatCurveEditor*> (CCWcurveEditorG->addCurve (CT_Flat, "", nullptr, false, false));

    ccshape->setIdentityValue (0.);
    ccshape->setResetCurve (FlatCurveType (defaultCurve.at (0)), defaultCurve);
    ccshape->setTooltip (M ("TP_WAVELET_CURVEEDITOR_CC_TOOLTIP"));

    CCWcurveEditorG->curveListComplete();
    CCWcurveEditorG->show();
    // -->

    edgBox->pack_start (*edgcont);
    edgBox->pack_start (*CCWcurveEditorG, Gtk::PACK_SHRINK, 4);

    medianlev->set_active (true);
    medianlevConn = medianlev->signal_toggled().connect ( sigc::mem_fun (*this, &Wavelet::medianlevToggled) );
    medianlev->set_tooltip_text (M ("TP_WAVELET_MEDILEV_TOOLTIP"));

    Gtk::HSeparator* const separatored1 = Gtk::manage (new  Gtk::HSeparator());
    edgBox->pack_start (*separatored1, Gtk::PACK_SHRINK, 2);

    Gtk::HBox* const eddebox = Gtk::manage (new Gtk::HBox());
    edgBox->pack_start (*eddebox);
    edgBox->pack_start (*medianlev);

    edgedetect->setAdjusterListener (this);
    edgedetect->set_tooltip_text (M ("TP_WAVELET_EDGEDETECT_TOOLTIP"));
    edgBox->pack_start (*edgedetect);

    edgedetectthr->setAdjusterListener (this);
    edgedetectthr->set_tooltip_text (M ("TP_WAVELET_EDGEDETECTTHR_TOOLTIP"));
    edgBox->pack_start (*edgedetectthr);

    edgedetectthr2->setAdjusterListener (this);
    edgBox->pack_start (*edgedetectthr2);

    edgBox->pack_start (*separatoredge, Gtk::PACK_SHRINK, 2);

    lipst->set_active (true);
    lipstConn = lipst->signal_toggled().connect ( sigc::mem_fun (*this, &Wavelet::lipstToggled) );
//  lipst->set_tooltip_text (M("TP_WAVELET_LIPST_TOOLTIP"));
    edgBox->pack_start (*lipst);

    edgesensi->setAdjusterListener (this);
    edgBox->pack_start (*edgesensi);

    edgeampli->setAdjusterListener (this);
    edgBox->pack_start (*edgeampli);

    Gtk::VBox* const ctboxES = Gtk::manage (new Gtk::VBox());

    ctboxES->set_spacing (2);

    Gtk::HBox* const ctboxNP = Gtk::manage (new Gtk::HBox());
    ctboxNP->pack_start (*labmNP, Gtk::PACK_SHRINK, 1);

    NPmethod->append (M ("TP_WAVELET_NPNONE"));
    NPmethod->append (M ("TP_WAVELET_NPLOW"));
    NPmethod->append (M ("TP_WAVELET_NPHIGH"));
    NPmethodconn = NPmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::NPmethodChanged) );
    NPmethod->set_tooltip_text (M ("TP_WAVELET_NPTYPE_TOOLTIP"));

    ctboxNP->pack_start (*NPmethod);
    ctboxES->pack_start (*ctboxNP);

    edgBox->pack_start (*ctboxES);


    ushamethod->append (M ("TP_WAVELET_USH"));
    ushamethod->append (M ("TP_WAVELET_SHA"));
    ushamethod->append (M ("TP_WAVELET_CLA"));
    ushamethodconn = ushamethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::ushamethodChanged) );
    ushamethod->set_tooltip_text (M ("TP_WAVELET_USH_TOOLTIP"));

    usharpmethod->append (M ("TP_WAVELET_USHARORIG"));
    usharpmethod->append (M ("TP_WAVELET_USHARWAVE"));
    usharpmethodconn = usharpmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::usharpmethodChanged) );
    usharpmethod->set_tooltip_text (M ("TP_WAVELET_USHARP_TOOLTIP"));


    usharpHBox->pack_start (*usharpLabel, Gtk::PACK_SHRINK, 0);
    usharpHBox->pack_start (*ushamethod);
//   usharpHBox->pack_start (*usharpmethod);

    mergeL->setAdjusterListener (this);
    mergeC->setAdjusterListener (this);

    ToolParamBlock* const edgBoxS = Gtk::manage (new ToolParamBlock());

    edgBoxS->pack_start (*usharpHBox);
    edgBoxS->pack_start (*mergeL);
    edgBoxS->pack_start (*mergeC);
    nextmergeL = mergeL->getValue();
    nextmergeC = mergeC->getValue();


//Retinex in Wavelet
    Gtk::VBox * retiBox = Gtk::manage (new Gtk::VBox());
    retiBox->set_border_width (4);
    retiBox->set_spacing (2);

    Gtk::VBox * retiBox2 = Gtk::manage (new Gtk::VBox());
    retiBox2->set_border_width (1);
    retiBox2->set_spacing (1);

    dhbox->pack_start (*labmdh, Gtk::PACK_SHRINK, 1);

    retinexMethod->append (M ("TP_RETINEX_LOW"));
    retinexMethod->append (M ("TP_RETINEX_UNIFORM"));
    retinexMethod->append (M ("TP_RETINEX_HIGH"));
    retinexMethod->set_active (0);
    retinexMethodConn = retinexMethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::retinexMethodChanged) );
    retinexMethod->set_tooltip_markup (M ("TP_WAVRETI_METHOD_TOOLTIP"));

    dhboxpro->pack_start (*labmdhpro, Gtk::PACK_SHRINK, 1);

    retinexMethodpro = Gtk::manage (new MyComboBoxText ());
    retinexMethodpro->append (M ("TP_WAVE_RESIDU"));
    retinexMethodpro->append (M ("TP_WAVE_FINA"));
    retinexMethodpro->set_active (0);
    retinexMethodproConn = retinexMethodpro->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::retinexMethodproChanged) );
    retinexMethodpro->set_tooltip_markup (M ("TP_WAVRETI_METHODPRO_TOOLTIP"));


    mMLabels->set_tooltip_markup (M ("TP_RETINEX_MLABEL_TOOLTIP"));

    transLabels->set_tooltip_markup (M ("TP_RETINEX_TLABEL_TOOLTIP"));

    gain->setAdjusterListener (this);
    offs->setAdjusterListener (this);
    str->setAdjusterListener (this);
    neigh->setAdjusterListener (this);
    vart->setAdjusterListener (this);
    scale->setAdjusterListener (this);


    limd->setAdjusterListener (this);
    chrrt->setAdjusterListener (this);

    // <Retinex Transmission curve
    CCWcurveEditorT->setCurveListener (this);

    rtengine::WaveletParams::getDefaultCCWCurveT (defaultCurve);
    cTshape = static_cast<FlatCurveEditor*> (CCWcurveEditorT->addCurve (CT_Flat, "", nullptr, false, false));

    cTshape->setIdentityValue (0.);
    cTshape->setResetCurve (FlatCurveType (defaultCurve.at (0)), defaultCurve);
    cTshape->setTooltip (M ("TP_RETINEX_TRANSMISSION_TOOLTIP"));

    CCWcurveEditorT->curveListComplete();
    CCWcurveEditorT->show();
    //  Gtk::HSeparator *separatorreti = Gtk::manage (new Gtk::HSeparator());

    // <Retinex Transmission gain curve
    CCWcurveEditorgainT->setCurveListener (this);

    rtengine::WaveletParams::getDefaultCCWgainCurveT (defaultCurve);
    cTgainshape = static_cast<FlatCurveEditor*> (CCWcurveEditorgainT->addCurve (CT_Flat, "", nullptr, false, false));

    cTgainshape->setIdentityValue (0.);
    cTgainshape->setResetCurve (FlatCurveType (defaultCurve.at (0)), defaultCurve);
    cTgainshape->setTooltip (M ("TP_RETINEX_TRANSMISSIONGAIN_TOOLTIP"));

    CCWcurveEditorgainT->curveListComplete();
    CCWcurveEditorgainT->show();

    // retiBox->pack_start(*separatorRT, Gtk::PACK_SHRINK);

    dhbox->pack_start (*retinexMethod);
    retiBox->pack_start (*dhbox);
    dhboxpro->pack_start (*retinexMethodpro);
    retiBox->pack_start (*dhboxpro);

    retiBox->pack_start (*mMLabels);
    mMLabels->show ();

    retiBox->pack_start (*transLabels);
    transLabels->show ();

    retiBox->pack_start (*transLabels2);
    transLabels2->show ();

    retiBox->pack_start (*str);
    retiBox->pack_start (*chrrt);
    retiBox->pack_start (*neigh);
    retiBox->pack_start (*vart);

    Gtk::VBox *gainVBox = Gtk::manage (new Gtk::VBox());

//    gainVBox->pack_start(*gain);
    gainVBox->pack_start (*CCWcurveEditorgainT, Gtk::PACK_SHRINK, 4);

    gainVBox->pack_start (*offs);
    gainFrame->add (*gainVBox);
    retiBox2->pack_start (*gainFrame);

    Gtk::VBox *tranVBox = Gtk::manage (new Gtk::VBox());

    tranVBox->pack_start (*CCWcurveEditorT, Gtk::PACK_SHRINK, 4);
    tranVBox->pack_start (*scale);
    tranVBox->pack_start (*limd);
    tranFrame->add (*tranVBox);
    retiBox2->pack_start (*tranFrame);

    Gtk::VBox *gaussVBox = Gtk::manage (new Gtk::VBox());

    gaussVBox->pack_start (*highlights);
    gaussVBox->pack_start (*h_tonalwidth);
    gaussVBox->pack_start (*shadows);
    gaussVBox->pack_start (*s_tonalwidth);
    gaussVBox->pack_start (*radius);
    gaussFrame->add (*gaussVBox);

    retiBox2->pack_start (*gaussFrame);

    highlights->setAdjusterListener (this);
    h_tonalwidth->setAdjusterListener (this);
    shadows->setAdjusterListener (this);
    s_tonalwidth->setAdjusterListener (this);
    radius->setAdjusterListener (this);


    expsettingreti->add (*retiBox2);
    retiBox->pack_start (*expsettingreti);

// Gamut
    ToolParamBlock* const conBox = Gtk::manage (new ToolParamBlock());

    median->set_active (true);
    medianConn = median->signal_toggled().connect ( sigc::mem_fun (*this, &Wavelet::medianToggled) );
    conBox->pack_start (*median);

    hueskin->set_tooltip_markup (M ("TP_WAVELET_HUESKIN_TOOLTIP"));

    //from -PI to +PI (radians) convert to hsv and draw bottombar
    const std::vector<GradientMilestone> milestones = {
        makeHsvGm (0.0000, 0.4199f, 0.5f, 0.5f), // hsv: 0.4199 rad: -3.14
        makeHsvGm (0.0540, 0.5000f, 0.5f, 0.5f), // hsv: 0.5    rad: -2.8
        makeHsvGm (0.1336, 0.6000f, 0.5f, 0.5f), // hsv: 0.60   rad: -2.3
        makeHsvGm (0.3567, 0.7500f, 0.5f, 0.5f), // hsv: 0.75   rad: -0.9
        makeHsvGm (0.4363, 0.8560f, 0.5f, 0.5f), // hsv: 0.856  rad: -0.4
        makeHsvGm (0.4841, 0.9200f, 0.5f, 0.5f), // hsv: 0.92   rad: -0.1
        makeHsvGm (0.5000, 0.9300f, 0.5f, 0.5f), // hsv: 0.93   rad:  0
        makeHsvGm (0.5366, 0.9600f, 0.5f, 0.5f), // hsv: 0.96   rad:  0.25
        makeHsvGm (0.5955, 1.0000f, 0.5f, 0.5f), // hsv: 1.     rad:  0.6
        makeHsvGm (0.6911, 0.0675f, 0.5f, 0.5f), // hsv: 0.0675 rad:  1.2
        makeHsvGm (0.7229, 0.0900f, 0.5f, 0.5f), // hsv: 0.09   rad:  1.4
        makeHsvGm (0.7707, 0.1700f, 0.5f, 0.5f), // hsv: 0.17   rad:  1.7
        makeHsvGm (0.8503, 0.2650f, 0.5f, 0.5f), // hsv: 0.265  rad:  2.1
        makeHsvGm (0.8981, 0.3240f, 0.5f, 0.5f), // hsv: 0.324  rad:  2.5
        makeHsvGm (1.0000, 0.4197f, 0.5f, 0.5f) // hsv: 0.419  rad:  3.14
    };

    hueskin->setBgGradient (milestones);
    conBox->pack_start (*hueskin);
    hueskin->setAdjusterListener (this);

    skinprotect->setAdjusterListener (this);
    conBox->pack_start (*skinprotect);
    skinprotect->set_tooltip_markup (M ("TP_WAVELET_SKIN_TOOLTIP"));

    curveEditorGAM->setCurveListener (this);

    Chshape = static_cast<FlatCurveEditor*> (curveEditorGAM->addCurve (CT_Flat, M ("TP_WAVELET_CURVEEDITOR_CH")));
    Chshape->setTooltip (M ("TP_WAVELET_CURVEEDITOR_CH_TOOLTIP"));
    Chshape->setCurveColorProvider (this, 5);
    curveEditorGAM->curveListComplete();
    Chshape->setBottomBarBgGradient (milestones);

    conBox->pack_start (*curveEditorGAM, Gtk::PACK_SHRINK, 4);

    avoid->set_active (true);
    avoidConn = avoid->signal_toggled().connect ( sigc::mem_fun (*this, &Wavelet::avoidToggled) );
    conBox->pack_start (*avoid);

// Residual Image
    ToolParamBlock* const resBox = Gtk::manage (new ToolParamBlock());
    /*
        Gtk::VBox * resBox2 = Gtk::manage (new Gtk::VBox());
        resBox2->set_border_width (1);
        resBox2->set_spacing (1);
    */
    rescon->setAdjusterListener (this);
    resBox->pack_start (*rescon, Gtk::PACK_SHRINK);

    resBox->pack_start (*thr);
    thr->setAdjusterListener (this);

    resconH->setAdjusterListener (this);
    resBox->pack_start (*resconH, Gtk::PACK_SHRINK);

    thrH->setAdjusterListener (this);
    resBox->pack_start (*thrH, Gtk::PACK_SHRINK);

    contrast->set_tooltip_text (M ("TP_WAVELET_CONTRA_TOOLTIP"));
    contrast->setAdjusterListener (this);
    resBox->pack_start (*contrast); //keep the possibility to reinstall

    reschro->setAdjusterListener (this);
    resBox->pack_start (*reschro);


    Gtk::Label* const labmTM = Gtk::manage (new Gtk::Label (M ("TP_WAVELET_TMTYPE") + ":"));
    Gtk::HBox* const ctboxTM = Gtk::manage (new Gtk::HBox());
    ctboxTM->pack_start (*labmTM, Gtk::PACK_SHRINK, 1);

    Gtk::HSeparator* const separatorR0 = Gtk::manage (new  Gtk::HSeparator());
    resBox->pack_start (*separatorR0, Gtk::PACK_SHRINK, 2);

    TMmethod->append (M ("TP_WAVELET_COMPCONT"));
    TMmethod->append (M ("TP_WAVELET_COMPTM"));
    TMmethodconn = TMmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::TMmethodChanged) );
    ctboxTM->pack_start (*TMmethod);
    resBox->pack_start (*ctboxTM);

    tmrs->set_tooltip_text (M ("TP_WAVELET_TMSTRENGTH_TOOLTIP"));

    resBox->pack_start (*tmrs);
    tmrs->setAdjusterListener (this);

    gamma->set_tooltip_text (M ("TP_WAVELET_COMPGAMMA_TOOLTIP"));
    resBox->pack_start (*gamma);
    gamma->setAdjusterListener (this);

    Gtk::HSeparator* const separatorR1 = Gtk::manage (new  Gtk::HSeparator());
    resBox->pack_start (*separatorR1, Gtk::PACK_SHRINK, 2);

    hueskin2->set_tooltip_markup (M ("TP_WAVELET_HUESKY_TOOLTIP"));
    hueskin2->setBgGradient (milestones);
    resBox->pack_start (*hueskin2);
    hueskin2->setAdjusterListener (this);

    sky->set_tooltip_text (M ("TP_WAVELET_SKY_TOOLTIP"));
    sky->setAdjusterListener (this);

    resBox->pack_start (*sky);

    // whole hue range
    const std::vector<GradientMilestone> milestones3 = makeWholeHueRange();

    curveEditorRES->setCurveListener (this);

    hhshape = static_cast<FlatCurveEditor*> (curveEditorRES->addCurve (CT_Flat, M ("TP_WAVELET_CURVEEDITOR_HH")));
    hhshape->setTooltip (M ("TP_WAVELET_CURVEEDITOR_HH_TOOLTIP"));
    hhshape->setCurveColorProvider (this, 5);
    curveEditorRES->curveListComplete();
    hhshape->setBottomBarBgGradient (milestones3);

    resBox->pack_start (*curveEditorRES, Gtk::PACK_SHRINK, 4);

    // Toning and Color Balance
    Gtk::HSeparator* const separatorCB = Gtk::manage (new  Gtk::HSeparator());

    Gtk::VBox* const chanMixerHLBox = Gtk::manage (new Gtk::VBox());
    Gtk::VBox* const chanMixerMidBox = Gtk::manage (new Gtk::VBox());
    Gtk::VBox* const chanMixerShadowsBox = Gtk::manage (new Gtk::VBox());



    cbenab->set_active (true);
    cbenabConn = cbenab->signal_toggled().connect ( sigc::mem_fun (*this, &Wavelet::cbenabToggled) );
    cbenab->set_tooltip_text (M ("TP_WAVELET_CB_TOOLTIP"));

    Gtk::Image* const iblueR   = Gtk::manage (new RTImage ("circle-blue-small.png"));
    Gtk::Image* const iyelL    = Gtk::manage (new RTImage ("circle-yellow-small.png"));
    Gtk::Image* const imagL    = Gtk::manage (new RTImage ("circle-magenta-small.png"));
    Gtk::Image* const igreenR  = Gtk::manage (new RTImage ("circle-green-small.png"));

    Gtk::Image* const  iblueRm  = Gtk::manage (new RTImage ("circle-blue-small.png"));
    Gtk::Image* const  iyelLm   = Gtk::manage (new RTImage ("circle-yellow-small.png"));
    Gtk::Image* const  imagLm   = Gtk::manage (new RTImage ("circle-magenta-small.png"));
    Gtk::Image* const  igreenRm = Gtk::manage (new RTImage ("circle-green-small.png"));

    Gtk::Image* const iblueRh  = Gtk::manage (new RTImage ("circle-blue-small.png"));
    Gtk::Image* const iyelLh   = Gtk::manage (new RTImage ("circle-yellow-small.png"));
    Gtk::Image* const imagLh   = Gtk::manage (new RTImage ("circle-magenta-small.png"));
    Gtk::Image* const igreenRh = Gtk::manage (new RTImage ("circle-green-small.png"));

    greenhigh = Gtk::manage (new Adjuster ("", -100., 100., 1., 0., igreenRh, imagLh));
    bluehigh = Gtk::manage (new Adjuster ("", -100., 100., 1., 0., iblueRh, iyelLh));
    greenmed = Gtk::manage (new Adjuster ("", -100., 100., 1., 0.,  igreenRm, imagLm));
    bluemed = Gtk::manage (new Adjuster ("", -100., 100., 1., 0. , iblueRm , iyelLm));
    greenlow = Gtk::manage (new Adjuster ("", -100., 100., 1., 0.,  igreenR, imagL));
    bluelow = Gtk::manage (new Adjuster ("", -100., 100., 1., 0., iblueR , iyelL));

    chanMixerHLBox->pack_start (*greenhigh);
    chanMixerHLBox->pack_start (*bluehigh);
    chanMixerMidBox->pack_start (*greenmed);
    chanMixerMidBox->pack_start (*bluemed);
    chanMixerShadowsBox->pack_start (*greenlow);
    chanMixerShadowsBox->pack_start (*bluelow);

    greenlow->setAdjusterListener (this);
    bluelow->setAdjusterListener (this);
    greenmed->setAdjusterListener (this);
    bluemed->setAdjusterListener (this);
    greenhigh->setAdjusterListener (this);
    bluehigh->setAdjusterListener (this);

    resBox->pack_start (*separatorCB, Gtk::PACK_SHRINK);

    chanMixerHLFrame->add (*chanMixerHLBox);
    chanMixerMidFrame->add (*chanMixerMidBox);
    chanMixerShadowsFrame->add (*chanMixerShadowsBox);
    resBox->pack_start (*cbenab);

    resBox->pack_start (*chanMixerHLFrame, Gtk::PACK_SHRINK);
    resBox->pack_start (*chanMixerMidFrame, Gtk::PACK_SHRINK);
    resBox->pack_start (*chanMixerShadowsFrame, Gtk::PACK_SHRINK);


    //RTImage *resetImg = Gtk::manage (new RTImage ("undo-small.png", "redo-small.png"));
    //neutral->set_image(*resetImg);
    Gtk::Button* const neutral = Gtk::manage (new Gtk::Button (M ("TP_COLORTONING_NEUTRAL")));
    neutral->set_tooltip_text (M ("TP_COLORTONING_NEUTRAL_TIP"));
    neutralconn = neutral->signal_pressed().connect ( sigc::mem_fun (*this, &Wavelet::neutral_pressed) );
    neutral->show();
    neutrHBox->pack_start (*neutral, Gtk::PACK_EXPAND_WIDGET);

    resBox->pack_start (*neutrHBox);

    // expTCresi->add (*resBox2);
    // resBox->pack_start (*expTCresi);

// Merge files
    Gtk::VBox * mergeBox = Gtk::manage (new Gtk::VBox());
    mergeBox->set_border_width (4);
    mergeBox->set_spacing (2);
    oldip = "";

    mg1box->pack_start (*labmmg1, Gtk::PACK_SHRINK, 1);

    mergMethod->append (M ("TP_WAVELET_SAVE_WATER"));
    mergMethod->append (M ("TP_WAVELET_SAVE_HDR"));
    mergMethod->append (M ("TP_WAVELET_SAVE_ZERO"));
    mergMethod->append (M ("TP_WAVELET_LOAD"));
    mergMethod->append (M ("TP_WAVELET_LOAD_ZERO"));
    mergMethod->append (M ("TP_WAVELET_LOAD_ZEROHDR"));

    mergMethod->set_active (0);
    mergMethodConn = mergMethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::mergMethodChanged) );
    mergMethod->set_tooltip_markup (M ("TP_WAVELET_VIEW_TOOLTIP"));
    mg1box->pack_start (*mergMethod);
    mergeBox->pack_start (*mg1box);

    mg2box->pack_start (*labmmg2, Gtk::PACK_SHRINK, 1);

    mergMethod2->append (M ("TP_WAVELET_ZERO_BEF"));
    mergMethod2->append (M ("TP_WAVELET_ZERO_AFT"));
    mergMethod2->set_active (0);
    mergMethod2Conn = mergMethod2->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::mergMethod2Changed) );
    mergMethod2->set_tooltip_markup (M ("TP_WAVELET_MERGMET2_TOOLTIP"));
    mg2box->pack_start (*mergMethod2);
    mergeBox->pack_start (*mg2box);


    hbin->set_spacing (2);

    inputeFile = Gtk::manage (new MyFileChooserButton (M ("TP_WAVELET_MERGELABEL"), Gtk::FILE_CHOOSER_ACTION_OPEN));
    inputeFile->set_tooltip_text (M ("TP_WAVELET_INPUTE_TOOLTIP"));

    bindCurrentFolder (*inputeFile, options.lastmergeDir);
    mFileReset = Gtk::manage (new Gtk::Button());
    mFileReset->set_image (*Gtk::manage (new RTImage ("cancel-small.png")));

    hbin->pack_start (*inLabel, Gtk::PACK_SHRINK, 0);
    hbin->pack_start (*inputeFile);
    hbin->pack_start (*mFileReset, Gtk::PACK_SHRINK, 0);
    mergeBox->pack_start (*hbin);

    inFile = inputeFile->signal_file_set().connect ( sigc::mem_fun (*this, &Wavelet::inputeChanged));//, true);
    mFileReset->signal_clicked().connect ( sigc::mem_fun (*this, &Wavelet::mFile_Reset), true );

    // Set filename filters
    b_filter_asCurrent = false;
    Glib::RefPtr<Gtk::FileFilter> filter_merge = Gtk::FileFilter::create();

    filter_merge->add_pattern ("*.mer");
    filter_merge->set_name (M ("FILECHOOSER_FILTER_MERGE"));
    inputeFile->add_filter (filter_merge);

    oldip = "";

    mgbox->pack_start (*labmmg, Gtk::PACK_SHRINK, 1);

    mergevMethod->append (M ("TP_WAVELET_VIEW_CURR"));
    mergevMethod->append (M ("TP_WAVELET_VIEW_CUNO"));
    mergevMethod->append (M ("TP_WAVELET_VIEW_PREV"));
    mergevMethod->append (M ("TP_WAVELET_VIEW_SAVE"));
    mergevMethod->set_active (0);
    mergevMethodConn = mergevMethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::mergevMethodChanged) );
    mergevMethod->set_tooltip_markup (M ("TP_WAVELET_VIEW2_TOOLTIP"));
    mgbox->pack_start (*mergevMethod);
    mergeBox->pack_start (*mgbox);

    mgVBox->set_border_width (1);
    mgVBox->set_spacing (1);

    mgBbox->pack_start (*labmmgB, Gtk::PACK_SHRINK, 1);

    mergBMethod->append (M ("TP_WAVELET_BLEND_HDR1"));
    mergBMethod->append (M ("TP_WAVELET_BLEND_HDR2"));
    mergBMethod->set_active (0);
    mergBMethodConn = mergBMethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::mergBMethodChanged) );
    mgBbox->pack_start (*mergBMethod);
    mgVBox->pack_start (*mgBbox);


    balanhig->setAdjusterListener (this);
    balanhig->set_tooltip_text (M ("TP_WAVELET_VERTSH_TOOLTIP"));
    mgVBox->pack_start (*balanhig);

    balanleft->setAdjusterListener (this);
    balanleft->set_tooltip_text (M ("TP_WAVELET_VERTSH_TOOLTIP"));
    mgVBox->pack_start (*balanleft);


    // Wavelet merge curve
    CCWcurveEditormerg->setCurveListener (this);

    rtengine::WaveletParams::getDefaultmergCurveT (defaultCurve);
    cmergshape = static_cast<FlatCurveEditor*> (CCWcurveEditormerg->addCurve (CT_Flat, "", nullptr, false, false));

    cmergshape->setIdentityValue (0.);
    cmergshape->setResetCurve (FlatCurveType (defaultCurve.at (0)), defaultCurve);
    cmergshape->setTooltip (M ("TP_WAVELET_MCURVE_TOOLTIP"));

    CCWcurveEditormerg->curveListComplete();
    CCWcurveEditormerg->show();
    mgVBox->pack_start (*CCWcurveEditormerg, Gtk::PACK_SHRINK, 4);

    CCWcurveEditormerg2->setCurveListener (this);

    rtengine::WaveletParams::getDefaultmerg2CurveT (defaultCurve);
    cmerg2shape = static_cast<FlatCurveEditor*> (CCWcurveEditormerg2->addCurve (CT_Flat, "", nullptr, false, false));

    cmerg2shape->setIdentityValue (0.);
    cmerg2shape->setResetCurve (FlatCurveType (defaultCurve.at (0)), defaultCurve);
    cmerg2shape->setTooltip (M ("TP_WAVELET_MCURVE2_TOOLTIP"));

    CCWcurveEditormerg2->curveListComplete();
    CCWcurveEditormerg2->show();
    mgVBox->pack_start (*CCWcurveEditormerg2, Gtk::PACK_SHRINK, 4);

    blend->setAdjusterListener (this);
    blend->set_tooltip_text (M ("TP_WAVELET_BLEND_TOOLTIP"));
    mgVBox->pack_start (*blend);

    blendc->setAdjusterListener (this);
    blendc->set_tooltip_text (M ("TP_WAVELET_BLENDC_TOOLTIP"));
    mgVBox->pack_start (*blendc);

    grad->setAdjusterListener (this);
    grad->set_tooltip_text (M ("TP_WAVELET_GRAD_TOOLTIP"));
    mgVBox->pack_start (*grad);


    Gtk::VBox *balMVBox = Gtk::manage (new Gtk::VBox());
    balMFrame->set_tooltip_text (M ("TP_WAVELET_BALMFR_TOOLTIP"));

    for (int i = 0; i < 9; i++) {
        Glib::ustring ss;

        switch ( i ) {
            case 0:
                ss = Glib::ustring::compose ( "%1 (%2)", (i + 1), M ("TP_WAVELET_FINEST"));
                break;

            case 8:
                ss = Glib::ustring::compose ( "%1 (%2)", (i + 1), M ("TP_WAVELET_LARGEST"));
                break;

            default:
                ss = Glib::ustring::compose ( "%1", (i + 1));
        }

        balmer[i] = Gtk::manage ( new Adjuster (ss, 0, 250, 1, 15 + 12 * i) );
        balmer[i]->setAdjusterListener (this);
        balmer[i]->set_tooltip_text (M ("TP_WAVELET_BALMERLUM_TOOLTIP"));

        balMVBox->pack_start (*balmer[i]);

    }

    for (int i = 0; i < 9; i++) {
        Glib::ustring ss;

        switch ( i ) {
            case 0:
                ss = Glib::ustring::compose ( "%1 (%2)", (i + 1), M ("TP_WAVELET_FINEST"));
                break;

            case 8:
                ss = Glib::ustring::compose ( "%1 (%2)", (i + 1), M ("TP_WAVELET_LARGEST"));
                break;

            default:
                ss = Glib::ustring::compose ( "%1", (i + 1));
        }

        balmer2[i] = Gtk::manage ( new Adjuster (ss, -20, 140, 1, 70) );
        balmer2[i]->setAdjusterListener (this);
        balmer2[i]->set_tooltip_text (M ("TP_WAVELET_BALMERLUM2_TOOLTIP"));

        balMVBox->pack_start (*balmer2[i]);

    }

    balmerres2->setAdjusterListener (this);
    balmerres2->set_tooltip_text (M ("TP_WAVELET_BALMERRES2_TOOLTIP"));
    balMVBox->pack_start (*balmerres2);

    balmerres->setAdjusterListener (this);
    balmerres->set_tooltip_text (M ("TP_WAVELET_BALMERRES_TOOLTIP"));
    balMVBox->pack_start (*balmerres);

    balmerch->setAdjusterListener (this);
    balmerch->set_tooltip_text (M ("TP_WAVELET_BALMERCH_TOOLTIP"));
    balMVBox->pack_start (*balmerch);

    Gtk::HSeparator *separatorsty = Gtk::manage (new  Gtk::HSeparator());
    balMVBox->pack_start (*separatorsty);

    curveEditorsty->setCurveListener (this);

    shstyshape = static_cast<FlatCurveEditor*> (curveEditorsty->addCurve (CT_Flat, M ("TP_WAVELET_CURVEEDITOR_STYH")));
    shstyshape->setTooltip (M ("TP_WAVELET_CURVEEDITOR_STYH_TOOLTIP"));
    shstyshape->setCurveColorProvider (this, 5);
    shstyshape->setBottomBarBgGradient (milestones);
    curveEditorsty->curveListComplete();

    balMVBox->pack_start (*curveEditorsty, Gtk::PACK_SHRINK, 4);

    Gtk::HSeparator *separatorsty2 = Gtk::manage (new  Gtk::HSeparator());
    balMVBox->pack_start (*separatorsty2);

    dirV->setAdjusterListener (this);
    dirV->set_tooltip_text (M ("TP_WAVELET_DIRV_TOOLTIP"));
    balMVBox->pack_start (*dirV);

    dirH->setAdjusterListener (this);
    dirH->set_tooltip_text (M ("TP_WAVELET_DIRV_TOOLTIP"));
    balMVBox->pack_start (*dirH);

    dirD->setAdjusterListener (this);
    dirD->set_tooltip_text (M ("TP_WAVELET_DIRV_TOOLTIP"));
    balMVBox->pack_start (*dirD);

    Gtk::HSeparator *separatorsty3 = Gtk::manage (new  Gtk::HSeparator());
    balMVBox->pack_start (*separatorsty3);


    Gtk::VBox *shaVBox = Gtk::manage (new Gtk::VBox());

    //  shaFrame->set_tooltip_text (M("TP_WAVELET_SHAF_TOOLTIP"));

    CCWcurveEditorsty->setCurveListener (this);
    CCWcurveEditorsty->setTooltip (M ("TP_WAVELET_STYLCURVE_TOOLTIP"));

    rtengine::WaveletParams::getDefaultstyCurveT (defaultCurve);
    cstyshape = static_cast<FlatCurveEditor*> (CCWcurveEditorsty->addCurve (CT_Flat, "", nullptr, false, false));

    cstyshape->setIdentityValue (0.);
    cstyshape->setResetCurve (FlatCurveType (defaultCurve.at (0)), defaultCurve);
    cstyshape->setTooltip (M ("TP_WAVELET_STYLCURVE_TOOLTIP"));

    CCWcurveEditorsty->curveListComplete();
    CCWcurveEditorsty->show();

    CCWcurveEditorsty2->setCurveListener (this);
    CCWcurveEditorsty2->setTooltip (M ("TP_WAVELET_STYLCURVE_TOOLTIP"));

    rtengine::WaveletParams::getDefaultsty2CurveT (defaultCurve);
    cstyshape2 = static_cast<FlatCurveEditor*> (CCWcurveEditorsty2->addCurve (CT_Flat, "", nullptr, false, false));

    cstyshape2->setIdentityValue (0.);
    cstyshape2->setResetCurve (FlatCurveType (defaultCurve.at (0)), defaultCurve);
    cstyshape2->setTooltip (M ("TP_WAVELET_STYLCURVE2_TOOLTIP"));

    CCWcurveEditorsty2->curveListComplete();
    CCWcurveEditorsty2->show();


    shapMethod->append (M ("TP_WAVELET_SHAPN"));
    shapMethod->append (M ("TP_WAVELET_SHAPT"));
    shapMethod->append (M ("TP_WAVELET_SHAPF"));
    shapMethod->set_active (0);
    shapMethodConn = shapMethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::shapMethodChanged) );
    shapMethod->set_tooltip_markup (M ("TP_WAVELET_SHAPMETHOD_TOOLTIP"));

    shapind->setAdjusterListener (this);
    shapind->set_tooltip_text (M ("TP_WAVELET_SHAPIND_TOOLTIP"));

    shaVBox->pack_start (*shapMethod);
    shaVBox->pack_start (*shapind);

    shaVBox->pack_start (*CCWcurveEditorsty, Gtk::PACK_SHRINK, 4);
    shaVBox->pack_start (*CCWcurveEditorsty2, Gtk::PACK_SHRINK, 4);
    shaFrame->add (*shaVBox);
    balMVBox->pack_start (*shaFrame);


    // hueskinsty->set_tooltip_markup (M ("TP_WAVELET_HUESKINSTY_TOOLTIP"));
    const std::vector<GradientMilestone> milestonesty = {
        makeHsvGm (0.0000, 0.4199f, 0.5f, 0.5f), // hsv: 0.4199 rad: -3.14
        makeHsvGm (0.0540, 0.5000f, 0.5f, 0.5f), // hsv: 0.5    rad: -2.8
        makeHsvGm (0.1336, 0.6000f, 0.5f, 0.5f), // hsv: 0.60   rad: -2.3
        makeHsvGm (0.3567, 0.7500f, 0.5f, 0.5f), // hsv: 0.75   rad: -0.9
        makeHsvGm (0.4363, 0.8560f, 0.5f, 0.5f), // hsv: 0.856  rad: -0.4
        makeHsvGm (0.4841, 0.9200f, 0.5f, 0.5f), // hsv: 0.92   rad: -0.1
        makeHsvGm (0.5000, 0.9300f, 0.5f, 0.5f), // hsv: 0.93   rad:  0
        makeHsvGm (0.5366, 0.9600f, 0.5f, 0.5f), // hsv: 0.96   rad:  0.25
        makeHsvGm (0.5955, 1.0000f, 0.5f, 0.5f), // hsv: 1.     rad:  0.6
        makeHsvGm (0.6911, 0.0675f, 0.5f, 0.5f), // hsv: 0.0675 rad:  1.2
        makeHsvGm (0.7229, 0.0900f, 0.5f, 0.5f), // hsv: 0.09   rad:  1.4
        makeHsvGm (0.7707, 0.1700f, 0.5f, 0.5f), // hsv: 0.17   rad:  1.7
        makeHsvGm (0.8503, 0.2650f, 0.5f, 0.5f), // hsv: 0.265  rad:  2.1
        makeHsvGm (0.8981, 0.3240f, 0.5f, 0.5f), // hsv: 0.324  rad:  2.5
        makeHsvGm (1.0000, 0.4197f, 0.5f, 0.5f) // hsv: 0.419  rad:  3.14

    };
    hueskinsty->setBgGradient (milestonesty);
    balMVBox->pack_start (*hueskinsty);
    hueskinsty->setAdjusterListener (this);

    shapedetcolor->setAdjusterListener (this);
    shapedetcolor->set_tooltip_text (M ("TP_WAVELET_SHAPEDETCOLOR_TOOLTIP"));
    balMVBox->pack_start (*shapedetcolor);



    balMFrame->add (*balMVBox);
    mgVBox->pack_start (*balMFrame);

    savelab = Gtk::manage (new Gtk::Button (M ("TP_WAVELET_SAVELAB")));
    savelab->set_image (*Gtk::manage (new RTImage ("save.png")));
    savelab->set_tooltip_markup (M ("TP_WAVELET_SAVELAB_TOOLTIP"));
    mergeBox->pack_start (*savelab);
//   ipc = ipDialog->signal_selection_changed().connect( sigc::mem_fun(*this, &Wavelet::ipSelectionChanged) );

    sizelab->setAdjusterListener (this);
    sizelab->set_tooltip_text (M ("TP_WAVELET_SIZELAB_TOOLTIP"));
    mergeBox->pack_start (*sizelab);


    mergeBox->pack_start (*mgVBox);

    savelab->signal_pressed().connect ( sigc::mem_fun (*this, &Wavelet::savelabPressed) );

    neutrHBox2->set_border_width (2);

    // neutral2 = Gtk::manage (new Gtk::Button (M ("TP_WAVELET_NEUTRAL2")));
    RTImage *resetImg2 = Gtk::manage (new RTImage ("undo-small.png", "redo-small.png"));
    neutral2->set_image (*resetImg2);
    //neutral2->set_tooltip_text (M("TP_WAVELET_NEUTRAL_TIP"));
    neutral2conn = neutral2->signal_pressed().connect ( sigc::mem_fun (*this, &Wavelet::neutral2_pressed) );
    neutral2->show();
    neutrHBox2->pack_start (*neutral2);
    mergeBox->pack_start (*neutrHBox2);


// Final Touchup
    Gtk::VBox* const ctboxBA = Gtk::manage (new Gtk::VBox());

    ctboxBA->set_spacing (2);

    Gtk::Label* const labmBA = Gtk::manage (new Gtk::Label (M ("TP_WAVELET_BATYPE") + ":"));
    Gtk::HBox* const ctboxFI = Gtk::manage (new Gtk::HBox());
    ctboxFI->pack_start (*labmBA, Gtk::PACK_SHRINK, 1);

    BAmethod->append (M ("TP_WAVELET_BANONE"));
    BAmethod->append (M ("TP_WAVELET_BASLI"));
    BAmethod->append (M ("TP_WAVELET_BACUR"));
    BAmethodconn = BAmethod->signal_changed().connect ( sigc::mem_fun (*this, &Wavelet::BAmethodChanged) );
    ctboxFI->pack_start (*BAmethod);
    ctboxBA->pack_start (*ctboxFI);

    balance->setAdjusterListener (this);
    balance->set_tooltip_text (M ("TP_WAVELET_BALANCE_TOOLTIP"));

    opacityCurveEditorW->setCurveListener (this);

    rtengine::WaveletParams::getDefaultOpacityCurveW (defaultCurve);
    opacityShape = static_cast<FlatCurveEditor*> (opacityCurveEditorW->addCurve (CT_Flat, "", nullptr, false, false));
    opacityShape->setIdentityValue (0.);
    opacityShape->setResetCurve (FlatCurveType (defaultCurve.at (0)), defaultCurve);
    opacityShape->setBottomBarBgGradient (milestones2);

    // This will add the reset button at the end of the curveType buttons
    opacityCurveEditorW->curveListComplete();
    opacityCurveEditorW->show();

    iter->setAdjusterListener (this);
    iter->set_tooltip_text (M ("TP_WAVELET_ITER_TOOLTIP"));

    Gtk::HSeparator* const separatorbalend = Gtk::manage (new  Gtk::HSeparator());

    opacityCurveEditorWL->setCurveListener (this);

    rtengine::WaveletParams::getDefaultOpacityCurveWL (defaultCurve);
    opacityShapeWL = static_cast<FlatCurveEditor*> (opacityCurveEditorWL->addCurve (CT_Flat, "", nullptr, false, false));
    opacityShapeWL->setIdentityValue (0.);
    opacityShapeWL->setResetCurve (FlatCurveType (defaultCurve.at (0)), defaultCurve);
    opacityShapeWL->setTooltip (M ("TP_WAVELET_OPACITYWL_TOOLTIP"));

    // This will add the reset button at the end of the curveType buttons
    opacityCurveEditorWL->curveListComplete();
    opacityCurveEditorWL->show();

    curveEditorG->setCurveListener (this);

    clshape = static_cast<DiagonalCurveEditor*> (curveEditorG->addCurve (CT_Diagonal, M ("TP_WAVELET_CURVEEDITOR_CL")));
    clshape->setTooltip (M ("TP_WAVELET_CURVEEDITOR_CL_TOOLTIP"));
    clshape->setBottomBarBgGradient (milestones2);
    clshape->setLeftBarBgGradient (milestones2);

    curveEditorG->curveListComplete();

    tmr->set_active (true);
    tmr->set_tooltip_text (M ("TP_WAVELET_BALCHRO_TOOLTIP"));
    tmrConn = tmr->signal_toggled().connect ( sigc::mem_fun (*this, &Wavelet::tmrToggled) );

    ToolParamBlock* const finalBox = Gtk::manage (new ToolParamBlock());

    labretifinbox->pack_start (*labretifin, Gtk::PACK_SHRINK, 1);

    finalBox->pack_start (*ctboxBA);
    finalBox->pack_start (*balance);

    finalBox->pack_start ( *opacityCurveEditorW, Gtk::PACK_SHRINK, 2);

    finalBox->pack_start (*iter);

    finalBox->pack_start (*tmr);
    finalBox->pack_start (*separatorbalend, Gtk::PACK_SHRINK, 2);
    finalBox->pack_start ( *opacityCurveEditorWL, Gtk::PACK_SHRINK, 2);

    finalBox->pack_start (*curveEditorG, Gtk::PACK_SHRINK, 4);
    finalBox->pack_start (*labretifinbox);

//-----------------------------

//<<<<<<< HEAD
    expsettings->add (*settingsBox);
    expsettings->setLevel (2);
    pack_start (*expsettings);

    expcontrast->add (*levBox);
    expcontrast->setLevel (2);
    pack_start (*expcontrast);

    expchroma->add (*chBox);
    expchroma->setLevel (2);
    pack_start (*expchroma);

    exptoning->add (*tonBox);
    exptoning->setLevel (2);
    pack_start (*exptoning);

    expnoise->add (*noiseBox);
    expnoise->setLevel (2);
    pack_start (*expnoise);

    expedge->add (*edgBox);
    expedge->setLevel (2);
    pack_start (*expedge);

    expedg1->add (*edgBoxS);
    expedg1->setLevel (2);
    pack_start (*expedg1);


    expreti->add (*retiBox);
    pack_start (*expreti);

    expgamut->add (*conBox);
    expgamut->setLevel (2);
    pack_start (*expgamut);

    expresid->add (*resBox);
    expresid->setLevel (2);
    pack_start (*expresid);

    expmerge->add (*mergeBox);
    pack_start (*expmerge);

    expfinal->add (*finalBox);
    expfinal->setLevel (2);
    pack_start (*expfinal);
    /*
    =======
    expsettings->add(*settingsBox, false);
    expsettings->setLevel(2);
    pack_start (*expsettings);

    expcontrast->add(*levBox, false);
    expcontrast->setLevel(2);
    pack_start (*expcontrast);

    expchroma->add(*chBox, false);
    expchroma->setLevel(2);
    pack_start (*expchroma);

    exptoning->add(*tonBox, false);
    exptoning->setLevel(2);
    pack_start (*exptoning);

    expnoise->add(*noiseBox, false);
    expnoise->setLevel(2);
    pack_start (*expnoise);

    expedge->add(*edgBox, false);
    expedge->setLevel(2);
    pack_start (*expedge);

    expgamut->add(*conBox, false);
    expgamut->setLevel(2);
    pack_start (*expgamut);

    expresid->add(*resBox, false);
    expresid->setLevel(2);
    pack_start(*expresid);

    expfinal->add(*finalBox, false);
    expfinal->setLevel(2);
    pack_start(*expfinal);
    >>>>>>> 5f97800
    */
}



Wavelet::~Wavelet ()
{
    idle_register.destroy();

    delete opaCurveEditorG;
    delete opacityCurveEditorG;
    delete CCWcurveEditorG;
    delete CCWcurveEditormerg;
    delete CCWcurveEditormerg2;
    delete CCWcurveEditorsty;
    delete CCWcurveEditorsty2;
    delete CCWcurveEditorT;
    delete CCWcurveEditorgainT;
    delete curveEditorRES;
    delete curveEditorsty;
    delete curveEditorGAM;
    delete curveEditorG;
    delete opacityCurveEditorW;
    delete opacityCurveEditorWL;

}


int WminmaxChangedUI (void* data)
{
    GThreadLock lock; // All GUI access from idle_add callbacks or separate thread HAVE to be protected
    (static_cast<Wavelet*> (data))->minmaxComputed_ ();
    return 0;
}

void Wavelet::minmaxChanged (double cdma, double cdmin, double mini, double maxi, double Tmean, double Tsigma, double Tmin, double Tmax)
{
    nextmin = cdmin;
    nextmax = cdma;
    nextminiT = mini;
    nextmaxiT = maxi;
    nextmeanT = Tmean;
    nextsigma = Tsigma;
    nextminT = Tmin;
    nextmaxT = Tmax;
    g_idle_add (WminmaxChangedUI, this);

}

bool Wavelet::minmaxComputed_ ()
{

    disableListener ();
    enableListener ();
    updateLabel ();
    updateTrans ();
    return false;

}

void Wavelet::updateLabel ()
{
    if (!batchMode) {
        float nX, nY;
        nX = nextmin;
        nY = nextmax;
        {
            mMLabels->set_text (
                Glib::ustring::compose (M ("TP_RETINEX_MLABEL"),
                                        Glib::ustring::format (std::fixed, std::setprecision (0), nX),
                                        Glib::ustring::format (std::fixed, std::setprecision (0), nY))
            );
        }
    }
}

void Wavelet::updateTrans ()
{
    if (!batchMode) {
        float nm, nM, nZ, nA, nB, nS;
        nm = nextminiT;
        nM = nextmaxiT;
        nZ = nextmeanT;
        nA = nextminT;
        nB = nextmaxT;
        nS = nextsigma;
        {
            transLabels->set_text (
                Glib::ustring::compose (M ("TP_RETINEX_TLABEL"),
                                        Glib::ustring::format (std::fixed, std::setprecision (1), nm),
                                        Glib::ustring::format (std::fixed, std::setprecision (1), nM),
                                        Glib::ustring::format (std::fixed, std::setprecision (1), nZ),
                                        Glib::ustring::format (std::fixed, std::setprecision (1), nS))
            );
            transLabels2->set_text (
                Glib::ustring::compose (M ("TP_RETINEX_TLABEL2"),
                                        Glib::ustring::format (std::fixed, std::setprecision (1), nA),
                                        Glib::ustring::format (std::fixed, std::setprecision (1), nB))
            );


        }
    }
}


int wavUpdateUI (void* data)
{
    GThreadLock lock; // All GUI acces from idle_add callbacks or separate thread HAVE to be protected
    (static_cast<Wavelet*> (data))->wavComputed_ ();
    return 0;
}

void Wavelet::wavChanged (double nlevel)
{
    nextnlevel = nlevel;

    const auto func = [] (gpointer data) -> gboolean {
        GThreadLock lock; // All GUI access from idle_add callbacks or separate thread HAVE to be protected
        static_cast<Wavelet*> (data)->wavComputed_();

        return FALSE;
    };

    idle_register.add (func, this);
}

bool Wavelet::wavComputed_ ()
{
    disableListener ();
    enableListener ();
    updatewavLabel ();
    return false;
}

void Wavelet::updatewavLabel ()
{
    if (!batchMode) {
        float lv;
        lv = nextnlevel;
        wavLabels->set_text (
            Glib::ustring::compose (M ("TP_WAVELET_LEVLABEL"),
                                    Glib::ustring::format (std::fixed, std::setprecision (0), lv))
        );
    }
}
void Wavelet::mFile_Reset()
{

    inChanged = true;

    // caution: I had to make this hack, because set_current_folder() doesn't work correctly!
    //          Because szeva doesn't exist since he was committed to happy hunting ground in Issue 316
    //          we can use him now for this hack
    inputeFile->set_filename (options.lastFlatfieldDir + "/szeva");
    // end of the hack

    if (!options.lastmergeDir.empty()) {
        inputeFile->set_current_folder (options.lastmergeDir);
    }


    if (listener) {
        listener->panelChanged (EvmFile, M ("GENERAL_NONE") );
    }

}

// Will only reset the channel mixer
// WARNING!  In mutiImage mode, and for sliders in ADD mode, this will reset the slider to 0, but not to the default value as in SET mode.
void Wavelet::neutral_pressed ()
{
    disableListener();
    greenlow->resetValue (false);
    bluelow->resetValue (false);
    greenmed->resetValue (false);
    bluemed->resetValue (false);
    greenhigh->resetValue (false);
    bluehigh->resetValue (false);

    enableListener();

    if (listener && getEnabled()) {
        listener->panelChanged (EvWavNeutral, M("GENERAL_RESET"));
    }
}

void Wavelet::read (const ProcParams* pp, const ParamsEdited* pedited)
{
    /*****************************************************************************************************
     *
     *                                        Disable listeners and signals
     *
     *****************************************************************************************************/

    disableListener ();
    Lmethodconn.block (true);
    CLmethodconn.block (true);
    Backmethodconn.block (true);
    Tilesmethodconn.block (true);
    retinexMethodConn.block (true);
    retinexMethodproConn.block (true);
    usharpmethodconn.block (true);
    ushamethodconn.block (true);
    daubcoeffmethodconn.block (true);
    Dirmethodconn.block (true);
    CHmethodconn.block (true);
    CHSLmethodconn.block (true);
    EDmethodconn.block (true);
    NPmethodconn.block (true);
    BAmethodconn.block (true);
    TMmethodconn.block (true);
    HSmethodconn.block (true);
    MedgreinfConn.block (true);
    cbenabConn.block (true);
    enableChromaConn.block (true);
    enableContrastConn.block (true);
    enableEdgeConn.block (true);
    enableFinalConn.block (true);
    enableNoiseConn.block (true);
    enableResidConn.block (true);
    enableToningConn.block (true);
    enableMergeConn.block (true);
    mergevMethodConn.block (true);
    mergMethodConn.block (true);
    mergBMethodConn.block (true);
    mergMethod2Conn.block (true);
    shapMethodConn.block (true);

    /*****************************************************************************************************
     *
     *                               Set the GUI reflecting the given ProcParams
     *
     *****************************************************************************************************/
    if (pp->wavelet.inpute.substr (0, 5) != "file:" && !inputeFile->get_filename().empty()) {
        inputeFile->set_filename (pp->wavelet.inpute);
    }

    oldip = pp->wavelet.inpute.substr (5); // cut of "file:"
    inputeFile->set_filename (pp->wavelet.inpute.substr (5));

    //HSmethod->set_active (1);   // Note: default values are controlled in rtengine::ProcParams::SetDefaults
    if (pp->wavelet.HSmethod == "without") {
        HSmethod->set_active (0);
    } else if (pp->wavelet.HSmethod == "with") {
        HSmethod->set_active (1);
    }

    //CHmethod->set_active (1);
    if (pp->wavelet.CHmethod == "without") {
        CHmethod->set_active (0);
    } else if (pp->wavelet.CHmethod == "with") {
        CHmethod->set_active (1);
    } else if (pp->wavelet.CHmethod == "link") {
        CHmethod->set_active (2);
    }

    //Medgreinf->set_active (1);
    if (pp->wavelet.Medgreinf == "more") {
        Medgreinf->set_active (0);
    } else if (pp->wavelet.Medgreinf == "none") {
        Medgreinf->set_active (1);
    } else if (pp->wavelet.Medgreinf == "less") {
        Medgreinf->set_active (2);
    }

    //CHSLmethod->set_active (1);
    if (pp->wavelet.CHSLmethod == "SL") {
        CHSLmethod->set_active (0);
    } else if (pp->wavelet.CHSLmethod == "CU") {
        CHSLmethod->set_active (1);
    }

    //EDmethod->set_active (1);
    if (pp->wavelet.EDmethod == "SL") {
        EDmethod->set_active (0);
    } else if (pp->wavelet.EDmethod == "CU") {
        EDmethod->set_active (1);
    }

    if (pp->wavelet.NPmethod == "none") {
        NPmethod->set_active (0);
    } else if (pp->wavelet.NPmethod == "low") {
        NPmethod->set_active (1);
    } else if (pp->wavelet.NPmethod == "high") {
        NPmethod->set_active (2);
    }

    if (pp->wavelet.mergMethod == "savwat") {
        mergMethod->set_active (0);
    } else if (pp->wavelet.mergMethod == "savhdr") {
        mergMethod->set_active (1);
    } else if (pp->wavelet.mergMethod == "savzero") {
        mergMethod->set_active (2);
    } else if (pp->wavelet.mergMethod == "load") {
        mergMethod->set_active (3);
    } else if (pp->wavelet.mergMethod == "loadzero") {
        mergMethod->set_active (4);
    } else if (pp->wavelet.mergMethod == "loadzerohdr") {
        mergMethod->set_active (5);
    }

    if (pp->wavelet.mergMethod2 == "befo") {
        mergMethod2->set_active (0);
    } else if (pp->wavelet.mergMethod2 == "after") {
        mergMethod2->set_active (1);
    }

    if (pp->wavelet.mergevMethod == "curr") {
        mergevMethod->set_active (0);
    } else if (pp->wavelet.mergevMethod == "cuno") {
        mergevMethod->set_active (1);
    } else if (pp->wavelet.mergevMethod == "first") {
        mergevMethod->set_active (2);
    } else if (pp->wavelet.mergevMethod == "save") {
        mergevMethod->set_active (3);
    }

//    if (pp->wavelet.mergBMethod == "water") {
//        mergBMethod->set_active (0);
//    } else
    if (pp->wavelet.mergBMethod == "hdr1") {
        mergBMethod->set_active (0);
    } else if (pp->wavelet.mergBMethod == "hdr2") {
        mergBMethod->set_active (1);
    }

    //BAmethod->set_active (0);
    if (pp->wavelet.BAmethod == "none") {
        BAmethod->set_active (0);
    } else if (pp->wavelet.BAmethod == "sli") {
        BAmethod->set_active (1);
    } else if (pp->wavelet.BAmethod == "cur") {
        BAmethod->set_active (2);
    }

    //TMmethod->set_active (1);
    if (pp->wavelet.TMmethod == "cont") {
        TMmethod->set_active (0);
    } else if (pp->wavelet.TMmethod == "tm") {
        TMmethod->set_active (1);
    }

//  else if (pp->wavelet.TMmethod=="both")
//      TMmethod->set_active (2);

    //Backmethod->set_active (3);
    if (pp->wavelet.Backmethod == "black") {
        Backmethod->set_active (0);
    } else if (pp->wavelet.Backmethod == "grey") {
        Backmethod->set_active (1);
    } else if (pp->wavelet.Backmethod == "resid") {
        Backmethod->set_active (2);
    }

    //CLmethod->set_active (3);
    if (pp->wavelet.CLmethod == "one") {
        CLmethod->set_active (0);
    } else if (pp->wavelet.CLmethod == "inf") {
        CLmethod->set_active (1);
    } else if (pp->wavelet.CLmethod == "sup") {
        CLmethod->set_active (2);
    } else if (pp->wavelet.CLmethod == "all") {
        CLmethod->set_active (3);
    }

    //Tilesmethod->set_active (2);
    if (pp->wavelet.Tilesmethod == "full") {
        Tilesmethod->set_active (0);
    } else if (pp->wavelet.Tilesmethod == "big") {
        Tilesmethod->set_active (1);
    } else if (pp->wavelet.Tilesmethod == "lit") {
        Tilesmethod->set_active (2);
    }

    //daubcoeffmethod->set_active (4);
    if (pp->wavelet.daubcoeffmethod == "2_") {
        daubcoeffmethod->set_active (0);
    } else if (pp->wavelet.daubcoeffmethod == "4_") {
        daubcoeffmethod->set_active (1);
    } else if (pp->wavelet.daubcoeffmethod == "6_") {
        daubcoeffmethod->set_active (2);
    } else if (pp->wavelet.daubcoeffmethod == "10_") {
        daubcoeffmethod->set_active (3);
    } else if (pp->wavelet.daubcoeffmethod == "14_") {
        daubcoeffmethod->set_active (4);
    }

    //Dirmethod->set_active (3);
    if (pp->wavelet.Dirmethod == "one") {
        Dirmethod->set_active (0);
    } else if (pp->wavelet.Dirmethod == "two") {
        Dirmethod->set_active (1);
    } else if (pp->wavelet.Dirmethod == "thr") {
        Dirmethod->set_active (2);
    } else if (pp->wavelet.Dirmethod == "all") {
        Dirmethod->set_active (3);
    }

    //ushamethod
    if (pp->wavelet.ushamethod == "none") {
        ushamethod->set_active (0);
    } else if (pp->wavelet.ushamethod == "sharp") {
        ushamethod->set_active (1);
    } else if (pp->wavelet.ushamethod == "clari") {
        ushamethod->set_active (2);
    }

    //usharpmethod
    if (pp->wavelet.usharpmethod == "orig") {
        usharpmethod->set_active (0);
    } else if (pp->wavelet.usharpmethod == "wave") {
        usharpmethod->set_active (1);
    }

    if (pp->wavelet.retinexMethod == "low") {
        retinexMethod->set_active (0);
    } else if (pp->wavelet.retinexMethod == "uni") {
        retinexMethod->set_active (1);
    } else if (pp->wavelet.retinexMethod == "high") {
        retinexMethod->set_active (2);
//    } else if (pp->wavelet.retinexMethod == "high") {
//        retinexMethod->set_active (3);
    }

    if (pp->wavelet.retinexMethodpro == "resid") {
        retinexMethodpro->set_active (0);
    } else if (pp->wavelet.retinexMethodpro == "fina") {
        retinexMethodpro->set_active (1);
    }

    if (pp->wavelet.shapMethod == "norm") {
        shapMethod->set_active (0);
    } else if (pp->wavelet.shapMethod == "three") {
        shapMethod->set_active (1);
    } else if (pp->wavelet.shapMethod == "four") {
        shapMethod->set_active (2);
    }

    int selectedLevel = pp->wavelet.Lmethod - 1;
    Lmethod->set_active (selectedLevel == -1 ? 4 : selectedLevel);

    ccshape->setCurve (pp->wavelet.ccwcurve);
    cTshape->setCurve (pp->wavelet.ccwTcurve);
    cTgainshape->setCurve (pp->wavelet.ccwTgaincurve);
    cmergshape->setCurve (pp->wavelet.ccwmergcurve);
    cmerg2shape->setCurve (pp->wavelet.ccwmerg2curve);
    cstyshape->setCurve (pp->wavelet.ccwstycurve);
    cstyshape2->setCurve (pp->wavelet.ccwsty2curve);
    opacityShapeRG->setCurve (pp->wavelet.opacityCurveRG);
    opacityShapeBY->setCurve (pp->wavelet.opacityCurveBY);
    opacityShape->setCurve (pp->wavelet.opacityCurveW);
    opacityShapeWL->setCurve (pp->wavelet.opacityCurveWL);
    hhshape->setCurve (pp->wavelet.hhcurve);
    shstyshape->setCurve (pp->wavelet.shstycurve);
    Chshape->setCurve (pp->wavelet.Chcurve);
    clshape->setCurve (pp->wavelet.wavclCurve);
    expcontrast->setEnabled (pp->wavelet.expcontrast);
    expchroma->setEnabled (pp->wavelet.expchroma);
    expedge->setEnabled (pp->wavelet.expedge);
    expreti->setEnabled (pp->wavelet.expreti);
    expresid->setEnabled (pp->wavelet.expresid);
//    expTCresi->setEnabled (pp->wavelet.expTCresi);
    expmerge->setEnabled (pp->wavelet.expmerge);
    expfinal->setEnabled (pp->wavelet.expfinal);
    exptoning->setEnabled (pp->wavelet.exptoning);
    expnoise->setEnabled (pp->wavelet.expnoise);

    setEnabled (pp->wavelet.enabled);
    avoidConn.block (true);
    avoid->set_active (pp->wavelet.avoid);
    avoidConn.block (false);
    tmrConn.block (true);
    tmr->set_active (pp->wavelet.tmr);
    tmrConn.block (false);
    medianConn.block (true);
    median->set_active (pp->wavelet.median);
    medianConn.block (false);
    medianlevConn.block (true);
    medianlev->set_active (pp->wavelet.medianlev);
    medianlevConn.block (false);
    linkedgConn.block (true);
    linkedg->set_active (pp->wavelet.linkedg);
    linkedgConn.block (false);
    cbenabConn.block (true);
    cbenab->set_active (pp->wavelet.cbenab);
    cbenabConn.block (false);

    lipstConn.block (true);
    lipst->set_active (pp->wavelet.lipst);
    lipstConn.block (false);
    //edgreinfConn.block (true);
    //edgreinf->set_active (pp->wavelet.edgreinf);
    //edgreinfConn.block (false);
    //lastedgreinf = pp->wavelet.edgreinf;
    lastmedian = pp->wavelet.median;
    lastmedianlev = pp->wavelet.medianlev;
    lastlinkedg = pp->wavelet.linkedg;
    lastcbenab = pp->wavelet.cbenab;
    lastlipst = pp->wavelet.lipst;
    lastavoid = pp->wavelet.avoid;
    lasttmr = pp->wavelet.tmr;
    rescon->setValue (pp->wavelet.rescon);
    resconH->setValue (pp->wavelet.resconH);
    reschro->setValue (pp->wavelet.reschro);
    tmrs->setValue (pp->wavelet.tmrs);
    gamma->setValue (pp->wavelet.gamma);
    sup->setValue (pp->wavelet.sup);
    sky->setValue (pp->wavelet.sky);
    thres->setValue (pp->wavelet.thres);
    chroma->setValue (pp->wavelet.chroma);
    chro->setValue (pp->wavelet.chro);
    contrast->setValue (pp->wavelet.contrast);
    edgrad->setValue (pp->wavelet.edgrad);
    edgval->setValue (pp->wavelet.edgval);
    edgthresh->setValue (pp->wavelet.edgthresh);
    thr->setValue (pp->wavelet.thr);
    thrH->setValue (pp->wavelet.thrH);
    skinprotect->setValue (pp->wavelet.skinprotect);
    hueskin->setValue<int> (pp->wavelet.hueskin);
    hueskin2->setValue<int> (pp->wavelet.hueskin2);
    hueskinsty->setValue<int> (pp->wavelet.hueskinsty);
    threshold->setValue (pp->wavelet.threshold);
    threshold2->setValue (pp->wavelet.threshold2);
    edgedetect->setValue (pp->wavelet.edgedetect);
    edgedetectthr->setValue (pp->wavelet.edgedetectthr);
    edgedetectthr2->setValue (pp->wavelet.edgedetectthr2);
    edgesensi->setValue (pp->wavelet.edgesensi);
    edgeampli->setValue (pp->wavelet.edgeampli);
    hllev->setValue<int> (pp->wavelet.hllev);
    bllev->setValue<int> (pp->wavelet.bllev);
    pastlev->setValue<int> (pp->wavelet.pastlev);
    satlev->setValue<int> (pp->wavelet.satlev);
    edgcont->setValue<int> (pp->wavelet.edgcont);

    greenlow->setValue (pp->wavelet.greenlow);
    bluelow->setValue (pp->wavelet.bluelow);
    greenmed->setValue (pp->wavelet.greenmed);
    bluemed->setValue (pp->wavelet.bluemed);
    greenhigh->setValue (pp->wavelet.greenhigh);
    bluehigh->setValue (pp->wavelet.bluehigh);

    level0noise->setValue<double> (pp->wavelet.level0noise);
    level1noise->setValue<double> (pp->wavelet.level1noise);
    level2noise->setValue<double> (pp->wavelet.level2noise);
    level3noise->setValue<double> (pp->wavelet.level3noise);
    strength->setValue (pp->wavelet.strength);
    balance->setValue (pp->wavelet.balance);
    balanleft->setValue (pp->wavelet.balanleft);
    balanhig->setValue (pp->wavelet.balanhig);
    sizelab->setValue (pp->wavelet.sizelab);
    balmerch->setValue (pp->wavelet.balmerch);
    shapedetcolor->setValue (pp->wavelet.shapedetcolor);
    dirV->setValue (pp->wavelet.dirV);
    dirH->setValue (pp->wavelet.dirH);
    dirD->setValue (pp->wavelet.dirD);
    shapind->setValue (pp->wavelet.shapind);
    balmerres->setValue (pp->wavelet.balmerres);
    balmerres2->setValue (pp->wavelet.balmerres2);
    grad->setValue (pp->wavelet.grad);
    blend->setValue (pp->wavelet.blend);
    blendc->setValue (pp->wavelet.blendc);
    iter->setValue (pp->wavelet.iter);
    mergeL->setValue (pp->wavelet.mergeL);
    mergeC->setValue (pp->wavelet.mergeC);
    nextmergeL = pp->wavelet.mergeL;
    nextmergeC = pp->wavelet.mergeC;
    gain->setValue (pp->wavelet.gain);
    offs->setValue (pp->wavelet.offs);
    str->setValue (pp->wavelet.str);
    neigh->setValue (pp->wavelet.neigh);
    vart->setValue (pp->wavelet.vart);
    limd->setValue (pp->wavelet.limd);
    scale->setValue (pp->wavelet.scale);
    chrrt->setValue (pp->wavelet.chrrt);
    radius->setValue        (pp->wavelet.radius);
    highlights->setValue    (pp->wavelet.highlights);
    h_tonalwidth->setValue  (pp->wavelet.htonalwidth);
    shadows->setValue       (pp->wavelet.shadows);
    s_tonalwidth->setValue  (pp->wavelet.stonalwidth);

    for (int i = 0; i < 9; i++) {
        correction[i]->setValue (pp->wavelet.c[i]);
    }

    for (int i = 0; i < 9; i++) {
        correctionch[i]->setValue (pp->wavelet.ch[i]);
    }

    for (int i = 0; i < 9; i++) {
        balmer[i]->setValue (pp->wavelet.bm[i]);
    }

    for (int i = 0; i < 9; i++) {
        balmer2[i]->setValue (pp->wavelet.bm2[i]);
    }

    /*****************************************************************************************************
     *
     *           Set the inconsistent state (for combobox, select the "GENERAL_UNCHANGED" entry)
     *
     *****************************************************************************************************/

    if (pedited) {
        if (!pedited->wavelet.Lmethod) {
            Lmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.CLmethod) {
            CLmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.Backmethod) {
            Backmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.Tilesmethod) {
            Tilesmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.usharpmethod) {
            usharpmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.ushamethod) {
            ushamethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.retinexMethod) {
            retinexMethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.retinexMethodpro) {
            retinexMethodpro->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.shapMethod) {
            shapMethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.daubcoeffmethod) {
            daubcoeffmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.Dirmethod) {
            Dirmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.CHmethod) {
            CHmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.CHSLmethod) {
            CHSLmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.EDmethod) {
            EDmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.NPmethod) {
            NPmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.BAmethod) {
            BAmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.TMmethod) {
            TMmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.HSmethod) {
            HSmethod->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        if (!pedited->wavelet.Medgreinf) {
            Medgreinf->set_active_text (M ("GENERAL_UNCHANGED"));
        }

        set_inconsistent (multiImage && !pedited->wavelet.enabled);
        ccshape->setUnChanged  (!pedited->wavelet.ccwcurve);
        cTshape->setUnChanged  (!pedited->wavelet.ccwTcurve);
        cTgainshape->setUnChanged  (!pedited->wavelet.ccwTgaincurve);
        cmergshape->setUnChanged  (!pedited->wavelet.ccwmergcurve);
        cmerg2shape->setUnChanged  (!pedited->wavelet.ccwmerg2curve);
        cstyshape->setUnChanged  (!pedited->wavelet.ccwstycurve);
        cstyshape2->setUnChanged  (!pedited->wavelet.ccwsty2curve);
        expcontrast->set_inconsistent   (!pedited->wavelet.expcontrast);
        expchroma->set_inconsistent   (!pedited->wavelet.expchroma);
        expedge->set_inconsistent   (!pedited->wavelet.expedge);
        expreti->set_inconsistent   (!pedited->wavelet.expreti);
        expresid->set_inconsistent   (!pedited->wavelet.expresid);
//       expTCresi->set_inconsistent   (!pedited->wavelet.expTCresi);
        expmerge->set_inconsistent   (!pedited->wavelet.expmerge);
        expfinal->set_inconsistent   (!pedited->wavelet.expfinal);
        exptoning->set_inconsistent   (!pedited->wavelet.exptoning);
        expnoise->set_inconsistent   (!pedited->wavelet.expnoise);
        opacityShapeRG->setCurve (pp->wavelet.opacityCurveRG);
        opacityShapeBY->setCurve (pp->wavelet.opacityCurveBY);
        opacityShape->setCurve (pp->wavelet.opacityCurveW);
        opacityShapeWL->setCurve (pp->wavelet.opacityCurveWL);
        hhshape->setUnChanged  (!pedited->wavelet.hhcurve);
        shstyshape->setUnChanged  (!pedited->wavelet.shstycurve);
        Chshape->setUnChanged  (!pedited->wavelet.Chcurve);
        clshape->setUnChanged  (!pedited->wavelet.wavclCurve);
        avoid->set_inconsistent (!pedited->wavelet.avoid);
        tmr->set_inconsistent (!pedited->wavelet.tmr);
        edgthresh->setEditedState (pedited->wavelet.edgthresh ? Edited : UnEdited);
        rescon->setEditedState (pedited->wavelet.rescon ? Edited : UnEdited);
        resconH->setEditedState (pedited->wavelet.resconH ? Edited : UnEdited);
        reschro->setEditedState (pedited->wavelet.reschro ? Edited : UnEdited);
        tmrs->setEditedState (pedited->wavelet.tmrs ? Edited : UnEdited);
        gamma->setEditedState (pedited->wavelet.gamma ? Edited : UnEdited);
        sup->setEditedState (pedited->wavelet.sup ? Edited : UnEdited);
        sky->setEditedState (pedited->wavelet.sky ? Edited : UnEdited);
        thres->setEditedState (pedited->wavelet.thres ? Edited : UnEdited);
        balance->setEditedState (pedited->wavelet.balance ? Edited : UnEdited);
        balanleft->setEditedState (pedited->wavelet.balanleft ? Edited : UnEdited);
        balanhig->setEditedState (pedited->wavelet.balanhig ? Edited : UnEdited);
        sizelab->setEditedState (pedited->wavelet.sizelab ? Edited : UnEdited);
        balmerch->setEditedState (pedited->wavelet.balmerch ? Edited : UnEdited);
        shapedetcolor->setEditedState (pedited->wavelet.shapedetcolor ? Edited : UnEdited);
        dirV->setEditedState (pedited->wavelet.dirV ? Edited : UnEdited);
        dirD->setEditedState (pedited->wavelet.dirD ? Edited : UnEdited);
        dirH->setEditedState (pedited->wavelet.dirH ? Edited : UnEdited);
        shapind->setEditedState (pedited->wavelet.shapind ? Edited : UnEdited);
        balmerres->setEditedState (pedited->wavelet.balmerres ? Edited : UnEdited);
        balmerres2->setEditedState (pedited->wavelet.balmerres2 ? Edited : UnEdited);
        grad->setEditedState (pedited->wavelet.grad ? Edited : UnEdited);
        blend->setEditedState (pedited->wavelet.blend ? Edited : UnEdited);
        blendc->setEditedState (pedited->wavelet.blendc ? Edited : UnEdited);
        iter->setEditedState (pedited->wavelet.iter ? Edited : UnEdited);
        threshold->setEditedState (pedited->wavelet.threshold ? Edited : UnEdited);
        threshold2->setEditedState (pedited->wavelet.threshold2 ? Edited : UnEdited);
        edgedetect->setEditedState (pedited->wavelet.edgedetect ? Edited : UnEdited);
        edgedetectthr->setEditedState (pedited->wavelet.edgedetectthr ? Edited : UnEdited);
        edgedetectthr2->setEditedState (pedited->wavelet.edgedetectthr2 ? Edited : UnEdited);
        edgesensi->setEditedState (pedited->wavelet.edgesensi ? Edited : UnEdited);
        edgeampli->setEditedState (pedited->wavelet.edgeampli ? Edited : UnEdited);
        chroma->setEditedState (pedited->wavelet.chroma ? Edited : UnEdited);
        chro->setEditedState (pedited->wavelet.chro ? Edited : UnEdited);

        greenlow->setEditedState (pedited->wavelet.greenlow ? Edited : UnEdited);
        bluelow->setEditedState (pedited->wavelet.bluelow ? Edited : UnEdited);
        greenmed->setEditedState (pedited->wavelet.greenmed ? Edited : UnEdited);
        bluemed->setEditedState (pedited->wavelet.bluemed ? Edited : UnEdited);
        greenhigh->setEditedState (pedited->wavelet.greenhigh ? Edited : UnEdited);
        bluehigh->setEditedState (pedited->wavelet.bluehigh ? Edited : UnEdited);

        median->set_inconsistent (!pedited->wavelet.median);
        medianlev->set_inconsistent (!pedited->wavelet.medianlev);
        linkedg->set_inconsistent (!pedited->wavelet.linkedg);
//      edgreinf->set_inconsistent (!pedited->wavelet.edgreinf);
        cbenab->set_inconsistent (!pedited->wavelet.cbenab);
        lipst->set_inconsistent (!pedited->wavelet.lipst);
        contrast->setEditedState (pedited->wavelet.contrast ? Edited : UnEdited);
        edgrad->setEditedState (pedited->wavelet.edgrad ? Edited : UnEdited);
        edgval->setEditedState (pedited->wavelet.edgval ? Edited : UnEdited);
        thr->setEditedState (pedited->wavelet.thr ? Edited : UnEdited);
        thrH->setEditedState (pedited->wavelet.thrH ? Edited : UnEdited);
        skinprotect->setEditedState (pedited->wavelet.skinprotect ? Edited : UnEdited);
        hueskin->setEditedState     (pedited->wavelet.hueskin ? Edited : UnEdited);
        hueskin2->setEditedState    (pedited->wavelet.hueskin2 ? Edited : UnEdited);
        hueskinsty->setEditedState     (pedited->wavelet.hueskinsty ? Edited : UnEdited);
        hllev->setEditedState   (pedited->wavelet.hllev ? Edited : UnEdited);
        bllev->setEditedState   (pedited->wavelet.bllev ? Edited : UnEdited);
        pastlev->setEditedState     (pedited->wavelet.pastlev ? Edited : UnEdited);
        satlev->setEditedState  (pedited->wavelet.satlev ? Edited : UnEdited);
        strength->setEditedState (pedited->wavelet.strength ? Edited : UnEdited);
        edgcont->setEditedState     (pedited->wavelet.edgcont ? Edited : UnEdited);
        level0noise->setEditedState     (pedited->wavelet.level0noise ? Edited : UnEdited);
        level1noise->setEditedState     (pedited->wavelet.level1noise ? Edited : UnEdited);
        level2noise->setEditedState     (pedited->wavelet.level2noise ? Edited : UnEdited);
        level3noise->setEditedState     (pedited->wavelet.level3noise ? Edited : UnEdited);
        mergeL->setEditedState (pedited->wavelet.mergeL ? Edited : UnEdited);
        mergeC->setEditedState (pedited->wavelet.mergeC ? Edited : UnEdited);
        gain->setEditedState (pedited->wavelet.gain ? Edited : UnEdited);
        offs->setEditedState (pedited->wavelet.offs ? Edited : UnEdited);
        str->setEditedState (pedited->wavelet.str ? Edited : UnEdited);
        neigh->setEditedState (pedited->wavelet.neigh ? Edited : UnEdited);
        vart->setEditedState (pedited->wavelet.vart ? Edited : UnEdited);
        limd->setEditedState (pedited->wavelet.limd ? Edited : UnEdited);
        scale->setEditedState (pedited->wavelet.scale ? Edited : UnEdited);
        chrrt->setEditedState (pedited->wavelet.chrrt ? Edited : UnEdited);
        radius->setEditedState       (pedited->wavelet.radius ? Edited : UnEdited);
        highlights->setEditedState   (pedited->wavelet.highlights ? Edited : UnEdited);
        h_tonalwidth->setEditedState (pedited->wavelet.htonalwidth ? Edited : UnEdited);
        shadows->setEditedState      (pedited->wavelet.shadows ? Edited : UnEdited);
        s_tonalwidth->setEditedState (pedited->wavelet.stonalwidth ? Edited : UnEdited);

        for (int i = 0; i < 9; i++) {
            correction[i]->setEditedState (pedited->wavelet.c[i] ? Edited : UnEdited);
        }

        for (int i = 0; i < 9; i++) {
            correctionch[i]->setEditedState (pedited->wavelet.ch[i] ? Edited : UnEdited);
        }

        for (int i = 0; i < 9; i++) {
            balmer[i]->setEditedState (pedited->wavelet.bm[i] ? Edited : UnEdited);
        }

        for (int i = 0; i < 9; i++) {
            balmer2[i]->setEditedState (pedited->wavelet.bm2[i] ? Edited : UnEdited);
        }

    }

    /*****************************************************************************************************
     *
     *        Update the GUI, all at once if not in Batch editing (in this case, display EVERYTHING)
     *
     *****************************************************************************************************/

    if (!batchMode) {
        int y;
        y = thres->getValue();
        int z;

        if (y == 2) {
            level2noise->set_sensitive (false);
            level3noise->set_sensitive (false);
        } else if (y == 3) {
            level3noise->set_sensitive (false);
            level2noise->set_sensitive (true);
        } else {
            level2noise->set_sensitive (true);
            level3noise->set_sensitive (true);
        }



        for (z = y; z < 9; z++) {
            correction[z]->hide();
        }

        for (z = 0; z < y; z++) {
            correction[z]->show();
        }

        for (z = y; z < 9; z++) {
            balmer[z]->hide();
        }

        for (z = 0; z < y; z++) {
            balmer[z]->show();
        }

        for (z = y; z < 9; z++) {
            balmer2[z]->hide();
        }

        for (z = 0; z < y; z++) {
            balmer2[z]->show();
        }

        if (pp->wavelet.CHSLmethod == "SL") {
            for (z = y; z < 9; z++) {
                correctionch[z]->hide();
            }

            for (z = 0; z < y; z++) {
                correctionch[z]->show();
            }
        }

        //adjusterUpdateUI(tmrs);
        HSmethodUpdateUI();
        CHmethodUpdateUI();
        //MedgreinfUpdateUI();
        //CHSLmethodUpdateUI();
        EDmethodUpdateUI();
        NPmethodUpdateUI();
        BAmethodUpdateUI();
        TMmethodUpdateUI();
        //BackmethodUpdateUI();
        CLmethodChanged();
        CLmethodUpdateUI();
        lipstUpdateUI ();
        //TilesmethodUpdateUI();
        //daubcoeffmethodUpdateUI();
        //DirmethodUpdateUI();
        //LmethodUpdateUI();
        enabledUpdateUI ();
        medianlevUpdateUI ();
        cbenabUpdateUI ();
        retinexMethodChanged ();
        retinexMethodproChanged ();
        mergevMethodChanged ();
        mergMethodChanged ();
        mergMethod2Changed ();
        mergBMethodChanged ();
        ushamethodChanged ();
        usharpmethodChanged ();
        shapMethodChanged ();

        if (z == 9) {
            sup->show();
        } else {
            sup->hide();
        }
    }

    /*****************************************************************************************************
     *
     *                                        Enable listeners and signals
     *
     *****************************************************************************************************/

    Lmethodconn.block (false);
    CLmethodconn.block (false);
    Backmethodconn.block (false);
    Tilesmethodconn.block (false);
    usharpmethodconn.block (false);
    ushamethodconn.block (false);
    daubcoeffmethodconn.block (false);
    CHmethodconn.block (false);
    CHSLmethodconn.block (false);
    EDmethodconn.block (false);
    NPmethodconn.block (false);
    BAmethodconn.block (false);
    TMmethodconn.block (false);
    HSmethodconn.block (false);
    Dirmethodconn.block (false);
    MedgreinfConn.block (false);
    enableChromaConn.block (false);
    enableContrastConn.block (false);
    enableEdgeConn.block (false);
    enableFinalConn.block (false);
    enableNoiseConn.block (false);
    enableResidConn.block (false);
    enableMergeConn.block (false);
    enableToningConn.block (false);
    retinexMethodConn.block (false);
    retinexMethodproConn.block (false);
    mergevMethodConn.block (false);
    mergMethodConn.block (false);
    mergMethod2Conn.block (false);
    mergBMethodConn.block (false);
    shapMethodConn.block (false);

    enableListener ();

}

void Wavelet::setEditProvider  (EditDataProvider *provider)
{
    ccshape->setEditProvider (provider);
    cTshape->setEditProvider (provider);
    cTgainshape->setEditProvider (provider);
    cstyshape->setEditProvider (provider);
    cstyshape2->setEditProvider (provider);
    cmergshape->setEditProvider (provider);
    cmerg2shape->setEditProvider (provider);
    opacityShapeRG->setEditProvider (provider);
    opacityShapeBY->setEditProvider (provider);
    opacityShape->setEditProvider (provider);
    opacityShapeWL->setEditProvider (provider);
    hhshape->setEditProvider (provider);
    shstyshape->setEditProvider (provider);
    Chshape->setEditProvider (provider);
    clshape->setEditProvider (provider);
}

void Wavelet::autoOpenCurve ()
{
    ccshape->openIfNonlinear();
    cTshape->openIfNonlinear();
    cTgainshape->openIfNonlinear();
    cmergshape->openIfNonlinear();
    cmerg2shape->openIfNonlinear();
    cstyshape->openIfNonlinear();
    cstyshape2->openIfNonlinear();
    //opacityShapeRG->openIfNonlinear();
    //opacityShapeBY->openIfNonlinear();
}

void Wavelet::write (ProcParams* pp, ParamsEdited* pedited)
{

    //  pp->wavelet.input = inputFile->get_filename();
    if (Glib::file_test (inputeFile->get_filename (), Glib::FILE_TEST_EXISTS) && !Glib::file_test (inputeFile->get_filename (), Glib::FILE_TEST_IS_DIR)) {
        pp->wavelet.inpute = "file:" + inputeFile->get_filename ();
    } else {
        pp->wavelet.inpute = "file:lab.dat";    // just a directory
    }

//printf("save Wave\n");
    Glib::ustring p = Glib::path_get_dirname (inputeFile->get_filename ());

    pp->wavelet.enabled        = getEnabled();
    pp->wavelet.avoid          = avoid->get_active ();
    pp->wavelet.tmr            = tmr->get_active ();
    pp->wavelet.rescon         = rescon->getValue();
    pp->wavelet.resconH        = resconH->getValue();
    pp->wavelet.reschro        = reschro->getValue();
    pp->wavelet.tmrs           = tmrs->getValue();
    pp->wavelet.gamma          = gamma->getValue();
    pp->wavelet.sup            = sup->getValue();
    pp->wavelet.sky            = sky->getValue();
    pp->wavelet.thres          = thres->getValue();
    pp->wavelet.chroma         = chroma->getValue();
    pp->wavelet.chro           = chro->getValue();
    pp->wavelet.median         = median->get_active ();
    pp->wavelet.medianlev      = medianlev->get_active ();
    pp->wavelet.linkedg        = linkedg->get_active ();
//  pp->wavelet.edgreinf       = edgreinf->get_active ();
    pp->wavelet.cbenab         = cbenab->get_active ();
    pp->wavelet.lipst          = lipst->get_active ();
    pp->wavelet.contrast       = contrast->getValue();
    pp->wavelet.edgrad         = edgrad->getValue();
    pp->wavelet.edgval         = edgval->getValue();
    pp->wavelet.edgthresh      = edgthresh->getValue();
    pp->wavelet.thr            = thr->getValue();
    pp->wavelet.thrH           = thrH->getValue();
    pp->wavelet.hueskin        = hueskin->getValue<int> ();
    pp->wavelet.hueskin2       = hueskin2->getValue<int> ();
    pp->wavelet.hueskinsty        = hueskinsty->getValue<int> ();
    pp->wavelet.skinprotect    = skinprotect->getValue();
    pp->wavelet.threshold      = threshold->getValue();
    pp->wavelet.threshold2     = threshold2->getValue();
    pp->wavelet.edgedetect     = edgedetect->getValue();
    pp->wavelet.edgedetectthr  = edgedetectthr->getValue();
    pp->wavelet.edgedetectthr2 = edgedetectthr2->getValue();
    pp->wavelet.edgesensi     = edgesensi->getValue();
    pp->wavelet.edgeampli     = edgeampli->getValue();
    pp->wavelet.hllev          = hllev->getValue<int> ();
    pp->wavelet.bllev          = bllev->getValue<int> ();
    pp->wavelet.edgcont        = edgcont->getValue<int> ();
    pp->wavelet.level0noise    = level0noise->getValue<double> ();
    pp->wavelet.level1noise    = level1noise->getValue<double> ();
    pp->wavelet.level2noise    = level2noise->getValue<double> ();
    pp->wavelet.level3noise    = level3noise->getValue<double> ();
    pp->wavelet.ccwcurve       = ccshape->getCurve ();
    pp->wavelet.ccwTgaincurve       = cTgainshape->getCurve ();
    pp->wavelet.ccwTcurve       = cTshape->getCurve ();
    pp->wavelet.ccwmergcurve       = cmergshape->getCurve ();
    pp->wavelet.ccwmerg2curve       = cmerg2shape->getCurve ();
    pp->wavelet.ccwstycurve       = cstyshape->getCurve ();
    pp->wavelet.ccwsty2curve       = cstyshape2->getCurve ();
    pp->wavelet.opacityCurveRG = opacityShapeRG->getCurve ();
    pp->wavelet.opacityCurveBY = opacityShapeBY->getCurve ();
    pp->wavelet.opacityCurveW  = opacityShape->getCurve ();
    pp->wavelet.opacityCurveWL = opacityShapeWL->getCurve ();
    pp->wavelet.hhcurve        = hhshape->getCurve ();
    pp->wavelet.shstycurve        = shstyshape->getCurve ();
    pp->wavelet.Chcurve        = Chshape->getCurve ();
    pp->wavelet.pastlev        = pastlev->getValue<int> ();
    pp->wavelet.satlev         = satlev->getValue<int> ();
    pp->wavelet.strength       = (int) strength->getValue();
    pp->wavelet.balance        = (int) balance->getValue();
    pp->wavelet.balanleft        = (int) balanleft->getValue();
    pp->wavelet.sizelab        = (int) sizelab->getValue();
    pp->wavelet.balmerch        = (int) balmerch->getValue();
    pp->wavelet.shapedetcolor        = (int) shapedetcolor->getValue();
    pp->wavelet.dirV        = (int) dirV->getValue();
    pp->wavelet.dirH        = (int) dirH->getValue();
    pp->wavelet.shapind        = (int) shapind->getValue();
    pp->wavelet.balmerres        = (int) balmerres->getValue();
    pp->wavelet.balmerres2        = (int) balmerres2->getValue();
    pp->wavelet.dirD        = (int) dirD->getValue();
    pp->wavelet.balanhig        = (int) balanhig->getValue();
    pp->wavelet.grad        = grad->getValue();
    pp->wavelet.blend        = (int) blend->getValue();
    pp->wavelet.blendc        = (int) blendc->getValue();
    pp->wavelet.mergeL       = (int) mergeL->getValue();
    pp->wavelet.mergeC       = (int) mergeC->getValue();
    nextmergeL = pp->wavelet.mergeL;
    nextmergeC = pp->wavelet.mergeC;
    pp->wavelet.gain       = gain->getValue();
    pp->wavelet.offs       = offs->getValue();
    pp->wavelet.str       = str->getValue();
    pp->wavelet.neigh       = neigh->getValue();
    pp->wavelet.vart       = vart->getValue();
    pp->wavelet.limd       = limd->getValue();
    pp->wavelet.scale       = (int)scale->getValue();
    pp->wavelet.chrrt       = chrrt->getValue();
    pp->wavelet.radius        = (int)radius->getValue ();
    pp->wavelet.highlights    = (int)highlights->getValue ();
    pp->wavelet.htonalwidth   = (int)h_tonalwidth->getValue ();
    pp->wavelet.shadows       = (int)shadows->getValue ();
    pp->wavelet.stonalwidth   = (int)s_tonalwidth->getValue ();

    pp->wavelet.greenlow       = greenlow->getValue ();
    pp->wavelet.bluelow        = bluelow->getValue ();
    pp->wavelet.greenmed       = greenmed->getValue ();
    pp->wavelet.bluemed        = bluemed->getValue ();
    pp->wavelet.greenhigh      = greenhigh->getValue ();
    pp->wavelet.bluehigh       = bluehigh->getValue ();
    pp->wavelet.expcontrast    = expcontrast->getEnabled();
    pp->wavelet.expchroma      = expchroma->getEnabled();
    pp->wavelet.expedge        = expedge->getEnabled();
    pp->wavelet.expreti        = expreti->getEnabled();
    pp->wavelet.expresid       = expresid->getEnabled();
//   pp->wavelet.expTCresi    = expTCresi->getEnabled();
    pp->wavelet.expmerge       = expmerge->getEnabled();
    pp->wavelet.expfinal       = expfinal->getEnabled();
    pp->wavelet.exptoning      = exptoning->getEnabled();
    pp->wavelet.expnoise       = expnoise->getEnabled();

    pp->wavelet.iter = (int) iter->getValue();
    pp->wavelet.wavclCurve = clshape->getCurve ();

    for (int i = 0; i < 9; i++) {
        pp->wavelet.c[i] = (int) correction[i]->getValue();
    }

    for (int i = 0; i < 9; i++) {
        pp->wavelet.ch[i] = (int) correctionch[i]->getValue();
    }

    for (int i = 0; i < 9; i++) {
        pp->wavelet.bm[i] = (int) balmer[i]->getValue();
    }

    for (int i = 0; i < 9; i++) {
        pp->wavelet.bm2[i] = (int) balmer2[i]->getValue();
    }

    if (pedited) {
        pedited->wavelet.inpute = inChanged;

        pedited->wavelet.enabled         = !get_inconsistent();
        pedited->wavelet.retinexMethod    = retinexMethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.retinexMethodpro    = retinexMethodpro->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.mergevMethod    = mergevMethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.mergMethod    = mergMethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.mergMethod2    = mergMethod2->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.mergBMethod    = mergBMethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.shapMethod    = shapMethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.avoid           = !avoid->get_inconsistent();
        pedited->wavelet.tmr             = !tmr->get_inconsistent();
        pedited->wavelet.median          = !median->get_inconsistent();
        pedited->wavelet.medianlev       = !medianlev->get_inconsistent();
        pedited->wavelet.linkedg         = !linkedg->get_inconsistent();
        pedited->wavelet.cbenab          = !cbenab->get_inconsistent();
        pedited->wavelet.lipst           = !lipst->get_inconsistent();
        pedited->wavelet.Medgreinf       =  Medgreinf->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.Lmethod         = Lmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.CLmethod        = CLmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.Backmethod      = Backmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.Tilesmethod     = Tilesmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.usharpmethod     = usharpmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.ushamethod     = ushamethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.daubcoeffmethod = daubcoeffmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.CHmethod        = CHmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.CHSLmethod      = CHSLmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.EDmethod        = EDmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.NPmethod        = NPmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.BAmethod        = BAmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.TMmethod        = TMmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.HSmethod        = HSmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.Dirmethod       = Dirmethod->get_active_text() != M ("GENERAL_UNCHANGED");
        pedited->wavelet.edgthresh       = edgthresh->getEditedState();
        pedited->wavelet.rescon          = rescon->getEditedState();
        pedited->wavelet.resconH         = resconH->getEditedState();
        pedited->wavelet.reschro         = reschro->getEditedState();
        pedited->wavelet.tmrs            = tmrs->getEditedState();
        pedited->wavelet.gamma           = gamma->getEditedState();
        pedited->wavelet.sup             = sup->getEditedState();
        pedited->wavelet.sky             = sky->getEditedState();
        pedited->wavelet.thres           = thres->getEditedState();
        pedited->wavelet.threshold       = threshold->getEditedState();
        pedited->wavelet.threshold2      = threshold2->getEditedState();
        pedited->wavelet.edgedetect      = edgedetect->getEditedState();
        pedited->wavelet.edgedetectthr   = edgedetectthr->getEditedState();
        pedited->wavelet.edgedetectthr2  = edgedetectthr2->getEditedState();
        pedited->wavelet.edgesensi       = edgesensi->getEditedState();
        pedited->wavelet.edgeampli       = edgeampli->getEditedState();
        pedited->wavelet.chroma          = chroma->getEditedState();
        pedited->wavelet.chro            = chro->getEditedState();
        pedited->wavelet.contrast        = contrast->getEditedState();
        pedited->wavelet.edgrad          = edgrad->getEditedState();
        pedited->wavelet.edgval          = edgval->getEditedState();
        pedited->wavelet.thr             = thr->getEditedState();
        pedited->wavelet.thrH            = thrH->getEditedState();
        pedited->wavelet.hueskin         = hueskin->getEditedState ();
        pedited->wavelet.hueskin2        = hueskin2->getEditedState ();
        pedited->wavelet.hueskinsty         = hueskinsty->getEditedState ();
        pedited->wavelet.skinprotect     = skinprotect->getEditedState();
        pedited->wavelet.hllev           = hllev->getEditedState ();
        pedited->wavelet.ccwcurve        = !ccshape->isUnChanged ();
        pedited->wavelet.ccwTcurve        = !cTshape->isUnChanged ();
        pedited->wavelet.ccwTgaincurve        = !cTgainshape->isUnChanged ();
        pedited->wavelet.ccwmergcurve        = !cmergshape->isUnChanged ();
        pedited->wavelet.ccwmerg2curve        = !cmerg2shape->isUnChanged ();
        pedited->wavelet.ccwstycurve        = !cstyshape->isUnChanged ();
        pedited->wavelet.ccwsty2curve        = !cstyshape2->isUnChanged ();
        pedited->wavelet.edgcont         = edgcont->getEditedState ();
        pedited->wavelet.level0noise     = level0noise->getEditedState ();
        pedited->wavelet.level1noise     = level1noise->getEditedState ();
        pedited->wavelet.level2noise     = level2noise->getEditedState ();
        pedited->wavelet.level3noise     = level3noise->getEditedState ();
        pedited->wavelet.opacityCurveRG  = !opacityShapeRG->isUnChanged ();
        pedited->wavelet.opacityCurveBY  = !opacityShapeBY->isUnChanged ();
        pedited->wavelet.opacityCurveW   = !opacityShape->isUnChanged ();
        pedited->wavelet.opacityCurveWL  = !opacityShapeWL->isUnChanged ();
        pedited->wavelet.hhcurve         = !hhshape->isUnChanged ();
        pedited->wavelet.shstycurve         = !shstyshape->isUnChanged ();
        pedited->wavelet.Chcurve         = !Chshape->isUnChanged ();
        pedited->wavelet.bllev           = bllev->getEditedState ();
        pedited->wavelet.pastlev         = pastlev->getEditedState ();
        pedited->wavelet.satlev          = satlev->getEditedState ();
        pedited->wavelet.strength        = strength->getEditedState ();
        pedited->wavelet.mergeL        = mergeL->getEditedState ();
        pedited->wavelet.mergeC        = mergeC->getEditedState ();
        pedited->wavelet.gain        = gain->getEditedState ();
        pedited->wavelet.offs        = offs->getEditedState ();
        pedited->wavelet.vart        = vart->getEditedState ();
        pedited->wavelet.limd        = limd->getEditedState ();
        pedited->wavelet.scale        = scale->getEditedState ();
        pedited->wavelet.chrrt        = chrrt->getEditedState ();
        pedited->wavelet.str        = str->getEditedState ();
        pedited->wavelet.neigh        = neigh->getEditedState ();
        pedited->wavelet.radius          = radius->getEditedState ();
        pedited->wavelet.highlights      = highlights->getEditedState ();
        pedited->wavelet.htonalwidth     = h_tonalwidth->getEditedState ();
        pedited->wavelet.shadows         = shadows->getEditedState ();
        pedited->wavelet.stonalwidth     = s_tonalwidth->getEditedState ();

        pedited->wavelet.greenlow        = greenlow->getEditedState ();
        pedited->wavelet.bluelow         = bluelow->getEditedState ();
        pedited->wavelet.greenmed        = greenmed->getEditedState ();
        pedited->wavelet.bluemed         = bluemed->getEditedState ();
        pedited->wavelet.greenhigh       = greenhigh->getEditedState ();
        pedited->wavelet.bluehigh        = bluehigh->getEditedState ();
        pedited->wavelet.balance         = balance->getEditedState ();
        pedited->wavelet.balanleft         = balanleft->getEditedState ();
        pedited->wavelet.balanhig        = balanhig->getEditedState ();
        pedited->wavelet.sizelab        = sizelab->getEditedState ();
        pedited->wavelet.balmerch        = balmerch->getEditedState ();
        pedited->wavelet.shapedetcolor        = shapedetcolor->getEditedState ();
        pedited->wavelet.dirV        = dirV->getEditedState ();
        pedited->wavelet.dirH        = dirH->getEditedState ();
        pedited->wavelet.dirD        = dirD->getEditedState ();
        pedited->wavelet.shapind        = shapind->getEditedState ();
        pedited->wavelet.balmerres        = balmerres->getEditedState ();
        pedited->wavelet.balmerres2        = balmerres2->getEditedState ();
        pedited->wavelet.grad        = grad->getEditedState ();
        pedited->wavelet.blend         = blend->getEditedState ();
        pedited->wavelet.blendc         = blendc->getEditedState ();
        pedited->wavelet.iter            = iter->getEditedState ();
        pedited->wavelet.wavclCurve      = !clshape->isUnChanged ();
        pedited->wavelet.expcontrast     = !expcontrast->get_inconsistent();
        pedited->wavelet.expchroma       = !expchroma->get_inconsistent();
        pedited->wavelet.expedge         = !expedge->get_inconsistent();
        pedited->wavelet.expreti         = !expreti->get_inconsistent();
        pedited->wavelet.expresid        = !expresid->get_inconsistent();
//        pedited->wavelet.expTCresi     = !expTCresi->get_inconsistent();
        pedited->wavelet.expmerge        = !expmerge->get_inconsistent();
        pedited->wavelet.expfinal        = !expfinal->get_inconsistent();
        pedited->wavelet.exptoning       = !exptoning->get_inconsistent();
        pedited->wavelet.expnoise        = !expnoise->get_inconsistent();

        for (int i = 0; i < 9; i++) {
            pedited->wavelet.c[i]        = correction[i]->getEditedState();
        }

        for (int i = 0; i < 9; i++) {
            pedited->wavelet.ch[i]       = correctionch[i]->getEditedState();
        }

        for (int i = 0; i < 9; i++) {
            pedited->wavelet.bm[i]        = balmer[i]->getEditedState();
        }

        for (int i = 0; i < 9; i++) {
            pedited->wavelet.bm2[i]        = balmer2[i]->getEditedState();
        }

    }

    if (CHmethod->get_active_row_number() == 0) {
        pp->wavelet.CHmethod = "without";
    } else if (CHmethod->get_active_row_number() == 1) {
        pp->wavelet.CHmethod = "with";
    } else if (CHmethod->get_active_row_number() == 2) {
        pp->wavelet.CHmethod = "link";
    }

    if (mergMethod->get_active_row_number() == 0) {
        pp->wavelet.mergMethod = "savwat";
    } else if (mergMethod->get_active_row_number() == 1) {
        pp->wavelet.mergMethod = "savhdr";
    } else if (mergMethod->get_active_row_number() == 2) {
        pp->wavelet.mergMethod = "savzero";
    } else if (mergMethod->get_active_row_number() == 3) {
        pp->wavelet.mergMethod = "load";
    } else if (mergMethod->get_active_row_number() == 4) {
        pp->wavelet.mergMethod = "loadzero";
    } else if (mergMethod->get_active_row_number() == 5) {
        pp->wavelet.mergMethod = "loadzerohdr";
    }

    if (mergMethod2->get_active_row_number() == 0) {
        pp->wavelet.mergMethod2 = "befo";
    } else if (mergMethod2->get_active_row_number() == 1) {
        pp->wavelet.mergMethod2 = "after";
    }

    if (mergevMethod->get_active_row_number() == 0) {
        pp->wavelet.mergevMethod = "curr";
    } else if (mergevMethod->get_active_row_number() == 1) {
        pp->wavelet.mergevMethod = "cuno";
    } else if (mergevMethod->get_active_row_number() == 2) {
        pp->wavelet.mergevMethod = "first";
    } else if (mergevMethod->get_active_row_number() == 3) {
        pp->wavelet.mergevMethod = "save";
    }

//   if (mergBMethod->get_active_row_number() == 0) {
//       pp->wavelet.mergBMethod = "water";
//   } else
    if (mergBMethod->get_active_row_number() == 0) {
        pp->wavelet.mergBMethod = "hdr1";
    } else if (mergBMethod->get_active_row_number() == 1) {
        pp->wavelet.mergBMethod = "hdr2";
    }


    if (Medgreinf->get_active_row_number() == 0) {
        pp->wavelet.Medgreinf = "more";
    } else if (Medgreinf->get_active_row_number() == 1) {
        pp->wavelet.Medgreinf = "none";
    } else if (Medgreinf->get_active_row_number() == 2) {
        pp->wavelet.Medgreinf = "less";
    }

    if (CHSLmethod->get_active_row_number() == 0) {
        pp->wavelet.CHSLmethod = "SL";
    } else if (CHSLmethod->get_active_row_number() == 1) {
        pp->wavelet.CHSLmethod = "CU";
    }

    if (EDmethod->get_active_row_number() == 0) {
        pp->wavelet.EDmethod = "SL";
    } else if (EDmethod->get_active_row_number() == 1) {
        pp->wavelet.EDmethod = "CU";
    }

    if (NPmethod->get_active_row_number() == 0) {
        pp->wavelet.NPmethod = "none";
    } else if (NPmethod->get_active_row_number() == 1) {
        pp->wavelet.NPmethod = "low";
    } else if (NPmethod->get_active_row_number() == 2) {
        pp->wavelet.NPmethod = "high";
    }

    if (BAmethod->get_active_row_number() == 0) {
        pp->wavelet.BAmethod = "none";
    } else if (BAmethod->get_active_row_number() == 1) {
        pp->wavelet.BAmethod = "sli";
    } else if (BAmethod->get_active_row_number() == 2) {
        pp->wavelet.BAmethod = "cur";
    }

    if (retinexMethod->get_active_row_number() == 0) {
        pp->wavelet.retinexMethod = "low";
    } else if (retinexMethod->get_active_row_number() == 1) {
        pp->wavelet.retinexMethod = "uni";
    } else if (retinexMethod->get_active_row_number() == 2) {
        pp->wavelet.retinexMethod = "high";
//   } else if (retinexMethod->get_active_row_number() == 3) {
//       pp->wavelet.retinexMethod = "high";
    }

    if (retinexMethodpro->get_active_row_number() == 0) {
        pp->wavelet.retinexMethodpro = "resid";
    } else if (retinexMethodpro->get_active_row_number() == 1) {
        pp->wavelet.retinexMethodpro = "fina";
    }

    if (shapMethod->get_active_row_number() == 0) {
        pp->wavelet.shapMethod = "norm";
    } else if (shapMethod->get_active_row_number() == 1) {
        pp->wavelet.shapMethod = "three";
    } else if (shapMethod->get_active_row_number() == 2) {
        pp->wavelet.shapMethod = "four";
    }


    if (TMmethod->get_active_row_number() == 0) {
        pp->wavelet.TMmethod = "cont";
    } else if (TMmethod->get_active_row_number() == 1) {
        pp->wavelet.TMmethod = "tm";
    }

    //  else if (TMmethod->get_active_row_number()==2)
    //      pp->wavelet.TMmethod = "both";

    if (HSmethod->get_active_row_number() == 0) {
        pp->wavelet.HSmethod = "without";
    } else if (HSmethod->get_active_row_number() == 1) {
        pp->wavelet.HSmethod = "with";
    }

    if (CLmethod->get_active_row_number() == 0) {
        pp->wavelet.CLmethod = "one";
    } else if (CLmethod->get_active_row_number() == 1) {
        pp->wavelet.CLmethod = "inf";
    } else if (CLmethod->get_active_row_number() == 2) {
        pp->wavelet.CLmethod = "sup";
    } else if (CLmethod->get_active_row_number() == 3) {
        pp->wavelet.CLmethod = "all";
    }

    if (Backmethod->get_active_row_number() == 0) {
        pp->wavelet.Backmethod = "black";
    } else if (Backmethod->get_active_row_number() == 1) {
        pp->wavelet.Backmethod = "grey";
    } else if (Backmethod->get_active_row_number() == 2) {
        pp->wavelet.Backmethod = "resid";
    }

    if (Tilesmethod->get_active_row_number() == 0) {
        pp->wavelet.Tilesmethod = "full";
    } else if (Tilesmethod->get_active_row_number() == 1) {
        pp->wavelet.Tilesmethod = "big";
    } else if (Tilesmethod->get_active_row_number() == 2) {
        pp->wavelet.Tilesmethod = "lit";
    }

    if (daubcoeffmethod->get_active_row_number() == 0) {
        pp->wavelet.daubcoeffmethod = "2_";
    } else if (daubcoeffmethod->get_active_row_number() == 1) {
        pp->wavelet.daubcoeffmethod = "4_";
    } else if (daubcoeffmethod->get_active_row_number() == 2) {
        pp->wavelet.daubcoeffmethod = "6_";
    } else if (daubcoeffmethod->get_active_row_number() == 3) {
        pp->wavelet.daubcoeffmethod = "10_";
    } else if (daubcoeffmethod->get_active_row_number() == 4) {
        pp->wavelet.daubcoeffmethod = "14_";
    }

    if (Dirmethod->get_active_row_number() == 0) {
        pp->wavelet.Dirmethod = "one";
    } else if (Dirmethod->get_active_row_number() == 1) {
        pp->wavelet.Dirmethod = "two";
    } else if (Dirmethod->get_active_row_number() == 2) {
        pp->wavelet.Dirmethod = "thr";
    } else if (Dirmethod->get_active_row_number() == 3) {
        pp->wavelet.Dirmethod = "all";
    }

    if (usharpmethod->get_active_row_number() == 0) {
        pp->wavelet.usharpmethod = "orig";
    } else if (usharpmethod->get_active_row_number() == 1) {
        pp->wavelet.usharpmethod = "wave";
    }

    if (ushamethod->get_active_row_number() == 0) {
        pp->wavelet.ushamethod = "none";
    } else if (ushamethod->get_active_row_number() == 1) {
        pp->wavelet.ushamethod = "sharp";
    } else if (ushamethod->get_active_row_number() == 2) {
        pp->wavelet.ushamethod = "clari";
    }

    pp->wavelet.Lmethod = Lmethod->get_active_row_number() + 1;
	
}

void Wavelet::mergMethod2Changed()
{
    if (listener) {
        listener->panelChanged (EvWavmergMethod2, mergMethod2->get_active_text ());
    }

}

void Wavelet::mergMethodChanged()
{
    int y = thres->getValue();

    if (mergMethod->get_active_row_number() == 0 && expmerge->getEnabled() == true) { //save water
        blend->hide();
        blendc->hide();
        balanhig->hide();
        CCWcurveEditormerg->hide();
        CCWcurveEditormerg2->hide();
        labmmgB->hide();
        grad->hide();
        balanleft->hide();
        sizelab->hide();
        balmerch->hide();
        shapedetcolor->hide();
        dirV->hide();
        dirD->hide();
        dirH->hide();
        shapind->hide();
        balmerres->hide();
        balmerres2->hide();
        balMFrame->hide();
        hueskinsty->hide();
        neutral2->hide();
        hbin->hide();
        mergBMethod->hide();
        savelab->show();
        mergevMethod->set_active (3);
        Backmethod->set_active (0);
        CLmethod->set_active (1);
        Lmethod->set_active (6);
        Dirmethod->set_active (3);
        Lmethod->set_sensitive (true);
        Dirmethod->set_sensitive (true);
        mg2box->hide();

        for (int z = 0; z < 9; z++) {
            balmer[z]->hide();
            balmer2[z]->hide();
        }


    } else if (mergMethod->get_active_row_number() == 1   && expmerge->getEnabled() == true) { //save hdr
        blend->hide();
        blendc->hide();
        CCWcurveEditormerg->hide();
        CCWcurveEditormerg2->hide();
        labmmgB->hide();
        balanhig->hide();
        grad->hide();
        balanleft->hide();
        sizelab->hide();
        balmerch->hide();
        neutral2->hide();
        shapedetcolor->hide();
        dirV->hide();
        dirD->hide();
        dirH->hide();
        shapind->hide();
        hueskinsty->hide();
        balmerres->hide();
        balmerres2->hide();
        hbin->hide();
        savelab->show();
        mergBMethod->hide();
        mergevMethod->set_active (3);
        Backmethod->set_active (2);
        CLmethod->set_active (2);
        Lmethod->set_active (1);//or zero if need
        Dirmethod->set_active (3);
        Lmethod->set_sensitive (true);
        Dirmethod->set_sensitive (true);

        balMFrame->hide();
        mg2box->hide();

        for (int z = 0; z < 9; z++) {
            balmer[z]->hide();
            balmer2[z]->hide();

        }

    } else if (mergMethod->get_active_row_number() == 2   && expmerge->getEnabled() == true) { //save zero

        blend->hide();
        blendc->hide();
        CCWcurveEditormerg->hide();
        CCWcurveEditormerg2->hide();
        labmmgB->hide();
        balanhig->hide();
        grad->hide();
        balanleft->hide();
        sizelab->show();
        balmerch->hide();
        shapedetcolor->hide();
        dirV->hide();
        dirD->hide();
        dirH->hide();
        shapind->hide();
        hueskinsty->hide();
        balmerres->hide();
        balmerres2->hide();
        neutral2->hide();
        hbin->hide();
        savelab->show();
        mergBMethod->hide();
        mergevMethod->set_active (3);
        Backmethod->set_active (1);
        CLmethod->set_active (3);
        Lmethod->set_active (3);//or zero if need
        Dirmethod->set_active (3);
        Lmethod->set_sensitive (false);
        Dirmethod->set_sensitive (false);
        balMFrame->hide();
        mg2box->hide();



        for (int z = 0; z < 9; z++) {
            balmer[z]->hide();
            balmer2[z]->hide();

        }


    } else if (mergMethod->get_active_row_number() == 3) { //load
        blendc->show();
        labmmgB->show();
        sizelab->hide();
        balmerch->hide();
        shapedetcolor->hide();
        dirV->hide();
        dirD->hide();
        dirH->hide();
        shapind->hide();
        hueskinsty->hide();
        balmerres->hide();
        balmerres2->hide();
        neutral2->hide();
        hbin->show();
        savelab->hide();
        mergBMethod->show();
        mergevMethod->set_active (0);
        Dirmethod->set_active (3);
        Backmethod->set_active (1);
        CLmethod->set_active (3);
        Lmethod->set_active (3);
        balMFrame->hide();
        CCWcurveEditormerg2->hide();
        mg2box->hide();

        for (int z = 0; z < 9; z++) {
            balmer[z]->hide();
            balmer2[z]->hide();

        }

        if (mergBMethod->get_active_row_number() == 1) { //slider HDR
            blend->show();
            CCWcurveEditormerg->hide();
            grad->show();
        }

        if (mergBMethod->get_active_row_number() == 0) { //curve HDR
            blend->hide();
            CCWcurveEditormerg->show();
            grad->hide();
        }
    } else if (mergMethod->get_active_row_number() == 4 || mergMethod->get_active_row_number() == 5) { //load zero or zerohdr
        blendc->hide();
        balanhig->show();
        labmmgB->hide();
        balanleft->show();
        sizelab->hide();
        hbin->show();
        savelab->hide();
        mergBMethod->hide();
        mergevMethod->set_active (0);
        Dirmethod->set_active (3);
        Backmethod->set_active (1);
        CLmethod->set_active (3);
        Lmethod->set_active (3);
        balMFrame->show();
        Tilesmethod->set_active (0);//force full image
        mg2box->show();

        if (mergMethod->get_active_row_number() == 5) {
            CCWcurveEditormerg2->show();
        } else {
            CCWcurveEditormerg2->hide();
        }

        if (mergMethod->get_active_row_number() == 4) {
            for (int z = 0; z < y; z++) {
                balmer[z]->show();
                balmer2[z]->hide();
            }

            for (int z = y; z < 9; z++) {
                balmer[z]->hide();
                balmer2[z]->hide();
            }
        }

        if (mergMethod->get_active_row_number() == 5) {

            for (int z = 0; z < y; z++) {
                balmer2[z]->show();
                balmer[z]->hide();

            }

            for (int z = y; z < 9; z++) {
                balmer2[z]->hide();
                balmer[z]->hide();

            }


        }

        balmerch->show();

        if (mergMethod->get_active_row_number() == 5) {
            shapedetcolor->hide();
            hueskinsty->hide();
            dirV->hide();
            dirD->hide();
            dirH->hide();
            shapind->hide();
            balmerres->hide();
            balmerres2->show();
            balanhig->show();
            balanleft->show();
            CCWcurveEditorsty2->hide();
            CCWcurveEditorsty->show();
            shapMethod->hide();
            shapind->hide();

        } else {
            shapedetcolor->show();
            hueskinsty->show();
            dirV->show();
            dirD->show();
            dirH->show();
            shapind->show();
            balmerres->show();
            balmerres2->hide();
            balanhig->hide();
            balanleft->hide();
            CCWcurveEditorsty2->show();
            CCWcurveEditorsty->hide();
            shapMethod->show();

            if (shapMethod->get_active_row_number() == 0) {
                shapind->hide();
            } else {
                shapind->show();
            }



        }

        neutral2->show();

        blend->hide();
        CCWcurveEditormerg->hide();
        grad->hide();

    }

    //mergBMethodChanged();

    if (listener) {
        listener->panelChanged (EvWavmergMethod, mergMethod->get_active_text ());
    }

}



void Wavelet::mergevMethodChanged()
{

    if (mergevMethod->get_active_row_number() != 0) {
        mgVBox->hide();
    } else {
        mgVBox->show();
    }

    if (listener) {
        listener->panelChanged (EvWavmergevMethod, mergevMethod->get_active_text ());
    }

}

void Wavelet::mergBMethodChanged()
{
    if (mergBMethod->get_active_row_number() == 1) { //slider HDR
        grad->show();
        blend->show();
        CCWcurveEditormerg->hide();
    } else if (mergBMethod->get_active_row_number() == 0  && mergMethod->get_active_row_number() < 4) { //curve HDR
        grad->hide();
        blend->hide();
        CCWcurveEditormerg->show();
    } else {//watermark
        //   blend->show();
        //   grad->hide();
        //   CCWcurveEditormerg->hide();
    }

    if (listener) {
        listener->panelChanged (EvWavmergBMethod, mergBMethod->get_active_text ());
    }

}

void Wavelet::shapMethodChanged()
{
    if (shapMethod->get_active_row_number() == 0) {
        shapind->hide();
    } else {
        shapind->show();
    }

    if (listener) {
        listener->panelChanged (EvWavshapMethod, shapMethod->get_active_text ());
    }
}


void Wavelet::retinexMethodChanged()
{
    if (retinexMethodpro->get_active_row_number() == 1) {
        labretifin->set_sensitive (true);
    }

    if (listener) {
        listener->panelChanged (EvWavretinexMethod, retinexMethod->get_active_text ());
    }
}

void Wavelet::retinexMethodproChanged()
{
    if (retinexMethodpro->get_active_row_number() == 0) {
        labretifin->set_sensitive (false);
    } else if (retinexMethodpro->get_active_row_number() == 1 /*&& retinexMethod->get_active_row_number() != 0*/) {
        labretifin->set_sensitive (true);

    }

    if (listener) {
        listener->panelChanged (EvWavretinexMethodpro, retinexMethodpro->get_active_text ());
    }
}

void Wavelet::inputeChanged()
{

    Glib::ustring profname;
    inChanged = true;
    profname = inputeFile->get_filename ();

    if (listener && profname != oldip) {
        listener->panelChanged (EvWaveinput, profname);
    }

    oldip = profname;

}


void Wavelet::curveChanged (CurveEditor* ce)
{

    if (listener && getEnabled()) {
        if (ce == ccshape) {
            listener->panelChanged (EvWavCCCurve, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == cTshape) {
            listener->panelChanged (EvWavCTCurve, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == cTgainshape) {
            listener->panelChanged (EvWavCTgainCurve, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == cmergshape) {
            listener->panelChanged (EvWavmergCurve, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == cmerg2shape) {
            listener->panelChanged (EvWavmerg2Curve, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == cstyshape) {
            listener->panelChanged (EvWavstyCurve, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == cstyshape2) {
            listener->panelChanged (EvWavsty2Curve, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == opacityShapeRG) {
            listener->panelChanged (EvWavColor, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == opacityShapeBY) {
            listener->panelChanged (EvWavOpac, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == opacityShape) {
            listener->panelChanged (EvWavopacity, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == opacityShapeWL) {
            listener->panelChanged (EvWavopacityWL, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == hhshape) {
            listener->panelChanged (EvWavHHCurve, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == shstyshape) {
            listener->panelChanged (EvWavshstyCurve, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == Chshape) {
            listener->panelChanged (EvWavCHCurve, M ("HISTORY_CUSTOMCURVE"));
        } else if (ce == clshape) {
            listener->panelChanged (EvWavCLCurve, M ("HISTORY_CUSTOMCURVE"));
        }
    }
}

void Wavelet::setDefaults (const ProcParams* defParams, const ParamsEdited* pedited)
{

    for (int i = 0; i < 9; i++) {
        correction[i]->setDefault (defParams->wavelet.c[i]);
    }

    for (int i = 0; i < 9; i++) {
        correctionch[i]->setDefault (defParams->wavelet.ch[i]);
    }

    for (int i = 0; i < 9; i++) {
        balmer[i]->setDefault (defParams->wavelet.bm[i]);
    }

    for (int i = 0; i < 9; i++) {
        balmer2[i]->setDefault (defParams->wavelet.bm2[i]);
    }

    strength->setDefault (defParams->wavelet.strength );
    mergeL->setDefault (defParams->wavelet.mergeL );
    mergeC->setDefault (defParams->wavelet.mergeC );
    gain->setDefault (defParams->wavelet.gain );
    offs->setDefault (defParams->wavelet.offs );
    vart->setDefault (defParams->wavelet.vart );
    limd->setDefault (defParams->wavelet.limd );
    scale->setDefault (defParams->wavelet.scale );
    chrrt->setDefault (defParams->wavelet.chrrt );
    str->setDefault (defParams->wavelet.str );
    neigh->setDefault (defParams->wavelet.neigh );
    radius->setDefault (defParams->wavelet.radius);
    highlights->setDefault (defParams->wavelet.highlights);
    h_tonalwidth->setDefault (defParams->wavelet.htonalwidth);
    shadows->setDefault (defParams->wavelet.shadows);
    s_tonalwidth->setDefault (defParams->wavelet.stonalwidth);

    balance->setDefault (defParams->wavelet.balance );
    balanleft->setDefault (defParams->wavelet.balanleft );
    balanhig->setDefault (defParams->wavelet.balanhig );
    sizelab->setDefault (defParams->wavelet.sizelab );
    balmerch->setDefault (defParams->wavelet.balmerch );
    shapedetcolor->setDefault (defParams->wavelet.shapedetcolor );
    dirV->setDefault (defParams->wavelet.dirV );
    dirH->setDefault (defParams->wavelet.dirH );
    dirD->setDefault (defParams->wavelet.dirD );
    shapind->setDefault (defParams->wavelet.shapind );
    balmerres->setDefault (defParams->wavelet.balmerres );
    balmerres2->setDefault (defParams->wavelet.balmerres2 );
    grad->setDefault (defParams->wavelet.grad );
    blend->setDefault (defParams->wavelet.blend );
    blendc->setDefault (defParams->wavelet.blendc );
    iter->setDefault (defParams->wavelet.iter );
    rescon->setDefault (defParams->wavelet.rescon);
    resconH->setDefault (defParams->wavelet.resconH);
    reschro->setDefault (defParams->wavelet.reschro);
    tmrs->setDefault (defParams->wavelet.tmrs);
    gamma->setDefault (defParams->wavelet.gamma);
    sup->setDefault (defParams->wavelet.sup);
    sky->setDefault (defParams->wavelet.sky);
    thres->setDefault (defParams->wavelet.thres);
    threshold->setDefault (defParams->wavelet.threshold);
    threshold2->setDefault (defParams->wavelet.threshold2);
    edgedetect->setDefault (defParams->wavelet.edgedetect);
    edgedetectthr->setDefault (defParams->wavelet.edgedetectthr);
    edgedetectthr2->setDefault (defParams->wavelet.edgedetectthr2);
    edgesensi->setDefault (defParams->wavelet.edgesensi);
    edgeampli->setDefault (defParams->wavelet.edgeampli);
    chroma->setDefault (defParams->wavelet.chroma);
    chro->setDefault (defParams->wavelet.chro);
    contrast->setDefault (defParams->wavelet.contrast);
    edgrad->setDefault (defParams->wavelet.edgrad);
    edgval->setDefault (defParams->wavelet.edgval);
    edgthresh->setDefault (defParams->wavelet.edgthresh);
    thr->setDefault (defParams->wavelet.thr);
    thrH->setDefault (defParams->wavelet.thrH);
    hueskin->setDefault<int> (defParams->wavelet.hueskin);
    hueskin2->setDefault<int> (defParams->wavelet.hueskin2);
    hueskinsty->setDefault<int> (defParams->wavelet.hueskinsty);
    hllev->setDefault<int> (defParams->wavelet.hllev);
    bllev->setDefault<int> (defParams->wavelet.bllev);
    pastlev->setDefault<int> (defParams->wavelet.pastlev);
    satlev->setDefault<int> (defParams->wavelet.satlev);
    edgcont->setDefault<int> (defParams->wavelet.edgcont);
    level0noise->setDefault<double> (defParams->wavelet.level0noise);
    level1noise->setDefault<double> (defParams->wavelet.level1noise);
    level2noise->setDefault<double> (defParams->wavelet.level2noise);
    level3noise->setDefault<double> (defParams->wavelet.level3noise);

    greenlow->setDefault (defParams->wavelet.greenlow);
    bluelow->setDefault (defParams->wavelet.bluelow);
    greenmed->setDefault (defParams->wavelet.greenmed);
    bluemed->setDefault (defParams->wavelet.bluemed);
    greenhigh->setDefault (defParams->wavelet.greenhigh);
    bluehigh->setDefault (defParams->wavelet.bluehigh);

    if (pedited) {
        greenlow->setDefaultEditedState (pedited->wavelet.greenlow ? Edited : UnEdited);
        bluelow->setDefaultEditedState (pedited->wavelet.bluelow ? Edited : UnEdited);
        greenmed->setDefaultEditedState (pedited->wavelet.greenmed ? Edited : UnEdited);
        bluemed->setDefaultEditedState (pedited->wavelet.bluemed ? Edited : UnEdited);
        greenhigh->setDefaultEditedState (pedited->wavelet.greenhigh ? Edited : UnEdited);
        bluehigh->setDefaultEditedState (pedited->wavelet.bluehigh ? Edited : UnEdited);

        rescon->setDefault (defParams->wavelet.rescon);
        resconH->setDefault (defParams->wavelet.resconH);
        reschro->setDefault (defParams->wavelet.reschro);
        tmrs->setDefault (defParams->wavelet.tmrs);
        gamma->setDefault (defParams->wavelet.gamma);
        sup->setDefault (defParams->wavelet.sup);
        sky->setDefaultEditedState (pedited->wavelet.sky ? Edited : UnEdited);
        thres->setDefaultEditedState (pedited->wavelet.thres ? Edited : UnEdited);
        threshold->setDefaultEditedState (pedited->wavelet.threshold ? Edited : UnEdited);
        threshold2->setDefaultEditedState (pedited->wavelet.threshold2 ? Edited : UnEdited);
        edgedetect->setDefaultEditedState (pedited->wavelet.edgedetect ? Edited : UnEdited);
        edgedetectthr->setDefaultEditedState (pedited->wavelet.edgedetectthr ? Edited : UnEdited);
        edgedetectthr2->setDefaultEditedState (pedited->wavelet.edgedetectthr2 ? Edited : UnEdited);
        edgesensi->setDefaultEditedState (pedited->wavelet.edgesensi ? Edited : UnEdited);
        edgeampli->setDefaultEditedState (pedited->wavelet.edgeampli ? Edited : UnEdited);
        chroma->setDefaultEditedState (pedited->wavelet.chroma ? Edited : UnEdited);
        chro->setDefaultEditedState (pedited->wavelet.chro ? Edited : UnEdited);
        contrast->setDefaultEditedState (pedited->wavelet.contrast ? Edited : UnEdited);
        edgrad->setDefaultEditedState (pedited->wavelet.edgrad ? Edited : UnEdited);
        edgval->setDefaultEditedState (pedited->wavelet.edgval ? Edited : UnEdited);
        edgthresh->setDefault (defParams->wavelet.edgthresh);
        thr->setDefaultEditedState (pedited->wavelet.thr ? Edited : UnEdited);
        thrH->setDefaultEditedState (pedited->wavelet.thrH ? Edited : UnEdited);
        skinprotect->setDefaultEditedState (pedited->wavelet.skinprotect ? Edited : UnEdited);
        hueskin->setDefaultEditedState (pedited->wavelet.hueskin ? Edited : UnEdited);
        hueskin2->setDefaultEditedState (pedited->wavelet.hueskin2 ? Edited : UnEdited);
        hueskinsty->setDefaultEditedState (pedited->wavelet.hueskinsty ? Edited : UnEdited);
        hllev->setDefaultEditedState (pedited->wavelet.hllev ? Edited : UnEdited);
        bllev->setDefaultEditedState (pedited->wavelet.bllev ? Edited : UnEdited);
        pastlev->setDefaultEditedState (pedited->wavelet.pastlev ? Edited : UnEdited);
        satlev->setDefaultEditedState (pedited->wavelet.satlev ? Edited : UnEdited);
        edgcont->setDefaultEditedState (pedited->wavelet.edgcont ? Edited : UnEdited);
        strength->setDefaultEditedState (pedited->wavelet.strength ? Edited : UnEdited);
        mergeL->setDefaultEditedState (pedited->wavelet.mergeL ? Edited : UnEdited);
        mergeC->setDefaultEditedState (pedited->wavelet.mergeC ? Edited : UnEdited);
        gain->setDefaultEditedState (pedited->wavelet.gain ? Edited : UnEdited);
        offs->setDefaultEditedState (pedited->wavelet.offs ? Edited : UnEdited);
        vart->setDefaultEditedState (pedited->wavelet.vart ? Edited : UnEdited);
        limd->setDefaultEditedState (pedited->wavelet.limd ? Edited : UnEdited);
        scale->setDefaultEditedState (pedited->wavelet.scale ? Edited : UnEdited);
        chrrt->setDefaultEditedState (pedited->wavelet.chrrt ? Edited : UnEdited);
        str->setDefaultEditedState (pedited->wavelet.str ? Edited : UnEdited);
        neigh->setDefaultEditedState (pedited->wavelet.neigh ? Edited : UnEdited);
        radius->setDefaultEditedState       (pedited->wavelet.radius ? Edited : UnEdited);
        highlights->setDefaultEditedState   (pedited->wavelet.highlights ? Edited : UnEdited);
        h_tonalwidth->setDefaultEditedState (pedited->wavelet.htonalwidth ? Edited : UnEdited);
        shadows->setDefaultEditedState      (pedited->wavelet.shadows ? Edited : UnEdited);
        s_tonalwidth->setDefaultEditedState (pedited->wavelet.stonalwidth ? Edited : UnEdited);

        balance->setDefaultEditedState (pedited->wavelet.balance ? Edited : UnEdited);
        balanleft->setDefaultEditedState (pedited->wavelet.balanleft ? Edited : UnEdited);
        balanhig->setDefaultEditedState (pedited->wavelet.balanhig ? Edited : UnEdited);
        sizelab->setDefaultEditedState (pedited->wavelet.sizelab ? Edited : UnEdited);
        balmerch->setDefaultEditedState (pedited->wavelet.balmerch ? Edited : UnEdited);
        shapedetcolor->setDefaultEditedState (pedited->wavelet.shapedetcolor ? Edited : UnEdited);
        dirV->setDefaultEditedState (pedited->wavelet.dirV ? Edited : UnEdited);
        dirH->setDefaultEditedState (pedited->wavelet.dirH ? Edited : UnEdited);
        dirD->setDefaultEditedState (pedited->wavelet.dirD ? Edited : UnEdited);
        shapind->setDefaultEditedState (pedited->wavelet.shapind ? Edited : UnEdited);
        balmerres->setDefaultEditedState (pedited->wavelet.balmerres ? Edited : UnEdited);
        balmerres2->setDefaultEditedState (pedited->wavelet.balmerres2 ? Edited : UnEdited);
        grad->setDefaultEditedState (pedited->wavelet.grad ? Edited : UnEdited);
        blend->setDefaultEditedState (pedited->wavelet.blend ? Edited : UnEdited);
        blendc->setDefaultEditedState (pedited->wavelet.blendc ? Edited : UnEdited);
        iter->setDefaultEditedState (pedited->wavelet.iter ? Edited : UnEdited);
        level0noise->setDefaultEditedState (pedited->wavelet.level0noise ? Edited : UnEdited);
        level1noise->setDefaultEditedState (pedited->wavelet.level1noise ? Edited : UnEdited);
        level2noise->setDefaultEditedState (pedited->wavelet.level2noise ? Edited : UnEdited);
        level3noise->setDefaultEditedState (pedited->wavelet.level3noise ? Edited : UnEdited);

        for (int i = 0; i < 9; i++) {
            correction[i]->setDefaultEditedState (pedited->wavelet.c[i] ? Edited : UnEdited);
        }

        for (int i = 0; i < 9; i++) {
            correctionch[i]->setDefaultEditedState (pedited->wavelet.ch[i] ? Edited : UnEdited);
        }

        for (int i = 0; i < 9; i++) {
            balmer[i]->setDefaultEditedState (pedited->wavelet.bm[i] ? Edited : UnEdited);
        }

        for (int i = 0; i < 9; i++) {
            balmer2[i]->setDefaultEditedState (pedited->wavelet.bm2[i] ? Edited : UnEdited);
        }

    } else {
        rescon->setDefaultEditedState (Irrelevant);
        resconH->setDefaultEditedState (Irrelevant);
        reschro->setDefaultEditedState (Irrelevant);
        tmrs->setDefaultEditedState (Irrelevant);
        gamma->setDefaultEditedState (Irrelevant);
        sup->setDefaultEditedState (Irrelevant);
        sky->setDefaultEditedState (Irrelevant);
        thres->setDefaultEditedState (Irrelevant);
        threshold->setDefaultEditedState (Irrelevant);
        threshold2->setDefaultEditedState (Irrelevant);
        edgedetect->setDefaultEditedState (Irrelevant);
        edgedetectthr->setDefaultEditedState (Irrelevant);
        edgedetectthr2->setDefaultEditedState (Irrelevant);
        edgesensi->setDefaultEditedState (Irrelevant);
        edgeampli->setDefaultEditedState (Irrelevant);
        chroma->setDefaultEditedState (Irrelevant);
        chro->setDefaultEditedState (Irrelevant);
        contrast->setDefaultEditedState (Irrelevant);
        edgrad->setDefaultEditedState (Irrelevant);
        edgval->setDefaultEditedState (Irrelevant);
        edgthresh->setDefaultEditedState (Irrelevant);
        thr->setDefaultEditedState (Irrelevant);
        thrH->setDefaultEditedState (Irrelevant);
        skinprotect->setDefaultEditedState (Irrelevant);
        hueskin->setDefaultEditedState (Irrelevant);
        hueskin2->setDefaultEditedState (Irrelevant);
        hueskinsty->setDefaultEditedState (Irrelevant);
        hllev->setDefaultEditedState (Irrelevant);
        bllev->setDefaultEditedState (Irrelevant);
        edgcont->setDefaultEditedState (Irrelevant);
        level0noise->setDefaultEditedState (Irrelevant);
        level1noise->setDefaultEditedState (Irrelevant);
        level2noise->setDefaultEditedState (Irrelevant);
        level3noise->setDefaultEditedState (Irrelevant);
        pastlev->setDefaultEditedState (Irrelevant);
        satlev->setDefaultEditedState (Irrelevant);
        strength->setDefaultEditedState (Irrelevant);
        mergeL->setDefaultEditedState (Irrelevant);
        mergeC->setDefaultEditedState (Irrelevant);
        gain->setDefaultEditedState (Irrelevant);
        offs->setDefaultEditedState (Irrelevant);
        vart->setDefaultEditedState (Irrelevant);
        limd->setDefaultEditedState (Irrelevant);
        scale->setDefaultEditedState (Irrelevant);
        chrrt->setDefaultEditedState (Irrelevant);
        str->setDefaultEditedState (Irrelevant);
        neigh->setDefaultEditedState (Irrelevant);
        balance->setDefaultEditedState (Irrelevant);
        iter->setDefaultEditedState (Irrelevant);
        balanleft->setDefaultEditedState (Irrelevant);
        balanhig->setDefaultEditedState (Irrelevant);
        sizelab->setDefaultEditedState (Irrelevant);
        balmerch->setDefaultEditedState (Irrelevant);
        shapedetcolor->setDefaultEditedState (Irrelevant);
        dirV->setDefaultEditedState (Irrelevant);
        dirH->setDefaultEditedState (Irrelevant);
        dirD->setDefaultEditedState (Irrelevant);
        shapind->setDefaultEditedState (Irrelevant);
        balmerres->setDefaultEditedState (Irrelevant);
        balmerres2->setDefaultEditedState (Irrelevant);
        grad->setDefaultEditedState (Irrelevant);
        blend->setDefaultEditedState (Irrelevant);
        blendc->setDefaultEditedState (Irrelevant);
        radius->setDefaultEditedState       (Irrelevant);
        highlights->setDefaultEditedState   (Irrelevant);
        h_tonalwidth->setDefaultEditedState (Irrelevant);
        shadows->setDefaultEditedState      (Irrelevant);
        s_tonalwidth->setDefaultEditedState (Irrelevant);

        for (int i = 0; i < 9; i++) {
            correction[i]->setDefaultEditedState (Irrelevant);
        }

        for (int i = 0; i < 9; i++) {
            correctionch[i]->setDefaultEditedState (Irrelevant);
        }

        for (int i = 0; i < 9; i++) {
            balmer[i]->setDefaultEditedState (Irrelevant);
        }

        for (int i = 0; i < 9; i++) {
            balmer2[i]->setDefaultEditedState (Irrelevant);
        }

    }
}
void Wavelet::adjusterChanged (ThresholdAdjuster* a, double newBottom, double newTop)
{
    if (listener && (multiImage || getEnabled()) ) {
        if (a == level0noise) {
            listener->panelChanged (EvWavlev0nois,
                                    Glib::ustring::compose (Glib::ustring (M ("TP_WAVELET_NOIS") + ": %1" + "\n" + M ("TP_WAVELET_STREN") + ": %2"), int (newTop), int (newBottom)));
        } else if (a == level1noise) {
            listener->panelChanged (EvWavlev1nois,
                                    Glib::ustring::compose (Glib::ustring (M ("TP_WAVELET_NOIS") + ": %1" + "\n" + M ("TP_WAVELET_STREN") + ": %2"), int (newTop), int (newBottom)));
        } else if (a == level2noise) {
            listener->panelChanged (EvWavlev2nois,
                                    Glib::ustring::compose (Glib::ustring (M ("TP_WAVELET_NOIS") + ": %1" + "\n" + M ("TP_WAVELET_STREN") + ": %2"), int (newTop), int (newBottom)));
        } else if (a == level3noise) {
            listener->panelChanged (EvWavlev3nois,
                                    Glib::ustring::compose (Glib::ustring (M ("TP_WAVELET_NOIS") + ": %1" + "\n" + M ("TP_WAVELET_STREN") + ": %2"), int (newTop), int (newBottom)));
        }

    }
}


void Wavelet::adjusterChanged2 (ThresholdAdjuster* a, int newBottomL, int newTopL, int newBottomR, int newTopR)
{
    if (listener && (multiImage || getEnabled()) ) {
        if (a == hueskin) {
            listener->panelChanged (EvWavHueskin, hueskin->getHistoryString());
        } else if (a == hueskin2) {
            listener->panelChanged (EvWavHueskin2, hueskin2->getHistoryString());
        } else if (a == hueskinsty) {
            listener->panelChanged (EvWavHueskinsty, hueskinsty->getHistoryString());
        } else if (a == hllev) {
            listener->panelChanged (EvWavlhl, hllev->getHistoryString());
        } else if (a == bllev) {
            listener->panelChanged (EvWavlbl, bllev->getHistoryString());
        } else if (a == pastlev) {
            listener->panelChanged (EvWavpast, pastlev->getHistoryString());
        } else if (a == satlev) {
            listener->panelChanged (EvWavsat, satlev->getHistoryString());
        } else if (a == edgcont) {
            listener->panelChanged (EvWavedgcont, edgcont->getHistoryString());
        }
    }
}

void Wavelet::HSmethodUpdateUI()
{
    if (!batchMode) {
        if (HSmethod->get_active_row_number() == 0) { //without
            hllev->hide();
            bllev->hide();
            threshold->hide();
            threshold2->hide();
        } else { //with
            hllev->show();
            bllev->show();
            threshold->show();
            threshold2->show();
        }
    }
}

void Wavelet::HSmethodChanged()
{
    HSmethodUpdateUI();

    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavHSmet, HSmethod->get_active_text ());
    }
}

void Wavelet::CHmethodUpdateUI()
{
    if (!batchMode) {
        if (CHmethod->get_active_row_number() == 0) {
            CHSLmethod->show();
            pastlev->hide();
            satlev->hide();
            chroma->hide();
            chro->hide();
            labmC->show();
            separatorNeutral->hide();
            neutralchButton->show();
            int y = thres->getValue();
            int z;

            if (y == 2) {
                level2noise->set_sensitive (false);
                level3noise->set_sensitive (false);
            } else if (y == 3) {
                level3noise->set_sensitive (false);
                level2noise->set_sensitive (true);
            } else {
                level2noise->set_sensitive (true);
                level3noise->set_sensitive (true);
            }

            for (z = y; z < 9; z++) {
                correctionch[z]->hide();
            }

            for (z = 0; z < y; z++) {
                correctionch[z]->show();
            }
        } else if (CHmethod->get_active_row_number() == 1) {
            CHSLmethod->show();
            pastlev->show();
            satlev->show();
            chroma->show();
            chro->hide();
            labmC->show();
            separatorNeutral->show();
            neutralchButton->show();
            int y = thres->getValue();
            int z;

            if (y == 2) {
                level2noise->set_sensitive (false);
                level3noise->set_sensitive (false);
            } else if (y == 3) {
                level3noise->set_sensitive (false);
                level2noise->set_sensitive (true);
            } else {
                level2noise->set_sensitive (true);
                level3noise->set_sensitive (true);
            }

            for (z = y; z < 9; z++) {
                correctionch[z]->hide();
            }

            for (z = 0; z < y; z++) {
                correctionch[z]->show();
            }
        } else {
            chro->show();
            pastlev->hide();
            satlev->hide();
            chroma->hide();
            CHSLmethod->hide();
            labmC->hide();
            separatorNeutral->hide();
            neutralchButton->hide();

            for (int i = 0; i < 9; i++) {
                correctionch[i]->hide();
            }
        }
    }
}

void Wavelet::CHmethodChanged()
{
    CHmethodUpdateUI();

    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavCHmet, CHmethod->get_active_text ());
    }
}

/*
void Wavelet::CHSLmethodChangedUI() {
    if (!batchMode) {
        if(CHSLmethod->get_active_row_number()==0 && CHmethod->get_active_row_number() != 2) {//SL
            //CLVcurveEditorG->hide();
            neutralchButton->show();
                int y=thres->getValue();
                int z;
                for(z=y;z<9;z++) correctionch[z]->hide();
                for(z=0;z<y;z++) correctionch[z]->show();
        }
        else if(CHSLmethod->get_active_row_number()==1) {//CU
            //CLVcurveEditorG->show();
            neutralchButton->hide();
            for (int i = 0; i < 9; i++) {
                correctionch[i]->hide();
            }
        }
    }
}
*/

void Wavelet::CHSLmethodChanged()
{
    /*
        CHSLmethodChangedUI();
        if (listener && (multiImage||enabled->get_active()) ) {
            listener->panelChanged (EvWavCHSLmet, CHSLmethod->get_active_text ());
        }
    */
}

void Wavelet::EDmethodUpdateUI()
{
    if (!batchMode) {
        if (EDmethod->get_active_row_number() == 0 ) { //SL
            CCWcurveEditorG->hide();
            edgcont->show();
        } else if (EDmethod->get_active_row_number() == 1) { //CU
            CCWcurveEditorG->show();
            edgcont->hide();
        }
    }
}
void Wavelet::EDmethodChanged()
{
    EDmethodUpdateUI();

    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavEDmet, EDmethod->get_active_text ());
    }
}

void Wavelet::NPmethodUpdateUI()
{
    if (!batchMode) {

    }
}
void Wavelet::NPmethodChanged()
{
    NPmethodUpdateUI();

    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavNPmet, NPmethod->get_active_text ());
    }
}



void Wavelet::BAmethodUpdateUI()
{
    if (!batchMode) {
        if (BAmethod->get_active_row_number() == 0 ) { //none
            balance->hide();
            opacityCurveEditorW->hide();
            iter->hide();

        } else if (BAmethod->get_active_row_number() == 1) { //sli
            opacityCurveEditorW->hide();
            balance->show();
            iter->show();
        } else if (BAmethod->get_active_row_number() == 2) { //CU
            opacityCurveEditorW->show();
            balance->hide();
            iter->show();
        }
    }
}
void Wavelet::BAmethodChanged()
{
    BAmethodUpdateUI();

    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavBAmet, BAmethod->get_active_text ());
    }
}


void Wavelet::TMmethodUpdateUI()
{
    /*
    if (!batchMode) {
        if(TMmethod->get_active_row_number()==0 ) {//
            tmr->hide();
        }
        else {
            tmr->show();
        }
    }
    */
}

void Wavelet::TMmethodChanged()
{
    TMmethodUpdateUI();

    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavTMmet, TMmethod->get_active_text ());
    }
}

/*
void Wavelet::BackmethodUpdateUI() {
    if (!batchMode) {
    }
}
*/

void Wavelet::BackmethodChanged()
{
    //BackmethodUpdateUI();
    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavBackmet, Backmethod->get_active_text ());
    }
}

void Wavelet::CLmethodUpdateUI()
{
    if (!batchMode) {
        if (CLmethod->get_active_row_number() == 0  && expmerge->getEnabled() == false &&  ushamethod->get_active_row_number() == 0) {
            CLmethod->set_active (0);
            Lmethod->set_sensitive (true);
            Dirmethod->set_sensitive (true);

        } else if (CLmethod->get_active_row_number() == 1 && expmerge->getEnabled() == false && ushamethod->get_active_row_number() == 0 ) {
            CLmethod->set_active (1);
            Lmethod->set_sensitive (true);
            Dirmethod->set_sensitive (true);

        } else if (CLmethod->get_active_row_number() == 2 && expmerge->getEnabled() == false && ushamethod->get_active_row_number() == 0) {
            CLmethod->set_active (2);
            Lmethod->set_sensitive (true);
            Dirmethod->set_sensitive (true);
        } else if (CLmethod->get_active_row_number() == 3 && expmerge->getEnabled() == false  && ushamethod->get_active_row_number() == 0) {
            CLmethod->set_active (3);
            Lmethod->set_sensitive (false); //false
            Dirmethod->set_sensitive (false); //false

        }
    }
}

void Wavelet::CLmethodChanged()
{
    CLmethodUpdateUI();

    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavCLmet, CLmethod->get_active_text ());
    }
}

/*
void Wavelet::TilesmethodUpdateUI() {
    if (!batchMode) {
    }
}
*/

void Wavelet::TilesmethodChanged()
{

    //TilesmethodUpdateUI();
    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavTilesmet, Tilesmethod->get_active_text ());
    }
}

void Wavelet::usharpmethodChanged()
{
    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavusharpmet, usharpmethod->get_active_text ());
    }
}

void Wavelet::ushamethodChanged()
{
    if (ushamethod->get_active_row_number() == 2  && expedge->getEnabled() == true) {
        Backmethod->set_active (2);
        CLmethod->set_active (2);
        Lmethod->set_active (6);
        Lmethod->set_sensitive (true);
        Dirmethod->set_sensitive (true);
        Dirmethod->set_active (3);
    } else if (ushamethod->get_active_row_number() == 1 && expedge->getEnabled() == true) {
        Backmethod->set_active (0);
        CLmethod->set_active (1);
        Lmethod->set_active (2);
        Dirmethod->set_active (3);
        Lmethod->set_sensitive (true);
        Dirmethod->set_sensitive (true);
    } else if (ushamethod->get_active_row_number() == 0 || expedge->getEnabled() == false) {
        Backmethod->set_active (1);
        CLmethod->set_active (3);
        Lmethod->set_active (3);
        Dirmethod->set_active (3);
        Lmethod->set_sensitive (false);
        Dirmethod->set_sensitive (false);
    }

    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavushamet, ushamethod->get_active_text ());
    }
}

/*
void Wavelet::daubcoeffmethodUpdateUI() {
    if (!batchMode) {
    }}
*/
void Wavelet::daubcoeffmethodChanged()
{
    //daubcoeffmethodUpdateUI();
    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavdaubcoeffmet, daubcoeffmethod->get_active_text ());
    }
}

/*
void Wavelet::MedgreinfUpdateUI() {
    if (!batchMode) {
    }
}
*/
void Wavelet::MedgreinfChanged()
{
    //MedgreinfUpdateUI();
    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavedgreinf, Medgreinf->get_active_text ());
    }
}

/*
void Wavelet::DirmethodUpdateUI() {
    if (!batchMode) {
    }
}
*/
void Wavelet::DirmethodChanged()
{
    //DirmethodUpdateUI();
    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavDirmeto, Dirmethod->get_active_text ());
    }
}

/*
void Wavelet::LmethodUpdateUI() {
    if (!batchMode) {
    }
}
*/
void Wavelet::LmethodChanged()
{
    //LmethodUpdateUI();
    if (listener && (multiImage || getEnabled()) ) {
        listener->panelChanged (EvWavLmet, Lmethod->get_active_text ());
    }
}

void Wavelet::setBatchMode (bool batchMode)
{
    Lmethod->append (M ("GENERAL_UNCHANGED"));
    CLmethod->append (M ("GENERAL_UNCHANGED"));
    Backmethod->append (M ("GENERAL_UNCHANGED"));
    Tilesmethod->append (M ("GENERAL_UNCHANGED"));
    usharpmethod->append (M ("GENERAL_UNCHANGED"));
    ushamethod->append (M ("GENERAL_UNCHANGED"));
    daubcoeffmethod->append (M ("GENERAL_UNCHANGED"));
    CHmethod->append (M ("GENERAL_UNCHANGED"));
    Medgreinf->append (M ("GENERAL_UNCHANGED"));
    CHSLmethod->append (M ("GENERAL_UNCHANGED"));
    EDmethod->append (M ("GENERAL_UNCHANGED"));
    NPmethod->append (M ("GENERAL_UNCHANGED"));
    BAmethod->append (M ("GENERAL_UNCHANGED"));
    TMmethod->append (M ("GENERAL_UNCHANGED"));
    HSmethod->append (M ("GENERAL_UNCHANGED"));
    Dirmethod->append (M ("GENERAL_UNCHANGED"));
    mergevMethod->append (M ("GENERAL_UNCHANGED"));
    mergMethod->append (M ("GENERAL_UNCHANGED"));
    mergMethod2->append (M ("GENERAL_UNCHANGED"));
    mergBMethod->append (M ("GENERAL_UNCHANGED"));
    CCWcurveEditorG->setBatchMode (batchMode);
    CCWcurveEditormerg->setBatchMode (batchMode);
    CCWcurveEditormerg2->setBatchMode (batchMode);
    CCWcurveEditorsty->setBatchMode (batchMode);
    CCWcurveEditorsty2->setBatchMode (batchMode);
    CCWcurveEditorT->setBatchMode (batchMode);
    CCWcurveEditorgainT->setBatchMode (batchMode);
    opaCurveEditorG->setBatchMode (batchMode);
    opacityCurveEditorG->setBatchMode (batchMode);
    opacityCurveEditorW->setBatchMode (batchMode);
    opacityCurveEditorWL->setBatchMode (batchMode);
    curveEditorRES->setBatchMode (batchMode);
    curveEditorsty->setBatchMode (batchMode);
    curveEditorGAM->setBatchMode (batchMode);
    rescon->showEditedCB ();
    resconH->showEditedCB ();
    reschro->showEditedCB ();
    tmrs->showEditedCB ();
    gamma->showEditedCB ();
    sup->showEditedCB ();
    sky->showEditedCB ();
    thres->showEditedCB ();
    threshold->showEditedCB ();
    threshold2->showEditedCB ();
    edgedetect->showEditedCB ();
    edgedetectthr->showEditedCB ();
    edgedetectthr2->showEditedCB ();
    edgesensi->showEditedCB ();
    edgeampli->showEditedCB ();
    chroma->showEditedCB ();
    chro->showEditedCB ();
    contrast->showEditedCB ();
    edgrad->showEditedCB ();
    edgval->showEditedCB ();
    edgthresh->showEditedCB ();
    thr->showEditedCB ();
    thrH->showEditedCB ();
    skinprotect->showEditedCB();
    hueskin->showEditedCB ();
    hueskin2->showEditedCB ();
    hueskinsty->showEditedCB ();
    hllev->showEditedCB ();
    bllev->showEditedCB ();
    pastlev->showEditedCB ();
    satlev->showEditedCB ();
    edgcont->showEditedCB ();
    strength->showEditedCB ();
    mergeL->showEditedCB ();
    mergeC->showEditedCB ();
    gain->showEditedCB ();
    offs->showEditedCB ();
    vart->showEditedCB ();
    limd->showEditedCB ();
    scale->showEditedCB ();
    radius->showEditedCB ();
    highlights->showEditedCB ();
    h_tonalwidth->showEditedCB ();
    shadows->showEditedCB ();
    s_tonalwidth->showEditedCB ();
    chrrt->showEditedCB ();
    str->showEditedCB ();
    neigh->showEditedCB ();
    balance->showEditedCB ();
    balanleft->showEditedCB ();
    balanhig->showEditedCB ();
    sizelab->showEditedCB ();
    balmerch->showEditedCB ();
    shapedetcolor->showEditedCB ();
    dirV->showEditedCB ();
    dirH->showEditedCB ();
    dirD->showEditedCB ();
    shapind->showEditedCB ();
    balmerres->showEditedCB ();
    balmerres2->showEditedCB ();
    grad->showEditedCB ();
    blend->showEditedCB ();
    blendc->showEditedCB ();
    iter->showEditedCB ();
    level0noise->showEditedCB ();
    level1noise->showEditedCB ();
    level2noise->showEditedCB ();
    level3noise->showEditedCB ();
    removeIfThere (this, savelab);

    ToolPanel::setBatchMode (batchMode);

    for (int i = 0; i < 9; i++) {
        correction[i]->showEditedCB();
    }

    for (int i = 0; i < 9; i++) {
        correctionch[i]->showEditedCB();
    }

    for (int i = 0; i < 9; i++) {
        balmer[i]->showEditedCB();
    }

    for (int i = 0; i < 9; i++) {
        balmer2[i]->showEditedCB();
    }

}

void Wavelet::neutral2_pressed ()
{
    for (int i = 0; i < 9; i++) {
        balmer[i]->setValue (15 + 12 * i);
        adjusterChanged (balmer[i], 15 + 12 * i);
    }

    for (int i = 0; i < 9; i++) {
        balmer2[i]->setValue (70);
        adjusterChanged (balmer2[i], 70);
    }

    hueskinsty->setValue (-5, 25, 170, 120);
    shapedetcolor->resetValue (false);
    dirV->resetValue (false);
    dirH->resetValue (false);
    dirD->resetValue (false);
    shapind->resetValue (false);
    cstyshape->reset();
    cstyshape2->reset();
    balmerres->resetValue (false);
    balmerres2->resetValue (false);
    balmerch->resetValue (false);
    balanhig->resetValue (false);
    balanleft->resetValue (false);
}

void Wavelet::savelabPressed ()
{

    if (!walistener) {
        return;
    }


    Gtk::FileChooserDialog dialog (M ("TP_WAVELET_SAVELAB"), Gtk::FILE_CHOOSER_ACTION_SAVE);

    bindCurrentFolder (dialog, options.lastProfilingReferenceDir);
    dialog.set_current_name (lastlabFilename);

    dialog.add_button (Gtk::StockID ("gtk-cancel"), Gtk::RESPONSE_CANCEL);
    dialog.add_button (Gtk::StockID ("gtk-save"), Gtk::RESPONSE_OK);


    Glib::RefPtr<Gtk::FileFilter> filter_mer = Gtk::FileFilter::create();

    filter_mer->set_name (M ("FILECHOOSER_FILTER_MER"));
    filter_mer->add_pattern ("*.mer");
    dialog.add_filter (filter_mer);

    dialog.show_all_children();

    bool done = false;
    //activation watermark or HDR
    //CLmethod->set_active (2);
    //CLmethodUpdateUI() ;

    do {
        int result = dialog.run();

        if (result != Gtk::RESPONSE_OK) {
            done = true;
        } else {
            std::string fname = dialog.get_filename();
            Glib::ustring ext = getExtension (fname);

            if (ext != "mer") {
                fname += ".mer";
            }

            if (confirmOverwrite (dialog, fname)) {
                walistener->savelabReference (fname);
                lastlabFilename = Glib::path_get_basename (fname);
                done = true;
            }
        }
    } while (!done);

    return;

}


void Wavelet::adjusterUpdateUI (Adjuster* a)
{
    /*
        if (!batchMode) {
            if (a == tmrs ) {
                float tm;
                tm=tmrs->getValue();
                if(tm==0.f) tmr->hide();
                else tmr->show();
            }
            else if (a == gamma ) {
                float tm;
                tm=tmrs->getValue();
                if(tm==0.f) tmr->hide();
                else tmr->show();
                );
            }
        }
    */
}

void Wavelet::adjusterChanged (Adjuster* a, double newval)
{
    if (listener && (multiImage || getEnabled()) ) {
        if (a == edgthresh) {
            listener->panelChanged (EvWavtiles, edgthresh->getTextValue());
        } else if (a == rescon ) {
            listener->panelChanged (EvWavrescon, rescon->getTextValue());
        } else if (a == resconH ) {
            listener->panelChanged (EvWavresconH, resconH->getTextValue());
        } else if (a == reschro ) {
            listener->panelChanged (EvWavreschro, reschro->getTextValue());
        } else if (a == tmrs ) {
            adjusterUpdateUI (a);
            listener->panelChanged (EvWavtmrs, tmrs->getTextValue());
        } else if (a == gamma ) {
            adjusterUpdateUI (a);
            listener->panelChanged (EvWavgamma, gamma->getTextValue());
        } else if (a == sky ) {
            listener->panelChanged (EvWavsky, sky->getTextValue());
        } else if (a == sup ) {
            listener->panelChanged (EvWavsup, sup->getTextValue());
        } else if (a == chroma ) {
            listener->panelChanged (EvWavchroma, chroma->getTextValue());
        } else if (a == chro ) {
            listener->panelChanged (EvWavchro, chro->getTextValue());
        } else if (a == contrast ) {
            listener->panelChanged (EvWavunif, contrast->getTextValue());
        } else if (a == thr ) {
            listener->panelChanged (EvWavthr, thr->getTextValue());
        } else if (a == thrH ) {
            listener->panelChanged (EvWavthrH, thrH->getTextValue());
        } else if (a == threshold ) {
            listener->panelChanged (EvWavThreshold, threshold->getTextValue());
        } else if (a == threshold2 ) {
            listener->panelChanged (EvWavThreshold2, threshold2->getTextValue());
        } else if (a == edgedetect ) {
            listener->panelChanged (EvWavedgedetect, edgedetect->getTextValue());
        } else if (a == edgedetectthr ) {
            listener->panelChanged (EvWavedgedetectthr, edgedetectthr->getTextValue());
        } else if (a == edgedetectthr2 ) {
            listener->panelChanged (EvWavedgedetectthr2, edgedetectthr2->getTextValue());
        } else if (a == edgesensi ) {
            listener->panelChanged (EvWavedgesensi, edgesensi->getTextValue());
        } else if (a == edgeampli ) {
            listener->panelChanged (EvWavedgeampli, edgeampli->getTextValue());
        } else if (a == edgrad ) {
            listener->panelChanged (EvWavedgrad, edgrad->getTextValue());
        } else if (a == edgval ) {
            listener->panelChanged (EvWavedgval, edgval->getTextValue());
        } else if (a == thres ) {
            int y = thres->getValue();
            int z;

            if (y == 2) {
                level2noise->set_sensitive (false);
                level3noise->set_sensitive (false);
            } else if (y == 3) {
                level3noise->set_sensitive (false);
                level2noise->set_sensitive (true);
            } else {
                level2noise->set_sensitive (true);
                level3noise->set_sensitive (true);
            }

            for (z = y; z < 9; z++) {
                correction[z]->hide();
            }

            for (z = 0; z < y; z++) {
                correction[z]->show();
            }

            for (z = y; z < 9; z++) {
                correctionch[z]->hide();
            }

            for (z = 0; z < y; z++) {
                correctionch[z]->show();
            }

            if (mergMethod->get_active_row_number() == 4) {
                for (z = y; z < 9; z++) {
                    balmer[z]->hide();
                }

                for (z = 0; z < y; z++) {
                    balmer[z]->show();
                }

                for (z = 0; z < 9; z++) {
                    balmer2[z]->hide();
                }
            }

            if (mergMethod->get_active_row_number() == 5) {
                for (z = y; z < 9; z++) {
                    balmer2[z]->hide();
                }

                for (z = 0; z < y; z++) {
                    balmer2[z]->show();
                }

                for (z = 0; z < 9; z++) {
                    balmer[z]->hide();
                }

            }

            if (z == 9) {
                sup->show();
            } else {
                sup->hide();
            }

            listener->panelChanged (EvWavthres, thres->getTextValue());
        } else if (a == skinprotect) {
            listener->panelChanged (EvWavSkin, skinprotect->getTextValue());
        } else if (a == strength) {
            listener->panelChanged (EvWavStrength, strength->getTextValue());
        } else if (a == mergeL) {
            listener->panelChanged (EvWavmergeL, mergeL->getTextValue());
        } else if (a == mergeC) {
            listener->panelChanged (EvWavmergeC, mergeC->getTextValue());
        } else if (a == gain) {
            listener->panelChanged (EvWavgain, gain->getTextValue());
        } else if (a == offs) {
            listener->panelChanged (EvWavoffs, offs->getTextValue());
        } else if (a == vart) {
            listener->panelChanged (EvWavvart, vart->getTextValue());
        } else if (a == radius) {
            listener->panelChanged (EvWavradius, radius->getTextValue());
        } else if (a == highlights) {
            listener->panelChanged (EvWavhighlights, highlights->getTextValue());
        } else if (a == h_tonalwidth) {
            listener->panelChanged (EWavh_tonalwidth, h_tonalwidth->getTextValue());
        } else if (a == shadows) {
            listener->panelChanged (EvWavshadows, shadows->getTextValue());
        } else if (a ==  s_tonalwidth) {
            listener->panelChanged (EvWavs_tonalwidth,  s_tonalwidth->getTextValue());
        } else if (a == limd) {
            listener->panelChanged (EvWavlimd, limd->getTextValue());
        } else if (a == scale) {
            listener->panelChanged (EvWavscale, scale->getTextValue());
        } else if (a == chrrt) {
            listener->panelChanged (EvWavchrrt, chrrt->getTextValue());
        } else if (a == str) {
            listener->panelChanged (EvWavstr, str->getTextValue());
        } else if (a == neigh) {
            listener->panelChanged (EvWavneigh, neigh->getTextValue());
        } else if (a == balance) {
            listener->panelChanged (EvWavbalance, balance->getTextValue());
        } else if (a == balanleft) {
            listener->panelChanged (EvWavbalanlow, balanleft->getTextValue());
        } else if (a == balanhig) {
            listener->panelChanged (EvWavbalanhigh, balanhig->getTextValue());
        } else if (a == sizelab) {
            listener->panelChanged (EvWavsizelab, sizelab->getTextValue());
        } else if (a == shapedetcolor) {
            listener->panelChanged (EvWavshapedet, shapedetcolor->getTextValue());
        } else if (a == dirV) {
            listener->panelChanged (EvWavdirV, dirV->getTextValue());
        } else if (a == dirH) {
            listener->panelChanged (EvWavdirH, dirH->getTextValue());
        } else if (a == dirD) {
            listener->panelChanged (EvWavdirD, dirD->getTextValue());
        } else if (a == shapind) {
            listener->panelChanged (EvWavshapind, shapind->getTextValue());
        } else if (a == balmerch) {
            listener->panelChanged (EvWavbalmerch, balmerch->getTextValue());
        } else if (a == balmerres) {
            listener->panelChanged (EvWavbalmerres, balmerres->getTextValue());
        } else if (a == balmerres2) {
            listener->panelChanged (EvWavbalmerres2, balmerres2->getTextValue());
        } else if (a == grad) {
            listener->panelChanged (EvWavgrad, grad->getTextValue());
        } else if (a == blend) {
            listener->panelChanged (EvWavblen, blend->getTextValue());
        } else if (a == blendc) {
            listener->panelChanged (EvWavblenc, blendc->getTextValue());
        } else if (a == iter) {
            listener->panelChanged (EvWaviter, iter->getTextValue());
        } else if (a == greenhigh ) {
            listener->panelChanged (EvWavgreenhigh, greenhigh->getTextValue());
        } else if (a == bluehigh ) {
            listener->panelChanged (EvWavbluehigh, bluehigh->getTextValue());
        } else if (a == greenmed ) {
            listener->panelChanged (EvWavgreenmed, greenmed->getTextValue());
        } else if (a == bluemed ) {
            listener->panelChanged (EvWavbluemed, bluemed->getTextValue());
        } else if (a == greenlow ) {
            listener->panelChanged (EvWavgreenlow, greenlow->getTextValue());
        } else if (a == bluelow ) {
            listener->panelChanged (EvWavbluelow, bluelow->getTextValue());
        }

        if ((a == correction[0] || a == correction[1] || a == correction[2] || a == correction[3] || a == correction[4] || a == correction[5] || a == correction[6] || a == correction[7] || a == correction[8])) {
            listener->panelChanged (EvWavelet,
                                    Glib::ustring::compose ("%1, %2, %3, %4, %5, %6, %7, %8, %9",
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correction[0]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correction[1]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correction[2]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correction[3]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correction[4]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correction[5]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correction[6]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correction[7]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correction[8]->getValue()))
                                   );
        } else if (a == correctionch[0] || a == correctionch[1] || a == correctionch[2] || a == correctionch[3] || a == correctionch[4] || a == correctionch[5] || a == correctionch[6] || a == correctionch[7] || a == correctionch[8] ) {
            listener->panelChanged (EvWaveletch,
                                    Glib::ustring::compose ("%1, %2, %3, %4, %5, %6, %7, %8, %9",
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correctionch[0]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correctionch[1]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correctionch[2]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correctionch[3]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correctionch[4]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correctionch[5]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correctionch[6]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correctionch[7]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), correctionch[8]->getValue()))
                                   );
        } else if (a == balmer[0] || a == balmer[1] || a == balmer[2] || a == balmer[3] || a == balmer[4] || a == balmer[5] || a == balmer[6] || a == balmer[7] || a == balmer[8] ) {
            listener->panelChanged (EvWaveletbalmer,
                                    Glib::ustring::compose ("%1, %2, %3, %4, %5, %6, %7, %8, %9",
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer[0]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer[1]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer[2]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer[3]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer[4]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer[5]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer[6]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer[7]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer[8]->getValue()))
                                   );
        } else if (a == balmer2[0] || a == balmer2[1] || a == balmer2[2] || a == balmer2[3] || a == balmer2[4] || a == balmer2[5] || a == balmer2[6] || a == balmer2[7] || a == balmer2[8] ) {
            listener->panelChanged (EvWaveletbalmer2,
                                    Glib::ustring::compose ("%1, %2, %3, %4, %5, %6, %7, %8, %9",
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer2[0]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer2[1]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer2[2]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer2[3]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer2[4]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer2[5]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer2[6]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer2[7]->getValue()),
                                            Glib::ustring::format (std::fixed, std::setprecision (0), balmer2[8]->getValue()))
                                   );
        }


    }
}

void Wavelet::enabledUpdateUI ()
{
    if (!batchMode) {
        int y = thres->getValue();
        int z;

        if (y == 2) {
            level2noise->set_sensitive (false);
            level3noise->set_sensitive (false);
        } else if (y == 3) {
            level3noise->set_sensitive (false);
            level2noise->set_sensitive (true);
        } else {
            level2noise->set_sensitive (true);
            level3noise->set_sensitive (true);
        }

        for (z = y; z < 9; z++) {
            correction[z]->hide();
        }

        for (z = 0; z < y; z++) {
            correction[z]->show();
        }

        if (z == 9) {
            sup->show();
        } else {
            sup->hide();
        }

//      adjusterUpdateUI(tmrs);
    }
}

void Wavelet::enabledChanged ()
{
    enabledUpdateUI();

    if (listener) {
        if (get_inconsistent()) {
            listener->panelChanged (EvWavEnabled, M ("GENERAL_UNCHANGED"));
        } else if (getEnabled()) {
            listener->panelChanged (EvWavEnabled, M ("GENERAL_ENABLED"));
        } else {
            listener->panelChanged (EvWavEnabled, M ("GENERAL_DISABLED"));
        }
    }
}

void Wavelet::medianToggled ()
{

    if (multiImage) {
        if (median->get_inconsistent()) {
            median->set_inconsistent (false);
            medianConn.block (true);
            median->set_active (false);
            medianConn.block (false);
        } else if (lastmedian) {
            median->set_inconsistent (true);
        }

        lastmedian = median->get_active ();
    }

    if (listener && (multiImage || getEnabled ())) {
        if (median->get_inconsistent()) {
            listener->panelChanged (EvWavmedian, M ("GENERAL_UNCHANGED"));
        } else if (median->get_active ()) {
            listener->panelChanged (EvWavmedian, M ("GENERAL_ENABLED"));
        } else {
            listener->panelChanged (EvWavmedian, M ("GENERAL_DISABLED"));
        }
    }
}

void Wavelet::medianlevUpdateUI ()
{
    if (!batchMode) {
        if (medianlev->get_active ()) {
            edgedetect->show();
            lipst->show();
            separatoredge->show();

            edgedetectthr->show();
            edgedetectthr2->show();

            //  edgesensi->show();
            //  edgeampli->show();
            //  NPmethod->show();
            //  labmNP->show();
            if (lipst->get_active ()) {
                edgesensi->show();
                edgeampli->show();
                NPmethod->show();
                labmNP->show();
            }

            else {
                edgesensi->hide();
                edgeampli->hide();
                NPmethod->hide();
                labmNP->hide();
            }
        } else {
            edgedetect->hide();
            lipst->hide();
            edgedetectthr->hide();
            edgedetectthr2->hide();
            edgesensi->hide();
            edgeampli->hide();
            separatoredge->hide();
            NPmethod->hide();
            labmNP->hide();

        }
    }
}

void Wavelet::medianlevToggled ()
{

    if (multiImage) {
        if (medianlev->get_inconsistent()) {
            medianlev->set_inconsistent (false);
            medianlevConn.block (true);
            medianlev->set_active (false);
            medianlevConn.block (false);
        } else if (lastmedianlev) {
            medianlev->set_inconsistent (true);
        }

        lastmedianlev = medianlev->get_active ();
    }

    medianlevUpdateUI();

    if (listener && (multiImage || getEnabled ())) {
        if (medianlev->get_inconsistent()) {
            listener->panelChanged (EvWavmedianlev, M ("GENERAL_UNCHANGED"));
        } else if (medianlev->get_active () ) {
            listener->panelChanged (EvWavmedianlev, M ("GENERAL_ENABLED"));
        } else {
            listener->panelChanged (EvWavmedianlev, M ("GENERAL_DISABLED"));
        }
    }

}

void Wavelet::linkedgToggled ()
{

    if (multiImage) {
        if (linkedg->get_inconsistent()) {
            linkedg->set_inconsistent (false);
            linkedgConn.block (true);
            linkedg->set_active (false);
            linkedgConn.block (false);
        } else if (lastlinkedg) {
            linkedg->set_inconsistent (true);
        }

        lastlinkedg = linkedg->get_active ();
    }

    if (listener && (multiImage || getEnabled ())) {
        if (linkedg->get_inconsistent()) {
            listener->panelChanged (EvWavlinkedg, M ("GENERAL_UNCHANGED"));
        } else if (linkedg->get_active () ) {
            listener->panelChanged (EvWavlinkedg, M ("GENERAL_ENABLED"));
        } else {
            listener->panelChanged (EvWavlinkedg, M ("GENERAL_DISABLED"));
        }
    }
}

void Wavelet::cbenabUpdateUI ()
{
    if (!batchMode) {
        if (cbenab->get_active ()) {
            chanMixerHLFrame->show();
            chanMixerMidFrame->show();
            chanMixerShadowsFrame->show();
            neutrHBox->show();
        } else {
            chanMixerHLFrame->hide();
            chanMixerMidFrame->hide();
            chanMixerShadowsFrame->hide();
            neutrHBox->hide();
        }
    }
}

void Wavelet::cbenabToggled ()
{
    if (multiImage) {
        if (cbenab->get_inconsistent()) {
            cbenab->set_inconsistent (false);
            cbenabConn.block (true);
            cbenab->set_active (false);
            cbenabConn.block (false);
        } else if (lastcbenab) {
            cbenab->set_inconsistent (true);
        }

        lastcbenab = cbenab->get_active ();
    }

    cbenabUpdateUI();

    if (listener && (multiImage || getEnabled ())) {
        if (cbenab->get_inconsistent() ) {
            listener->panelChanged (EvWavcbenab, M ("GENERAL_UNCHANGED"));
        } else if (cbenab->get_active () ) {
            listener->panelChanged (EvWavcbenab, M ("GENERAL_ENABLED"));
        } else {
            listener->panelChanged (EvWavcbenab, M ("GENERAL_DISABLED"));
        }
    }

}



void Wavelet::lipstUpdateUI ()
{
    if (!batchMode) {
        if (lipst->get_active ()) {
            NPmethod->show();
            edgesensi->show();
            edgeampli->show();
            labmNP->show();
        } else  {
            NPmethod->hide();
            edgesensi->hide();
            edgeampli->hide();
            labmNP->hide();

        }
    }
}


void Wavelet::lipstToggled ()
{

    if (multiImage) {
        if (lipst->get_inconsistent()) {
            lipst->set_inconsistent (false);
            lipstConn.block (true);
            lipst->set_active (false);
            lipstConn.block (false);
        } else if (lastlipst) {
            lipst->set_inconsistent (true);
        }

        lastlipst = lipst->get_active ();
    }

    lipstUpdateUI();

    if (listener && (multiImage || getEnabled ())) {
        if (lipst->get_inconsistent()) {
            listener->panelChanged (EvWavlipst, M ("GENERAL_UNCHANGED"));
        } else if (lipst->get_active ()) {
            listener->panelChanged (EvWavlipst, M ("GENERAL_ENABLED"));
        } else {
            listener->panelChanged (EvWavlipst, M ("GENERAL_DISABLED"));
        }
    }
}

void Wavelet::avoidToggled ()
{

    if (multiImage) {
        if (avoid->get_inconsistent()) {
            avoid->set_inconsistent (false);
            avoidConn.block (true);
            avoid->set_active (false);
            avoidConn.block (false);
        } else if (lastavoid) {
            avoid->set_inconsistent (true);
        }

        lastavoid = avoid->get_active ();
    }

    if (listener && (multiImage || getEnabled ())) {
        if (avoid->get_inconsistent()) {
            listener->panelChanged (EvWavavoid, M ("GENERAL_UNCHANGED"));
        } else if (avoid->get_active ()) {
            listener->panelChanged (EvWavavoid, M ("GENERAL_ENABLED"));
        } else {
            listener->panelChanged (EvWavavoid, M ("GENERAL_DISABLED"));
        }
    }
}

void Wavelet::tmrToggled ()
{

    if (multiImage) {
        if (tmr->get_inconsistent()) {
            tmr->set_inconsistent (false);
            tmrConn.block (true);
            tmr->set_active (false);
            tmrConn.block (false);
        } else if (lasttmr) {
            tmr->set_inconsistent (true);
        }

        lasttmr = tmr->get_active ();
    }

    if (listener && (multiImage || getEnabled ())) {
        if (tmr->get_inconsistent()) {
            listener->panelChanged (EvWavtmr, M ("GENERAL_UNCHANGED"));
        } else if (tmr->get_active ()) {
            listener->panelChanged (EvWavtmr, M ("GENERAL_ENABLED"));
        } else {
            listener->panelChanged (EvWavtmr, M ("GENERAL_DISABLED"));
        }
    }
}


void Wavelet::colorForValue (double valX, double valY, enum ColorCaller::ElemType elemType, int callerId, ColorCaller *caller)
{

    float R = 0.f, G = 0.f, B = 0.f;

    if (elemType == ColorCaller::CCET_VERTICAL_BAR) {
        valY = 0.5;
    }

    if (callerId == 1) {         // ch - main curve

        Color::hsv2rgb01 (float (valX), float (valY), 0.5f, R, G, B);
    }
    /*  else if (callerId == 2) {    // cc - bottom bar

        //  float value = (1.f - 0.7f) * float(valX) + 0.7f;
            float value = (1.f - 0.7f) * float(valX) + 0.7f;
            // whole hue range
            // Y axis / from 0.15 up to 0.75 (arbitrary values; was 0.45 before)
            Color::hsv2rgb01(float(valY), float(valX), value, R, G, B);
        }
        */
    else if (callerId == 4) {    // LH - bottom bar
        Color::hsv2rgb01 (float (valX), 0.5f, float (valY), R, G, B);
    } else if (callerId == 5) {  // HH - bottom bar
        float h = float ((valY - 0.5) * 0.3 + valX);

        if (h > 1.0f) {
            h -= 1.0f;
        } else if (h < 0.0f) {
            h += 1.0f;
        }

        Color::hsv2rgb01 (h, 0.5f, 0.5f, R, G, B);
    }

    caller->ccRed = double (R);
    caller->ccGreen = double (G);
    caller->ccBlue = double (B);
}
void Wavelet::setAdjusterBehavior (bool multiplieradd, bool thresholdadd, bool threshold2add, bool thresadd, bool chroadd, bool chromaadd, bool contrastadd, bool skinadd, bool reschroadd, bool tmrsadd, bool resconadd, bool resconHadd, bool thradd, bool thrHadd, bool skyadd, bool edgradadd, bool edgvaladd, bool strengthadd,  bool gammaadd, bool edgedetectadd, bool edgedetectthradd , bool edgedetectthr2add)
{

    for (int i = 0; i < 9; i++) {
        correction[i]->setAddMode (multiplieradd);
    }

    threshold->setAddMode (thresholdadd);
    skinprotect->setAddMode (skinadd);
    threshold2->setAddMode (threshold2add);
    thres->setAddMode (thresadd);
    chro->setAddMode (chroadd);
    chroma->setAddMode (chromaadd);
    contrast->setAddMode (contrastadd);
    rescon->setAddMode (resconadd);
    resconH->setAddMode (resconHadd);
    reschro->setAddMode (reschroadd);
    tmrs->setAddMode (tmrsadd);
    thr->setAddMode (thradd);
    thrH->setAddMode (thrHadd);
    sky->setAddMode (skyadd);
    edgrad->setAddMode (edgradadd);
    edgval->setAddMode (edgvaladd);
    strength->setAddMode (strengthadd);
    gamma->setAddMode (gammaadd);
    edgedetect->setAddMode (edgedetectadd);
    edgedetectthr->setAddMode (edgedetectthradd);
    edgedetectthr2->setAddMode (edgedetectthr2add);
}


void Wavelet::neutralPressed ()
{
    for (int i = 0; i < 9; i++) {
        correction[i]->setValue (0);
        adjusterChanged (correction[i], 0);
    }
}

void Wavelet::neutralchPressed ()
{

    for (int i = 0; i < 9; i++) {
        correctionch[i]->setValue (0);
        adjusterChanged (correctionch[i], 0);
    }
}


void Wavelet::contrastPlusPressed ()
{

    for (int i = 0; i < 9; i++) {
        int inc = 1 * (9 - i);
        correction[i]->setValue (correction[i]->getValue() + inc);
        adjusterChanged (correction[i], correction[i]->getValue());
    }
}


void Wavelet::contrastMinusPressed ()
{

    for (int i = 0; i < 9; i++) {
        int inc = -1 * (9 - i);
        correction[i]->setValue (correction[i]->getValue() + inc);
        adjusterChanged (correction[i], correction[i]->getValue());
    }
}

void Wavelet::foldAllButMe (GdkEventButton* event, MyExpander *expander)
{
    if (event->button == 3) {
        expsettings->set_expanded (expsettings == expander);
        expcontrast->set_expanded (expcontrast == expander);
        expchroma->set_expanded (expchroma == expander);
        exptoning->set_expanded (exptoning == expander);
        expnoise->set_expanded (expnoise == expander);
        expedge->set_expanded (expedge == expander);
        expedg1->set_expanded (expedg1 == expander);
        expreti->set_expanded (expreti == expander);
        expgamut->set_expanded (expgamut == expander);
        expresid->set_expanded (expresid == expander);
        expmerge->set_expanded (expmerge == expander);
        expfinal->set_expanded (expfinal == expander);
        expsettingreti->set_expanded (expsettingreti == expander);

    }
}

void Wavelet::enableToggled (MyExpander *expander)
{
    if (expander == expedge) {
        ushamethodChanged();
    };


    if (expander == expmerge) {
        mergMethodChanged();
    }

    if (listener) {
        rtengine::ProcEvent event = NUMOFEVENTS;

        if (expander == expcontrast) {
            event = EvWavenacont;
        } else if (expander == expchroma) {
            event = EvWavenachrom;
        } else if (expander == exptoning) {
            event = EvWavenatoning;
        } else if (expander == expnoise) {
            event = EvWavenanoise;
        } else if (expander == expedge) {
            event = EvWavenaedge;
        } else if (expander == expreti) {
            event = EvWavenareti;
        } else if (expander == expresid) {
            event = EvWavenares;
        } else if (expander == expmerge) {
            event = EvWavenamerge;
        } else if (expander == expfinal) {
            event = EvWavenafin;
        } else
            // unknown expander, returning !
        {
            return;
        }

        if (expander->get_inconsistent()) {
            listener->panelChanged (event, M ("GENERAL_UNCHANGED"));
        } else if (expander->getEnabled()) {
            listener->panelChanged (event, M ("GENERAL_ENABLED"));
        } else {
            listener->panelChanged (event, M ("GENERAL_DISABLED"));
        }
    }
}



void Wavelet::writeOptions (std::vector<int> &tpOpen)
{
    tpOpen.push_back (expsettings->get_expanded ());
    tpOpen.push_back (expcontrast->get_expanded ());
    tpOpen.push_back (expchroma->get_expanded ());
    tpOpen.push_back (exptoning->get_expanded ());
    tpOpen.push_back (expnoise->get_expanded ());
    tpOpen.push_back (expedge->get_expanded ());
    tpOpen.push_back (expedg1->get_expanded ());
    tpOpen.push_back (expreti->get_expanded ());
    tpOpen.push_back (expgamut->get_expanded ());
    tpOpen.push_back (expresid->get_expanded ());
    tpOpen.push_back (expmerge->get_expanded ());
    tpOpen.push_back (expfinal->get_expanded ());
    tpOpen.push_back (expsettingreti->get_expanded ());

}

void Wavelet::updateToolState (std::vector<int> &tpOpen)
{
    if (tpOpen.size() >= 13) {
        expsettings->set_expanded (tpOpen.at (0));
        expcontrast->set_expanded (tpOpen.at (1));
        expchroma->set_expanded (tpOpen.at (2));
        exptoning->set_expanded (tpOpen.at (3));
        expnoise->set_expanded (tpOpen.at (4));
        expedge->set_expanded (tpOpen.at (5));
        expedg1->set_expanded (tpOpen.at (6));
        expreti->set_expanded (tpOpen.at (7));
        expgamut->set_expanded (tpOpen.at (8));
        expresid->set_expanded (tpOpen.at (9));
        expmerge->set_expanded (tpOpen.at (10));
        expfinal->set_expanded (tpOpen.at (11));
        expsettingreti->set_expanded (tpOpen.at (12));
    }
}

