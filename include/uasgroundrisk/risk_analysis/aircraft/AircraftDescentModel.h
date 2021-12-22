/*
 * AircraftModel.h
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_RISK_ANALYSIS_AIRCRAFT_AIRCRAFTDESCENTMODEL_H_
#define UASGROUNDRISK_SRC_RISK_ANALYSIS_AIRCRAFT_AIRCRAFTDESCENTMODEL_H_

#include <string>
#include <vector>

namespace ugr
{
	namespace risk
	{
		/**
		 * A POCO encapsulating impact variables.
		 */
		struct ImpactDataStruct
		{
			/// The distance of the impact away from initial Loss of Control
			double impactDistance;
			/// The velocity at the point of impact
			double impactVelocity;
			/// The impact angle in degrees
			double impactAngle;
			/// The time between Loss of Control and impact
			double impactTime;
		};

		/**
		 * A limited model of an aircraft and its descent.
		 *
		 * All units are MKS.
		 */
		class DescentModel
		{
		public:
			DescentModel() = delete;
			DescentModel(double mass, double width, double length, std::string name);

			/**
			 * The impact data of the aircraft from a given altitude and
			 * velocity components
			 * @param altitude the altitude in metres
			 * @param velX the horizontal velocity in m/s
			 * @param velZ the vertical velocity in m/s
			 * @return an ImpactDataStruct of the impact
			 */
			virtual ImpactDataStruct impact(double altitude, double velX, double velZ) const = 0;

			/**
			 * A vectorised version of above
			 */
			std::vector<ImpactDataStruct> impact(const std::vector<double>& altitude, const std::vector<double>& velX,
			                                     const std::vector<double>& velZ) const;

			std::string getName() const { return name; }

			virtual ~DescentModel() = default;

			/* Basic params */
			double mass;
			double width;
			double length;
			std::string name;
		};

		class GlideDescentModel final : public DescentModel
		{
		public:
			GlideDescentModel() = delete;
			GlideDescentModel(double mass, double width, double length, double glideAirspeed,
			                  double glideRatio);

			/**
			 * The impact data of the aircraft from a given altitude and
			 * velocity components
			 * @param altitude the altitude in metres
			 * @param velX the horizontal velocity in m/s
			 * @param velZ the vertical velocity in m/s
			 * @return an ImpactDataStruct of the impact
			 */
			ImpactDataStruct impact(double altitude, double velX, double velZ) const override;
		protected:
			double glideAirspeed;
			double glideRatio;
		};

		class BallisticDescentModel final : public DescentModel
		{
		public:
			BallisticDescentModel() = delete;
			BallisticDescentModel(double mass, double width, double length, double ballisticFrontalArea,
			                      double ballisticDragCoeff);

			/**
			 * The impact data of the aircraft from a given altitude and
			 * velocity components
			 * @param altitude the altitude in metres
			 * @param velX the horizontal velocity in m/s
			 * @param velZ the vertical velocity in m/s
			 * @return an ImpactDataStruct of the impact
			 */
			ImpactDataStruct impact(double altitude, double velX, double velZ) const override;
		protected:
			double ballisticFrontalArea;
			double ballisticDragCoeff;

			double c;
			double gamma;
		};

		class ParachuteDescentModel final : public DescentModel
		{
		public:
			ParachuteDescentModel() = delete;
			ParachuteDescentModel(double mass, double width, double length, double parachuteDragCoeff,
			                      double parachuteArea,
			                      double parachuteDeployTime);

			/**
			 * The impact data of the aircraft from a given altitude and
			 * velocity components
			 * @param altitude the altitude in metres
			 * @param velX the horizontal velocity in m/s
			 * @param velZ the vertical velocity in m/s
			 * @return an ImpactDataStruct of the impact
			 */
			ImpactDataStruct impact(double altitude, double velX, double velZ) const override;
		protected:
			double parachuteDragCoeff;
			double parachuteArea;
			double parachuteDeployTime;
		};
	} // namespace risk
} // namespace ugr

#endif // UASGROUNDRISK_SRC_RISK_ANALYSIS_AIRCRAFT_AIRCRAFTDESCENTMODEL_H_
