/*
 *	Gage Ervin and Emmanuel Lopez		Team 1671 
 *	what happened to the other 7?
 *	historical references: Vriska and Doctaoctagonapus
 */


/*
 * COPILOT CONTROL
 * 1 to turn (hold)
 * 2 to shoot
 * trigers will move arm
 * intake is right thumbstick
 * WHEN SHOOTING ---- IMPORTANT!!!  UNLESS AT LEAST ONE TARGET IS IN SITE OR WE GET A COMPASS LATER DO NOT SHOOT
 * 
 * PILOT
 * car drive, but turns full forward/back at extreme
 * 1 will switch drive
*/
 

/*
 * determine lighting values
 *
 * launch enc for speed ratio
 *
 * make warning if out of range
 * 
*/


#include <iostream>
//for console IO
#include "WPILib.h"
//duh
#include "nivision.h"
//for cams
#include "cmath"

#define IO (DriverStation::GetInstance()->GetEnhancedIO())
//IO from driver station
AxisCamera &camera = AxisCamera::GetInstance();
//output from cameras to driverstation (so we can see it)
ColorImage image(IMAQ_IMAGE_RGB);
//create an image to buffer pics


class DoctaEight : public SimpleRobot
{
	//declarations
	
	BinaryImage* binImg;
	//make image
	
	
	Encoder LTopEnc, LBotEnc;
	//launch system encoders
	Joystick pilot, copilot;
	//
	CANJaguar lefty, righty, leftyB, rightyB, intake, lift, LTop, LBot, arm;
	//left and right motors, recieve ball, lift ball to launching system, launch system, platform arm
	signed char negate, choiceTarget, distanceTarget;
	//negate for turning drive
	double distance, firstTarget, secondTarget, thirdTarget, decrement;
	//calculated distance to target, to slow motors as aiming
	
public:
	DoctaEight(void):
		
		//constructs
		
