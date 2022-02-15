#ifndef AIRCRAFTMODEL_H
#define AIRCRAFTMODEL_H
#include <vector>
#include <memory>

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

            template <typename DescentModel, typename...Args>
            void addDescentModel(Args&&... modelArgs)
            {
                descents.emplace_back(
                    std::unique_ptr<DescentModel>(
                        new DescentModel(mass, width, length, std::forward<Args>(modelArgs)...))
                );
            }

            AircraftStateModel state;
            double mass;
            double width;
            double length;
            std::vector<std::unique_ptr<DescentModel>> descents;
        };
    }
}
#endif // AIRCRAFTMODEL_H
