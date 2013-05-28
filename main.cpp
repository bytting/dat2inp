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
// HEADER INCLUDES

#include <cassert>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <cctype>
#include "main.h"
#include "SimpleOpt.h"
#include <Windows.h>

using namespace std;    

//==================================================
// CONVERT A PRIMITIVE TYPE TO A STRING        

template<class T>
std::string to_string(const T& val)
{    
    stringstream ss;
    ss << val;    
    return ss.str();
}

//==================================================
// CONVERT A SLICE OF A CHARACTER STRING TO A PRIMITIVE TYPE    

template<class T>
T convert(const char* src)
{        
    T t;
    memcpy((void*)&t, (void*)src, sizeof(T));
    return t;
}

//==================================================
// FUNCTION DECLARATIONS AND GLOBALS

void print_version(ostream& out);
void print_usage(ostream& out);
string get_args_error(int error);
bool ends_with(const string& full, const string& ending);
void extract_string(const char* src, char* dest);
void dump(const IO_Header& io, ostream& out);
void generate_inp(const IO_Header& io, ostream& out);

enum { OPT_VERSION, OPT_USAGE, OPT_HELP, OPT_STDOUT, OPT_DUMP, OPT_DEFDETLIMLIB };

CSimpleOpt::SOption g_command_line_options[] =
{
    { OPT_VERSION, 		("--version"),     						SO_NONE		},
    { OPT_USAGE, 		("--usage"), 							SO_NONE		},
    { OPT_HELP, 		("--help"), 							SO_NONE		},        
    { OPT_STDOUT, 		("--stdout"), 							SO_NONE		},            
    { OPT_DUMP, 		("--dump"), 							SO_NONE		},
	{ OPT_DEFDETLIMLIB,	("--default-detection-limit-library"),	SO_MULTI	},
    SO_END_OF_OPTIONS
};

char* prog_name;

//==================================================
// Program: dat2inp
// 
// This program is a utility program for gamma10 to convert .DAT files into .INP files.
// The somewhat akward code used to extract data from the DAT file is 
// due to the reverse engineering process of the DAT file format.

