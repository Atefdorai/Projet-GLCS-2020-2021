// the implemented class
#include "hdf5writer.hpp"

// headers
#include <chrono>
#include <iostream>
#include <thread>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#include <mpi.h>

#include <hdf5.h>

using std::cout;
using std::endl;
using std::flush;
using std::this_thread::sleep_for;
using std::chrono::millisconds;
using std::to_string;

HDF5Writer::HDF5Writer(const Configuration& config)
	: conf(config)  //define an object that contains the configuration 
{

	const string file_name = to_string(conf.output_file());

	// define block_height and block_width
	int block_width  = conf.global_shape()[ DX ];
	int block_height = conf.global_shape()[ DY ];

	// create the file dataspace
	int x_size = conf.dist_extents()[ DX ];
	int y_size = conf.dist_extents()[ DY ];
	const vector<hsize_t> file_dims{block_height * y_size, block_width * x_size};
	file_space = H5Screate_simple(file_dims.size(), file_dims.data(), NULL);

}

void HDF5Writer::simulation_updated(const Distributed2DField& data)
{	

	// initialize the MPI library
	MPI_Init(&argc, &argv);

	// create the dataset		
	dataset= H5Dcreate(h5file, iter_cur, H5T_NATIVE_DOUBLE, file_space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

	// store the data to be writen in the file
	vector<double> data(height * width);

        for ( int _jj = data.distribution().extent( DY )-1; _jj >=0 ; --_jj ) {
                for ( int jj = data.noghost_view().extent( DY )-1; jj >=0 ; --jj ) {
                        for ( int _ii = 0; _ii < data.distribution().extent( DX ); ++_ii ) {
                                if ( data.distribution().coord( DX ) == _ii && data.distribution().coord( DY ) == _jj ) {

                                        for ( int ii = 0; ii < data.noghost_view().extent( DX ); ++ii ) {
                                                if ( 0 == data.noghost_view(yy, xx) ) {
                                                        data[jj * data.noghost_view().extent(DX) + ii] = NAN;
                                                } else {
                                                        data[jj * data.noghost_view().extent(DX) + ii] = data.noghost_view(jj, ii);
                                                }
                                        }                                       
                                }
                        }
                }
        }
		
	// create the file access property list
	const hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
	H5Pset_fapl_mpio(fapl, data.distribution.communicator(), MPI_INFO_NULL);

	// create the file
	const hid_t h5file = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);

	// create the region we want to write in the file dataspace
	int rank_y = data.distribution().coord()[ DY ]; 
	int rank_x = data.distribution().coord()[ DX ];	
	const vector<hsize_t> file_start {rank_y * block_height, rank_x * block_width};
	const vector<hsize_t> file_count {data.noghost_view().exten(DY), data.noghost_view().extent(DX)};
	H5Select_hyperslab(h5file, H5S_SELECT_SET, file_start.data(), NULL, file_count().data(), NULL);

	// create the memory dataspace
	const vector<hsize_t> mem_dims {data.noghost_view().extent(DY), data.noghost_view().extent(Dx)};
	const hid_t mem_space = H5Screate_simple(mem_dims.size(), mem_dims.data(), NULL);
	
	// select the region we want to write in the memory dataspace	
	const vector<hsize_t> mem_start {1, 1};
	const vector<hsize_t> mem_count {data.noghost_view().extent(DY), data.noghost_view().extent(DX)};
	H5Sselect_hyperslab(mem_space, H5S_SELECT_SET, mem_start.data(), NULL, mem_count.data(), NULL);
	
	// create the data transfer property list
	const hid_t xfer_plist = H5Pcreate(H5P_DATASET_XFER);
	H5Pset_dxpl_mpio(xfer_plist, H5FD_MPIO_COLLECTIVE);
	
	// write data to HDF5!
	H5Dwrite(dataset, H5T_NATIVE_DOUBLE, mem_space, file_space, m_xfer_plist, data.data());


	H5Pclose(fapl);
	H5Pclose(xfer_plist);
	H5Sclose(mem_space);
	H5Dclose(dataset);
	H5Sclose(file_space);
	H5Fclose(h5file);
	
	// finalize MPI
	MPI_Finalize();
}
