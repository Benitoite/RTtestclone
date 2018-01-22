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

using namespace rtengine;
using namespace rtengine::procparams;

Cat02adap::Cat02adap() : FoldableToolPanel(this, "cat02adap", M("TP_CAT02_LABEL"), true, true)
{

    cat02 = Gtk::manage(new Adjuster(M("TP_CAT02_SLI"), 0, 100, 1, 0));

    if (cat02->delay < options.adjusterMaxDelay) {
        cat02->delay = options.adjusterMaxDelay;
    }

    cat02->throwOnButtonRelease();
    cat02->addAutoButton(M("TP_CAT02_DEGREE_AUTO_TOOLTIP"));
    cat02->set_tooltip_markup(M("TP_CAT02_CAT_TOOLTIP"));

    pack_start(*cat02);

    cat02->setAdjusterListener(this);

    gree = Gtk::manage(new Adjuster(M("TP_CAT02_GREE"), 0.9, 1.1, 0.001, 1));

    if (gree->delay < options.adjusterMaxDelay) {
        gree->delay = options.adjusterMaxDelay;
    }

    gree->throwOnButtonRelease();
    gree->addAutoButton(M("TP_CAT02_DEGREE_AUTO_TOOLTIP"));
    gree->set_tooltip_markup(M("TP_CAT02_GREE_TOOLTIP"));

    labena = Gtk::manage(new Gtk::Label(M("TP_CAT02_CIECAM_ENA")));
    labdis = Gtk::manage(new Gtk::Label(M("TP_CAT02_CIECAM_DISA")));

    gree->setAdjusterListener(this);

    pack_start(*gree);

    pack_start(*labena);
    pack_start(*labdis);


    show_all_children();
    labdis->hide();
}

void Cat02adap::read(const ProcParams* pp, const ParamsEdited* pedited)
{

    disableListener();

    if (pedited) {
        cat02->setEditedState(pedited->cat02adap.cat02 ? Edited : UnEdited);
        cat02->setAutoInconsistent(multiImage && !pedited->cat02adap.autocat02);

        set_inconsistent(multiImage && !pedited->cat02adap.enabled);
        gree->setEditedState(pedited->cat02adap.gree ? Edited : UnEdited);
        gree->setAutoInconsistent(multiImage && !pedited->cat02adap.autogree);

    }

    lastAutocat02 = pp->cat02adap.autocat02;
    lastAutogree = pp->cat02adap.autogree;

    setEnabled(pp->cat02adap.enabled);

    cat02->setValue(pp->cat02adap.cat02);
    cat02->setAutoValue(pp->cat02adap.autocat02);
    gree->setValue(pp->cat02adap.gree);
    gree->setAutoValue(pp->cat02adap.autogree);

    enableListener();
}

void Cat02adap::write(ProcParams* pp, ParamsEdited* pedited)
{

    pp->cat02adap.cat02    = cat02->getValue();
    pp->cat02adap.enabled   = getEnabled();
    pp->cat02adap.autocat02  = cat02->getAutoValue();
    pp->cat02adap.gree    = gree->getValue();
    pp->cat02adap.autogree  = gree->getAutoValue();

    if (pedited) {
        pedited->cat02adap.cat02        = cat02->getEditedState();
        pedited->cat02adap.enabled       = !get_inconsistent();
        pedited->cat02adap.autocat02  = !cat02->getAutoInconsistent();
        pedited->cat02adap.gree        = gree->getEditedState();
        pedited->cat02adap.autogree  = !gree->getAutoInconsistent();

    }
}

void Cat02adap::setDefaults(const ProcParams* defParams, const ParamsEdited* pedited)
{

    cat02->setDefault(defParams->cat02adap.cat02);
    gree->setDefault(defParams->cat02adap.gree);

    if (pedited) {
        cat02->setDefaultEditedState(pedited->cat02adap.cat02 ? Edited : UnEdited);
        gree->setDefaultEditedState(pedited->cat02adap.gree ? Edited : UnEdited);
    } else {
        cat02->setDefaultEditedState(Irrelevant);
        gree->setDefaultEditedState(Irrelevant);
    }
}

