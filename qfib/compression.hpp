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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/packing.hpp>
#include "glm/ext.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"


#include <bitset>

#include <vector>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <chrono>
#include "saveload.hpp"
#include <thread>

///
/// \brief Function to display an advance bar when compressing
/// \param percent Current percentage for the bar
/// \param optionalText Text to display after the bar
///

inline void advanceBarCout(float percent, string optionalText = "")
{
	int nbBar = int(percent/5);
	if(percent < 100)
		std::cout << std::setfill('0') << std::setw(2) << int(percent) << " % |";
	else
	{
		std::cout << "Done |";
		return;
	}
	for(int i = 0; i < nbBar; ++i)
		std::cout << "-";
	for(int i = nbBar; i < 20; ++i)
		std::cout << " ";
	std::cout << "|  " << optionalText << "  \r";
	std::cout.flush();
}

namespace fc
{
	///
	/// \brief Function to map vectors from the half-sphere to the spherical cap parametrized by RATIO
	/// \param v Vector to map
	/// \param ratio Ratio to use
	/// \return The mapped vector
	///

	inline glm::vec3 uniformMappingAxisX(const glm::vec3 & v, const float & ratio)// from half sphere to RATIO spherical cap
	{
		//v has been normalized previously
		glm::vec3 ret;
		ret.x = float((1.0 - ratio) + v.x * ratio);// mix(1.0,v.x,ratio);
		float temp = std::sqrt((1.0000001f-ret.x*ret.x)/(1.0000001f-v.x*v.x));
		ret.y = v.y * temp;
		ret.z = v.z * temp;
		return ret;
	}

	///
	/// \brief Function to unmap vectors from the spherical cap parametrized by RATIO to the half-sphere
	/// \param v Vector to unmap
	/// \param ratio Ratio to use
	/// \return The unmapped vector
	///

	inline glm::vec3 uniformUNMappingAxisX(const glm::vec3 & v, const float & ratio)// from RATIO spherical cap to half sphere
	{
		//v has been normalized previously
		glm::vec3 ret;
		ret.x = float((v.x+ratio-1.0)/ratio); //clamp((v.x+ratio-1.0)/ratio,0.0,1.0);
		float temp = sqrt(max((1.0000001f-ret.x*ret.x)/(1.000001f-v.x*v.x), 0.f));
		ret.y = v.y * temp;
		ret.z = v.z * temp;
		return glm::vec3(glm::normalize(glm::dvec3(ret)));// fc::safeNormalize(ret);
	}

	///
	/// \brief Function to quantize a vector an a half octahedron
	/// \param dir Direction to quantize
	/// \return The quantized direction, on 8 bits
	///

	inline uint8_t quantizeHalfOctaedron(glm::vec3 dir)//assuming T = uint8
	{
		glm::vec2 v;
		dir = normalize(dir);
		if (dir.x < 0.0f)//over the half sphere pointing to X
		{
			float n1 = float(1.0 / (fabs(dir.y) + fabs(dir.z)));
			v.x = dir.y*n1;//between -1 and 1
			v.y = dir.z*n1;//between -1 and 1
		}
		else
		{
		float n1 = float(1.0 / (fabs(dir.x) + fabs(dir.y) + fabs(dir.z)));
			v.x = dir.y*n1;//between -1 and 1
			v.y = dir.z*n1;//between -1 and 1
		}

		float qsize = 16.0f;//size of quartet

		//0.0<=(1.0f+v.x+v.y)*0.5f<=1.0f
		uint8_t q = uint8_t(floorf(0.5f + (qsize-1.0f)*(1.0f+v.x+v.y)*0.5f));//between 0 and 15

		uint8_t byteVal = q;
		q = uint8_t(floorf(0.5f + (qsize-1.0f)*(1.0f+v.x-v.y)*0.5f)) & 0x0f;
		byteVal =	byteVal << 4;
		byteVal += q;
		return byteVal;
	}

	///
	/// \brief Function to unquantize a vector on a half octahedron
	/// \param byteVal Quantized direction to unquantize
	/// \return The unquantized direction
	///

