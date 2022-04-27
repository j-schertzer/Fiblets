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

#pragma once

#include "compression.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

using namespace std;

//Version number of this version of the software
static const uint8_t versionNumber = 2;

///
/// \brief The CompressedTractogram struct
/// contains the compressed fibers of the tractogram as well as multiple information on the data
///

struct CompressedTractogram
{
	uint64_t nbOfFibers = 0;///\brief Number of fibers in the tractogram
	uint64_t nbOfPts = 0;///\brief Total number of points in the tractogram
	unsigned nbPtsPerBlock = 60;///\brief Number of points per block (in [0, 64])
	unsigned packSize;///\brief Size of a block in bytes (Can be computed from nbPtsPerBlock so is not recorded)
	//Data is in [0, 0, 0], [1, 1, 1], centered in [0.5, 0.5, 0.5]
	float scaling;///\brief Scaling factor
	glm::fvec3 offset;///\brief Offset factor
	float ratio;///\brief Ratio used for the compression
	float stepsize;///\brief Average stepsize of the original dataset
	float maxError;///\brief Maximum compression error (mm)
	float meanError;///\brief Average compression error (mm)
	std::vector<uint64_t> startPositionOfSSBOs;///\brief Starting positions of the SSBO in packedCompressedFibers
	std::vector<glm::uvec2> fiberBlockLut;///\brief Look up table used for fiber selection, .x = SSBO ID<<16 + length of fiber in blocks, .y = BeginBlock ID
	std::vector<std::vector<uint8_t>> packedCompressedFibers;///\brief The compressed fibers

	///
	/// \brief Function to compute the pack size from the number of points per block
	///

	void computePackSize()
	{
		if ((nbPtsPerBlock + 16) % 4 == 0)
			packSize = (nbPtsPerBlock + 16);
		else
			packSize = ((nbPtsPerBlock + 16) + (4 - (nbPtsPerBlock + 16) % 4));
	}
};

///
/// \brief Function to compress a tractogram
/// \param inputFileName Path of the input tractogram
/// \param tracto Tractogram compressed
///

inline void compress(const std::string & inputFileName, CompressedTractogram & tracto);

///
/// \brief Function to evaluate if at least one value of a glm::vec3 is Nan
/// \param v glm::vec3 to evaluate
/// \return true if at least one coordinate is Nan, false otherwise
///

inline bool isnan(const glm::vec3 v)
{
   return (isnan(float(v.x)) || isnan(float(v.y)) || isnan(float(v.z)));
}

///
/// \brief Function to evaluate if at least one value of a glm::vec3 is Inf
/// \param v glm::vec3 to evaluate
/// \return true if at least one coordinate is Inf, false otherwise
///

inline bool isinf(const glm::vec3 v)
{
   return (isinf(float(v.x)) || isinf(float(v.y)) || isinf(float(v.z)));
}

///
/// \brief Function that opens a tck file and reads its header
/// \param filename Path of the tck file to open
/// \param file Fstream that is used to open the file to (remains open)
/// \param nbOfFibers Number of fibers read from the input file
/// \param offset Offset in bytes where the actual data start appearing in the tck file (retrieved from file)
/// \return
///

inline bool readTckHeader(const string & filename, fstream & file, uint64_t & nbOfFibers, unsigned int & offset)
{
	if (filename.find(".tck")==string::npos)
	{
		cerr << "File must be in the tck format" << endl;
		return false;
	}
	file.open(filename, ios_base::in | ios_base::binary);
	if (!file.is_open())
	{
		cerr << "Impossible to find file " << filename << endl;
		return false;
	}
	string message;
	file >> message;
	if (message!="mrtrix")
	{
		cerr << "Tck file not correct" << endl;
		return false;
	}
	file >> message;
	if (message!="tracks")
	{
		cerr << "Tck file not correct" << endl;
		return false;
	}
	bool count = false, float32le = false;
	while (message!="END")
	{
		file >> message;
		if (message=="count:")
		{
			file >> message;
			cout << stoi(message) << " fibers found ..." << endl;
			nbOfFibers = stoi(message);
			count = true;
		}
		if (message.find("datatype")!=string::npos)
		{
			file >> message;
			if (message!="Float32LE")
			{
				cerr << "Format not supported: " << message << endl;
				return false;
			}
			else
				float32le = true;
		}
		if (message.find("file")!=string::npos)
		{
			file >> message;
			file >> message;
			offset = stoi(message);
		}
	}
	if (offset==0)
	{
		cerr << "File needs to be specified in tck file: " << filename << endl;
		return false;
	}
	if (!(float32le && count))
	{
		cerr << "Error with file header, count or datatype not specified" << endl;
		return false;
	}
	return true;
}

