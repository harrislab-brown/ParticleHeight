#include <opencv2/highgui.hpp>
#include <iostream>
#include "ParticleFinder.h"

// window names
std::string sViewerWindowName = "ParticleHeight";
std::string sTrackbarWindowName = "Image Processing Parameters";

// viewer parameters
int nCurrentFrame = 0;
unsigned char nZoomLevel = 1;
std::vector<cv::Mat> vecVideoFrames;  // vector to store video frames
cv::Size sizeVideo;  // width and height of the video

// image processor and particle finder
ph::Settings* pSettings;
ph::ImageProcessor* imProcessor;
ph::ParticleFinder* pFinder;

// setup viewer controls
bool bSubtractBackground = false;
bool bMorphOpen = false;
bool bMorphClose = false;
bool bFindCircles = false;
bool bFindHeight = false;
bool bTransformWithRay = false;
bool bUseHoughTransform = false;

void usage()
{
	// print the options for using the application
	std::cerr << "USAGE: ParticleHeight {-h|-s[-r][n]|-c|-p[-r]} videoFile [refVideoFile] [settingsFile] [outCSV] [outVideo]" << std::endl;
	std::cerr << "                                                                                " << std::endl;
	std::cerr << " -h | -help          print this help" << std::endl;
	std::cerr << " -s | -setup         interactively configure the video processing settings" << std::endl;
	std::cerr << "  n                   number of frames to load during setup (default 10)" << std::endl;
	std::cerr << " -c | -calibrate     calibrate optical parameters using list of known particle heights" << std::endl;
	std::cerr << " -p | -process       process a video or batch of videos" << std::endl;
	std::cerr << " -r | -ref           reference image is provided in separate file" << std::endl;
	std::cerr << "                                                                                " << std::endl;
	std::cerr << " videoFile            8-bit single channel AVI file, first frame can be ref image" << std::endl;
	std::cerr << "                          in processing mode, this can be a directory containing all videos to be processed" << std::endl;
	std::cerr << " refVideoFile         AVI file named \"ref\" containing a single frame of the ref image, necessary if videoFile doesn't contain it" << std::endl;
	std::cerr << " settingsFile         txt file from which processing settings are read, setup mode will write here" << std::endl;
	std::cerr << " outCSV               stores the results from processing a video" << std::endl;
	std::cerr << " outVideo             stores a video illustrating the particle finding process for debugging" << std::endl;
	exit(0);
}

void error(const char* msg)
{
	std::cerr << "ERROR: " << ((msg) ? msg : "") << std::endl;
	exit(0);
}

std::string getExt(const char* filename)
{
	if (filename)
	{
		std::string f(filename);
		int i, n = (int)f.size();
		for (i = n - 1; i >= 0; i--)
			if (filename[i] == '.')
				break;
		if (i >= 0)
		{
			std::string ext(filename + i + 1);
			return ext;
		}
		else
			return "";
	}
	else
		return "";
}

