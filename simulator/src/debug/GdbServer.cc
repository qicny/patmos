//
//  This file is part of the Patmos Simulator.
//  The Patmos Simulator is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  The Patmos Simulator is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with the Patmos Simulator. If not, see <http://www.gnu.org/licenses/>.
//
//
//  gdb server implementation, used for gdb debugging.
//

#include "debug/GdbServer.h"

namespace patmos
{

  //////////////////////////////////////////////////////////////////
  // Exceptions
  //////////////////////////////////////////////////////////////////

  GdbConnectionFailedException::GdbConnectionFailedException()
    : m_whatMessage("Error: GdbServer: Could not get a connection to the gdb client.")
  {
  }
  GdbConnectionFailedException::~GdbConnectionFailedException() throw()
  {
  }
  const char* GdbConnectionFailedException::what() const throw()
  {
    return m_whatMessage.c_str();
  }
  

  //////////////////////////////////////////////////////////////////
  // GdbServer implementation
  //////////////////////////////////////////////////////////////////
  
  GdbServer::GdbServer(DebugInterface &debugInterface)
    : m_debugInterface(debugInterface)
  {
  }

  void GdbServer::Init()
  {
    throw GdbConnectionFailedException();
  }

  // Implement DebugClient
  void GdbServer::BreakpointHit(const Breakpoint &bp)
  {
  }
}