///
/// \brief Function that computes the boundaries of a single fiber as well as the ratio used for compression
/// \param fiber Fiber for which to evaluate the boundaries and the ratio
/// \param minbounds Set of minimum coordinates of the fiber
/// \param maxbounds Set of maximum coordinates of the fiber
/// \param mdot Minimum dot product encountered in the fiber
/// \param stepsize Stepsize - used to compute the average stepsize for a set of fibers
/// \param approxNbPts The approximate number of points in the tractogram - used for the average stepsize computation
///

inline void boundariesAndRatio(std::vector<glm::vec3> fiber, glm::fvec3 & minbounds, glm::fvec3 & maxbounds, float & mdot, double & stepsize, const double approxNbPts)
{
	glm::fvec3 v1, v2;
	v1 = glm::normalize(fiber[1] - fiber[0]);
	stepsize += (glm::length(fiber[1] - fiber[0])) / approxNbPts;
	for (unsigned i=0; i<2; i++)
	{
		minbounds = glm::min(minbounds, fiber[i]);
		maxbounds = glm::max(maxbounds, fiber[i]);
	}
	for (unsigned i=2; i<fiber.size(); i++)
	{
		minbounds = glm::min(minbounds, fiber[i]);
		maxbounds = glm::max(maxbounds, fiber[i]);
		stepsize += (glm::length(fiber[i] - fiber[i - 1])) / approxNbPts;
		v2 = glm::normalize(fiber[i] - fiber[i-1]);
		float dot = glm::dot(v2, v1);
		mdot = (dot <  mdot) ? dot : mdot;
		v1 = v2;
	}
}

///
/// \brief Function to load a tractogram in the tck format, compute normalization parameters and its ratio for compression - this function does not compress the tractogram
/// \param filename Path of the tck file
/// \param tracto Tractogram used only to store the computed ratio and parameters - this function does not compress the tractogram
///

