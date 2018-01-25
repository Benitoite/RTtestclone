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

    rtengine::ProcEvent EvlocalWBAutotemp;
    rtengine::ProcEvent EvlocalWBAutogreen;
    rtengine::ProcEvent EvlocalWBAutoequal;
    rtengine::ProcEvent EvlocalWBAutocat02;
    rtengine::ProcEvent EvlocalWBAutoytint;
    rtengine::ProcEvent EvlocalWBMethod;
    rtengine::ProcEvent EvlocalWBSmet;
    rtengine::ProcEvent EvlocalWBDegree;
    rtengine::ProcEvent EvlocalWBlocY;
    rtengine::ProcEvent EvlocalWBlocX;
    rtengine::ProcEvent EvlocalWBlocYT;
    rtengine::ProcEvent EvlocalWBlocXL;
    rtengine::ProcEvent EvlocalWBsensi;
    rtengine::ProcEvent EvlocalWBtransit;
    rtengine::ProcEvent EvlocalWBcat02;
    rtengine::ProcEvent EvlocalWBytint;
    rtengine::ProcEvent EvlocalWBtemp;
    rtengine::ProcEvent EvlocalWBgreen;
    rtengine::ProcEvent EvlocalWBequal;
    rtengine::ProcEvent EvlocalWBcircrad;
    rtengine::ProcEvent EvlocalWBCenter;
    rtengine::ProcEvent EvlocalWBEnabled;

    int lastObject;
    void foldAllButMe(GdkEventButton* event, MyExpander *expander);
    void enableToggled(MyExpander *expander);

    IdleRegister idle_register;

    Gtk::HBox *editHBox;
    Gtk::ToggleButton* edit;

    MyExpander* const expsettings;
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
    Adjuster* const cat02;
    Adjuster* const ytint;
    /*
    Adjuster* const hueref;
    Adjuster* const chromaref;
    Adjuster* const lumaref;
    */
    MyComboBoxText*   const Smethod;
    MyComboBoxText*   const wbshaMethod;

    Gtk::Frame* const shapeFrame;
    Gtk::Frame* const artifFrame;
    Gtk::Frame* const superFrame;
    Gtk::Frame* const cat02Frame;

    Gtk::Label* const labqual;
    Gtk::Label* const labmS;
    Gtk::Label* const labmeth;

    Gtk::HBox* const ctboxS;
    Gtk::HBox* const qualbox;
    Gtk::HBox* const ctboxmet;

    Adjuster* temp;
    Adjuster* green;
    Adjuster* equal;
    Gtk::Label* ttLabels;
    Gtk::Label* metLabels;



    Gtk::Button* spotbutton;

    sigc::connection  editConn;


    sigc::connection enablewbConn;


    sigc::connection  Smethodconn;
    sigc::connection wbshaMethodConn;


    double nexttemp;
    double nexttint;
    double nextequal;
    double next_temp;
    double next_green;
    double next_equal;
    int nextCadap;
    double nextGree;

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

    void editToggled();
    bool lastAutotemp;
    bool lastAutogreen;
    bool lastAutoequal;
    bool lastAutocat02;
    bool lastAutoytint;


public:

    Localwb();
    ~Localwb();

    void writeOptions(std::vector<int> &tpOpen);
    void updateToolState(std::vector<int> &tpOpen);

    void read(const rtengine::procparams::ProcParams* pp, const ParamsEdited* pedited = nullptr);
    void write(rtengine::procparams::ProcParams* pp, ParamsEdited* pedited = nullptr);
    void setDefaults(const rtengine::procparams::ProcParams* defParams, const ParamsEdited* pedited = nullptr);
    void setBatchMode(bool batchMode);

    void adjusterChanged(Adjuster* a, double newval);
    void enabledChanged();
    void adjusterAutoToggled(Adjuster* a, bool newval);

    void setAdjusterBehavior(bool hadd, bool sadd, bool lcadd);
    void trimValues(rtengine::procparams::ProcParams* pp);

    void SmethodChanged();
    void wbshaMethodChanged();
    void setEditProvider(EditDataProvider* provider);

    void updateLabel();
    void WBTChanged(double temp);
    bool WBTComputed_();

    void WBGChanged(double green);
    bool WBGComputed_();

    void WBEChanged(double equal);
    bool WBEComputed_();

    void cat02catChanged(int cat);
    bool cat02catComputed_();

    void cat02greeChanged(double ytin);
    bool cat02greeComputed_();

    CursorShape getCursor(int objectID);
    bool mouseOver(int modifierKey);
    bool button1Pressed(int modifierKey);
    bool button1Released();
    bool drag1(int modifierKey);
    void switchOffEditMode();
    void updateGeometry(const int centerX_, const int centerY_, const int circrad_, const int locY_, const double degree_, const int locX_, const int locYT_, const int locXL_, const int fullWidth = -1, const int fullHeight = -1);


};

#endif
