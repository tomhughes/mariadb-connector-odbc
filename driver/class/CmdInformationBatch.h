/************************************************************************************
   Copyright (C) 2022 MariaDB Corporation AB

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc.,
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
*************************************************************************************/


#ifndef _CMDINFORMATIONBATCH_H_
#define _CMDINFORMATIONBATCH_H_

#include "CmdInformation.h"

namespace mariadb
{

class CmdInformationBatch : public CmdInformation
{

  std::vector<int64_t>updateCounts;
  std::size_t expectedSize;
  int32_t autoIncrement;
  int64_t insertIdNumber ; /*0*/
  bool hasException= false;
  bool rewritten= false;

public:
  CmdInformationBatch(std::size_t expectedSize);
  ~CmdInformationBatch();

  void addErrorStat();
  void reset();
  void addResultSetStat();
  void addSuccessStat(int64_t updateCount);
  std::vector<int64_t>& getUpdateCounts();
  std::vector<int64_t>& getServerUpdateCounts();
  int64_t getUpdateCount();
  int32_t getCurrentStatNumber();
  bool moreResults();
  inline uint32_t hasMoreResults() override { return 0U; }
  bool isCurrentUpdateCount();
  void setRewrite(bool rewritten);
};

}
#endif