	inline glm::vec3 unQuantizeHalfOctaedron(uint8_t byteVal)//assuming T = uint8
	{
		float qsize = 16.0f;//size of quartet

		float q2 = float(byteVal & 0x0f) / (qsize-1.0f)*2.0f-1.0f; //between -1.0 and 1.0
		float q1 = float(byteVal>>4) / (qsize-1.0f)*2.0f-1.0f;

		glm::vec3 v;
		v.y = (q1 + q2)*0.5f;
		v.z = (q1 - q2)*0.5f;
		v.x = 1.0f - fabs(v.y) - fabs(v.z);
		return glm::normalize(v);
	}

	///
	/// \brief Function to compress a single fiber and pack it
	/// \param fiber Uncompressed fiber to compress and pack
	/// \param tracto Compressed tractogram in which the fiber number ID will be saved compressed and packed
	/// \param id Id of the fiber inside the tractogram (is recorded in the compressed fiber)
	/// \param meanError Average error of compression (same for all fibers)
	/// \param maxError Maximum error of compression (same for all fibers)
	/// \param minDot Minimum dot product of the direction mapped with the vector (1, 0, 0), to evaluate compression performance and behaviour (should be positive or really close to 0)
	/// \param nbMinDotNeg Number of time the minimum dot product is negative (should be 0 or really small)
	///

