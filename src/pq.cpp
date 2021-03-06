#include <cstdio>
#include <iostream>
#include <string>
#include <sys/time.h>
//#include <yalaa.hpp>
#include "pq_io.hpp"
#include "pq_type.hpp"
#include "pq_wrapper.hpp"
#ifdef	OPENMP
#include "pq_core_openmp.hpp"
#else
#include "pq_core.hpp"
#endif

using namespace std;

//template <typename T>
//T eval_func(const T& x0, const T& x1)
//{
//	return x0 * x1;
//}

int main(const int argc, const char *argv[])
{	
	unsigned int	mode;
	unsigned int    no_C;							// number of contour -> number of segment = no_C-1
	unsigned int	no_pt;
	unsigned int	no_link;
	unsigned int 	no_deg;							// number of vertices on each polar-form contour
	unsigned int    seg_idx[2]; 					// indices of the selected segments

	double3 		*pt;     						// input: points to be tested
	double3 		C[MAX_NUM_SEGMENTS+1]; 			// input: contour centres
	double3 		M[MAX_NUM_SEGMENTS+1]; 			// input: contour normals
	double3			W[MAX_NUM_SEGMENTS+1][MAX_NUM_DEGREES]; // input: contour points coordinate P
	unsigned int    L[MAX_NUM_LINKS];      			// input: starting index for each link
	unsigned int	max_m[MAX_NUM_SEGMENTS+1]; 		// input: plane to be omitted
	double			phi[MAX_NUM_SEGMENTS+1][MAX_NUM_DEGREES]; // input: polar point angles
	double  		*min_dist;     					// output: deviation distances of each point
	double3 		*out_pt; 						// output: point lying in-between v2 and v3 in point mode
	unsigned int	*out_pt_idx; 					// output: contour index of the point in point mode
	unsigned int	link_pt_idx[MAX_NUM_LINKS]; 	// output: contour index of the point in link mode
	double  		dev[MAX_NUM_LINKS];      		// output: deviation distances of each link
	double			pt_contour_dev[MAX_NUM_SEGMENTS+1];
	unsigned int	pt_contour_idx[MAX_NUM_SEGMENTS+1];

	struct timeval tv1, tv2;
	unsigned long long processing_time;

	pq_io		data;
	pq_wrapper	wrap;
	pq_core		core;

	pt = (double3*)malloc(sizeof(double3)*MAX_NUM_POINTS);
	min_dist = (double*)malloc(sizeof(double)*MAX_NUM_POINTS);
	out_pt = (double3*)malloc(sizeof(double3)*MAX_NUM_POINTS);
	out_pt_idx = (unsigned int*)malloc(sizeof(unsigned int)*MAX_NUM_POINTS);

	//yalaa::aff_e_d x0(iv_t(-1.0, 1.0));
	//yalaa::aff_e_d x1(iv_t(-1.0, 1.0));
	//std::cout << "SHCB  over [-1,1]^2: "<< eval_func(x0, x1) << std::endl;

	// Read data
	std::cout << "Reading data..." << std::endl;
	if (data.data_in(argc, argv, mode, no_pt, pt, no_link, L, no_C, no_deg, C, M, W, seg_idx)==-1)
		return -1;

	// Pre-processing
	gettimeofday(&tv1, NULL);
	// Host: Pre-process contour data
	std::cout << "Pre-processing..." << std::endl;
	wrap.pre_process(no_C, no_deg, max_m, phi, C, M, W);
	// FPGA: Update content in ROMs - C, M, W

	// Call PQ core
	std::cout << "Core computation..." << std::endl;
#ifdef	OPENMP
#pragma omp parallel for num_threads(NT)
#endif
	// FPGA: Unroll loop here (Multiple kernels)
	for (unsigned int i=0; i<no_pt; i++)
		core.pt_dist(no_C, no_deg, seg_idx, i, pt[i], max_m, phi, C, M, W, min_dist, out_pt, out_pt_idx);

	// Post-processing
	// Host: Post-process output data
	std::cout << "Post-processing..." << std::endl;
	wrap.post_process(mode, no_link, no_pt, L, out_pt_idx, min_dist, dev, link_pt_idx, pt_contour_dev, pt_contour_idx);
	gettimeofday(&tv2, NULL);
	processing_time = (tv2.tv_sec - tv1.tv_sec)*1000000 + (tv2.tv_usec - tv1.tv_usec);
	printf("Info: Processing time is %lu us.\n", (long unsigned int)processing_time);

	// Write results to files
	data.data_out(mode, no_link, no_pt, seg_idx, min_dist, out_pt, out_pt_idx, link_pt_idx, dev, pt_contour_dev, pt_contour_idx);

	free(pt);
	free(min_dist);
	free(out_pt);
	free(out_pt_idx);

	return 0;
}