void updateFrame()
{
	cv::Mat matShowFrame = vecVideoFrames[nCurrentFrame].clone();  // get a copy of the current frame

	if (bSubtractBackground) imProcessor->subtractBackground(matShowFrame);
	if (bMorphClose) imProcessor->morphClose(matShowFrame);
	if (bMorphOpen) imProcessor->morphOpen(matShowFrame);
	if (bFindCircles)
	{
		std::vector<cv::Vec3f> vecCircles;
		if (bUseHoughTransform)
			vecCircles = imProcessor->findCirclesHough(matShowFrame);
		else
			vecCircles = imProcessor->findCirclesEDT(matShowFrame);
		cv::cvtColor(matShowFrame, matShowFrame, cv::COLOR_GRAY2BGR);  // convert frame to RGB

		// draw the circles
		for (auto c : vecCircles)
		{
			cv::Point center(c[0], c[1]);
			cv::circle(matShowFrame, center, 5, cv::Scalar(0, 255, 0), -1);  // circle center
			cv::circle(matShowFrame, center, c[2], cv::Scalar(0, 255, 0), 3, cv::LINE_AA);  // circle outline
		}

		// indicate the circle finding method used
		if (bUseHoughTransform)
			cv::putText(matShowFrame, "Hough", cv::Point(3, 12), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 200), 1, cv::LINE_AA);
		else
			cv::putText(matShowFrame, "EDT", cv::Point(3, 12), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 200), 1, cv::LINE_AA);
	}
	if (bFindHeight)
	{
		std::list<ph::Particle> listParticles = pFinder->findParticles(matShowFrame);
		cv::cvtColor(matShowFrame, matShowFrame, cv::COLOR_GRAY2BGR);  // convert frame to RGB

		// draw the outlines of the particles: green if solitary, red if overlapping
		// also draw the transformed reference image on the particles
		for (auto& p : listParticles)
		{
			float xPos, yPos;
			p.getPositionPx(xPos, yPos);
			float r = p.getRadiusPx();
			cv::Point center(xPos, yPos);
			cv::Scalar color = p.hasNeighbors() ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
			cv::circle(matShowFrame, center, r, color, 2, cv::LINE_AA);  // circle outline

			// create region to transform
			cv::Rect rectRegion((int)xPos - (pSettings->nDICRegionSize >> 1), (int)yPos - (pSettings->nDICRegionSize >> 1), pSettings->nDICRegionSize, pSettings->nDICRegionSize);
			cv::Mat matTransformed;

			if (!p.hasNeighbors())
			{
				if (bTransformWithRay)
				{
					// for testing, use ray tracing to transform the single particles
					// construct the optical scene
					ph::OpticalScene scene(pSettings->fEtaLiquid);
					scene.addMedium(std::make_shared<ph::OpticalPattern>(ph::vf3(0, 0, 0), ph::vf3(0, 0, 1)));  // pattern
					scene.addMedium(std::make_shared<ph::OpticalLayer>(ph::vf3(0, 0, 0), ph::vf3(0, 0, pSettings->fChannelWallThickness), ph::vf3(0, 0, 1), pSettings->fEtaGlass));  // bottom channel wall
					scene.addMedium(std::make_shared<ph::OpticalSphere>(p.getPositionReal(), p.getRadiusReal(), pSettings->fEtaParticle));  // particle

					// apply the transformation using the image processor object
					matTransformed = imProcessor->transformRef(rectRegion, ph::TransformMultiple(&p, pSettings, &scene));

					cv::putText(matShowFrame, "Ray", cv::Point(3, 12), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 200), 1, cv::LINE_AA);
				}
				else
				{
					// use analytical model to transform the image
					matTransformed = imProcessor->transformRef(rectRegion, ph::TransformSingle(&p, pSettings));

					cv::putText(matShowFrame, "Analytical", cv::Point(3, 12), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 200), 1, cv::LINE_AA);
				}
			}
			else
			{
				// get the neighboring particles
				std::vector<ph::Particle*> vecN = p.getNeighbors();

				// construct the optical scene
				ph::OpticalScene scene(pSettings->fEtaLiquid);
				scene.addMedium(std::make_shared<ph::OpticalPattern>(ph::vf3(0, 0, 0), ph::vf3(0, 0, 1)));  // pattern
				scene.addMedium(std::make_shared<ph::OpticalLayer>(ph::vf3(0, 0, 0), ph::vf3(0, 0, pSettings->fChannelWallThickness), ph::vf3(0, 0, 1), pSettings->fEtaGlass));  // bottom channel wall
				scene.addMedium(std::make_shared<ph::OpticalSphere>(p.getPositionReal(), p.getRadiusReal(), pSettings->fEtaParticle));  // particle
				for (auto n : vecN)
					scene.addMedium(std::make_shared<ph::OpticalSphere>(n->getPositionReal(), n->getRadiusReal(), pSettings->fEtaParticle));  // add each neighbor

				// apply the transformation using the image processor object
				matTransformed = imProcessor->transformRef(rectRegion, ph::TransformMultiple(&p, pSettings, &scene));
			}

			// draw the transformed region over the current frame
			cv::cvtColor(matTransformed, matTransformed, cv::COLOR_GRAY2BGR);  // convert to color
			cv::applyColorMap(matTransformed, matTransformed, cv::COLORMAP_PARULA);  // apply colormap
			matTransformed.copyTo(matShowFrame(rectRegion));  // overlay on the main particle image
		}

		// second loop to draw numbers on the particles
		for (auto& p : listParticles)
		{
			float xPos, yPos;
			p.getPositionPx(xPos, yPos);
			cv::Rect rectRegion((int)xPos - (pSettings->nDICRegionSize >> 1), (int)yPos - (pSettings->nDICRegionSize >> 1), pSettings->nDICRegionSize, pSettings->nDICRegionSize);
			// print correlation coefficient above DIC region
			cv::putText(matShowFrame, std::to_string(p.getConfidence()), cv::Point(rectRegion.x, rectRegion.y - 4), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
			// print the particle height below the DIC region
			cv::putText(matShowFrame, std::to_string(p.getPositionReal().z), cv::Point(rectRegion.x, rectRegion.y + rectRegion.height + 14), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
		}
	}

	cv::resizeWindow(sViewerWindowName, nZoomLevel * sizeVideo.width / 3, nZoomLevel * sizeVideo.height / 3);
	cv::imshow(sViewerWindowName, matShowFrame);  // display the frame
}

