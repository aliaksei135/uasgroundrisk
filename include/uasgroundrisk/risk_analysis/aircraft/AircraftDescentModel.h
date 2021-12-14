/*
 * AircraftModel.h
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_RISK_ANALYSIS_AIRCRAFT_AIRCRAFTDESCENTMODEL_H_
#define UASGROUNDRISK_SRC_RISK_ANALYSIS_AIRCRAFT_AIRCRAFTDESCENTMODEL_H_

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
		 * Based on the CasEx model @link
		 * https://github.com/JARUS-QM/casex/blob/master/casex/aircraft_specs.py
		 *
		 * All units are MKS.
		 */
		class AircraftDescentModel
		{
		public:
			/* Basic params */
			double mass;
			double width;
			double length;
			double cruiseSpeed;

			/* Ballistic Params */
			double ballisticFrontalArea;
			double ballisticDragCoeff;

			/* Glide Params */
			double glideAirspeed;
			double glideRatio;

			AircraftDescentModel(double mass, double width, double length,
			                     double cruiseSpeed, double ballisticFrontalArea,
			                     double ballisticDragCoeff, double glideAirspeed,
			                     double glideRatio);

			/**
			 * The glide impact data of the aircraft from a given altitude
			 * @param altitude the altitude in metres
			 * @return an ImpactDataStruct of the impact
			 */
			ImpactDataStruct glideImpact(double altitude) const;
			/**
			 * A vectorised version of glideImpact
			 */
			std::vector<ImpactDataStruct>
			glideImpact(const std::vector<double>& altitude) const;

			/**
			 * The ballistic impact data of the aircraft from a given altitude and
			 * velocity components
			 * @param altitude the altitude in metres
			 * @param velX the horizontal velocity in m/s
			 * @param velZ the vertical velocity in m/s
			 * @return an ImpactDataStruct of the impact
			 */
			ImpactDataStruct ballisticImpact(double altitude, double velX,
			                                 double velZ) const;
			/**
			 * A vectorised version of ballisticImpact. All inputs must be of a common
			 * length
			 */
			std::vector<ImpactDataStruct> ballisticImpact(const std::vector<double>& altitude,
			                                              const std::vector<double>& velX,
			                                              const std::vector<double>& velZ) const;

		protected:
			double c;
			double gamma;
		};
	} // namespace risk
} // namespace ugr

#endif // UASGROUNDRISK_SRC_RISK_ANALYSIS_AIRCRAFT_AIRCRAFTDESCENTMODEL_H_
