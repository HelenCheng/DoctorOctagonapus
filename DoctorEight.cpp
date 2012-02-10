/*
 * TODO LIST
 * 
 * RGB VALUES FOR BETTER LIGHT
 * 
 * FB GET -> MUST USE PID TO SLOW AS TURNING TO AIM AND STOP AIMING LOOP IN AUTONOMOUS WHEN ON TARGET
 * 
 * complete shooter function-> USE ARC CALCULATION
 * 
 * set camera to allow anonymous viewing to outline target(available in the web console)
 * http://firstforge.wpi.edu/integration/viewcvs/viewcvs.cgi/trunk/extensions/camera/SquareTracker/src/edu/wpi/first/wpilibj/examples/SquareTrackerExtension.java?root=smart_dashboard&system=exsy1002&view=markup
 * 
 * cleanup highly messy code, break back into sepparate files, et all
 */


/*
 * COPILOT CONTROL
 *
 * A to turn (hold)
 *
 * B to shoot
 * 
 * Y to switch drive  <-not in right now
 *
 * trigers will move arm
 *
 * intake is left thumbstick
 * 
 * WHEN SHOOTING ---- IMPORTANT!!!
 * UNLESS AT LEAST ONE TARGET IS IN SITE OR WE GET A COMPASS LATER DO NOT AIM OR SHOOT BECAUSE NOTHING WILL HAPPEN
 */


/*
 * PILOT
 *
 * press 4 to switch between modified arcade and tank
 *
 * 1 will switch drive
*/


#include "WPILib.h"
#include "Vision/RGBImage.h"
#include "Vision/BinaryImage.h"
#include "cmath"

#define IO (DriverStation::GetInstance()->GetEnhancedIO())
#define CAMERAHEIGHT 80
#define angle 54

static AxisCamera &camera = AxisCamera::GetInstance("10.16.71.11");
















class DoctaEight : public SimpleRobot
{	
	DriverStationLCD* driverOut;
	
	BinaryImage *thresholdImage;
	BinaryImage *bigObjectsImage;
	BinaryImage *convexHullImage;
	BinaryImage *filteredImage;
	vector<ParticleAnalysisReport> *particles;
	
	Encoder LTopEnc, LBotEnc;
	//launch system encoders
	
	Joystick pilot, copilot;
	
	CANJaguar lefty, righty, leftyB, rightyB, intake, arm, LTop, LBot;
	//left and right motors, recieve ball, lift ball to launching system, launch system, platform arm

	signed char negate, choiceTarget, distanceTarget, drive;
	//negate for turning drive, choice target target selected, distance target for getting distance, itt for itterations
	
	double firstTarget, secondTarget, thirdTarget, xCircle;
	//targets (fourth is not one two or three XD), decriment to slow motors as aiming

	bool limitedDistance, cycle, flag, aimin, shootin;
	//switch between primary and secondary distance tracking based on number of targets
	
public:
	DoctaEight::DoctaEight(void):
	pilot(1),
	copilot(2),
	//controller(USB port)
	
	lefty(4),
	leftyB(5),
	righty(2),
	rightyB(3),
	
	arm(9),
	intake(8),
	LTop(6),
	LBot(7),
	//jag(CANjag number)
	LTopEnc(1,2),
	LBotEnc(3,4)
	//encoders(AChannel, BChannel)
	{
		GetWatchdog().Kill();
		
		driverOut  = DriverStationLCD::GetInstance();
		
		//camera.WriteMaxFPS(30);
		//camera.WriteBrightness(30);
		//camera.WriteWhiteBalance(AxisCamera::kWhiteBalance_Hold);
		//camera.WriteResolution(AxisCamera::kResolution_640x360);
		//camera.WriteColorLevel(100);
		//camera.WriteCompression(30);
		//lower easier on CRIO and harder on cam

		limitedDistance = 0;
		flag = 0;
		aimin=0;
		shootin=0;
		cycle = 0;
		drive=1;
		negate=1;

		lefty.ChangeControlMode(CANJaguar::kPercentVbus);
		righty.ChangeControlMode(CANJaguar::kPercentVbus);
		leftyB.ChangeControlMode(CANJaguar::kPercentVbus);
		rightyB.ChangeControlMode(CANJaguar::kPercentVbus);
		intake.ChangeControlMode(CANJaguar::kPercentVbus);
		LTop.ChangeControlMode(CANJaguar::kPercentVbus);
		LBot.ChangeControlMode(CANJaguar::kPercentVbus);
		//CANJags currently % (-1 to 1)

		LTopEnc.Reset();
		LBotEnc.Reset();
		//reset encoders
		LTopEnc.Start();
		LBotEnc.Start();
		//start encoders
	}
	

	
	
		
	void aim(void)
	{
		GetWatchdog().Kill();
		aimin = 1;
		while (copilot.GetRawButton(1))
		{
			GetWatchdog().Kill();
			targetSelect();//refresh target to aim at
			if (choiceTarget!=-1 and particles->size() < 5)//if there is a target
			{
				//camera.GetImage(&image);//store camera feed to image
				output();
				//HERE		Turn
				//call shoot when no longer turning
			}
		}
		aimin = 0;
	}
	
