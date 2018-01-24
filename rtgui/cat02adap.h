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
#pragma once

#include <gtkmm.h>
#include "adjuster.h"
#include "toolpanel.h"

class CAT02Adaptation: public ToolParamBlock, public AdjusterListener, public rtengine::AutoCAT02Listener, public FoldableToolPanel {
private:
    Adjuster* amount;
    Adjuster* luminanceScaling;
    bool lastAutoAmount;
    bool lastAutoLuminanceScaling;
    IdleRegister idle_register;
    int nextAmount;
    bool nextciecam;
    double nextLuminanceScaling;
    Gtk::Label* labena;
    Gtk::Label* labdis;

    rtengine::ProcEvent EvCAT02AdaptationEnabled;
    rtengine::ProcEvent EvCAT02AdaptationAmount;
    rtengine::ProcEvent EvCAT02AdaptationAutoAmount;
    rtengine::ProcEvent EvCAT02AdaptationLuminanceScaling;
    rtengine::ProcEvent EvCAT02AdaptationAutoLuminanceScaling;
    
public:
    CAT02Adaptation();

    void read(const rtengine::procparams::ProcParams* pp, const ParamsEdited* pedited = nullptr);
    void write(rtengine::procparams::ProcParams* pp, ParamsEdited* pedited = nullptr);
    void setDefaults(const rtengine::procparams::ProcParams* defParams, const ParamsEdited* pedited = nullptr);
    void setBatchMode(bool batchMode);

    void adjusterChanged(Adjuster* a, double newval);
    void adjusterAutoToggled(Adjuster* a, bool newval);

    void enabledChanged();
    void cat02AmountChanged(int amount, bool ciecamEnabled);
    bool cat02AmountComputed_();
    void cat02LuminanceScalingChanged(double scaling);
    bool cat02LuminanceScalingComputed_();

    void trimValues(rtengine::procparams::ProcParams* pp);
};