int main(int argc, char **argv) 
{        
    prog_name = argv[0];                
    
    // PROCESS COMMAND LINE OPTIONS    
    
    bool use_stdout = false;    
    bool use_dump = false;    
	bool has_defdetlimlib = false;
	string defdetlimlib = "";
    
    CSimpleOpt args(argc, argv, g_command_line_options);               
    
    while (args.Next()) 
    {
		if (args.LastError() != SO_SUCCESS) 
		{
			cerr << get_args_error(args.LastError()) << "\n\n";
			print_usage(cerr);
			return 1;
		}
		
		switch(args.OptionId()) 
		{
			case OPT_VERSION: print_version(cout); return 0;
			case OPT_HELP:	      
			case OPT_USAGE: print_usage(cout); return 0;
			case OPT_STDOUT: use_stdout = true; break;	    
			case OPT_DUMP: use_dump = true; break;	    
			case OPT_DEFDETLIMLIB: 
				char **margs = args.MultiArg(1);
				if(!margs)
				{
					print_usage(cerr);
					return 1;
				}
				defdetlimlib = margs[0];
				has_defdetlimlib = true;
				break;	    
		}
    }        
    
    if(args.FileCount())
    {
		print_usage(cerr);
		return 1;
    }
    
    // HEADER AND DATA STRUCTURE DECLARATIONS    
        
    IO_Header io;    
    vector<string> files, error_messages;
	vector<unsigned int> file_sizes;    
	unsigned int processed_files = 0, max_file_size = 0;            
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError;		
	string dir = ".\\*.DAT";		
	
	hFind = FindFirstFile(dir.c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 	
	{
		clog << "No .DAT files found in current directory. Exiting..." << endl;
	    return 0;	
	}
		
	files.push_back(FindFileData.cFileName);	
	file_sizes.push_back((unsigned int)FindFileData.nFileSizeLow);
	max_file_size = (unsigned int)FindFileData.nFileSizeLow > max_file_size ? (unsigned int)FindFileData.nFileSizeLow : max_file_size;
      	
	while (FindNextFile(hFind, &FindFileData) != 0) 	
	{
		files.push_back(FindFileData.cFileName);	
		file_sizes.push_back((unsigned int)FindFileData.nFileSizeLow);
		max_file_size = (unsigned int)FindFileData.nFileSizeLow > max_file_size ? (unsigned int)FindFileData.nFileSizeLow : max_file_size;
	}
    
	dwError = GetLastError();
	FindClose(hFind);
	if (dwError != ERROR_NO_MORE_FILES) 	
	{
		cerr << "Failed reading directory " + dir << endl;
		return 1;
	}
    
	char *buffer = new (nothrow) char[max_file_size + 1];
	if(!buffer)
	{
		cerr << "Failed to allocate buffer[" << max_file_size + 1 << "]" << endl;
		return 1;
	}
	else clog << "Allocated buffer[" << max_file_size + 1 << "]" << endl;	

    // PROCESS EACH DAT FILE    
    
	for(unsigned int i=0; i<files.size(); i++)
    {	
		memset((void*)&io, 0, sizeof(io));    
		memset((void*)buffer, 0, sizeof(buffer));				
		
		// READ THE DAT FILE INTO A BUFFER	
	
		ifstream fio(files[i].c_str(), fstream::binary);        
		if(!fio.good())
		{			
			error_messages.push_back("UNABLE TO OPEN FILE: " + files[i]);
			continue;
		}			

		if(!fio.read(buffer, file_sizes[i]))
		{
			fio.close();
			error_messages.push_back("UNABLE TO READ FILE: " + files[i]);
			continue;
		}
		fio.close();							
	
		// FILL THE IO_Header STRUCTURE WITH DATA EXTRACTED FROM THE DAT BUFFER	
		
		extract_string(buffer, io.spectrum_identifier);				
		extract_string(buffer + 5, io.sample_identifier);					
		extract_string(buffer + 46, io.project);					
		extract_string(buffer + 51, io.sample_location);					
		io.latitude = convert<float>(buffer + 82);				
		io.latitude_unit = *(buffer + 86);			
		io.longitude = convert<float>(buffer + 87);				
		io.longitude_unit = *(buffer + 91);					
		io.sample_height = convert<float>(buffer + 92);						
		io.sample_weight = convert<float>(buffer + 96);				
		io.sample_density = convert<float>(buffer + 100);			
		io.sample_volume = convert<float>(buffer + 104);					
		io.sample_quantity = convert<float>(buffer + 108);			
		io.sample_uncertainty = convert<float>(buffer + 112);					
		extract_string(buffer + 128, io.sampling_start);		
		extract_string(buffer + 141, io.sampling_stop);		
		extract_string(buffer + 154, io.reference_time);		
		extract_string(buffer + 167, io.measurement_start);		
		extract_string(buffer + 180, io.measurement_stop);		
		io.FWHMPS = convert<float>(buffer + 245);			
		io.FWHMAN = convert<float>(buffer + 249);			
		io.THRESH = convert<float>(buffer + 253);			
		io.BSTF = convert<float>(buffer + 257);			
		io.ETOL = convert<float>(buffer + 261);			
		io.LOCH = convert<float>(buffer + 265);				
		io.ICA = convert<short>(buffer + 269);							
		io.real_time = convert<int>(buffer + 193);				
		io.live_time = convert<int>(buffer + 197);								
		io.measurement_time = convert<int>(buffer + 201);									
		io.dead_time = (float)io.real_time - io.live_time;
		io.dead_time /= (float)io.live_time;
		io.dead_time *= 100.0f;		
		extract_string(buffer + 116, io.sample_unit);			
		extract_string(buffer + 119, io.detector_identifier);			
		extract_string(buffer + 122, io.year);			
		extract_string(buffer + 125, io.beaker_identifier);		
		extract_string(buffer + 209, io.nuclide_library);	
		extract_string(buffer + 222, io.lim_file);
		if(!strlen(io.lim_file) && has_defdetlimlib)
			strcpy(io.lim_file, defdetlimlib.c_str());
		extract_string(buffer + 271, io.energy_file);	
		extract_string(buffer + 284, io.pef_file);	
		extract_string(buffer + 297, io.tef_file);	
		extract_string(buffer + 310, io.background_file);	
		io.channel_count = convert<int>(buffer + 235);						
		extract_string(buffer + 239, io.format);	
		io.record_length = convert<int>(buffer + 243);						
	
		io.PA1 = convert<int>(buffer + 323);		
		io.PA2 = convert<int>(buffer + 327);		
		io.PA3 = convert<int>(buffer + 331);		
		io.PA4 = convert<int>(buffer + 335);		
		io.PA5 = convert<int>(buffer + 339);		
		io.PA6 = convert<int>(buffer + 343);		
	
		io.print_out = convert<short>(buffer + 347);		
		io.plot_out = convert<short>(buffer + 349);		
		io.disk_out = convert<short>(buffer + 351);		
		io.ex_print_out = convert<short>(buffer + 353);		
		io.ex_disk_out = convert<short>(buffer + 355);		
	
		io.PO1 = convert<int>(buffer + 357);		
		io.PO2 = convert<int>(buffer + 361);		
		io.PO3 = convert<int>(buffer + 365);		
		io.PO4 = convert<int>(buffer + 369);		
		io.PO5 = convert<int>(buffer + 373);		
		io.PO6 = convert<int>(buffer + 377);		
	
		io.complete = convert<short>(buffer + 381);		
		io.analysed = convert<short>(buffer + 383);		
	
		io.ST1 = convert<int>(buffer + 385);		
		io.ST2 = convert<int>(buffer + 387);		
		io.ST3 = convert<int>(buffer + 389);		
		io.ST4 = convert<int>(buffer + 391);		
		io.ST5 = convert<int>(buffer + 393);		
		io.ST6 = convert<int>(buffer + 395);				
	
		// WRITE RESULTS BASED ON COMMAND LINE OPTIONS	
		
		if(use_dump)
			dump(io, cout);	
		else if(use_stdout)
			generate_inp(io, cout);
		else
		{
			string fname = files[i].substr(0, files[i].length() - 4) + ".INP";
			ofstream out(fname.c_str(), fstream::binary);
			if(!out.good())
			{
				cerr << "FAILED TO OPEN FILE FOR WRITING: " << fname << endl;
				return 1;
			}
			generate_inp(io, out);
			out.close();
		}		

		++processed_files;
		clog << files[i] << " converted successfully" << endl;
    }   

	delete [] buffer;
    
    // PRINT STATUS INFORMATION
    
	for(vector<string>::iterator it = error_messages.begin(); it != error_messages.end(); ++it)
		cerr << *it << endl;

    clog << "Of " << files.size() << " DAT files, " << processed_files << " was successfully converted" << endl;	
    
    return 0;
}

//==================================================
// FUNCTION TO WRITE VERSION INFORMATION    

void print_version(ostream& out)
{        
    out << PROG_VERSION_MAJOR << "." << PROG_VERSION_MINOR;        
}

//==================================================
// FUNCTION TO WRITE USAGE INFORMATION    

void print_usage(ostream& out)
{        
    out << prog_name << " ";
    print_version(out);
    out << " - 2011 Dag Robole, Norwegian Radiation Protection Authority\n\n";
    out << "This program is a utility program for gamma10.\n";
    out << "It will convert any .DAT files in the current directory into .INP files.\n\n";        
    out << "\t--version\n\t\tPrint version information and exit\n\n";
    out << "\t--usage | --help\n\t\tPrint this message and exit\n\n";
    out << "\t--stdout\n\t\tWrite results to standard output instead of .INP files\n\n";
    out << "\t--dump\n\t\tWrite results to standard output instead of .INP files in debug friendly format\n\n";
	out << "\t--default-detection-limit-library <filename>\n\t\tUse <filename> as the default detection limit library in DAT files\n";
	out << "\t\twhere this field is empty.\n\t\tThe new version of gamma10 need a filename here so dont forget to supply it\n\n";
    out << "Examples:\n\t" << prog_name << " --default-detection-limit-library mdalib01.lib\n\t" << prog_name << " --stdout\n" << endl;        
}

//==================================================
// FUNCTION RETURNING A STRING REPRESENTATION OF A SinpleOpt ERROR    

string get_args_error(int error)
{        
    switch (error) 
    {
	case SO_SUCCESS: return "Success";
	case SO_OPT_INVALID: return "Unrecognized option";
	case SO_OPT_MULTIPLE: return "Option matched multiple strings";
	case SO_ARG_INVALID: return "Option does not accept argument";
	case SO_ARG_INVALID_TYPE: return "Invalid argument format";
	case SO_ARG_MISSING: return "Required argument is missing";
	case SO_ARG_INVALID_DATA: return "Invalid argument data";
	default: return "Unknown error";
    }
}

//==================================================
// FUNCTION USED TO DETERMINE IF A STRING IS FOUND AT THE END OF ANOTHER STRING    

bool ends_with(const string& full, const string& ending)
{      
    string f = full;
    string e = ending;
    transform(f.begin(), f.end(), f.begin(), ::toupper);
    transform(e.begin(), e.end(), e.begin(), ::toupper);
    if (f.length() > e.length())     
        return (0 == f.compare (f.length() - e.length(), e.length(), e));
    else return false;    
}

//==================================================
// FUNCTION USED TO EXTRACT AND TRIM A STRING FROM THE DAT BUFFER.
// PASCAL STRINGS SEEMS TO STORE THE STRING LENGTH AT THE FIRST BYTES AND WITH
// NO NULL TERMINATING CHARACTER, SO A WORKAROUND IS NEEDED    
    
void extract_string(const char* src, char* dest)
{            
    unsigned char len = (unsigned char)*src;	        
    strncpy(dest, src + sizeof(unsigned char), len);
    *(dest + len) = 0;
    
    // Trimming the end of the string
    while(--len > 0)
    {
		if(isspace(*(dest + len)) || !*(dest + len))
			*(dest + len) = 0;
		else break;		
    }
    
    if(!len && isspace(*dest))
		*dest = 0;
}

//==================================================
// FUNCTION TO DUMP THE IO_Header DEBUG INFORMATION TO A STREAM    

void dump(const IO_Header& io, ostream& out)
{        
    out << "spectrum identifier: " << io.spectrum_identifier << endl;
    out << "sample identifier: " << io.sample_identifier << endl;
    out << "project: " << io.project << endl;
    out << "sample location: " << io.sample_location << endl;    
    out << "latitude: " << io.latitude << endl;
    out << "latitude unit: " << io.latitude_unit << endl;
    out << "longitude: " << io.longitude << endl;
    out << "longitude unit: " << io.longitude_unit << endl;
    out << "sample height: " << io.sample_height << endl;
    out << "sample weight: " << io.sample_weight << endl;
    out << "sample density: " << io.sample_density << endl;
    out << "sample volume: " << io.sample_volume << endl;		
    out << "sample quantity: " << io.sample_quantity << endl;	
    out << "sample uncertainty: " << io.sample_uncertainty << endl;	
    out << "sampling start: " << io.sampling_start << endl;	
    out << "sampling stop: " << io.sampling_stop << endl;	
    out << "reference time: " << io.reference_time << endl;
    out << "measurement start: " << io.measurement_start << endl;
    out << "measurement stop: " << io.measurement_stop << endl;
    out << "format: " << io.format << endl;
    out << "FWHMPS: " << io.FWHMPS << endl;
    out << "FWHMAN: " << io.FWHMAN << endl;
    out << "THRESH: " << io.THRESH << endl;
    out << "BSTF: " << io.BSTF << endl;
    out << "ETOL: " << io.ETOL << endl;
    out << "LOCH: " << io.LOCH << endl;
    out << "ICA: " << io.ICA << endl;	
    out << "live time: " << io.live_time << endl;
    out << "real time: " << io.real_time << endl;	
    out << "dead time: " << io.dead_time << endl;	
    out << "measurement time: " << io.measurement_time << endl;	
    out << "channel count: " << io.channel_count << endl;	
    out << "record length: " << io.record_length << endl;	
    out << "sample unit: " << io.sample_unit << endl;	
    out << "detector id: " << io.detector_identifier << endl;	
    out << "year: " << io.year << endl;	
    out << "beaker id: " << io.beaker_identifier << endl;	
    out << "nuclide library: " << io.nuclide_library << endl;
    out << "energy file: " << io.energy_file << endl;
    out << "pef file: " << io.pef_file << endl;
    out << "tef file: " << io.tef_file << endl;
    out << "background file: " << io.background_file << endl;
    out << "LIM file: " << io.lim_file << endl << endl;
}

//==================================================
// FUNCTION TO WRITE THE IO_Header INFORMATION TO A STREAM    

void generate_inp(const IO_Header& io, ostream& out)
{                 
    out.setf(ios_base::scientific);
    out.precision(14);
    
    out << io.spectrum_identifier << endl;
    out << io.sample_identifier << endl;
    out << io.project << endl;
    out << io.sample_location << endl;    
    out << io.latitude << endl;
    out << io.latitude_unit << endl;
    out << io.longitude << endl;
    out << io.longitude_unit << endl;
    out << io.sample_height << endl;
    out << io.sample_weight << endl;
    out << io.sample_density << endl;
    out << io.sample_volume << endl;
    out << io.sample_quantity << endl;
	out << io.sample_uncertainty << endl;
    out << io.sample_unit << endl;
    out << io.detector_identifier << endl;
    out << io.year << endl;
    out << io.beaker_identifier << endl;
    out << io.sampling_start << endl;
    out << io.sampling_stop << endl;
    out << io.reference_time << endl;
    out << io.measurement_start << endl;
    out << io.measurement_stop << endl;
    out << io.real_time << endl;
    out << io.live_time << endl;
    out << io.measurement_time << endl;        
    out << io.dead_time << endl;        
    out << io.nuclide_library << endl;	
    out << io.lim_file << endl;
    out << io.channel_count << endl;
    out << io.format << endl;
    out << io.record_length << endl;                
    out << io.FWHMPS << endl;
    out << io.FWHMAN << endl;
    out << io.THRESH << endl;
    out << io.BSTF << endl;
    out << io.ETOL << endl;
    out << io.LOCH << endl;
    out << io.ICA << endl;
    out << io.energy_file << endl;
    out << io.pef_file << endl;
    out << io.tef_file << endl;
    out << io.background_file << endl;
    out << io.PA1 << endl;
    out << io.PA2 << endl;
    out << io.PA3 << endl;
    out << io.PA4 << endl;
    out << io.PA5 << endl;
    out << io.PA6 << endl;
    out << io.print_out << endl;
    out << io.plot_out << endl;
    out << io.disk_out << endl;
    out << io.ex_print_out << endl;
    out << io.ex_disk_out << endl;
    out << io.PO1 << endl;
    out << io.PO2 << endl;
    out << io.PO3 << endl;
    out << io.PO4 << endl;
    out << io.PO5 << endl;
    out << io.PO6 << endl;
    out << io.complete << endl;
    out << io.analysed << endl;
    out << io.ST1 << endl;
    out << io.ST2 << endl;
    out << io.ST3 << endl;
    out << io.ST4 << endl;
    out << io.ST5 << endl;
    out << io.ST6 << endl;
    
    out.unsetf(ios_base::scientific);
}

//==================================================
