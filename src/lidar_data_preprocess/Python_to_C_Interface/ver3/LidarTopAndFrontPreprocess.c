#include <stdio.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> 
#include <string.h>
#include <cstring>
#include <unistd.h>
#include <sstream>
#include <iomanip>  
#include <vector>
#include <math.h>
#include <iostream>
#include <cstdlib>
#include <iostream>
#include <fstream>    
#include <string>
#include <vector>

using namespace std;

extern "C"
{	
	typedef struct PointT {
		float x;
		float y;
		float z;
		float intensity;
	} PointT;

	void createTopAndFrontMaps(const void * raw_data, int num, const void * top_data, const void * front_data, float x_MIN, float x_MAX, float y_MIN, float y_MAX, float z_MIN, float z_MAX, float x_DIVISION, float y_DIVISION, float z_DIVISION, int X_SIZE, int Y_SIZE, int Z_SIZE, float phi_MIN, float phi_MAX, float theta_MIN, float theta_MAX, float phi_DIVISION, float theta_DIVISION, float C_SIZE, float R_SIZE, float F_SIZE)
    {	    
		float * raw_cube = (float *) raw_data;
		float * top_cube = (float *) top_data;
		float * front_cube = (float *) front_data;

		const float RAD_TO_DEG = 180/3.141596;		
		int FV_CENTER_C = C_SIZE/2;
		int FV_CENTER_R = R_SIZE/2;

		//int32_t num = 123586;
		float *px = raw_cube+0;
		float *py = raw_cube+1;
		float *pz = raw_cube+2;
		float *pr = raw_cube+3;

		//3D grid box index
		int X = 0;	
		int Y = 0;
		int Z = 0;

		//height features X_SIZE * Y_SIZE * Z_SIZE
		vector<vector<vector<float> > > height_maps;

		//max height X_SIZE * Y_SIZE * 1  (used to record current highest cell for X,Y while parsing data)
		vector<vector<float> > max_height_map;

		//density feature X_SIZE * Y_SIZE * 1
		vector<vector<float> > density_map;

		//intensity feature X_SIZE * Y_SIZE * 1
		vector<vector<float > > intensity_map;

		height_maps.resize(X_SIZE);
		max_height_map.resize(X_SIZE);
		density_map.resize(X_SIZE);
		intensity_map.resize(X_SIZE);

		for (int i=0; i<X_SIZE; i++)
		{
			height_maps[i].resize(Y_SIZE);
			max_height_map[i].resize(Y_SIZE);
			density_map[i].resize(Y_SIZE);
			intensity_map[i].resize(Y_SIZE);

			for (int j=0; j<Y_SIZE; j++)
			{
				height_maps[i][j].resize(Z_SIZE);
				
				//initialization for height_maps
				for (int k=0; k<Z_SIZE; k++)
					height_maps[i][j][k] = 0;	//value stored inside always >= 0 (relative to z_MIN, unit : m)
			
				//initialization for density_map, max_height_map, intensity_map
				density_map[i][j] = 0;	//value stored inside always >= 0, usually < 1 ( log(count#+1)/log(64), no unit )
				max_height_map[i][j] = 0;	//value stored inside always >= 0  (relative to z_MIN, unit : m)
				intensity_map[i][j] = 0;	//value stored inside always >= 0 && <=255 (range=0~255, no unit)
			}
		}

		//allocate point cloud for temporally data visualization (only used for validation)
		vector<std::vector<PointT> > height_cloud_vec;

		// Initialize the vector of vectors with a dummy. The dummy is later deleted.
		for(int i = 0; i < Z_SIZE; i++){
			std::vector<PointT> dummy;
			PointT dummy_member;
			dummy.push_back(dummy_member);
			height_cloud_vec.push_back(dummy);
		}

		vector<PointT> intensity_cloud;
		vector<PointT> density_cloud;

		
		int front_cube_size = C_SIZE * R_SIZE * F_SIZE;
		// float front_cube[front_cube_size];	// dimensions 400x400x8

		// initialize the front cube to be all zeros
		for(int i = 0; i < front_cube_size; i++){
			front_cube[i] = 0;
		}

		for (int32_t i=0; i<num; i++) {
			PointT point;
			point.x = *px;
			point.y = *py;
			point.z = *pz;
			point.intensity = (*pr) * 255;	// Kitti data normalized between 0 and 1
		
			//X = (int)((point.x-x_MIN)/x_DIVISION);
			//Y = (int)((point.y-y_MIN)/y_DIVISION);
			//Z = (int)((point.z-z_MIN)/z_DIVISION);

			X = (int)floor((point.x-x_MIN)/x_DIVISION);	// X_SIZE is 400	qxs
		    Y = (int)floor((point.y-y_MIN)/y_DIVISION);   // Y_SIZE is 400	qys
		    Z = (int)floor((point.z-z_MIN)/z_DIVISION);   // Z_SIZE is 6	qzs
	
			//For every point in each cloud, only select points inside a predefined 3D grid box
			if (X >= 1 && Y>= 1 && Z >=0 && X < X_SIZE && Y < Y_SIZE && Z < Z_SIZE)  // 1 used for X,Y offset compensation
			{
				// For every point in predefined 3D grid box.....
				// height map
				if ((point.z - z_MIN) > height_maps[X][Y][Z])
				{	
					height_maps[X][Y][Z] = point.z - z_MIN;
					
					//Save to point cloud for visualization -----				
					PointT grid_point;
					grid_point.x = X-1;
					grid_point.y = Y-1;
					grid_point.z = 0;
					//grid_point.intensity = point.z - z_MIN;
					grid_point.intensity = (point.z - z_MIN)/z_DIVISION;
					// height_cloud_vec[Z].push_back(grid_point);

					height_cloud_vec.at(Z).push_back(grid_point);
				}

				// density map
				density_map[X][Y]++;	// update count#, need to be normalized afterwards
				PointT grid_point;
				grid_point.x = X-1;
				grid_point.y = Y-1;
				grid_point.z = 0;
				grid_point.intensity = density_map[X][Y];
				density_cloud.push_back(grid_point);
			
				// intensity map
				if ((point.z - z_MIN) > max_height_map[X][Y])
				{
					max_height_map[X][Y] = point.z - z_MIN;
					intensity_map[X][Y] = point.intensity;

					//Save to point cloud for visualization -----
					PointT grid_point;
					grid_point.x = X-1;
					grid_point.y = Y-1;
					grid_point.z = 0;
					grid_point.intensity = point.intensity;
					intensity_cloud.push_back(grid_point);
				}
				
				int R = (int)(atan2(point.z, sqrt(pow(point.x,2)+pow(point.y,2))) * RAD_TO_DEG / phi_DIVISION );
				int C = (int)(atan2(point.y, point.x) * RAD_TO_DEG / theta_DIVISION);

				if ((R+FV_CENTER_R) <= R_SIZE && (C+FV_CENTER_C) <= C_SIZE && (R+FV_CENTER_R) >=0 && (C+FV_CENTER_C) >=0)
				{
					int c_coord = C+FV_CENTER_C;	//x
					int r_coord = R+FV_CENTER_R;	//y		
					
					int array_idx;
					
					//front height map
					array_idx = C_SIZE * (F_SIZE) * r_coord + (F_SIZE) * c_coord + 0;
					front_cube[array_idx] = (int)((point.z-z_MIN)/(z_MAX-z_MIN) *255);
					//front distance map
					array_idx = C_SIZE * (F_SIZE) * r_coord + (F_SIZE) * c_coord + 1;
					front_cube[array_idx] = (int)(sqrt(pow(point.x,2)+pow(point.y,2))/120 *255);
					//front intensity map
					array_idx = C_SIZE * (F_SIZE) * r_coord + (F_SIZE) * c_coord + 2;
					//front_cube[array_idx] = (int)((point.intensity-0)/255 *255);
					front_cube[array_idx] = (int)((point.intensity-0) *255);
				}
				else
				{
				/*
					std::cout<< "R_SIZE (Vertical) :"<<R_SIZE <<",C_SIZE (Horizontal) :" << C_SIZE<<std::endl;
					std::cout<< "FV_CENTER_R:"<<FV_CENTER_R  <<",FV_CENTER_C:" << FV_CENTER_R <<std::endl;
					std::cout<< "R:"<< R <<",C:" << C <<std::endl;
					std::cout<< "R+FV_CENTER_R:"<<R+FV_CENTER_R <<",C+FV_CENTER_C:" << C+FV_CENTER_C <<std::endl;
					std::cout<< "Data is out of image range"<<std::endl;
				*/
				;
				}


			}
		    px+=4; py+=4; pz+=4; pr+=4;
		}


		// erase the dummy vector added at the start
		for(int i = 0; i < Z_SIZE; i++){
			height_cloud_vec.at(i).erase(height_cloud_vec.at(i).begin());
		}

		// normalization
		for (unsigned int i=0; i < density_cloud.size(); i++){
//			float val = log10(density_cloud.at(i).intensity+1)/log10(64);
			float val = log(density_cloud.at(i).intensity+1)/log(32);	//In DiDi, we use velodyne 32 beam

			if(val < 1)	
            	density_cloud.at(i).intensity = val;
            else
            	density_cloud.at(i).intensity = 1;
		}

		int top_cube_size = X_SIZE * Y_SIZE * (Z_SIZE + 2);
		// float top_cube[top_cube_size];	// dimensions 400x400x8

		// initialize the top cube to be all zeros
		for(int i = 0; i < top_cube_size; i++){
			top_cube[i] = 0;
		}
		
		// Treat the height maps first
		vector<PointT> *cloud_demo;

		for (int k = 0; k<Z_SIZE; k++){
			// *cloud_demo = height_cloud_vec[k];
			cloud_demo = &height_cloud_vec.at(k);
		  	int cloud_size = cloud_demo->size();
			int plane_idx = k;	// because this is the kth plane (starting from zero)

			for(int i = 0; i < cloud_size; i++){				
				int x_coord = (int)cloud_demo->at(i).x;
				int y_coord = (int)cloud_demo->at(i).y;

				// array_idx equals 400*8*y + 8*x + plane_idx where plane_idx goes from 0 to 5 (for the 6 height maps)
				// The formula for array_idx below was empirically calculated to fit the exact order in which the elements
				// of a 3D array are stored in memory. Elements are read starting with the top left corner of the bottom plane
				// the 3D array, then moving up in the z direction, then across (x) and then below (y)
				int array_idx = Y_SIZE * (Z_SIZE + 2) * x_coord + (Z_SIZE + 2) * y_coord + plane_idx;

				float value = cloud_demo->at(i).intensity;

				//top_cube[array_idx] = value;
				top_cube[array_idx] = value-k;	//normalized relative to the current grid
			}
		}

		// Treat the intensity map
		cloud_demo = &intensity_cloud;
		int cloud_size = cloud_demo->size();
		int plane_idx = Z_SIZE;	// because this is the 6th plane (starting from zero)

		for(int i = 0; i < cloud_size; i++){
			int x_coord = (int)cloud_demo->at(i).x;
			int y_coord = (int)cloud_demo->at(i).y;

			// array_idx equals 400*8*x + 8*y + 7 where 7 is the 7th (starting at 0) 400x400 plane
			int array_idx = Y_SIZE * (Z_SIZE + 2) * x_coord + (Z_SIZE + 2) * y_coord + plane_idx;
			float value = cloud_demo->at(i).intensity;

			top_cube[array_idx] = value;
		}

		// Treat the density map	(last layer)
		cloud_demo = &density_cloud;
		cloud_size = cloud_demo->size();
		plane_idx = Z_SIZE + 1;	// because this is the 7th plane (starting from zero)

		for(int i = 0; i < cloud_size; i++){
			int x_coord = (int)cloud_demo->at(i).x;
			int y_coord = (int)cloud_demo->at(i).y;

			// array_idx equals 400*8*x + 8*y + 6 where 6 is the 6th (starting at 0) 400x400 plane
			int array_idx = Y_SIZE * (Z_SIZE + 2) * x_coord + (Z_SIZE + 2) * y_coord + plane_idx;
			float value = cloud_demo->at(i).intensity;

			top_cube[array_idx] = value;
		}
		
		// cout << "C++ part exited. LiDAR data pre-processing completed" << endl;
    }
}
