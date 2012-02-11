



//CAM ONLY VERSION






#include "WPILib.h"
#include "Vision/RGBImage.h"
#include "Vision/BinaryImage.h"
#include "cmath"

#define CAMERAHEIGHT 80
#define ANGLEOFLAUNCH 45
#define angle 54
#define pi 3.14159265358979323846264
static AxisCamera &camera = AxisCamera::GetInstance("10.16.71.11");

DriverStationLCD* driverOut = DriverStationLCD::GetInstance();

class DoctaEight : public SimpleRobot
{	

	Joystick copilot;
	BinaryImage *thresholdImage;
	BinaryImage *bigObjectsImage;
	BinaryImage *convexHullImage;
	BinaryImage *filteredImage;
	vector<ParticleAnalysisReport> *particles;

	signed char distanceTarget;
	//negate for turning drive, choice target target selected, distance target for getting distance, itt for itterations
	
	double firstTarget, secondTarget, thirdTarget;
	//targets (fourth is not one two or three XD), decriment to slow motors as aiming

	bool limitedDistance, cycle, flag, aimin, shootin;
	//switch between primary and secondary distance tracking based on number of targets
	struct CamDataStruct//group for camera data use UpdateCamData before using these
	{
		//char choiceTarget;
		int numTargets;
		double toCenter;
		double centerY;
		double centerX;
		double firstY;
		double secondY;
		double thirdY;
		double fourthY;
		double area;
		double height;
		double distanceYnorm;
	}CamData;
public:
	DoctaEight::DoctaEight(void):
	copilot(2)
	//controller(USB port)
	//encoders(AChannel, BChannel)
	{
		GetWatchdog().Kill();
		
		driverOut->UpdateLCD();
		//camera.WriteMaxFPS(30);
		//camera.WriteBrightness(30);
		//camera.WriteWhiteBalance(AxisCamera::kWhiteBalance_Hold);
		//camera.WriteResolution(AxisCamera::kResolution_640x360);
		//camera.WriteColorLevel(100);
		//camera.WriteCompression(30);
		//lower easier on CRIO and harder on cam
		limitedDistance = 0;
	}

	
	void DoctaEight::Autonomous(void)
	{
		GetWatchdog().Kill();
		while (IsAutonomous() && IsEnabled())
		{
			output();
		}
	}
	void DoctaEight::OperatorControl(void)
	{
		GetWatchdog().Kill();
		//output();
	}
	
	void targetSelect(int &choiceTarget)//pick the target to shoot at (highest visible) then the one to distance with
	{
		GetWatchdog().Kill();
		UpdateCamData();
				
		if (3 == CamData.numTargets or 4 == CamData.numTargets)//if 3 or 4 targets visible
		{
			//choose target to aim/shoot at -> highest target
			if (CamData.firstY > CamData.secondY && CamData.firstY > CamData.thirdY)
				choiceTarget = 1;
			else if (CamData.secondY > CamData.thirdY && CamData.secondY > CamData.firstY)
				choiceTarget = 2;
			else if (CamData.thirdY > CamData.secondY && CamData.thirdY > CamData.firstY)
				choiceTarget = 3;
			else {choiceTarget = 4;}

			//find lowest target for distance tracking purposes
			if (CamData.firstY < CamData.secondY && CamData.firstY < CamData.thirdY)
				distanceTarget = 1;
			else if (CamData.secondY < CamData.thirdY && CamData.secondY < CamData.firstY)
				distanceTarget = 2;
			else if (CamData.thirdY < CamData.secondY && CamData.thirdY < CamData.firstY)
				distanceTarget = 3;
			else 
				distanceTarget = 4;

			limitedDistance = 0;//switch between high and low accuracy distance tracking (high requires 3 or 4 particles)
		}
		else if (2 == CamData.numTargets)//if 2 targets visible
		{

			//select target to aim/shoot at
			if (CamData.firstY > CamData.secondY)
				choiceTarget = 1;
			else
				choiceTarget = 2;

			limitedDistance = 1;//use single target target tracking
		}
		else if (1 == CamData.numTargets)
		{
			//aim/shoot at only target
			choiceTarget = 1;
			limitedDistance = 1;
		}
		else
			choiceTarget = -1;
	}

	double fOfX(double x)//part of accurate distance finding in getDistance()
	{
		GetWatchdog().Kill();
		return 	(atan((107.5-CAMERAHEIGHT)/(x)) //angle from top triangle (triangle formed by camera target and line at camera's height)
				+atan((CAMERAHEIGHT-31.5)/(x))); //angle from bottom triangle
	}
	
