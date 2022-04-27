// --------------------------------------------------------------------------
// Source code provided FOR REVIEW ONLY, as part of the submission entitled
// "Fiblets for Real-Time Rendering of Massive Brain Tractograms".
//
// A proper version of this code will be released if the paper is accepted
// with the proper licence, documentation and bug fix.
// Currently, this material has to be considered confidential and shall not
// be used outside the review process.
//
// All right reserved. The Authors
// --------------------------------------------------------------------------

#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <QApplication>
#include "mainwindow.h"
#include <qfib.hpp>

int exitUsage(const string& prog)
{
	std::cout << "--------------Help-----------------\n"
		<< "Usage: " << prog << " -i NAME_OF_INPUT_FILE [OPTIONS]\n"
		<< "Or for manual use: " << prog << "\n"
		<< "Formats supported :\n"
		<< "Input: tck, fft\n"
		<< "Output: fft\n"
		<< "By default, compress the tck input file, except with '-p' and '-c' options\n"
		<< "\n-------Options---------\n"
		<< "-h: display this help\n"
		<< "-o output: specify the name of the output file for compression, default: Input with new extension\n"
		<< "-c: capture a full hd frame and exit\n"
		<< "-p: compute performances using the input file\n"
		<< "-----------------------------------\n";
	return 1;
}

int parse_command_line(int argc, char** argv)
{
	string input_file, output_file = "";
	unsigned nb_pts_per_block = 60;
	bool compression = true;
	bool capture_image = false;
	bool analysis_mode = false;
	bool plyExport = false;
	bool meshPlyExport = false;
	for(int a = 1; a < argc; a++)
	{
		auto consume_arg = [&]() -> std::string
		{
			if(a + 1 < argc)
			{
				std::string tmp = argv[a + 1];
				char c = '\0';
				argv[a] = &c; argv[++a] = &c;
				return tmp;
			}
			printf("Warning: missing argument for command '%s'\n", argv[a]);
			return "";
		};
		auto retreive_unsigned = [&]() -> unsigned
		{
			if (a + 1 < argc)
			{
				return unsigned(stoi(argv[a+1]));
			}
			exitUsage(argv[0]);
			return 0;
		};
		if(strcmp(argv[a], "-h") == 0) return exitUsage(argv[0]);
		else if(strcmp(argv[a], "-c") == 0) {capture_image = true; compression = false;}
		else if(strcmp(argv[a], "-i") == 0) input_file = consume_arg();
		else if(strcmp(argv[a], "-o") == 0) output_file = consume_arg();
		else if(strcmp(argv[a], "-b") == 0) nb_pts_per_block = retreive_unsigned();
		else if(strcmp(argv[a], "-p") == 0) {analysis_mode = true; compression = false;}
		else if(strcmp(argv[a], "-e") == 0) {plyExport = true;}
		else if(strcmp(argv[a], "-m") == 0) {plyExport = true; meshPlyExport = true;}
	}
	auto hasEnding = [](const string & path, const string & extension) -> bool
	{
		if (path.length() < extension.length()) return false;
		return (path.compare(path.length() - extension.length(), extension.length(), extension) == 0);
	};
	if (output_file == "" && !plyExport)
				output_file = input_file.substr(0, input_file.length()-4) + ".fft";
	if (output_file == "" && plyExport)
				output_file = input_file.substr(0, input_file.length()-4) + ".ply";
	if (plyExport)
	{
		loadTckAndSavePly(input_file, output_file, meshPlyExport);
		exit(EXIT_SUCCESS);
	}
	if (compression && (!hasEnding(input_file, "tck") || !hasEnding(output_file, "fft") || nb_pts_per_block > 64))
		return exitUsage(argv[0]);
	if (compression)
		return loadTckAndSaveFft(input_file, output_file, nb_pts_per_block);
	else
	{
		QApplication a(argc, argv);
		MainWindow w(analysis_mode, input_file, capture_image);
		w.show();
		return a.exec();
	}
	return 1;
}

int main(int argc, char** argv)
{
	if (argc>1)//Command line
		return parse_command_line(argc, argv);
	else
	{
		QApplication a(argc, argv);
		MainWindow w;
		w.show();

		return a.exec();
	}
}