		pilot(1),
		copilot(2),
		//controller(USB port)
		lefty(1),
		righty(2),
		arm(3),
		leftyB(4),
		rightyB(5),
		intake(6),
		lift(7),
		LTop(8),
		LBot(9),
		//jag(CANjag number)
		LTopEnc(1,2),
		LBotEnc(3,4)
		//encoders(AChannel, BChannel)
		{
			binImg = image.ThresholdRGB(192, 256, 192, 156, 192, 256);
			//HSL values (MUST BE FOUND BY EXPERIMENT)
			camera.WriteMaxFPS(6);
			//FPS
			camera.WriteBrightness(0);
			//
			camera.WriteWhiteBalance(AxisCamera::kWhiteBalance_Automatic);
			//white balance can be automatic or other
			camera.WriteResolution(AxisCamera::kResolution_320x240);
			//resolution

			decrement=1;
			negate=1;
			
			lefty.ChangeControlMode(CANJaguar::kPercentVbus);
			righty.ChangeControlMode(CANJaguar::kPercentVbus);
			leftyB.ChangeControlMode(CANJaguar::kPercentVbus);
			rightyB.ChangeControlMode(CANJaguar::kPercentVbus);
			intake.ChangeControlMode(CANJaguar::kPercentVbus);
			lift.ChangeControlMode(CANJaguar::kPercentVbus);
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
	
	//selects target
	void targetSelect(void)
	{
		camera.GetImage(&image);
		//gets image from cam
		vector<ParticleAnalysisReport>* particles = binImg->GetOrderedParticleAnalysisReports();
		//finds targets
		

		ParticleAnalysisReport& par = (*particles)[1];
		firstTarget = par.center_mass_y;
		par = (*particles)[2];
		secondTarget = par.center_mass_y;
		par = (*particles)[3];
		thirdTarget = par.center_mass_y;
		
		if (firstTarget > secondTarget && firstTarget > thirdTarget)
		{
			choiceTarget = 1;
		}
		else if (secondTarget > thirdTarget && secondTarget > firstTarget)
		{
			choiceTarget = 2;
		}
		else if (thirdTarget > secondTarget && thirdTarget > firstTarget)
		{
			choiceTarget = 3;
		}
		else {choiceTarget = 4;}
		//above chooses target to shoot at - highest target
		
		if (firstTarget < secondTarget && firstTarget < thirdTarget)
		{
			distanceTarget = 1;
		}
		else if (secondTarget < thirdTarget && secondTarget < firstTarget)
		{
			distanceTarget = 2;
		}
		else if (thirdTarget < secondTarget && thirdTarget < firstTarget)
		{
			distanceTarget = 3;
		}
		else {distanceTarget = 4;}
		
		//above chooses target to get range by - lowest target
	}
	//above selects target
	
	void aim(void)
	{
		targetSelect();
		
		
		//find what point motors stop then this should be slightly above
		while (/*decrement > .2 or decrement < -.2 && */ copilot.GetRawButton(1))
		{
			

			RainbowDash();
			
			camera.GetImage(&image);
			//gets image from cam
			vector<ParticleAnalysisReport>* particles = binImg->GetOrderedParticleAnalysisReports();
			//finds targets
			cout << "Number of targets: " << particles->size();
			//output number of targets
			
			ParticleAnalysisReport& par = (*particles)[choiceTarget];
			//get report on target
			
			if (par.center_mass_x_normalized > -.3  && par.center_mass_x_normalized < .3)
			{
				decrement = par.center_mass_x_normalized*2;
			}
			else
				decrement = 1;
			//lower speed if aiming at target or set back to 1
			
			
			if(par.center_mass_x_normalized > 0)//right; 
			{
				righty.Set(0);
				rightyB.Set(0);
				lefty.Set(decrement);
				leftyB.Set(decrement);
			}
			else if(par.center_mass_x_normalized < 0)//left
			{
				lefty.Set(0);
				leftyB.Set(0);
				righty.Set(decrement);
				rightyB.Set(decrement);
			}
		}
		//above aims
	}
	
	
	
	
	void shoot(void)
	{
		camera.GetImage(&image);
		//gets image from cam
		vector<ParticleAnalysisReport>* particles = binImg->GetOrderedParticleAnalysisReports();
		//finds targets
		
		ParticleAnalysisReport& par = (*particles)[distanceTarget];
		
		distance = 9//half height of target in inches over target to get adjacent
						/tan(//tan of this to get opposite over adjacent
								54*//angle of lens vision
									((par.particleArea/24)//to get height in pixels
										/par.imageHeight)//above divided to get ratio of size
											/2);//to get half of angle and therefore right triangle
			
		//SHOOT HERE!
	}
	
	
	
	
	void drive(void)
	{				
		if (pilot.GetY() > 0 && pilot.GetX() >= 0)//forward right
		{
			lefty.Set(pilot.GetY() * negate * decrement);//left motors full
			leftyB.Set(pilot.GetY() * negate * decrement);//left motors full
			righty.Set(( pilot.GetY() - pilot.GetX() * 2 ) * negate * decrement);//right motors full dec by twiceX abs X
			rightyB.Set(( pilot.GetY() - pilot.GetX() * 2 ) * negate * decrement);
		}//(so up to x = 0 right rev and when y negative, backward curve)
		else if (pilot.GetY() < 0 && pilot.GetX() > 0)//backward left
		{
			righty.Set(pilot.GetY() * negate * -1 * decrement);
			rightyB.Set(pilot.GetY() * negate * -1 * decrement);
			lefty.Set((pilot.GetY() - pilot.GetX() * 2) * negate * decrement);
			leftyB.Set((pilot.GetY() - pilot.GetX() * 2) * negate * decrement);
		}
		else if (pilot.GetY() > 0 && pilot.GetX() <= 0)//forward left
		{
			righty.Set(pilot.GetY() * negate * decrement);
			rightyB.Set(pilot.GetY() * negate * decrement);
			lefty.Set((pilot.GetX() * 2 + pilot.GetY()) * negate * decrement);
			leftyB.Set((pilot.GetX() * 2 + pilot.GetY()) * negate * decrement);
		}
		else if (pilot.GetY() < 0 && pilot.GetX() < 0)//back right
		{
			lefty.Set(pilot.GetY() * negate * -1 * decrement);//left full back
			leftyB.Set(pilot.GetY() * negate * -1 * decrement);//left full back
			righty.Set(( pilot.GetY() + pilot.GetX() * 2 ) * negate * decrement);//right morots full dec by twice abs X
			rightyB.Set(( pilot.GetY() + pilot.GetX() * 2 ) * negate * decrement);
		}
		else
		{
			righty.Set(pilot.GetY() *negate * decrement);
			rightyB.Set(pilot.GetY() *negate * decrement);
			lefty.Set(pilot.GetY() *negate * decrement);
			leftyB.Set(pilot.GetY() *negate * decrement);
		}
	}
	
	
	
	void RainbowDash(void)//pony works like c code braces, like this rainbow.Add<Typegoeshere>(variable)
	{
		Dashboard &rainbow = DriverStation::GetInstance()->GetHighPriorityDashboardPacker();
		rainbow.AddCluster();						//displays the target nubmers
			rainbow.AddDouble(firstTarget);
			rainbow.AddDouble(secondTarget);
			rainbow.AddDouble(thirdTarget);
		rainbow.FinalizeCluster();
		rainbow.AddCluster();						/////displays the distance from the target, and
			rainbow.AddDouble(distance);			//and the diffrence number for angling
			rainbow.AddDouble(decrement);
		rainbow.FinalizeCluster();
		rainbow.Finalize();//need this for the ending
	}
	
	
	
	
	void Autonomous(void)
	{
		GetWatchdog().Kill();
		while (IsAutonomous())
		{
			aim();
			shoot();
			//aim and shoot
		}
	}
	
	
	
	
	void OperatorControl(void)
	{
		GetWatchdog().Kill();
		while (IsOperatorControl())
		{
			RainbowDash();
			
			if (copilot.GetTop())
				arm.Set(-1);
			else if (copilot.GetTrigger())
				arm.Set(1);
			else
				arm.Set(0);
			//move arm
			
			
			if (copilot.GetRawButton(1))
			{
				aim();
				//aim
			}
			else
				decrement = 1;
			//if not aiming and shooting, return motor power
			
			if (copilot.GetRawButton(2))
				shoot();
			
			intake.Set(copilot.GetTwist());
			//take the balls

			
			if (pilot.GetRawButton(1)){negate *= -1;}
			//to reverse drive
			
			drive();//drive system
			
		}
		LTopEnc.Stop();
		LBotEnc.Stop();
		//stops encoders
	}
};

START_ROBOT_CLASS(DoctaEight);
//called by driverstation
