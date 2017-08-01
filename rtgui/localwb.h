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
#ifndef _LOCALWB_H_
#define _LOCALWB_H_

#include <gtkmm.h>
#include "adjuster.h"
#include "toolpanel.h"
#include "edit.h"
#include "guiutils.h"

class Localwb :
    public ToolParamBlock,
    public AdjusterListener,
    public FoldableToolPanel,
    public rtengine::localrgbListener,
    public EditSubscriber

{

//protected:

private:
    int lastObject;
    void foldAllButMe (GdkEventButton* event, MyExpander *expander);
    void enableToggled (MyExpander *expander);

    IdleRegister idle_register;

    Gtk::HBox *editHBox;
    Gtk::ToggleButton* edit;

    MyExpander* const expsettings;
    MyExpander* const expwb;

    Adjuster* nbspot;

    Adjuster* const anbspot;
    Adjuster* const locX;
    Adjuster* const locXL;
    Adjuster* const degree;
    Adjuster* const locY;
    Adjuster* const locYT;
    Adjuster* const centerX;
    Adjuster* const centerY;
    Adjuster* const circrad;
    Adjuster* const thres;
    Adjuster* const proxi;
    Adjuster* const sensi;
    Adjuster* const transit;
    Adjuster* const retrab;
    Adjuster* const hueref;
    Adjuster* const chromaref;
    Adjuster* const lumaref;

    MyComboBoxText*   const Smethod;
    MyComboBoxText*   const qualityMethod;
    MyComboBoxText*   const wbMethod;
    MyComboBoxText*   const wbcamMethod;

    Gtk::Frame* const shapeFrame;
    Gtk::Frame* const artifFrame;
    Gtk::Frame* const superFrame;

    Gtk::Label* const labqual;
    Gtk::Label* const labmS;
    Gtk::Label* const labcam;

    Gtk::HBox* const ctboxS;
    Gtk::HBox* const qualbox;
    Gtk::HBox* const cambox;

    Adjuster* temp;
    Adjuster* green;
    Adjuster* equal;
    Gtk::Label* ttLabels;
    Gtk::Label* metLabels;



    Gtk::Button* spotbutton;

    sigc::connection  editConn;


    sigc::connection enablewbConn;


    sigc::connection  Smethodconn;
    sigc::connection qualityMethodConn;
    sigc::connection wbMethodConn;
    sigc::connection wbcamMethodConn;

    Gtk::CheckButton* gamma;
    sigc::connection gammaconn;
    bool lastgamma;

    double nexttemp;
    double nexttint;
    double nextequal;
    double next_temp;
    double next_green;
    int next_wbauto;
    int nextmeth;

    double draggedPointOldAngle;
    double draggedPointAdjusterAngle;
    double draggedFeatherOffset;
    double draggedlocYOffset;
    double draggedlocXOffset;
    double draggedlocYTOffset;
    double draggedlocXLOffset;
    rtengine::Coord draggedCenter;

    void editToggled ();



public:

    Localwb ();
    ~Localwb ();

    void writeOptions (std::vector<int> &tpOpen);
    void updateToolState (std::vector<int> &tpOpen);

    void read           (const rtengine::procparams::ProcParams* pp, const ParamsEdited* pedited = nullptr);
    void write          (rtengine::procparams::ProcParams* pp, ParamsEdited* pedited = nullptr);
    void setDefaults    (const rtengine::procparams::ProcParams* defParams, const ParamsEdited* pedited = nullptr);
    void setBatchMode   (bool batchMode);

    void adjusterChanged (Adjuster* a, double newval);
    void enabledChanged  ();

    void setAdjusterBehavior (bool hadd, bool sadd, bool lcadd);
    void trimValues          (rtengine::procparams::ProcParams* pp);

    void SmethodChanged      ();
    void gamma_toggled ();
    void qualityMethodChanged();
    void wbMethodChanged();
    void wbcamMethodChanged();
    void setEditProvider (EditDataProvider* provider);
    bool localwbComputed_ ();

    void temptintChanged (double ctemp, double ctint, double cequal, int meth);
    bool temptintComputed_ ();
    void updateLabel      ();
    void WBChanged           (double temp, double green, int wbauto);

    CursorShape getCursor (int objectID);
    bool mouseOver (int modifierKey);
    bool button1Pressed (int modifierKey);
    bool button1Released();
    bool drag1 (int modifierKey);
    void switchOffEditMode ();
    void updateGeometry (const int centerX_, const int centerY_, const int circrad_, const int locY_, const double degree_, const int locX_, const int locYT_, const int locXL_, const int fullWidth = -1, const int fullHeight = -1);


};

#endif
