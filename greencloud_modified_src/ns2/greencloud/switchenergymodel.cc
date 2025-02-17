/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 */

#include "switchenergymodel.h"
#include "scheduler.h"
#include <iostream>
static class SwitchEnergyModelClass : public TclClass {
public:
	SwitchEnergyModelClass() : TclClass("SwitchEnergyModel") {}
	TclObject* create(int argc, const char*const*argv) {
		return (new SwitchEnergyModel());
	}
} class_switchenergymodel;

SwitchEnergyModel::SwitchEnergyModel() : eConsumed_(0.0), eChassis_(0.0), eLineCard_(0.0), ePort_(0.0), eSimEnd_(0.0), eDVFS_enabled_(0), eDNS_enabled_(0), eDNS_delay_(0.0), eEnabled_(0), eCurrentRate_(0.0), eActivePorts_(0), eSimDuration_(0.0), classifier_(NULL), energytimer_(this)
{ 
	bind("eConsumed_", &eConsumed_);
	bind("eCurrentRate_", &eCurrentRate_);
	bind("eChassis_", &eChassis_);
	bind("eLineCard_", &eLineCard_);
	bind("ePort_", &ePort_);
	bind("eSimEnd_", &eSimEnd_);
	bind("eDVFS_enabled_", &eDVFS_enabled_);			/* ON when DVFS is enabled */
	bind("eDNS_enabled_", &eDNS_enabled_);			/* ON when DNS is enabled */
	bind("eDNS_delay_", &eDNS_delay_);
}

SwitchEnergyModel::~SwitchEnergyModel()
{
}

void SwitchEnergyModel::start()
{
	eEnabled_ = 1;
	eLastSample_ = Scheduler::instance().clock();
	eSimDuration_ = eSimEnd_ - eLastSample_;

	if (classifier_) eActivePorts_ = classifier_->maxslot();

	if (eDNS_enabled_) eCurrentRate_ = 0.0;
	else computeCurrentRate();
}

void SwitchEnergyModel::stop()
{
	updateEnergy(0, 0);
}

double SwitchEnergyModel::computeCurrentRate()
{
	eCurrentRate_ = eChassis_ + eLineCard_ + eActivePorts_*ePort_;
	return eCurrentRate_;
}

void SwitchEnergyModel::updateEnergy(int curslot, int nports)
{
	if (eEnabled_ == 0) return;

	/* Compute energy spent since last call */

	if (nports != eActivePorts_) {
		eConsumed_ += eCurrentRate_*(Scheduler::instance().clock() - eLastSample_)/3600;	// update energy
		eActivePorts_ = nports;								// update number of active ports
		computeCurrentRate();
		eLastSample_ = Scheduler::instance().clock();

		/* if DNS is enabled start sleep-mode timer */
		if ((eDNS_enabled_)&& (eDNS_delay_)) energytimer_.resched(eDNS_delay_);
	}

}

int SwitchEnergyModel::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) { 
			start();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "stop") == 0) { 
			stop();
			return (TCL_OK);
		}
	}
	return (SwitchEnergyModel::command(argc, argv));	
}

void SwitchEnergyModel::timeout(){

	eConsumed_ += eCurrentRate_*(Scheduler::instance().clock() - eLastSample_)/3600;	// update energy
	eCurrentRate_ = 0.0;
	eLastSample_ = Scheduler::instance().clock();
}

void SwitchEnergyTimer::expire(Event *)
{
	em_->timeout();
}
