#pragma once

// library headers
#include <configuration.hpp>
#include "simulationobserver.hpp"

/*An implementation of SimulationObserver that writes data to an hdf5 file*/
class HDF5Writer
		: public SimulationObserver
{
	// retrive the configuraiton parameters
	Configuration conf;

public:
	void simulation_updated(const Distributed2DField& data) override;
};