void onTrackbarUpdate(int value, void* data)
{
	updateFrame();
}

void configureSettings(std::string& sVideoIn, std::string& sRefVid, int nSetupFrames, std::string& sSettings)
{
	cv::VideoCapture cap(sVideoIn);  // create video capture object
	if (!cap.isOpened()) error("unable to open video");

	// read the ref image
	// read the ref image
	cv::Mat matFrame;
	if (sRefVid == "")
	{
		// separate ref video not provided so the ref image should be the first frame
		cap >> matFrame;
	}
	else
	{
		// get the reference image from the provided reference video
		cv::VideoCapture capRef(sRefVid);
		if (!cap.isOpened()) error("unable to open reference video");
		capRef >> matFrame;
		capRef.release();  // clean up the capture object
	}
	cv::cvtColor(matFrame, matFrame, cv::COLOR_BGR2GRAY);  // convert to grayscale
	vecVideoFrames.push_back(matFrame);  // store reference frame
	sizeVideo = vecVideoFrames[0].size();  // get size of the reference frame

	// create the image processor and particle finder objects
	pSettings = new ph::Settings();
	if (sSettings != "")
	{
		pSettings->setFile(sSettings.c_str());
		int l = pSettings->load();
		if (l < 0)
			std::cout << "failed to open settings file, using defaults" << std::endl;
		else
			std::cout << "opened settings file, loaded " << l << " setting values" << std::endl;
	}

	// create objects and pass settings
	imProcessor = new ph::ImageProcessor(vecVideoFrames[0], pSettings);
	pFinder = new ph::ParticleFinder(imProcessor, pSettings);
	ph::Particle::setSettings(pSettings);

	// read the rest of the frames
	for (int n = 1; n < nSetupFrames; n++)
	{
		cap >> matFrame;  // store the next available frame
		if (matFrame.empty()) break;  // check for video end

		cv::cvtColor(matFrame, matFrame, cv::COLOR_BGR2GRAY);  // convert to grayscale
		vecVideoFrames.push_back(matFrame);  // push the new frame

		// align the frame to the ref image
		std::cout << "aligned frame " << n << " with correlation coefficient " << imProcessor->alignToRef(matFrame) << "\r";
	}
	std::cout << std::endl << "opened and aligned all " << vecVideoFrames.size() << " frames" << std::endl;
	cap.release();  // release the video capture object

	// create windows
	cv::namedWindow(sViewerWindowName, cv::WINDOW_NORMAL);
	cv::namedWindow(sTrackbarWindowName, cv::WINDOW_AUTOSIZE);
	cv::resizeWindow(sTrackbarWindowName, 800, 700);
	cv::createTrackbar("BackgroundThreshold", sTrackbarWindowName, &(pSettings->nBackgroundThreshold), 300, onTrackbarUpdate);
	cv::createTrackbar("CloseSize", sTrackbarWindowName, &(pSettings->nCloseSize), 20, onTrackbarUpdate);
	cv::createTrackbar("CloseIter", sTrackbarWindowName, &(pSettings->nCloseIter), 20, onTrackbarUpdate);
	cv::createTrackbar("OpenSize", sTrackbarWindowName, &(pSettings->nOpenSize), 20, onTrackbarUpdate);
	cv::createTrackbar("OpenIter", sTrackbarWindowName, &(pSettings->nOpenIter), 20, onTrackbarUpdate);
	cv::createTrackbar("CircleSize", sTrackbarWindowName, &(pSettings->nCircleSize), 100, onTrackbarUpdate);
	cv::createTrackbar("CircleSizeRange", sTrackbarWindowName, &(pSettings->nCircleSizeRange), 50, onTrackbarUpdate);
	cv::createTrackbar("CircleThreshold", sTrackbarWindowName, &(pSettings->nCircleThreshold), 20, onTrackbarUpdate);
	cv::createTrackbar("CircleMinDist", sTrackbarWindowName, &(pSettings->nCircleMinDist), 200, onTrackbarUpdate);
	cv::createTrackbar("CircleIntensity", sTrackbarWindowName, &(pSettings->nCircleIntensity), 255, onTrackbarUpdate);
	cv::createTrackbar("HMaxParam", sTrackbarWindowName, &(pSettings->nHMaxParam), 1000, onTrackbarUpdate);
	cv::createTrackbar("OverlapPenalty", sTrackbarWindowName, &(pSettings->nOverlapPenalty), 1000, onTrackbarUpdate);

	// view the loaded video
	nCurrentFrame = 0;
	while (true)
	{
		updateFrame(); // update the frame
		char cKeyPressed = (char)cv::waitKey(0); // handle user input
		if (cKeyPressed == 'q') break;  // quit

		switch (cKeyPressed)
		{
		case 'd':  // frame right
			nCurrentFrame = nCurrentFrame == vecVideoFrames.size() - 1 ? 0 : ++nCurrentFrame;
			break;
		case 'a':  // frame left
			nCurrentFrame = nCurrentFrame == 0 ? vecVideoFrames.size() - 1 : --nCurrentFrame;
			break;
		case 's':  // decrease window size
			nZoomLevel = nZoomLevel == 1 ? 1 : --nZoomLevel;
			break;
		case 'w':  // increase window size
			nZoomLevel++;
			break;
		case 'b':  // subtract background
		{
			bFindHeight = false;
			bSubtractBackground = !bSubtractBackground;
			break;
		}
		case 'n':  // filter noise
		{
			bFindHeight = false;
			bMorphOpen = !bMorphOpen;
			break;
		}
		case 'm':  // morphological operations
		{
			bFindHeight = false;
			bMorphClose = !bMorphClose;
			break;
		}
		case 'c':  // find circles
		{
			bFindHeight = false;
			bFindCircles = !bFindCircles;
			break;
		}
		case 'f':  // find particle heights
		{
			bFindHeight = !bFindHeight;
			bSubtractBackground = false;
			bMorphClose = false;
			bMorphOpen = false;
			bFindCircles = false;
			break;
		}
		case 'r':  // transform with ray or analytical model
			bTransformWithRay = !bTransformWithRay;
			break;
		case 'h':  // use Hough or EDT to find circles
			bUseHoughTransform = !bUseHoughTransform;
			break;
		}
	}

	// save the settings to file
	if (sSettings != "")
	{
		std::cout << "save the current settings? (y/n): ";
		char c;
		std::cin >> c;
		if (c == 'y')
			if (pSettings->save())
				std::cout << "settings saved" << std::endl;
			else
				std::cout << "failed to save settings" << std::endl;
	}

	// delete objects
	delete pFinder;
	delete imProcessor;

	// close the opencv window
	cv::destroyAllWindows();
}