inline void loadTckFibersNormalizeAndComputeRatio(const string & filename, CompressedTractogram & tracto)
{
	tracto.nbOfFibers = 0;
	fstream file;
	unsigned headerLength = 0;
	if (!readTckHeader(filename, file, tracto.nbOfFibers, headerLength))
		exit(EXIT_FAILURE);

	glm::fvec3 minbounds(std::numeric_limits<float>::max()), maxbounds(std::numeric_limits<float>::min());
	float mdot = 2.0;
	file.seekg(headerLength, file.beg);
	glm::fvec3 newPoint = glm::vec3(0.f,0.f,0.f);
	//We start by computing an order of magnitude of the number of segment per fiber (average on the first 100 fibers)
	uint64_t approxNbPtsPerFibers = 0;
	uint8_t nbFibs = 0;
	while (!isinf(newPoint) && nbFibs <= 100)
	{
		file.read(reinterpret_cast<char *>(&newPoint), sizeof (glm::vec3));
		if (isnan(newPoint) || isinf(newPoint))
		{
			nbFibs++;
			if (isinf(newPoint))
			{
				break;
			}
			file.read(reinterpret_cast<char *>(&newPoint), sizeof (glm::vec3));
			if (!isnan(newPoint) && !isinf(newPoint))
				approxNbPtsPerFibers++;
		}
		else
		{
			approxNbPtsPerFibers++;
		}
	}
	double approxNbPts = approxNbPtsPerFibers * (tracto.nbOfFibers / double(nbFibs));
	file.seekg(headerLength, file.beg);
	newPoint = glm::vec3(0.f,0.f,0.f);
	std::vector<glm::fvec3> newFiber;
	newFiber.reserve(approxNbPtsPerFibers * 2);
	tracto.nbOfPts = 0;
	double computeStepsize = 0.0;

	tracto.startPositionOfSSBOs.clear();
	tracto.fiberBlockLut.clear();
	try {
		tracto.fiberBlockLut.reserve(tracto.nbOfFibers);
	}
	catch (const std::length_error& le) {
		std::cerr << "Length error: " << le.what() << '\n';
	}
	unsigned numOfSSBO = 0;
	uint32_t fblockStart = 0;
	uint64_t nbOfBlocks = 0;
	uint64_t totalNbOfBlocks = 0;
	const unsigned maxBlocksInSSBO = unsigned(std::floor(pow(2, 31) / tracto.packSize));//2^31 bytes max in an SSBO (GLint), and we do groups of PACK_SIZE bytes, 2^31 / 80 = 26843545.6 for 80 bytes

	while (!isinf(newPoint))
	{
		file.read(reinterpret_cast<char *>(&newPoint), sizeof (glm::vec3));
		if (isnan(newPoint) || isinf(newPoint))
		{
			uint32_t nbBlocksInFiber = uint32_t(std::ceil((newFiber.size()) / float(tracto.nbPtsPerBlock)) + 0.5f);
			if (nbOfBlocks + nbBlocksInFiber > maxBlocksInSSBO)
			{
				totalNbOfBlocks += nbOfBlocks;
				tracto.startPositionOfSSBOs.push_back(totalNbOfBlocks);
				nbOfBlocks = nbBlocksInFiber;
				numOfSSBO++;
				fblockStart = 0;
			}
			else
				nbOfBlocks += nbBlocksInFiber;
			tracto.fiberBlockLut.push_back(glm::uvec2((numOfSSBO<<16)+nbBlocksInFiber, fblockStart));
			fblockStart += nbBlocksInFiber;
			boundariesAndRatio(newFiber, minbounds, maxbounds, mdot, computeStepsize, approxNbPts);
			tracto.nbOfPts += newFiber.size();
			newFiber.clear();
			if (isinf(newPoint))
			{
				break;
			}
			file.read(reinterpret_cast<char *>(&newPoint), sizeof (glm::vec3));
			if (!isnan(newPoint) && !isinf(newPoint))
				newFiber.push_back(newPoint);
		}
		else
		{
			newFiber.push_back(newPoint);
		}
	}
	tracto.startPositionOfSSBOs.push_back(totalNbOfBlocks + nbOfBlocks);
	tracto.stepsize = float(computeStepsize * (approxNbPts / double(tracto.nbOfPts)));

	std::cout << "Stepsize: " << tracto.stepsize << std::endl;
	std::cout << "Approximation of the total number of points: " << approxNbPts << std::endl;
	std::cout << "Total number of points: " << tracto.nbOfPts << std::endl;
	file.close();

	tracto.offset = -0.5f * (maxbounds + minbounds);
	maxbounds += tracto.offset;
	minbounds += tracto.offset;
	tracto.scaling = std::max({maxbounds.x, maxbounds.y, maxbounds.z}) - std::min({minbounds.x, minbounds.y, minbounds.z});

	//Ratio
	const float E = float(tracto.scaling / std::pow(2, 16));
	const float A = atan((2 * E)/(tracto.stepsize - 2*E));
	tracto.ratio = std::min(2.0f, 1.f - cos(acos(mdot) + A));

	tracto.stepsize *= 1.f / tracto.scaling;
}

///
/// \brief Function to save a compressed tractogram to the fft file format
/// \param filename Path of the fft file to save
/// \param tracto Compressed tractogram to save
///

