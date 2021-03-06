#include <ED/PIDManager.hpp>
#include <Ports/Motor.hpp>
#include <Subsystems/IntakeAngle.hpp>
#include <Subsystems/OI.hpp>
#include <Subsystems/Sensors.hpp>
#include <Utils.hpp>
#include <WPILib.h>

class IntakeAnglePID : public ED::PIDManager
{
public:
	IntakeAnglePID() : PIDManager(0.02, 0.0001, 0.0)
	{
		autoClearAccumulatedError(true);
	}

protected:
	float returnPIDInput()
	{
		return Sensors::getIntakeAngle();
	}

	void usePIDOutput(float output, float feed_forward)
	{
		IntakeAngle::setSpeed(output);
	}
};

namespace IntakeAngle
{
	const float MOTOR_SPEED = 0.5;
	const float ACCEPTABLE_ERROR = 5.0;
	
	const float ANGLE_PRESETS[] = {
		-30.0,
		-15.0,
		3.0,
		30.0,
		60.0,
		90.0
	};
	
	State state = State::WAITING;

	IntakeAnglePID* pid_manager = nullptr;
	SpeedController* angle_motor = nullptr;
	
	void setState(State new_state);

	void initialize()
	{
		pid_manager = new IntakeAnglePID();
		angle_motor = Utils::constructMotor(MotorPorts::INTAKE_ANGLE_MOTOR);
	}

	void process()
	{
		switch (state) {
		case State::DISABLED:
		case State::WAITING:
			pid_manager->enable(false);
			setSpeed(0.0);
			break;
			
		case State::MANUAL_CONTROL:
			pid_manager->enable(false);
			break;
			
		case State::REACHING_ANGLE:
			if (!pid_manager->isEnabled()) {
				float error = pid_manager->getTarget() - Sensors::getIntakeAngle();
				if (fabs(error) > ACCEPTABLE_ERROR) {
					if (error > 0) {
						setDirection(Utils::VerticalDirection::UP);
					}
					else { // error < 0
						setDirection(Utils::VerticalDirection::DOWN);
					}
				}
				else {
					// this will probably lead to the intake jerking up over and over again, trying to stay in the zone
					setDirection(Utils::VerticalDirection::V_STILL);
				}
			}
			break;
		}
	}

	void processPID()
	{
		pid_manager->process();
	}

	void enablePID(bool enable)
	{
		pid_manager->enable(enable);
	}

	void setSpeed(float speed)
	{
		if (state != State::DISABLED) {
			angle_motor->Set(speed);
		}
	}

	void setDirection(Utils::VerticalDirection dir)
	{
		switch (dir) {
		case Utils::VerticalDirection::UP:
			setSpeed(MOTOR_SPEED);
			break;
		case Utils::VerticalDirection::V_STILL:
			setSpeed(0.0);
			break;
		case Utils::VerticalDirection::DOWN:
			setSpeed(-MOTOR_SPEED);
			break;
		}
	}

	float getSpeed()
	{
		return angle_motor->Get();
	}

	Utils::VerticalDirection getDirection()
	{
		float speed = getSpeed();
		if (speed > 0.0) {
			return Utils::VerticalDirection::UP;
		}
		else if (speed < 0.0) {
			return Utils::VerticalDirection::DOWN;
		}
		return Utils::VerticalDirection::V_STILL;
	}
	
	void engageManualControl()
	{
		pid_manager->enable(false);
		setState(State::MANUAL_CONTROL);
	}
	
	void goToAngle(float degrees)
	{
		pid_manager->setTarget(degrees);
		pid_manager->enable(OI::isPIDEnabled());
		setState(State::REACHING_ANGLE);
	}
	
	void interrupt()
	{
		pid_manager->enable(false);
		setState(State::WAITING);
	}
	
	int getPresetCount()
	{
		return sizeof(ANGLE_PRESETS) / sizeof(*ANGLE_PRESETS);
	}
	
	float getAnglePreset(int index)
	{
		int count = getPresetCount();
		if (index >= count) {
			return ANGLE_PRESETS[count - 1];
		}
		if (index < 0) {
			return ANGLE_PRESETS[0];
		}
		return ANGLE_PRESETS[index];
	}
	
	void setState(State new_state)
	{
		if (new_state != state) {
			// handle the state we're exiting
			switch (state) {
			case State::DISABLED:
				return; // if this subsystem is disabled, do not allow a reenable
			
			case State::WAITING:
				break;
				
			case State::MANUAL_CONTROL:
			case State::REACHING_ANGLE:
				setDirection(Utils::VerticalDirection::V_STILL);
				break;
			}
		}
		
		state = new_state;
	}
	
	State getState()
	{
		return state;
	}
}
