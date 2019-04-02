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
#include "cat02adap.h"
#include <cmath>
#include <iomanip>
#include "guiutils.h"
#include "eventmapper.h"
#include "options.h"

using namespace rtengine;
using namespace rtengine::procparams;
extern Options options;

CAT02Adaptation::CAT02Adaptation(): FoldableToolPanel(this, "cat02adap", M("TP_CAT02ADAPTATION_LABEL"), true, true)
{
    auto m = ProcEventMapper::getInstance();
    EvCAT02AdaptationEnabled = m->newEvent(ALLNORAW, "HISTORY_MSG_CAT02ADAPTATION_ENABLED");
    EvCAT02AdaptationAmount = m->newEvent(ALLNORAW, "HISTORY_MSG_CAT02ADAPTATION_AMOUNT");
    EvCAT02AdaptationAutoAmount = m->newEvent(ALLNORAW, "HISTORY_MSG_CAT02ADAPTATION_AUTO_AMOUNT");
    EvCAT02AdaptationLuminanceScaling = m->newEvent(ALLNORAW, "HISTORY_MSG_CAT02ADAPTATION_LUMINANCE_SCALING");
    EvCAT02AdaptationAutoLuminanceScaling = m->newEvent(ALLNORAW, "HISTORY_MSG_CAT02ADAPTATION_AUTO_LUMINANCE_SCALING");
    
    amount = Gtk::manage(new Adjuster(M("TP_CAT02ADAPTATION_AMOUNT"), 0, 100, 1, 0));

    if (amount->delay < options.adjusterMaxDelay) {
        amount->delay = options.adjusterMaxDelay;
    }

    amount->throwOnButtonRelease();
    amount->addAutoButton(M("TP_CAT02ADAPTATION_AUTO_AMOUNT_TOOLTIP"));
    amount->set_tooltip_markup(M("TP_CAT02ADAPTATION_AMOUNT_TOOLTIP"));

    pack_start(*amount);

    amount->setAdjusterListener(this);

    luminanceScaling = Gtk::manage(new Adjuster(M("TP_CAT02ADAPTATION_LUMINANCE_SCALING"), 0.9, 1.1, 0.001, 1));

    if (luminanceScaling->delay < options.adjusterMaxDelay) {
        luminanceScaling->delay = options.adjusterMaxDelay;
    }

    luminanceScaling->throwOnButtonRelease();
    luminanceScaling->addAutoButton(M("TP_CAT02ADAPTATION_AUTO_LUMINANCE_SCALING_TOOLTIP"));
    luminanceScaling->set_tooltip_markup(M("TP_CAT02ADAPTATION_LUMINANCE_SCALING_TOOLTIP"));

    labena = Gtk::manage(new Gtk::Label(M("TP_CAT02ADAPTATION_CIECAM_ENABLED")));
    labdis = Gtk::manage(new Gtk::Label(M("TP_CAT02ADAPTATION_CIECAM_DISABLED")));

    luminanceScaling->setAdjusterListener(this);

    pack_start(*luminanceScaling);

    pack_start(*labena);
    pack_start(*labdis);


    show_all_children();
    labdis->hide();
}

void CAT02Adaptation::read(const ProcParams* pp, const ParamsEdited* pedited)
{

    disableListener();

    if (pedited) {
        amount->setEditedState(pedited->cat02adap.amount ? Edited : UnEdited);
        amount->setAutoInconsistent(multiImage && !pedited->cat02adap.autoAmount);

        set_inconsistent(multiImage && !pedited->cat02adap.enabled);
        luminanceScaling->setEditedState(pedited->cat02adap.luminanceScaling ? Edited : UnEdited);
        luminanceScaling->setAutoInconsistent(multiImage && !pedited->cat02adap.autoLuminanceScaling);

    }

    lastAutoAmount = pp->cat02adap.autoAmount;
    lastAutoLuminanceScaling = pp->cat02adap.autoLuminanceScaling;

    setEnabled(pp->cat02adap.enabled);

    amount->setValue(pp->cat02adap.amount);
    amount->setAutoValue(pp->cat02adap.autoAmount);
    luminanceScaling->setValue(pp->cat02adap.luminanceScaling);
    luminanceScaling->setAutoValue(pp->cat02adap.autoLuminanceScaling);

    enableListener();
}

void CAT02Adaptation::write(ProcParams* pp, ParamsEdited* pedited)
{

    pp->cat02adap.amount    = amount->getValue();
    pp->cat02adap.enabled   = getEnabled();
    pp->cat02adap.autoAmount  = amount->getAutoValue();
    pp->cat02adap.luminanceScaling    = luminanceScaling->getValue();
    pp->cat02adap.autoLuminanceScaling  = luminanceScaling->getAutoValue();

    if (pedited) {
        pedited->cat02adap.amount        = amount->getEditedState();
        pedited->cat02adap.enabled       = !get_inconsistent();
        pedited->cat02adap.autoAmount  = !amount->getAutoInconsistent();
        pedited->cat02adap.luminanceScaling        = luminanceScaling->getEditedState();
        pedited->cat02adap.autoLuminanceScaling  = !luminanceScaling->getAutoInconsistent();

    }
}