inline void saveFft(string filename, const CompressedTractogram & tracto)
{
	fstream file;
	if (filename.find(".fft") == string::npos)
		filename = filename + ".fft";
	file.open(filename, ios_base::out | ios_base::binary);
	if (!file.is_open())
	{
		cerr << "Impossible to write file " << filename << endl;
		exit(EXIT_FAILURE);
	}
	file.write(reinterpret_cast<const char *>(&versionNumber), sizeof(uint8_t));
	file.write(reinterpret_cast<const char *>(&tracto.nbOfFibers), sizeof(uint64_t));
	cout << "nbOfFibers: " << tracto.nbOfFibers << endl;
	file.write(reinterpret_cast<const char *>(&tracto.nbOfPts), sizeof(uint64_t));
	unsigned nbOfBlocks = (unsigned)(tracto.packedCompressedFibers.size());
	cout << "nbOfBlocks: " << nbOfBlocks << endl;
	file.write(reinterpret_cast<const char *>(&nbOfBlocks), sizeof(unsigned));
	file.write(reinterpret_cast<const char *>(&tracto.nbPtsPerBlock), sizeof(unsigned));
	file.write(reinterpret_cast<const char *>(&tracto.scaling), sizeof(float));
	file.write(reinterpret_cast<const char *>(&tracto.offset), sizeof(glm::fvec3));
	file.write(reinterpret_cast<const char *>(&tracto.ratio), sizeof(float));
	file.write(reinterpret_cast<const char *>(&tracto.stepsize), sizeof(float));
	file.write(reinterpret_cast<const char *>(&tracto.maxError), sizeof(float));
	file.write(reinterpret_cast<const char *>(&tracto.meanError), sizeof(float));
	uint8_t nbOfSSBOs = uint8_t(tracto.startPositionOfSSBOs.size());
	cout << "NbOfSSBOs: " << unsigned(nbOfSSBOs) << endl;
	file.write(reinterpret_cast<const char *>(&nbOfSSBOs), sizeof (uint8_t));
	for (unsigned i=0; i<nbOfSSBOs; i++)
		file.write(reinterpret_cast<const char *>(&tracto.startPositionOfSSBOs[i]), sizeof (uint64_t));
	for (unsigned i=0; i<tracto.nbOfFibers; i++)
		file.write(reinterpret_cast<const char *>(&tracto.fiberBlockLut[i]), sizeof (glm::uvec2));
	for (unsigned i = 0; i < tracto.packedCompressedFibers.size(); i++)
		file.write(reinterpret_cast<const char *>(&tracto.packedCompressedFibers[i][0]), tracto.packSize * sizeof(uint8_t));
}

///
/// \brief Function to load an fft file containing a compressed tractogram
/// \param filename Path of the fft file to read
/// \param tracto Compressed tractogram read from file
/// \return True is the tractogram was loaded successfully, false otherwise
///

inline bool loadFft(const string filename, CompressedTractogram & tracto)
{
	fstream file;
	file.open(filename, ios_base::in | ios_base::binary);
	if (!file.is_open())
	{
		cerr << "Impossible to find file " << filename << endl;
		return false;
	}
	uint8_t version;
	file.read(reinterpret_cast<char *>(&version), sizeof(uint8_t));
	if (version != versionNumber)
	{
		cerr << "Error, version of qfib needs to be 2" << endl;
		return false;
	}
	file.read(reinterpret_cast<char *>(&tracto.nbOfFibers), sizeof(uint64_t));
	cout << tracto.nbOfFibers << " fibers found ..." << endl;
	file.read(reinterpret_cast<char *>(&tracto.nbOfPts), sizeof(uint64_t));
	cout << "Total number of points: " << tracto.nbOfPts << endl;
	unsigned nbOfBlocks;
	file.read(reinterpret_cast<char *>(&nbOfBlocks), sizeof(unsigned));
	tracto.packedCompressedFibers.resize(nbOfBlocks);
	file.read(reinterpret_cast<char *>(&tracto.nbPtsPerBlock), sizeof(unsigned));
	tracto.computePackSize();
	file.read(reinterpret_cast<char *>(&tracto.scaling), sizeof(float));
	file.read(reinterpret_cast<char *>(&tracto.offset), sizeof(glm::fvec3));
	file.read(reinterpret_cast<char *>(&tracto.ratio), sizeof(float));
	file.read(reinterpret_cast<char *>(&tracto.stepsize), sizeof(float));
	file.read(reinterpret_cast<char *>(&tracto.maxError), sizeof(float));
	cout << "Max error: " << tracto.maxError << " mm" << endl;
	file.read(reinterpret_cast<char *>(&tracto.meanError), sizeof(float));
	cout << "Average error: " << tracto.meanError << " mm" << endl;
	uint8_t nbOfSSBOs;
	file.read(reinterpret_cast<char *>(&nbOfSSBOs), sizeof (uint8_t));
	tracto.startPositionOfSSBOs.resize(nbOfSSBOs);
	for (unsigned i=0; i<nbOfSSBOs; i++)
		file.read(reinterpret_cast<char *>(&tracto.startPositionOfSSBOs[i]), sizeof (uint64_t));
	tracto.fiberBlockLut.resize(tracto.nbOfFibers);
	for (unsigned i=0; i<tracto.nbOfFibers; i++)
		file.read(reinterpret_cast<char *>(&tracto.fiberBlockLut[i]), sizeof (glm::uvec2));
	for (unsigned i = 0; i < tracto.packedCompressedFibers.size(); i++)
	{
		tracto.packedCompressedFibers[i].resize(tracto.packSize);
		file.read(reinterpret_cast<char *>(&tracto.packedCompressedFibers[i][0]), tracto.packSize * sizeof(uint8_t));
	}
	return true;
}

