#include <ED/Utils.hpp>
#include <Coordination.hpp>
#include <Ports/OI.hpp>
#include <Subsystems/ClimberArm.hpp>
#include <Subsystems/HolderWheels.hpp>
#include <Subsystems/IntakeAngle.hpp>
#include <Subsystems/IntakeRoller.hpp>
#include <Subsystems/Mobility.hpp>
#include <Subsystems/OI.hpp>
#include <Subsystems/Sensors.hpp>
#include <Subsystems/ShooterPitch.hpp>
#include <Subsystems/ShooterWheels.hpp>
#include <Subsystems/Winches.hpp>
#include <Utils.hpp>
#include <WPILib.h>

namespace OI
{
	const float JOYSTICK_DEADZONE = 0.1;
	
	Joystick* left_joy;
	Joystick* right_joy;
	
	Joystick* buttons_joy1;
	Joystick* buttons_joy2;
	
	bool last_pid_switch = false;
	bool last_shooter_wheels_switch = false;
	int last_intake_angle_dial = -1;
	int last_shooter_pitch_dial = -1;
	int last_shooter_wheels_dial = -1;
	Utils::VerticalDirection last_intake_angle_dir = Utils::VerticalDirection::V_STILL;
	Utils::HorizontalDirection last_intake_roller_dir = Utils::HorizontalDirection::H_STILL;

	float getJoystickAnalogPort(Joystick* joy, unsigned int port, float deadzone = 0.0);
	
	void mobilityProcess();
	void intakeProcess(); // includes HolderWheels
	void shooterPitchProcess();
	void shooterWheelsProcess();
	void climberProcess(); // includes arm and winches
	
	void initialize()
	{
		right_joy = new Joystick(OIPorts::RIGHT_JOYSTICK);
		left_joy = new Joystick(OIPorts::LEFT_JOYSTICK);
		
		buttons_joy1 = new Joystick(OIPorts::BUTTONS_JOYSTICK1);
		buttons_joy2 = new Joystick(OIPorts::BUTTONS_JOYSTICK2);
	}
	
	void process()
	{
		bool sensor_switch = buttons_joy1->GetRawButton(OIPorts::SENSOR_ENABLE_SWITCH);
		Sensors::enableGyro(sensor_switch);
		Sensors::enableShooterAngle(sensor_switch);
		Sensors::enableIntakeAngle(sensor_switch);
		Sensors::enableShooterTach(sensor_switch);
		Sensors::enableLidar(sensor_switch);
		Sensors::enableDriveEncoders(sensor_switch);
		Sensors::enableBallLimit(sensor_switch);
		Sensors::enableShooterLimit(sensor_switch);
		Sensors::enablePDP(sensor_switch);
		
		////// PID & Sensor enable //////
		bool pid_switch;
		if (sensor_switch) {
			pid_switch = buttons_joy1->GetRawButton(OIPorts::PID_ENABLE_SWITCH);
		}
		else {
			pid_switch = false;
		}
		if (pid_switch != pid_switch) {
			IntakeAngle::enablePID(pid_switch);
			ShooterPitch::enablePID(pid_switch);
			ShooterWheels::enablePID(pid_switch);
			
			last_pid_switch = pid_switch;
		}
		
		mobilityProcess();
		intakeProcess();
		shooterPitchProcess();
		shooterWheelsProcess();
		climberProcess();
	}
	
	bool isPIDEnabled()
	{
		return buttons_joy1->GetRawButton(OIPorts::PID_ENABLE_SWITCH);
	}
	
	float getJoystickAnalogPort(Joystick* joy, unsigned int port, float deadzone)
	{
		float joy_value = -joy->GetRawAxis(port);
		
		if (deadzone != 0.0 && ED::valueInRange(joy_value, -deadzone, deadzone)) {
			return 0.0;
		}
		
		return -joy->GetRawAxis(port);
	}
	
	void mobilityProcess()
	{
		float left_joy_speed = getJoystickAnalogPort(left_joy, OIPorts::JOYSTICK_Y_PORT, JOYSTICK_DEADZONE);
		float right_joy_speed = getJoystickAnalogPort(right_joy, OIPorts::JOYSTICK_X_PORT, JOYSTICK_DEADZONE);
		if (left_joy->GetRawButton(OIPorts::B_DRIVE_STRAIGHT_LEFT)) {
			Mobility::driveStraight(left_joy_speed);
		}
		else if (right_joy->GetRawButton(OIPorts::B_DRIVE_STRAIGHT_RIGHT)) {
			Mobility::driveStraight(right_joy_speed);
		}
		else {
			Mobility::engageManualControl();
			Mobility::setLeftSpeed(left_joy_speed);
			Mobility::setRightSpeed(right_joy_speed);
		}
	}
	
