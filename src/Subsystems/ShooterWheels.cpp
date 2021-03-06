#include <math.h>
#include <ED/PIDManager.hpp>
#include <Ports/Motor.hpp>
#include <Subsystems/OI.hpp>
#include <Subsystems/Sensors.hpp>
#include <Subsystems/ShooterWheels.hpp>
#include <Utils.hpp>
#include <WPILib.h>

const float F_COEFFICIENT = 0.00016;
class ShooterWheelsPID : public ED::PIDManager
{
public:
	ShooterWheelsPID() : PIDManager(0.001, 0.0, 0.001)
	{
		autoClearAccumulatedError(true);
	}

protected:
	float returnPIDInput()
	{
		return Sensors::getShooterWheelRate();
	}

	float getFeedForwardOutput(float new_target)
	{
		return F_COEFFICIENT * (new_target / 5000.0);
	}

	void usePIDOutput(float output, float feed_forward)
	{
		ShooterWheels::setSpeed(output + feed_forward);
	}

};

namespace ShooterWheels
{
	const float RPM_PRESETS[] = {
		2000.0,
		2600.0,
		3200.0,
		3800.0,
		4400.0,
		5000.0
	};
	
	const float SPEED_PRESETS[] = {
		0.5,
		0.6,
		0.7,
		0.8,
		0.9,
		1.0
	};
	
	const float ACCEPTABLE_RATE_ERROR = 25.0;
	
	State state = State::WAITING;
	
	ShooterWheelsPID* pid_manager = nullptr;
	SpeedController* wheels_motor = nullptr;
	
	Timer* target_timer;
	int on_target_count = 0;
	
	void setState(State new_state);
	
	void initialize()
	{
		pid_manager = new ShooterWheelsPID();
		wheels_motor = Utils::constructMotor(MotorPorts::SHOOTER_WHEELS_MOTOR);
		
		target_timer = new Timer();
	}

	void process()
	{
		switch (state) {
		case State::DISABLED:
			break;
		
		case State::WAITING:
			break;
		
		case State::MANUAL_CONTROL:
			break;
		
		case State::MAINTAINING_RATE:
			if (fabs(Sensors::getShooterWheelRate() - pid_manager->getTarget()) < ACCEPTABLE_RATE_ERROR) {
				if (target_timer->HasPeriodPassed(0.2)) {
					++on_target_count;
				}
			}
			else {
				on_target_count = 0;
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
			wheels_motor->Set(speed);
		}
	}
	
	void setRate(float rate)
	{
		if (OI::isPIDEnabled()) {
			pid_manager->enable(true);
			pid_manager->setTarget(rate);
			
			target_timer->Start();
			target_timer->Reset();
			on_target_count = 0;
			
			setState(State::MAINTAINING_RATE);
		}
		else {
			setSpeed(rate / getRPMPreset(getPresetCount() - 1));
			setState(State::MANUAL_CONTROL);
		}
	}
	
	float getSpeed()
	{
		return wheels_motor->Get();
	}
	
	int getPresetCount()
	{
		return sizeof(SPEED_PRESETS) / sizeof(*SPEED_PRESETS);
	}
	
	float getSpeedPreset(int index)
	{
		int preset_count = getPresetCount();
		if (index >= preset_count) {
			return SPEED_PRESETS[preset_count - 1];
		}
		if (index < 0) {
			return SPEED_PRESETS[0];
		}
		return SPEED_PRESETS[index];
	}
	
	float getRPMPreset(int index)
	{
		int preset_count = getPresetCount();
		if (index >= preset_count) {
			return RPM_PRESETS[preset_count - 1];
		}
		if (index < 0) {
			return RPM_PRESETS[0];
		}
		return RPM_PRESETS[index];
	}
	
	bool atRate()
	{
		return on_target_count > 5 && state == State::MAINTAINING_RATE;
	}
	
	void interrupt()
	{
		setState(State::WAITING);
	}
	
	void engageManualControl()
	{
		setState(State::MANUAL_CONTROL);
	}
	
	State getState()
	{
		return state;
	}
	
	void setState(State new_state)
	{
		if (state != new_state) {
			switch (state) {
			case State::DISABLED:
				return;
			
			case State::WAITING:
				break;
			
			case State::MANUAL_CONTROL:
			case State::MAINTAINING_RATE:
				setSpeed(0.0);
				break;
			}
			
			state = new_state;
		}
	}
}