	inline void compressFiberAndPack(const std::vector<glm::fvec3> & fiber, CompressedTractogram & tracto, const unsigned id, float & meanError, float & maxError, float & minDot, unsigned & nbMinDotNeg)
	{
		assert(fiber.size() > 1);
		bool start = true;

		const glm::vec3 X = glm::vec3(1.0, 0.0, 0.0);//local axis from which local new direction can't exceed RATIO angle
		const glm::vec3 C = glm::vec3(-0.703023156622, 0.039959276793, 0.71004344758);//glm::normalize(glm::vec3(-1.000014, 0.05684, 1.01));//custom made vector to ensure frame propagation

		glm::uvec2 blockLut = tracto.fiberBlockLut[id];
		uint64_t blockPos = blockLut.y;
		unsigned ssbo = (blockLut.x >> 16);
		if (ssbo > 0)
			blockPos += uint64_t(tracto.startPositionOfSSBOs[ssbo - 1]);

		unsigned i = 0, p = 0;
		while(i < fiber.size())
		{
			std::vector<uint8_t> &currentPack = tracto.packedCompressedFibers[blockPos];
			currentPack.resize(tracto.packSize, uint8_t(0));

			// We encode the fiberID using 4 bytes, meaning that we can have 4.2 billions fibers
			currentPack[12] = ((id >> 24) & 0xff);
			currentPack[13] = ((id >> 16) & 0xff);
			currentPack[14] = ((id >> 8) & 0xff);
			currentPack[15] = (0xff & id);

			//Block ID
			currentPack[17] = p;
			glm::fvec3 first = fiber[i];

			if(i + 1 < fiber.size())
			{
				glm::fvec3 second = fiber[i + 1];
				uint32_t a = glm::packUnorm2x16(glm::vec2(first.x, first.y));
				uint32_t b = glm::packUnorm2x16(glm::vec2(first.z, second.x));
				uint32_t c = glm::packUnorm2x16(glm::vec2(second.y, second.z));
				for (unsigned j=0; j<4; j++)
				{
					currentPack[j] = (a >> (3-j)*8) & 0xff;
					currentPack[j+4] = (b >> (3-j)*8) & 0xff;
					currentPack[j+8] = (c >> (3-j)*8) & 0xff;
				}
				i += 2;

				unsigned nbPointsPack = std::min(unsigned(tracto.nbPtsPerBlock), unsigned(fiber.size() - (tracto.nbPtsPerBlock * p)));
				//We encode the fact that this block is the first and/or the last using 1 bit each
				bool end = (nbPointsPack == unsigned(fiber.size() - (tracto.nbPtsPerBlock * p)));//It is the end if the all the remaining points are in this block
				uint8_t state = (int(!start) << 1) | (int(!end) & 0x1);
				// we encode the number of points using 6 bits
				currentPack[16] = ((state << 6) & 0xc0) | (uint8_t(nbPointsPack) & 0x3f);

				start = false;//Only the first block corresponds to the start of the fiber

				//here begins compression, quantization
				glm::vec3 initPoint[2];//recreate loss by quantization on 16 bits
				glm::vec2 temp = glm::unpackUnorm2x16(glm::packUnorm2x16(glm::vec2(fiber[i - 2].x, fiber[i - 2].y)));
				initPoint[0].x = temp.x;initPoint[0].y = temp.y;
				temp = glm::unpackUnorm2x16(glm::packUnorm2x16(glm::vec2(fiber[i - 2].z, fiber[i - 1].x)));
				initPoint[0].z = temp.x;initPoint[1].x = temp.y;
				temp = glm::unpackUnorm2x16(glm::packUnorm2x16(glm::vec2(fiber[i - 1].y, fiber[i - 1].z)));
				initPoint[1].y = temp.x;initPoint[1].z = temp.y;
				glm::vec3 axis = glm::normalize(initPoint[1] - initPoint[0]);//we need to normalize here for stability, as stepsize is not exactly the length, especially with the quantization

				for (unsigned j=0; j<2; j++)
				{
					float err = glm::distance(fiber[i - 2 + j], initPoint[j]);
					maxError = err > maxError ? err : maxError;
					meanError += err;
				}

				glm::mat3 frame;
				frame[0] = axis;//normalized
				frame[1] = glm::normalize(glm::cross(frame[0],C));
				frame[2] = glm::normalize(glm::cross(frame[0], frame[1]));//numeric precision error on norm can be neglected (to avoid cost of a normalize() in shader)

				//here frame * X = frame[0] = axis (column vector)
				// this matrix tranform words frame to the first local frame (rotation only)

				glm::fvec3 currentpoint = initPoint[1];// fiber[i - 1];

				glm::fvec3 vGlobal;//in world space
				glm::fvec3 vLocal;// such as vLocal is close to X
				unsigned off = 0;
				for(; i < tracto.nbPtsPerBlock * p + nbPointsPack; ++i )
				{
					float currentLength = glm::length(fiber[i] - currentpoint);
					float lengthRatio = currentLength / tracto.stepsize;
					if (lengthRatio < 0.85f || lengthRatio > 1.15f)//15% of margin
						cerr << "Stepsize is not constant in the tractogram, results will be incorrect  " << currentLength << " / " << tracto.stepsize << endl;
					vGlobal = (fiber[i] - currentpoint) / currentLength;//glm::normalize(fiber[i] - currentpoint); // vGlobal is the direction of the line to be compressed (in global frame)
					glm::fmat3 U = glm::transpose(frame);// this is the inverse of frame
					vLocal = glm::vec3(glm::normalize(glm::dvec3(U * vGlobal)));// fc::safeNormalize(U * vGlobal);// glm::normalize(U * vGlobal);//normalization to avoid numeric issues

					//packedFiber[p].compressedPoints[off].setCombinedValue(quantizeHalfOctaedron(fc::uniformUNMappingAxisX(vLocal, ratio)));
					currentPack[18 + off] = quantizeHalfOctaedron(fc::uniformUNMappingAxisX(vLocal, tracto.ratio));

					float dot = glm::dot(X, fc::uniformUNMappingAxisX(vLocal, tracto.ratio));
					minDot = (dot <  minDot) ? dot : minDot;
					if (dot < 0)
						nbMinDotNeg++;

					//building the updated local frame :
					glm::mat3 Lframe;//frame direction has to be propagated using a rule WITHIN THE LOCAL FRAME
					Lframe[0] = glm::normalize(fc::uniformMappingAxisX(unQuantizeHalfOctaedron(currentPack[18 + off]), tracto.ratio));//vLocal (the "new" X of the local frame)
					Lframe[1] = glm::normalize(glm::cross(Lframe[0],C));// rule using custom vector C in order to keep track of global frame rotation in the local frame
					Lframe[2] = glm::normalize(glm::cross(Lframe[0], Lframe[1]));// straight forward

					//flushing the global frame from the updated local frame
					//we previously saw that GLOBAL to LOCAL <=> left-multiplicate by inverse(frame) ( = transpose(frame))
					//so LOCAL to GLOBAL <=> left-multiplicate by frame
					frame = frame * Lframe;// updating global frame from newly built local frame
					currentpoint += (tracto.stepsize * frame[0]);//because frame[0] = new axis in Global frame
					float err = glm::distance(fiber[i], currentpoint);
					maxError = err > maxError ? err : maxError;
					meanError += err;
					off++;
				}
				//cout << endl;
				p++;

			}
			else // pack with only one point
			{
				//We compute the error introduced by the quantization of the first point
				glm::vec3 initPoint;//recreate loss by quantization on 16 bits
				glm::vec2 temp = glm::unpackUnorm2x16(glm::packUnorm2x16(glm::vec2(fiber[i].x, fiber[i].y)));
				initPoint.x = temp.x;initPoint.y = temp.y;
				temp = glm::unpackUnorm2x16(glm::packUnorm2x16(glm::vec2(fiber[i].z, 0.f)));
				initPoint.z = temp.x;
				float err = glm::distance(fiber[i], initPoint);
				maxError = err > maxError ? err : maxError;
				meanError += err;

				//The second point is built following the direction from previous block in order to be able to retreive the tangent
				glm::fvec3 second = fiber[i] + (fiber[i] - fiber[i-1]);//fiber[i-1] must exist, otherwise it means that we have fibers with a single point
				uint32_t a = glm::packUnorm2x16(glm::vec2(first.x, first.y));
				uint32_t b = glm::packUnorm2x16(glm::vec2(first.z, second.x));
				uint32_t c = glm::packUnorm2x16(glm::vec2(second.y, second.z));
				for (unsigned j=0; j<4; j++)
				{
					currentPack[j] = (a >> (3-j)*8) & 0xff;
					currentPack[j+4] = (b >> (3-j)*8) & 0xff;
					currentPack[j+8] = (c >> (3-j)*8) & 0xff;
				}

				//We encode the fact that this block is the first and/or the last using 1 bit each
				bool end = true;
				uint8_t state = (int(!start) << 1) | (int(!end) & 0x1);
				// we encode the number of points using 6 bits
				currentPack[16] = ((state << 6) & 0xc0) | (uint8_t(1) & 0x3f);
				p++;
				i++;
				break;
			}
			blockPos++;
		}
	}
}

