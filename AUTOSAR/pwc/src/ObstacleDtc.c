#include "PWC.h"
#include "Rte_ObstacleDtcType.h"

void ObstacleDtcRunnable(void)
{
	IoHwAb_LevelType value;
	uint8 status;
	Rte_Call_isObstacle_Read(&value, &status);

	if (value)
		Rte_IWrite_ObstacleDtcRunnable_obstacle_isPresent(IOHWAB_LOW);
	else
		Rte_IWrite_ObstacleDtcRunnable_obstacle_isPresent(IOHWAB_HIGH);
}
