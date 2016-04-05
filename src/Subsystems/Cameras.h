#ifndef SRC_SUBSYSTEMS_CAMERAS_H_
#define SRC_SUBSYSTEMS_CAMERAS_H_

namespace Cameras
{
	void initialize();
	void process();

	void refreshContours();
	bool canSeeGoal();

	/**
	 * @return range [0, 1] representing the percentage distance from the left side of the camera view
	 */
	float getTargetX();
	/**
	 * @return range [0, 1] representing the percentage distance from the top side of the camera view
	 */
	float getTargetY();

	float getTargetWidth();
	float getTargetHeight();

	float getDistanceFromTarget();
	float getTargettingPitch();

	int getHorizontalPixelsFromTarget();

	bool isTargettingEnabled();
}

#endif /* SRC_SUBSYSTEMS_CAMERAS_H_ */