///
/// \brief Function to load a tck file and directly save it to the fft format without visualizing it
/// \param input Path of the tck file to read from and compress
/// \param output Path of the fft file in which to save the compressed tractogram
/// \param nb_pts_per_block Number of points per block to use for compression
/// \return 0 if it succeeds
///

inline int loadTckAndSaveFft(const string input, const string output, const unsigned nb_pts_per_block)
{
	fstream input_file, output_file;
	input_file.open(input, ios_base::in | ios_base::binary);
	if (!input_file.is_open())
	{
		cerr << "Impossible to find file " << input << endl;
		exit(EXIT_FAILURE);
	}
	output_file.open(output, ios_base::out | ios_base::binary);
	if (!output_file.is_open())
	{
		cerr << "Impossible to write file " << output << endl;
		exit(EXIT_FAILURE);
	}
	CompressedTractogram tracto;
	tracto.nbPtsPerBlock = nb_pts_per_block;
	tracto.computePackSize();
	cout << "Loading and compressing " << input << endl;
	compress(input, tracto);
	cout << "DONE" << endl << endl;
	cout << "Saving " << output << endl;
	saveFft(output, tracto);
	cout << "DONE" << endl;
	return 0;
}

///
/// \brief Function to load a tck file and save it as a ply file
/// \param input Path of the tck file to read
/// \param output Path of the ply file to save
/// \param exportAsMesh If true, exports the tractogram as a mesh (degenerated triangles), otherwise only the points are saved without connectivity
///

inline void loadTckAndSavePly(const string input, const string output, const bool exportAsMesh)
{
	fstream input_file, output_file;
	output_file.open(output, ios_base::out | ios_base::binary);
	if (!output_file.is_open())
	{
		cerr << "Impossible to write file " << output << endl;
		exit(EXIT_FAILURE);
	}
	unsigned headerLength = 0;
	uint64_t nbOfFibers = 0;
	if (!readTckHeader(input, input_file, nbOfFibers, headerLength))
		exit(EXIT_FAILURE);

	std::vector<glm::vec3> points;
	std::vector<uint32_t> indices;//No more than 2^32-1 points
	std::vector<uint32_t> begOfFibers;
	points.reserve(nbOfFibers);
	if (exportAsMesh)
	{
		indices.reserve(nbOfFibers);
		begOfFibers.reserve(nbOfFibers);
		begOfFibers.push_back(0);
	}

	input_file.seekg(headerLength, input_file.beg);
	glm::vec3 newPoint = glm::vec3(0.f,0.f,0.f);
	uint32_t i=0;
	while (!isinf(newPoint))
	{
		input_file.read(reinterpret_cast<char *>(&newPoint), sizeof (glm::vec3));
		if (isnan(newPoint) || isinf(newPoint))
		{
			if (exportAsMesh)
				begOfFibers.push_back(i);
			if (isinf(newPoint))
			{
				break;
			}
			input_file.read(reinterpret_cast<char *>(&newPoint), sizeof (glm::vec3));
			if (!isnan(newPoint) && !isinf(newPoint))
			{
				points.push_back(newPoint);
				i++;
			}
		}
		else
		{
			points.push_back(newPoint);
			i++;
		}
	}
	input_file.close();
	cout << points.size() << " points encountered" << endl;

	if (exportAsMesh)
	{
		for (unsigned i=0; i<begOfFibers.size() - 1; i++)
		{
			uint32_t startIndex = begOfFibers[i];
			for (unsigned j=0; j<begOfFibers[i + 1] - begOfFibers[i] - 1; j++)
			{
				indices.push_back(startIndex + j);
				indices.push_back(startIndex + j + 1);
				indices.push_back(startIndex + j + 1);
			}
		}
		cout << indices.size() << " indices created" << endl;
	}


	output_file << "ply" << endl;
	output_file << "format binary_little_endian 1.0" << endl;
	output_file << "element vertex " << int(points.size()) << endl;
	output_file << "property float x" << endl;
	output_file << "property float y" << endl;
	output_file << "property float z" << endl;
	if (exportAsMesh)
	{
		output_file << "element face " << int(indices.size() / 3) << endl;
		output_file << "property list uchar int vertex_index" << endl;
	}
	output_file << "end_header" << endl;
	for (unsigned int i=0; i<points.size(); i++)
		output_file.write(reinterpret_cast<const char *>(&(points[i])), sizeof (glm::vec3));
	if (exportAsMesh)
	{
		int triangleSize = 3;
		for (unsigned int i=0; i<indices.size()/3; i++)
		{
			output_file.write(reinterpret_cast<const char *>(&triangleSize), sizeof (unsigned char));
			output_file.write(reinterpret_cast<const char *>(&(indices[i*3])), sizeof (int));
			output_file.write(reinterpret_cast<const char *>(&(indices[i*3 + 1])), sizeof (int));
			output_file.write(reinterpret_cast<const char *>(&(indices[i*3 + 2])), sizeof (int));
		}
	}
	output_file.close();
}

