//==================================================
// Copyright (C) 2011 by Norwegian Radiation Protection Authority
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// Authors: Dag Robøle,
//
//==================================================

#ifndef MAIN_H
#define MAIN_H

//==================================================

#define PROG_VERSION_MAJOR	1
#define PROG_VERSION_MINOR	2

//==================================================

struct IO_Header
{
    char spectrum_identifier[6];		// 4
    char sample_identifier[42];			// 40 
    char project[6];				// 4
    char sample_location[32];			// 30    
    float latitude;
    char latitude_unit;
    float longitude;
    char longitude_unit;
    float sample_height;
    float sample_weight; 
    float sample_density; 
    float sample_volume; 
    float sample_uncertainty; 
    float sample_quantity;
    char sample_unit[4];			// 2
    char detector_identifier[4];		// 2
    char year[4];				// 2
    char beaker_identifier[4];			// 2
    char sampling_start[14]; 			// 12
    char sampling_stop[14];			// 12
    char reference_time[14];			// 12
    char measurement_start[14];			// 12
    char measurement_stop[14];			// 12
    int real_time; 
    int live_time; 
    int measurement_time;
    float dead_time;
    char nuclide_library[14];			// 12
    char lim_file[14];				// 12
    int channel_count;
    char format[4];
    short record_length;
    float FWHMPS; 
    float FWHMAN; 
    float THRESH; 
    float BSTF; 
    float ETOL; 
    float LOCH;
    short ICA;
    char energy_file[14];			// 12
    char pef_file[14];				// 12
    char tef_file[14];				// 12
    char background_file[14];			// 12
    int PA1; 
    int PA2; 
    int PA3; 
    int PA4; 
    int PA5; 
    int PA6;
    short print_out; 
    short plot_out; 
    short disk_out; 
    short ex_print_out; 
    short ex_disk_out;
    int PO1; 
    int PO2; 
    int PO3; 
    int PO4; 
    int PO5; 
    int PO6;
    short complete; 
    short analysed;    
    short ST1; 
    short ST2; 
    short ST3; 
    short ST4; 
    short ST5; 
    short ST6;    
};

//==================================================

#endif // MAIN_H

//==================================================