void processVideo(std::string& sVideoIn, std::string& sRefVid, std::string& sVideoOut, std::string& sOutput, std::string& sSettings)
{
	cv::VideoCapture cap(sVideoIn);  // create video capture object
	if (!cap.isOpened()) error("unable to open video");

	// read the ref image
	cv::Mat matRef;
	if (sRefVid == "")
	{
		// separate ref video not provided so the ref image should be the first frame
		cap >> matRef;
	}
	else
	{
		// get the reference image from the provided reference video
		cv::VideoCapture capRef(sRefVid);
		if (!cap.isOpened()) error("unable to open reference video");
		capRef >> matRef;
		capRef.release();  // clean up the capture object
	}
	cv::cvtColor(matRef, matRef, cv::COLOR_BGR2GRAY);  // convert ref image to grayscale

	// create the image processor and particle finder objects
	ph::Settings settings;
	if (sSettings != "")
	{
		settings.setFile(sSettings.c_str());
		int l = settings.load();
		if (l < 0)
			std::cout << "failed to open settings file, using defaults" << std::endl;
		else
			std::cout << "opened settings file, loaded " << l << " setting values" << std::endl;
	}

	// create objects and pass settings
	ph::ImageProcessor imProcessor(matRef, &settings);
	ph::ParticleFinder pFinder(&imProcessor, &settings);
	ph::Particle::setSettings(&settings);

	// open file to save results
	std::ofstream outputFile;
	bool bWriteCSV = false;
	if (sOutput != "")
	{
		outputFile.open(sOutput);
		if (outputFile.is_open())
		{
			bWriteCSV = true;
			outputFile << "frame,x,y,z,confidence\n";
		}
		else
			error("could not open output file");
	}
	
	// get the video writer ready
	cv::VideoWriter writer;
	bool bWriteVideo = false;
	if (sVideoOut != "")
	{
		int codec = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
		double fps = 30.0;
		writer.open(sVideoOut, codec, fps, matRef.size());

		if (writer.isOpened())
			bWriteVideo = true;
		else
			error("unable to open output video writer");
	}

	// read the rest of the frames
	int n = 1;
	while (true)
	{
		cv::Mat matFrame;
		cap >> matFrame;  // store the next available frame
		if (matFrame.empty()) break;  // check for video end

		cv::cvtColor(matFrame, matFrame, cv::COLOR_BGR2GRAY);  // convert to grayscale

		std::cout << "processing frame " << n << std::endl;

		// align the frame to the ref image
		std::cout << "aligned frame with correlation coefficient " << imProcessor.alignToRef(matFrame) << std::endl;

		// find the particles
		if (bWriteCSV)
		{
			std::list<ph::Particle> listParticles = pFinder.findParticles(matFrame);

			// write to csv file
			// convert to the coordinate system in the paper
			for (auto& p : listParticles)
				outputFile << n << ","
				<< p.getPositionReal().y << ","
				<< p.getPositionReal().z - settings.fChannelWallThickness << ","
				<< p.getPositionReal().x << ","
				<< p.getConfidence() << "\n";
		}

		// write the processed frame
		if (bWriteVideo)
		{
			// save the frame as a binary image of the particles from which concentration profiles or spatiotemporal plots can be made
			imProcessor.subtractBackground(matFrame);
			imProcessor.morphClose(matFrame);
			imProcessor.morphOpen(matFrame);
			cv::cvtColor(matFrame, matFrame, cv::COLOR_GRAY2RGB);
			writer.write(matFrame);
		}

		n++;
	}

	std::cout << "finished processing video" << std::endl;
	outputFile.close();  // close the output file
	cap.release();  // release the video capture object
}

