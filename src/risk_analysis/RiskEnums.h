/*
 * RiskEnums.h
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_RISK_ANALYSIS_RISKENUMS_H_
#define UASGROUNDRISK_SRC_RISK_ANALYSIS_RISKENUMS_H_

namespace ugr {
namespace risk {
enum class RiskType {
  // IMPACT,  /// The risk of UAS impact with the ground
  STRIKE,  /// The risk of UAS striking a person
  FATALITY /// The risk of UAS causing a fatality
};
} // namespace risk
} // namespace ugr

#endif // UASGROUNDRISK_SRC_RISK_ANALYSIS_RISKENUMS_H_