void CAT02Adaptation::setDefaults(const ProcParams* defParams, const ParamsEdited* pedited)
{

    amount->setDefault(defParams->cat02adap.amount);
    luminanceScaling->setDefault(defParams->cat02adap.luminanceScaling);

    if (pedited) {
        amount->setDefaultEditedState(pedited->cat02adap.amount ? Edited : UnEdited);
        luminanceScaling->setDefaultEditedState(pedited->cat02adap.luminanceScaling ? Edited : UnEdited);
    } else {
        amount->setDefaultEditedState(Irrelevant);
        luminanceScaling->setDefaultEditedState(Irrelevant);
    }
}

void CAT02Adaptation::adjusterChanged(Adjuster* a, double newval)
{

    if (listener && getEnabled()) {
        if (a == amount) {
            listener->panelChanged(EvCAT02AdaptationAmount, amount->getTextValue());
        } else if (a == luminanceScaling) {
            listener->panelChanged(EvCAT02AdaptationLuminanceScaling, luminanceScaling->getTextValue());
        }

        //listener->panelChanged(EvCat02cat02, Glib::ustring::format(std::setw(2), std::fixed, std::setprecision(1), a->getValue()));
    }
}

void CAT02Adaptation::adjusterChanged(ThresholdAdjuster* a, double newBottom, double newTop)
{
}

void CAT02Adaptation::adjusterChanged(ThresholdAdjuster* a, double newBottomLeft, double newTopLeft, double newBottomRight, double newTopRight)
{
}

void CAT02Adaptation::adjusterChanged(ThresholdAdjuster* a, int newBottom, int newTop)
{
}

void CAT02Adaptation::adjusterChanged(ThresholdAdjuster* a, int newBottomLeft, int newTopLeft, int newBottomRight, int newTopRight)
{
}

void CAT02Adaptation::adjusterChanged2(ThresholdAdjuster* a, int newBottomL, int newTopL, int newBottomR, int newTopR)
{
}


void CAT02Adaptation::adjusterAutoToggled(Adjuster* a, bool newval)
{

    if (multiImage) {
        if (amount->getAutoInconsistent()) {
            amount->setAutoInconsistent(false);
            amount->setAutoValue(false);
        } else if (lastAutoAmount) {
            amount->setAutoInconsistent(true);
        }

        lastAutoAmount = amount->getAutoValue();

        if (luminanceScaling->getAutoInconsistent()) {
            luminanceScaling->setAutoInconsistent(false);
            luminanceScaling->setAutoValue(false);
        } else if (lastAutoLuminanceScaling) {
            luminanceScaling->setAutoInconsistent(true);
        }

        lastAutoLuminanceScaling = luminanceScaling->getAutoValue();

    }

    if (listener && (multiImage || getEnabled())) {

        if (a == amount) {
            if (amount->getAutoInconsistent()) {
                listener->panelChanged(EvCAT02AdaptationAutoAmount, M("GENERAL_UNCHANGED"));
            } else if (amount->getAutoValue()) {
                listener->panelChanged(EvCAT02AdaptationAutoAmount, M("GENERAL_ENABLED"));
            } else {
                listener->panelChanged(EvCAT02AdaptationAutoAmount, M("GENERAL_DISABLED"));
            }
        }

        if (a == luminanceScaling) {
            if (luminanceScaling->getAutoInconsistent()) {
                listener->panelChanged(EvCAT02AdaptationAutoLuminanceScaling, M("GENERAL_UNCHANGED"));
            } else if (luminanceScaling->getAutoValue()) {
                listener->panelChanged(EvCAT02AdaptationAutoLuminanceScaling, M("GENERAL_ENABLED"));
            } else {
                listener->panelChanged(EvCAT02AdaptationAutoLuminanceScaling, M("GENERAL_DISABLED"));
            }
        }




    }
}

void CAT02Adaptation::cat02AmountChanged(int amoun, bool ciecamEnabled)
{
        idle_register.add(
        [this, amoun, ciecamEnabled]() -> bool {
            int amo = amoun;
            amount->setValue(amo);

            bool necie = ciecamEnabled;
            if (necie) {
                labena->show();
                labdis->hide();
            } else {
                labena->hide();
                labdis->show();
            }
            
            return false;
        }
        );
    
}
void CAT02Adaptation::cat02LuminanceScalingChanged(double scaling)
{
        idle_register.add(
        [this, scaling]() -> bool {
            double scal = scaling;
              luminanceScaling->setValue(scal);
            return false;
        }
        );
}

void CAT02Adaptation::enabledChanged()
{
    if (listener) {
        if (get_inconsistent()) {
            listener->panelChanged(EvCAT02AdaptationEnabled, M("GENERAL_UNCHANGED"));
        } else if (getEnabled()) {
            listener->panelChanged(EvCAT02AdaptationEnabled, M("GENERAL_ENABLED"));
        } else {
            listener->panelChanged(EvCAT02AdaptationEnabled, M("GENERAL_DISABLED"));
        }
    }
}

void CAT02Adaptation::setBatchMode(bool batchMode)
{

    ToolPanel::setBatchMode(batchMode);
    amount->showEditedCB();
    luminanceScaling->showEditedCB();
}

void CAT02Adaptation::trimValues(rtengine::procparams::ProcParams* pp)
{
    luminanceScaling->trimValue(pp->cat02adap.amount);
    amount->trimValue(pp->cat02adap.amount);
}