struct calibrateData
{
	ph::Settings* settings;
	std::vector<cv::Mat> vecFrames;
	std::vector<float> vecHeight;
};

double calculateHeightResiduals(unsigned n, const double* param, double* grad, void* data)
{
	// param -- eta values, channel thickness
	// data -- vector of heights, vector of video frames, pointer to settings object

	calibrateData* d = (calibrateData*)data;
	ph::Settings* s = d->settings;

	// update the settings with the passed parameters
	s->fEtaGlass = param[0];
	s->fEtaLiquid = param[1];
	s->fEtaParticle = param[2];
	s->fChannelWallThickness = param[3];

	int nHeights = d->vecHeight.size();
	int nTrials = d->vecFrames.size() / (nHeights + 1);

	double ssr = 0;
	for (int j = 0; j < nTrials; j++)
	{
		// create the particle finder
		ph::ImageProcessor imProcessor(d->vecFrames[(nHeights + 1) * j], s);
		ph::ParticleFinder pFinder(&imProcessor, s, false);
		for (int i = 0; i < nHeights; i++)
		{
			// find the particles
			std::list<ph::Particle> listParticles = pFinder.findParticles(d->vecFrames[(nHeights + 1) * j + i + 1], true);
			ssr += ((double)listParticles.front().getPositionReal().z - d->vecHeight[i]) * ((double)listParticles.front().getPositionReal().z - d->vecHeight[i]);
		}
	}

	std::cout << "objective function called: eta_g = " << param[0]
		<< ", eta_l = " << param[1] << ", eta_p = " << param[2]
		<< ", wall_t = " << param[3] << "  |  SSR = " << ssr << std::endl;

	return ssr;
}