	void getDistance(double &aproximation)//find distance
	{
		GetWatchdog().Kill();
		
		UpdateCamData();
		
		int choiceTarget;
		targetSelect(choiceTarget);
		
		
		if (choiceTarget !=-1 && CamData.numTargets < 5)//if there is a target
		{
			if (limitedDistance == 1)//if must use less accurate tracking
			{
				aproximation = 9//half height of target in inches over target to get adjacent
								/tan(//tan of this to get opposite over adjacent
										angle*
											((CamData.area/24)//to get height in pixels
												/CamData.height)//above divided to get ratio of size
													/2);//to get half of angle and therefore right triangle
			}
			else//better tracking
			{
				
				double theta=(CamData.centerY-CamData.distanceYnorm)/240*angle;//the distance is turned into an angle (refer to fofx(x))
				//107.5 is top target height 31.5 is bottom target height
				
				double dotbinary=54;
				for(int i=0; i<30; i++)//binary approximation-> guesses using 1/2 distances until
											//tlar(that looks about right) -- function too complex for now
				{
				GetWatchdog().Kill();
				dotbinary/=2; //this is the number which modifies the approximation
				if(fOfX(aproximation+dotbinary)>theta) //if the value to be added overshoots it does not add
					aproximation+=dotbinary;
				}
			}
			
			
		}
		if (choiceTarget == -1 or choiceTarget > 4)//if there is no target return -1 to indicate target not found -- must be interpeted in 'shoot'
			aproximation= -1;
	}
	
	//numTargets, toCenter, centerY, centerX, firstY, secondY, thirdY, fourthY, area, height, distanceYnorm
	void UpdateCamData()
	{
		//gives cam info
		int choiceTarget;
		targetSelect(choiceTarget);
		Threshold threshold(60, 140, 20, 110, 0, 60);
		ParticleFilterCriteria2 criteria[] = {{IMAQ_MT_BOUNDING_RECT_WIDTH, 30, 400, false, false},
											{IMAQ_MT_BOUNDING_RECT_HEIGHT, 40, 400, false, false}};
		ColorImage image(IMAQ_IMAGE_RGB);//make image
		camera.GetImage(&image);//store camera feed to image
		thresholdImage = image.ThresholdRGB(threshold);//get ready to abliterate insignifigant objects
		bigObjectsImage = thresholdImage->RemoveSmallObjects(false, 2);  // remove small objects (noise)
		convexHullImage = bigObjectsImage->ConvexHull(false);  // fill in partial and full rectangles
		filteredImage = convexHullImage->ParticleFilter(criteria, 2);  // find the rectangles
		particles = filteredImage->GetOrderedParticleAnalysisReports();  // get the results		

			ParticleAnalysisReport& par = (*particles)[choiceTarget];//get values for centering from selected target
			CamData.numTargets = particles->size();
			CamData.centerX = par.center_mass_x_normalized;
			CamData.centerY = par.center_mass_y_normalized;
			
			par = (*particles)[1];
			CamData.firstY = par.center_mass_y;
			par = (*particles)[2];
			CamData.secondY = par.center_mass_y;
			par = (*particles)[3];
			CamData.thirdY = par.center_mass_y;
			par = (*particles)[4];
			CamData.fourthY = par.center_mass_y;
			
			par = (*particles)[distanceTarget];
			CamData.area = par.particleArea;
			CamData.height = par.imageHeight;
			
			CamData.distanceYnorm = par.center_mass_y_normalized;
		

		delete filteredImage;
		delete convexHullImage;
		delete bigObjectsImage;
		delete thresholdImage;
	}
		
	void output (void)
	{
		UpdateCamData();
		int choiceTarget;
		double aproximation;
		//GET INFO FROM values()
		if (IsAutonomous())
			driverOut->Printf(DriverStationLCD::kUser_Line1, 1, "Auto");
		else if (IsOperatorControl())
			driverOut->Printf(DriverStationLCD::kUser_Line1, 1, "Opp");
		
		driverOut->UpdateLCD();
		
		targetSelect(choiceTarget);
		getDistance(aproximation);

		driverOut->Printf(DriverStationLCD::kUser_Line3, 3, "Distance: %d", aproximation);
		
		driverOut->Printf(DriverStationLCD::kUser_Line3, 3, "Number of targets: %n", CamData.numTargets);
		driverOut->Printf(DriverStationLCD::kUser_Line4, 4, "Target selected: %n", choiceTarget);
		driverOut->Printf(DriverStationLCD::kUser_Line5, 5, "Double to 0: %d", CamData.centerX);
		
		if (choiceTarget == -1)
			driverOut->Printf(DriverStationLCD::kUser_Line6, 6, "no target");
		else if (choiceTarget > 4)
			driverOut->Printf(DriverStationLCD::kUser_Line6, 6, "too many targets");
		driverOut->UpdateLCD();
	}

};
START_ROBOT_CLASS(DoctaEight);