///
/// \brief Function to load and compress a tck file - requires the ratio and the stepsize to be already computed
/// \param filename Path of the input tck file
/// \param tracto Compressed tractogram that is filled by the function - already contains the ratio and the stepsize
/// \param nbMinDotNeg Number of time the minimum dot product is negative (should be 0 or really small)
/// \return Minimum dot product of the direction mapped with the vector (1, 0, 0), to evaluate compression performance and behaviour (should be positive or really close to 0)
///

inline float loadAndCompressTckFibers(const string & filename, CompressedTractogram & tracto, unsigned & nbMinDotNeg)
{
	float minDot = 2.f;
	nbMinDotNeg = 0;

	tracto.packedCompressedFibers.clear();

	tracto.nbOfFibers = 0;
	fstream file;
	unsigned headerLength = 0;
	if (!readTckHeader(filename, file, tracto.nbOfFibers, headerLength))
		exit(EXIT_FAILURE);

	try {
		tracto.packedCompressedFibers.resize(tracto.startPositionOfSSBOs[tracto.startPositionOfSSBOs.size() - 1]);
	}
	catch (const std::length_error& le) {
		std::cerr << "Length error: " << le.what() << '\n';
	}

	file.seekg(headerLength, file.beg);
	glm::fvec3 newPoint = glm::vec3(0.f,0.f,0.f);
	std::vector<glm::fvec3> newFiber;
	unsigned id = 0;
	tracto.maxError = -1.f;
	double meanerror = 0.0;
	std::vector<std::vector<glm::fvec3>> fiberGroup;

	unsigned nbOfThreads = 10000;
	unsigned nbThreadsUsed=0;
	std::vector<std::thread> threads;
	threads.resize(nbOfThreads);
	std::vector<double> meanErrorThread(nbOfThreads, 0.0);
	std::vector<float> maxErrorThread(nbOfThreads, 0.f);
	std::vector<float> minDotThread(nbOfThreads, 2.f);
	std::vector<unsigned> nbMinDotNegThread(nbOfThreads, 0);

	unsigned bufferSize = 200;
	fiberGroup.reserve(bufferSize);
	unsigned counter = 0;
	advanceBarCout(0.f, "Compressing...");
	while (!isinf(newPoint))
	{
		file.read(reinterpret_cast<char *>(&newPoint), sizeof (glm::vec3));
		if (isnan(newPoint) || isinf(newPoint))
		{
			fiberGroup.push_back(newFiber);
			counter++;
			if (counter >= bufferSize)
			{
				threads[nbThreadsUsed] = std::thread([=, &tracto, &meanErrorThread, &maxErrorThread, &minDotThread, &nbMinDotNegThread]()
				{
					for (unsigned i = 0; i < counter; i++)
					{
						float tmpmean = 0.f;
						fc::compressFiberAndPack(fiberGroup[i], tracto, id + i, tmpmean, maxErrorThread[nbThreadsUsed], minDotThread[nbThreadsUsed], nbMinDotNegThread[nbThreadsUsed]);
						meanErrorThread[nbThreadsUsed] += double(tmpmean)/double(tracto.nbOfPts);
					}
				});
				for (unsigned i=0; i<fiberGroup.size(); i++)
					fiberGroup[i].clear();
				fiberGroup.clear();
				id += counter;
				advanceBarCout(id / float(tracto.nbOfFibers) * 100.f, "Compressing...");
				counter = 0;
				nbThreadsUsed++;
				if (nbThreadsUsed >= nbOfThreads)
				{
					//Wait for all threads to finish
					for (std::thread &t: threads)
						if (t.joinable())
							t.join();
					nbThreadsUsed = 0;
				}
			}
			newFiber.clear();
			if (isinf(newPoint))
			{
				break;
			}
			file.read(reinterpret_cast<char *>(&newPoint), sizeof (glm::vec3));
			if (!isnan(newPoint) && !isinf(newPoint))
			{
				newFiber.push_back((newPoint + tracto.offset) * 1.f/tracto.scaling + glm::vec3(0.5f));
			}
		}
		else
		{
			newFiber.push_back((newPoint + tracto.offset) * 1.f/tracto.scaling + glm::vec3(0.5f));
		}
	}
	for (unsigned i = 0; i < counter; i++)
	{
		float tmpmean = 0.f;
		fc::compressFiberAndPack(fiberGroup[i], tracto, id + i, tmpmean, maxErrorThread[nbThreadsUsed], minDotThread[nbThreadsUsed], nbMinDotNegThread[nbThreadsUsed]);
		meanErrorThread[nbThreadsUsed] += double(tmpmean)/double(tracto.nbOfPts);
	}
	fiberGroup.clear();
	//Wait for all threads to finish
	for (std::thread &t: threads)
		if (t.joinable())
			t.join();
	for (unsigned i=0; i<nbOfThreads; i++)
	{
		minDot = std::min(minDot, minDotThread[i]);
		nbMinDotNeg += nbMinDotNegThread[i];
		tracto.maxError = std::max(tracto.maxError, maxErrorThread[i]);
		meanerror += meanErrorThread[i];
	}

	//Errors are recorded in mm
	tracto.maxError *= tracto.scaling;
	tracto.meanError = float(meanerror * tracto.scaling);
	std::cout << "\nMax error: " << tracto.maxError << " mm" << std::endl;
	std::cout << "Average error: " << tracto.meanError << " mm" << std::endl;
	file.close();

	return minDot;
}

///
/// \brief Function to compress a tractogram
/// \param inputFileName Path of the input tractogram
/// \param tracto Tractogram compressed
///

inline void compress(const std::string & inputFileName, CompressedTractogram & tracto)
{
	std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();
	std::cout << "Quantization precision: 8 bits." << std::endl;


	std::cout << "Loading file " << inputFileName << ", computing ratio and normalization ... " << std::endl;
	loadTckFibersNormalizeAndComputeRatio(inputFileName, tracto);

	std::cout << "Compressing fibers ..." << std::endl;
	std::cout << "ratio : " << tracto.ratio << std::endl;

	unsigned nbMinDotNeg = 0;
	float minDot = loadAndCompressTckFibers(inputFileName, tracto, nbMinDotNeg);
	cout << "Minimum dot product: " << minDot << "   (Must be positive or really close to 0)" << endl;//Must be positive, but as close as 0 as possible
	if (minDot < 0.f)
		cout << "Minimum dot product was negative " << nbMinDotNeg << " times (This value should be really low compared to the number of fibers)" << endl;
	std::cout << "Compression time was: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << " sec" << std::endl;
}