void calibrate(std::string& sVideoIn, std::vector<float> vecHeight, std::string& sSettings)
{
	// calibration video should have n * (len(vecHeight) + 1) frames where n is the integral number of calibration trials. 
	// each (n + 1)th frame should be a reference image and the subsequent frames should be the particle images corresponding
	// to each element in vecHeight

	// load in the settings
	ph::Settings settings;
	if (sSettings != "")
	{
		settings.setFile(sSettings.c_str());
		int l = settings.load();
		if (l < 0)
			std::cout << "failed to open settings file, using defaults" << std::endl;
		else
			std::cout << "opened settings file, loaded " << l << " setting values" << std::endl;
	}
	ph::Particle::setSettings(&settings);

	// load in the video frames
	cv::VideoCapture cap(sVideoIn);  // create video capture object
	if (!cap.isOpened()) error("unable to open video");
	std::vector<cv::Mat> vecFrames;
	ph::ImageProcessor imProcessor;
	imProcessor.setSettings(&settings);
	int n = 0;
	while (true)
	{
		cv::Mat matFrame;
		cap >> matFrame;  // store the next available frame
		if (matFrame.empty()) break;  // check for video end

		cv::cvtColor(matFrame, matFrame, cv::COLOR_BGR2GRAY);  // convert to grayscale

		// check if the frame is a reference image or not
		if (n % (vecHeight.size() + 1) == 0)
		{
			std::cout << "frame " << n << " is a reference image" << std::endl;
			imProcessor.setRef(matFrame);
		}
		else
		{
			// align the frame to the ref image
			std::cout << "aligned frame " << n << " with correlation coefficient " << imProcessor.alignToRef(matFrame) << std::endl;
		}

		vecFrames.push_back(matFrame);
		n++;
	}

	// create the data object for the optimizer
	calibrateData data = { &settings, vecFrames, vecHeight };

	// set up NLopt optimizer
	nlopt::opt optimizer(nlopt::algorithm::LN_NELDERMEAD, 4);
	optimizer.set_min_objective(calculateHeightResiduals, &data);
	std::vector<double> vecInitialStep = {0.03, 0.03, 0.03, 0.03};
	optimizer.set_initial_step(vecInitialStep);
	optimizer.set_ftol_abs(0.0001); // TODO: find a good convergence condition here

	// get the initial guess for the parameters from the loaded settings
	std::vector<double> param = {settings.fEtaGlass, settings.fEtaLiquid, settings.fEtaParticle, settings.fChannelWallThickness};

	// perform the calibration
	try
	{
		// perform optimization
		double ssr;
		optimizer.optimize(param /* initial guess */, ssr /* final f value */);

		std::cout << "calibration completed with SSR " << ssr << "\n"
			<< "eta glass: " << param[0] << "\n"
			<< "eta liquid: " << param[1] << "\n"
			<< "eta particle: " << param[2] << "\n"
			<< "wall thickness: " << param[3] << std::endl;

		// update the settings and save
		settings.fEtaGlass = param[0];
		settings.fEtaLiquid = param[1];
		settings.fEtaParticle = param[2];
		settings.fChannelWallThickness = param[3];
		if (sSettings != "")
		{
			std::cout << "save the current settings? (y/n): ";
			char c;
			std::cin >> c;
			if (c == 'y')
				if (settings.save())
					std::cout << "settings saved" << std::endl;
				else
					std::cout << "failed to save settings" << std::endl;
		}
	}
	catch (std::exception& e)
	{
		std::cout << "NLopt failed: " << e.what() << std::endl;
	}
}

