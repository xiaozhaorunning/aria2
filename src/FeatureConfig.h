/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#ifndef _D_FEATURE_CONFIG_H_
#define _D_FEATURE_CONFIG_H_

#include "common.h"
#include <map>
#include <string>
#include <deque>

namespace aria2 {

typedef std::map<std::string, uint16_t> PortMap;
typedef std::map<std::string, bool> FeatureMap;

class FeatureConfig {
private:
  static FeatureConfig* featureConfig;

  PortMap defaultPorts;
  FeatureMap supportedFeatures;
  std::deque<std::string> features;

  FeatureConfig();
  ~FeatureConfig() {}
public:
  static FeatureConfig* getInstance() {
    if(!featureConfig) {
      featureConfig = new FeatureConfig();
    }
    return featureConfig;
  }

  static void release() {
    delete featureConfig;
    featureConfig = 0;
  }

  uint16_t getDefaultPort(const std::string& protocol) const {
    PortMap::const_iterator itr = defaultPorts.find(protocol);
    if(itr == defaultPorts.end()) {
      return 0;
    } else {
      return itr->second;
    }
  }

  bool isSupported(const std::string& feature) const {
    FeatureMap::const_iterator itr = supportedFeatures.find(feature);
    if(itr == supportedFeatures.end()) {
      return false;
    } else {
      return itr->second;
    }
  }

  const std::deque<std::string>& getFeatures() const {
    return features;
  }

  std::string getConfigurationSummary() const {
    std::string summary;
    for(std::deque<std::string>::const_iterator itr = features.begin();
	itr != features.end(); itr++) {
      summary += *itr;
      if(isSupported(*itr)) {
	summary += ": yes";
      } else {
	summary += ": no";
      }
      summary += "\n";
    }
    return summary;
  }
};

} // namespace aria2

#endif // _D_FEATURE_CONFIG_H_
