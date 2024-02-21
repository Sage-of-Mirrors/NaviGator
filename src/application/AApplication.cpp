#include "application/AApplication.hpp"
#include "application/ATime.hpp"


void AApplication::Run() {
	Clock::time_point lastFrameTime, thisFrameTime;

	while (true) {
		lastFrameTime = thisFrameTime;
		thisFrameTime = AUtil::GetTime();

		if (!Execute(AUtil::GetDeltaTime(lastFrameTime, thisFrameTime)))
			break;
	}
}