int main(int argc, char** argv)
{
	// parse command line input
	if (argc < 3) usage();
	
	enum Mode {PROCESS, CALIBRATE, SETUP} mode;
	if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "-help") usage();
	else if (std::string(argv[1]) == "-p" || std::string(argv[1]) == "-process") mode = PROCESS;
	else if (std::string(argv[1]) == "-c" || std::string(argv[1]) == "-calibrate") mode = CALIBRATE;
	else if (std::string(argv[1]) == "-s" || std::string(argv[1]) == "-setup") mode = SETUP;
	else error("unrecognized mode flag");

	std::string sVideoInPath = "";
	std::string sRefVid = "";
	std::string sSettingsPath = "";
	std::string sOutPath = "";
	std::string sVideoOutPath = "";
	bool bRefVid = false;
	int nSetupFrames = 10;
	float fKnownHeight;
	std::vector<float> vecKnownHeights;

	for (int i = 2; i < argc; i++)
	{
		char* arg = argv[i];
		if (mode == SETUP && sscanf_s(arg, "%d", &nSetupFrames) == 1) { /* number of frames to load for setup */ }
		else if (mode == CALIBRATE && sscanf_s(arg, "%f", &fKnownHeight) == 1)
			vecKnownHeights.push_back(fKnownHeight);
		else if ((mode == SETUP || mode == PROCESS) && (std::string(arg) == "-r" || std::string(arg) == "-ref"))
			bRefVid = true;
		else if (sVideoInPath == "" && getExt(arg) == "avi")
			sVideoInPath = std::string(arg);
		else if (sRefVid == "" && getExt(arg) == "avi" && bRefVid)
			sRefVid = std::string(arg);
		else if (sSettingsPath == "" && getExt(arg) == "txt")
			sSettingsPath = std::string(arg);
		else if (sOutPath == "" && getExt(arg) == "csv")
			sOutPath = std::string(arg);
		else if (sVideoInPath != "" && getExt(arg) == "avi")
			sVideoOutPath = std::string(arg);
		else
			error("unrecognized argument");
	}

	if (sVideoInPath == "") error("no video file");
	if (bRefVid && sRefVid == "") error("no reference video");

	switch (mode)
	{
	case PROCESS:
	{
		// process all frames of the video
		if (sOutPath == "" && sVideoOutPath == "") error("output file path required in process mode");
		processVideo(sVideoInPath, sRefVid, sVideoOutPath, sOutPath, sSettingsPath);
		break;
	}
	case CALIBRATE:
	{
		// calibrate optical parameters with known height particle video
		calibrate(sVideoInPath, vecKnownHeights, sSettingsPath);
		break;
	}
	case SETUP:
	{
		// interactively configure the image processing settings
		configureSettings(sVideoInPath, sRefVid, nSetupFrames, sSettingsPath);
		break;
	}
	}

	return 0;
}