void Cat02adap::adjusterChanged(Adjuster* a, double newval)
{

    if (listener && getEnabled()) {
        if (a == cat02) {
            listener->panelChanged(EvCat02cat02, cat02->getTextValue());
        } else if (a == gree) {
            listener->panelChanged(EvCat02gree, gree->getTextValue());
        }

        //listener->panelChanged(EvCat02cat02, Glib::ustring::format(std::setw(2), std::fixed, std::setprecision(1), a->getValue()));
    }
}

void Cat02adap::adjusterAutoToggled(Adjuster* a, bool newval)
{

    if (multiImage) {
        if (cat02->getAutoInconsistent()) {
            cat02->setAutoInconsistent(false);
            cat02->setAutoValue(false);
        } else if (lastAutocat02) {
            cat02->setAutoInconsistent(true);
        }

        lastAutocat02 = cat02->getAutoValue();

        if (gree->getAutoInconsistent()) {
            gree->setAutoInconsistent(false);
            gree->setAutoValue(false);
        } else if (lastAutogree) {
            gree->setAutoInconsistent(true);
        }

        lastAutogree = gree->getAutoValue();

    }

    if (listener && (multiImage || getEnabled())) {

        if (a == cat02) {
            if (cat02->getAutoInconsistent()) {
                listener->panelChanged(EvCATAutocat02, M("GENERAL_UNCHANGED"));
            } else if (cat02->getAutoValue()) {
                listener->panelChanged(EvCATAutocat02, M("GENERAL_ENABLED"));
            } else {
                listener->panelChanged(EvCATAutocat02, M("GENERAL_DISABLED"));
            }
        }

        if (a == gree) {
            if (gree->getAutoInconsistent()) {
                listener->panelChanged(EvCATAutogree, M("GENERAL_UNCHANGED"));
            } else if (gree->getAutoValue()) {
                listener->panelChanged(EvCATAutogree, M("GENERAL_ENABLED"));
            } else {
                listener->panelChanged(EvCATAutogree, M("GENERAL_DISABLED"));
            }
        }




    }
}

void Cat02adap::cat02catChanged(int cat, int ciecam)
{
    nextCadap = cat;
    nextciecam = ciecam;

    const auto func = [](gpointer data) -> gboolean {
        static_cast<Cat02adap*>(data)->cat02catComputed_();
        return FALSE;
    };

    idle_register.add(func, this);
}

bool Cat02adap::cat02catComputed_()
{

    disableListener();
    cat02->setValue(nextCadap);
    labdis->hide();

    if (nextciecam == 1) {
        labena->show();
        labdis->hide();
    } else {
        labena->hide();
        labdis->show();
    }

    enableListener();

    return false;
}

void Cat02adap::cat02greeChanged(double gree)
{
    nextGree = gree;

    const auto func = [](gpointer data) -> gboolean {
        static_cast<Cat02adap*>(data)->cat02greeComputed_();
        return FALSE;
    };

    idle_register.add(func, this);
}

bool Cat02adap::cat02greeComputed_()
{

    disableListener();
    gree->setValue(nextGree);
    enableListener();

    return false;
}

void Cat02adap::enabledChanged()
{
    if (listener) {
        if (get_inconsistent()) {
            listener->panelChanged(EvCat02enabled, M("GENERAL_UNCHANGED"));
        } else if (getEnabled()) {
            listener->panelChanged(EvCat02enabled, M("GENERAL_ENABLED"));
        } else {
            listener->panelChanged(EvCat02enabled, M("GENERAL_DISABLED"));
        }
    }
}

void Cat02adap::setBatchMode(bool batchMode)
{

    ToolPanel::setBatchMode(batchMode);
    cat02->showEditedCB();
    gree->showEditedCB();
}
/*
void Cat02adap::setAdjusterBehavior (bool cat02add)
{

    cat02->setAddMode(cat02add);
}
*/
void Cat02adap::trimValues(rtengine::procparams::ProcParams* pp)
{
    gree->trimValue(pp->cat02adap.cat02);

    cat02->trimValue(pp->cat02adap.cat02);
}
