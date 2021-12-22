#ifndef AIRCRAFTMODEL_H
#define AIRCRAFTMODEL_H
#include <vector>

#include "AircraftDescentModel.h"
#include "AircraftStateModel.h"

namespace ugr
{
	namespace risk
	{
		class AircraftModel
		{
		public:
			AircraftModel() = default;

			AircraftModel(const double mass, const double width, const double length): mass(mass), width(width),
				length(length)
			{
			}

			AircraftModel(const AircraftModel& other) = delete;
			AircraftModel(AircraftModel&& other) noexcept = default;
			AircraftModel& operator=(const AircraftModel& other) = delete;
			AircraftModel& operator=(AircraftModel&& other) noexcept = default;

			AircraftStateModel state;
			std::vector<std::unique_ptr<DescentModel>> descents;
			double mass;
			double width;
			double length;
		};
	}
}
#endif // AIRCRAFTMODEL_H
