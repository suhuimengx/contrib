/*
 * Copyright (c) 2023 NJU
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Xiaoyu Liu <xyliu0119@163.com>
 */

#ifndef PER_TAG_H
#define PER_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class PerTag : public Tag
{
public:
  static TypeId GetTypeId (void);

  PerTag ();
  void SetPer (double per);
  double GetPer (void) const;

  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer start) const;
  virtual void Deserialize (TagBuffer start);

private:
  double m_per;
};

} // namespace ns3

#endif /* PER_TAG_H */