///
/// \brief Function to load fibers from a tck file - used for uncompressed fibers drawing
/// \param filename Path of the file to load
/// \param fiberPts List of points read from the file, already in different vectors corresponding to vbos
/// \param begOfFibers List of the indices of the beginning of each fiber, correspond to the fiberPts vectors
///

inline void loadTckFibers(const string & filename, std::vector<std::vector<glm::vec4>> & fiberPts, std::vector<std::vector<uint32_t> >& begOfFibers)
{
	for (unsigned i=0; i<fiberPts.size(); i++)
		fiberPts[i].clear();
	for (unsigned i=0; i<begOfFibers.size(); i++)
		begOfFibers[i].clear();
	fiberPts.clear();
	begOfFibers.clear();
	fstream file;
	uint32_t headerLength = 0;
	uint64_t nbOfFibers = 0;
	if (!readTckHeader(filename, file, nbOfFibers, headerLength))
		exit(EXIT_FAILURE);
	file.seekg(headerLength, file.beg);
	glm::fvec3 newPoint = glm::fvec3(0.f, 0.f, 0.f);
	unsigned nbOfVBOs = 1;
	begOfFibers.resize(nbOfVBOs);
	begOfFibers[0].push_back(0);
	fiberPts.resize(nbOfVBOs);
	uint32_t i = 0;
	uint32_t j = 0;
	std::vector<glm::vec4> fiberBuffer;
	fiberBuffer.reserve(2000);
	while (!isinf(newPoint))
	{
		file.read(reinterpret_cast<char *>(&newPoint), sizeof(glm::fvec3));
		if (isnan(newPoint) || isinf(newPoint))
		{
			j++;
			if (i > 134217727) //2^31/(4*4) - 1
			{
				nbOfVBOs++;
				begOfFibers.resize(nbOfVBOs);
				begOfFibers[nbOfVBOs - 1].push_back(0);
				i = uint32_t(fiberBuffer.size());
				fiberPts.resize(nbOfVBOs);
			}
			begOfFibers[nbOfVBOs - 1].push_back(i);
			for (unsigned nbFib = 0; nbFib < fiberBuffer.size(); nbFib++)
				fiberPts[nbOfVBOs - 1].push_back(fiberBuffer[nbFib]);
			fiberBuffer.clear();

			if (isinf(newPoint))
			{
				break;
			}
			file.read(reinterpret_cast<char *>(&newPoint), sizeof(glm::fvec3));
			if (!isnan(newPoint) && !isinf(newPoint))
			{
				fiberBuffer.push_back(glm::vec4(newPoint, glm::uintBitsToFloat(j)));
				i++;
			}

		}
		else
		{
			fiberBuffer.push_back(glm::vec4(newPoint, glm::uintBitsToFloat(j)));
			i++;
		}
	}
	file.close();
}