	void intakeProcess()
	{
		int dial = ED::convertVoltage(getJoystickAnalogPort(buttons_joy1, OIPorts::INTAKE_ANGLE_DIAL) + 1.0, IntakeAngle::getPresetCount(), 2.0);
		
		Utils::VerticalDirection intake_angle_dir = Utils::VerticalDirection::V_STILL;
		if (buttons_joy1->GetRawButton(OIPorts::MOVE_INTAKE_UP_BUTTON)) {
			intake_angle_dir = Utils::VerticalDirection::UP;
		}
		else if (buttons_joy1->GetRawButton(OIPorts::MOVE_INTAKE_DOWN_BUTTON)) {
			intake_angle_dir = Utils::VerticalDirection::DOWN;
		}
		else {
			intake_angle_dir = Utils::VerticalDirection::V_STILL;
		}
		
		switch (intake_angle_dir) {
		case Utils::VerticalDirection::UP:
		case Utils::VerticalDirection::DOWN:
			IntakeAngle::engageManualControl();
			IntakeAngle::setDirection(intake_angle_dir);
			break;
			
		case Utils::VerticalDirection::V_STILL:
			////// Intake angle dial //////
			if (dial != last_intake_angle_dial) {
				IntakeAngle::goToAngle(IntakeAngle::getAnglePreset(dial));
			}
			else if (intake_angle_dir != last_intake_angle_dir) {
				if (Sensors::isIntakeAngleEnabled()) {
					IntakeAngle::goToAngle(Sensors::getIntakeAngle());
				}
				else {
					IntakeAngle::setDirection(Utils::VerticalDirection::V_STILL);
				}
			}
			break;
		}
		
		if (buttons_joy1->GetRawButton(OIPorts::INTAKE_BELT_INWARD_SWITCH)) {
			last_intake_roller_dir = Utils::HorizontalDirection::IN;

			HolderWheels::engageManualControl();
			HolderWheels::setDirection(last_intake_roller_dir);
			IntakeRoller::setDirection(last_intake_roller_dir);
		}
		else if (buttons_joy1->GetRawButton(OIPorts::INTAKE_BELT_OUTWARD_SWITCH)) {
			last_intake_roller_dir = Utils::HorizontalDirection::OUT;

			HolderWheels::engageManualControl();
			HolderWheels::setDirection(last_intake_roller_dir);
			IntakeRoller::setDirection(last_intake_roller_dir);
		}
		else if (last_intake_roller_dir != Utils::HorizontalDirection::H_STILL) {
			last_intake_roller_dir = Utils::HorizontalDirection::H_STILL;

			HolderWheels::engageManualControl();
			HolderWheels::setDirection(last_intake_roller_dir);
			IntakeRoller::setDirection(last_intake_roller_dir);
		}

		last_intake_angle_dial = dial; // prevent the dial from taking control after manual controls are over
		last_intake_angle_dir = intake_angle_dir;
	}
	
	void shooterPitchProcess()
	{
		////// Shooter pitch dial //////
		int dial = ED::convertVoltage(getJoystickAnalogPort(buttons_joy1, OIPorts::SHOOTER_PITCH_DIAL) + 1.0, ShooterPitch::getPresetCount(), 2.0);
		if (dial != last_shooter_pitch_dial) { // if the dial has been moved
			ShooterPitch::goToAngle(ShooterPitch::getAnglePreset(dial));
			
			last_shooter_pitch_dial = dial;
		}
	}
	
	void shooterWheelsProcess()
	{
		bool shooter_switch = buttons_joy1->GetRawButton(OIPorts::SHOOTER_WHEELS_SWITCH);
		
		// the current position of the shooter dial
		int dial = ED::convertVoltage(getJoystickAnalogPort(buttons_joy1, OIPorts::SHOOTER_SPEED_DIAL) + 1.0, ShooterWheels::getPresetCount(), 2.0);
		
		float speed; // is used later for the shoot button as well; might refer to either `rate` or `speed`
		if (Sensors::isShooterTachEnabled()) {
			speed = ShooterWheels::getRPMPreset(dial);
		}
		else {
			speed = ShooterWheels::getSpeedPreset(dial);
		}
		
		if (shooter_switch) {
			if (dial != last_shooter_wheels_dial) {
				if (Sensors::isShooterTachEnabled()) {
					ShooterWheels::setRate(speed);
				}
				else {
					ShooterWheels::engageManualControl();
					ShooterWheels::setSpeed(speed);
				}
				
				last_shooter_wheels_dial = dial;
			}
		}
		else if (last_shooter_wheels_switch) { // if the switch was just turned off
			ShooterWheels::setSpeed(0.0);
			last_shooter_wheels_dial = -1; // forces a shooter update when the switch is turned back on
		}
		last_shooter_wheels_switch = shooter_switch;
		
		if (buttons_joy1->GetRawButton(OIPorts::SHOOT_BUTTON)) {
			Coordination::shootBall(speed);
		}
	}
	
	void climberProcess()
	{
		////// Winches & Climber Arm //////
		bool winch_switch = buttons_joy2->GetRawButton(OIPorts::MANUAL_WINCH_ENABLE_SWITCH);
		if (winch_switch) {
			Winches::setFrontSpeed(getJoystickAnalogPort(buttons_joy2, OIPorts::FRONT_WINCH_JOYSTICK, JOYSTICK_DEADZONE));
			Winches::setFrontSpeed(getJoystickAnalogPort(buttons_joy2, OIPorts::BACK_WINCH_JOYSTICK, JOYSTICK_DEADZONE));
		}
		else {
			ClimberArm::setSpeed(getJoystickAnalogPort(buttons_joy2, OIPorts::FRONT_WINCH_JOYSTICK));
		}
	}
	
}