	void shoot(void)
	{
		GetWatchdog().Kill();
		shootin =1;
		
		//should have a while is _ or IsAutonomous portion to exit if timeout in autonomous
		//ALSO shoot must exit if no targets!
		//ALSO IF aproximation= -1 no target or too many (though this shouldnt execute then)
		
		getDistance();
		//if 0, too close to see target-- set jags low
		//use lTop lBot and encoders
		output();
			
		//SHOOT HERE!
		shootin = 0;
	}
	

	
	
	
	
	void DoctaEight::Autonomous(void)
	{
		GetWatchdog().Kill();
		output();
		aim();
	}
	void DoctaEight::OperatorControl(void)
	{
		GetWatchdog().Kill();

		while (IsOperatorControl())
		{
			output();
			arm.Set(copilot.GetZ());
			//move simple platform arm
						
			intake.Set(copilot.GetY());
			//activate roller to take the balls

			tardis();//robot drive <-below
						
			if (copilot.GetRawButton(1))
			{
				aim();
			}
		}
		LTopEnc.Stop();
		LBotEnc.Stop();
		//stops encoders
	}
	
	void targetSelect(void)
	{
		GetWatchdog().Kill();
		int numTargetsC;
		double toCenterC, centerYC, centerXC, firstYC, secondYC, thirdYC, fourthYC, areaC, heightC, distanceYnormC;
		values(numTargetsC, toCenterC, centerYC, centerXC, firstYC, secondYC, thirdYC, fourthYC, areaC, heightC, distanceYnormC);
		
		
		if (3 == numTargetsC or 4 == numTargetsC)//if 3 or 4 targets visible
		{
			
			//choose target to aim/shoot at -> highest target
			if (firstYC > secondYC && firstYC > thirdYC)
				choiceTarget = 1;
			else if (secondYC > thirdYC && secondYC > firstYC)
				choiceTarget = 2;
			else if (thirdYC > secondYC && thirdYC > firstYC)
				choiceTarget = 3;
			else {choiceTarget = 4;}

			//find lowest target for distance tracking purposes
			if (firstYC < secondYC && firstYC < thirdYC)
				distanceTarget = 1;
			else if (secondYC < thirdYC && secondYC < firstYC)
				distanceTarget = 2;
			else if (thirdYC < secondYC && thirdYC < firstYC)
				distanceTarget = 3;
			else 
				distanceTarget = 4;

			limitedDistance = 0;//switch between high and low accuracy distance tracking (high requires 3 or 4 particles)
		}
		else if (2 == numTargetsC)//if 2 targets visible
		{

			//select target to aim/shoot at
			if (firstYC > secondYC)
				choiceTarget = 1;
			else
				choiceTarget = 2;

			limitedDistance = 1;//use single target target tracking
		}
		else if (1 == numTargetsC)
		{
			//aim/shoot at only target
			choiceTarget = 1;
			limitedDistance = 1;
		}
		else
			choiceTarget = -1;
	}

	double fOfX(double x)//part of accurate distance finding
	{
		GetWatchdog().Kill();
		return 	(atan((107.5-CAMERAHEIGHT)/(x)) //angle from top triangle (triangle formed by camera target and line at camera's height)
				+atan((CAMERAHEIGHT-31.5)/(x))); //angle from bottom triangle
	}
	
