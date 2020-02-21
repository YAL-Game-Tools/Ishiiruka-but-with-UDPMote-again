#pragma once

#include <string>

#include "Common/Common.h"
#include "Common/IniFile.h"
#include "InputCommon/ControllerEmu/ControllerEmu.h"
#include "InputCommon/ControllerEmu/ControlGroup/ControlGroup.h"
#include "InputCommon/UDPWiimote.h"

class UDPWrapper : public ControllerEmu::ControlGroup
{
public:
  UDPWiimote* inst;
  int index;
  bool updIR, updAccel, updButt, updNun, updNunAccel,
      udpEn;  // upd from update and udp from... well... UDP
  bool showMessageBoxOnHost;
  std::string port;

  UDPWrapper(int index, const char* const _name);
  virtual void LoadConfig(IniFile::Section* sec, const std::string& defdev = "",
                          const std::string& base = "") override;
  virtual void SaveConfig(IniFile::Section* sec, const std::string& defdev = "",
                          const std::string& base = "") override;
  void ShowHostMessageBox();
  void Refresh();
  virtual ~UDPWrapper();
};