	double getDistance()//find distance
	{
		GetWatchdog().Kill();
		
		targetSelect();
		
		int numTargetsC;
		double toCenterC, centerYC, centerXC, firstYC, secondYC, thirdYC, fourthYC, areaC, heightC, distanceYnormC;
		values(numTargetsC, toCenterC, centerYC, centerXC, firstYC, secondYC, thirdYC, fourthYC, areaC, heightC, distanceYnormC);
		
		double aproximation=0;
		
		if (choiceTarget !=-1 && numTargetsC < 5)//if there is a target
		{
			if (limitedDistance == 1)//if must use less accurate tracking
			{
				aproximation = 9//half height of target in inches over target to get adjacent
								/tan(//tan of this to get opposite over adjacent
										angle*
											((areaC/24)//to get height in pixels
												/heightC)//above divided to get ratio of size
													/2);//to get half of angle and therefore right triangle
			}
			else//better tracking
			{
				
				double theta=(centerYC-distanceYnormC)/240*angle;//the distance is turned into an angle (refer to fofx(x))
				//107.5 is top target height 31.5 is bottom target height
				
				double dotbinary=54;
				for(int i=0; i<30; i++)/*binary approximation-> guesses using 1/2 distances until
											tlar(that looks about right) -- function too complex for now*/
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
		return aproximation;
	}
	
	double values (int numTargets, double toCenter, double centerY, double centerX, double firstY, double secondY, double thirdY, double fourthY, double area, double height, double distanceYnorm)
	{
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
			numTargets = particles->size();
			centerX = par.center_mass_x_normalized;
			centerY = par.center_mass_y_normalized;
			
			par = (*particles)[1];
			firstY = par.center_mass_y;
			par = (*particles)[2];
			secondY = par.center_mass_y;
			par = (*particles)[3];
			thirdY = par.center_mass_y;
			par = (*particles)[4];
			fourthY = par.center_mass_y;
			
			par = (*particles)[distanceTarget];
			area = par.particleArea;
			height = par.imageHeight;
			
			distanceYnorm = par.center_mass_y_normalized;
		

		delete filteredImage;
		delete convexHullImage;
		delete bigObjectsImage;
		delete thresholdImage;
		return (toCenter, numTargets, centerY, centerX, firstY, secondY, thirdY, fourthY, area, height, distanceYnorm);
	}
	
	void output (void)
	{
		int numTargetsC;
		double toCenterC, centerYC, centerXC, firstYC, secondYC, thirdYC, fourthYC, areaC, heightC, distanceYnormC;
		values(numTargetsC, toCenterC, centerYC, centerXC, firstYC, secondYC, thirdYC, fourthYC, areaC, heightC, distanceYnormC);
				
		if (IsAutonomous())
			driverOut->PrintfLine(DriverStationLCD::kUser_Line1, "Auto");
		else if (IsOperatorControl())
			driverOut->PrintfLine(DriverStationLCD::kUser_Line1, "Opp");
		
		if (aimin)
			driverOut->PrintfLine(DriverStationLCD::kUser_Line2, "aiming");
		if (shootin)
			driverOut->PrintfLine(DriverStationLCD::kUser_Line2, "shootin");
		
		driverOut->PrintfLine(DriverStationLCD::kUser_Line3, "Number of targets: %n", numTargetsC);
		driverOut->PrintfLine(DriverStationLCD::kUser_Line4, "Target selected: %n", choiceTarget);
		driverOut->PrintfLine(DriverStationLCD::kUser_Line5, "Double to 0: %d", centerXC);
		
		if (choiceTarget == -1)
			driverOut->PrintfLine(DriverStationLCD::kUser_Line6, "no target");
		else if (choiceTarget > 4)
			driverOut->PrintfLine(DriverStationLCD::kUser_Line6, "too many targets");
		driverOut->UpdateLCD();
	}
	
	void DoctaEight::leftyrighty(double left, double right)//set drive motors on either side
	{
		GetWatchdog().Kill();
		righty.Set(right);
		rightyB.Set(right);
		lefty.Set(left);
		leftyB.Set(left);
	}
	
	void DoctaEight::tardis(void)
	{
		if (pilot.GetRawButton(1) && cycle == 0)
		{
			negate *= -1;
			cycle = 1;
		}
		if (pilot.GetRawButton(4) && cycle == 0)
		{
			//drive *= -1;	GET Y FOR RIGHT THUMB??
			cycle = 1;
		}
		else if (!pilot.GetRawButton(1))
			cycle = 0;
		//to reverse drive			
		
		if (drive == 1)
		{
			if (pilot.GetY() > .01 && pilot.GetTwist() >= 0)
			{
				if (negate == 1)
					leftyrighty(negate*(pilot.GetY() - pilot.GetTwist()*2*pilot.GetY()), negate*pilot.GetY());
				else
					leftyrighty(negate*pilot.GetY(), negate*(pilot.GetY() - pilot.GetTwist()*2*pilot.GetY()));
			}
			else if (pilot.GetY() > .01 && pilot.GetTwist() <= 0)
			{
				if (negate ==1)
					leftyrighty(negate*pilot.GetY(), negate*(pilot.GetY() + pilot.GetTwist()*2*pilot.GetY()));
				else
					leftyrighty(negate*(pilot.GetY() + pilot.GetTwist()*2*pilot.GetY()), negate*pilot.GetY());
			}		
			else if (pilot.GetY() < -.01 && pilot.GetTwist() >= 0)
			{
				if (negate == 1)
					leftyrighty(negate*(pilot.GetY() - pilot.GetTwist()*2*pilot.GetY()), negate*pilot.GetY());
				else
					leftyrighty(negate*pilot.GetY(), negate*(pilot.GetY() - pilot.GetTwist()*2*pilot.GetY()));
			}
			else if (pilot.GetY() < -.01 && pilot.GetTwist() <= 0)
			{
				if (negate == 1)
					leftyrighty(negate*pilot.GetY(), negate*(pilot.GetY() + pilot.GetTwist()*2*pilot.GetY()));
				else
					leftyrighty(negate*(pilot.GetY() + pilot.GetTwist()*2*pilot.GetY()), negate*pilot.GetY());
			}
			else
			{
				if (negate == 1)
					leftyrighty(negate*pilot.GetTwist(), negate*(-pilot.GetTwist()));
				else
					leftyrighty(negate*(-pilot.GetTwist()), negate*pilot.GetTwist());
			}
		}
		else if (drive == -1)
			leftyrighty(negate*pilot.GetY(), negate*pilot.GetTwist());
	}

};
START_ROBOT_CLASS(DoctaEight);
