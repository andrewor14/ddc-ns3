#include "priority-tag.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (PriorityTag);

TypeId
PriorityTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PriorityTag")
                      .SetParent<Tag> ()
                      .AddConstructor<PriorityTag>()
                      ;
  return tid;
}

TypeId
PriorityTag::GetInstanceTypeId (void) const
{
  return GetTypeId();
}

PriorityTag::PriorityTag()
{
  m_flowid = 0;
  m_priority = 0;
  m_seq = 0;
}

void PriorityTag::SetId(uint32_t flowid)
{
  m_flowid = flowid;
}
uint32_t PriorityTag::GetId() const
{
  return m_flowid;
}

void PriorityTag::SetPriority(uint8_t priority)
{
  m_priority = priority;
}
uint8_t PriorityTag::GetPriority() const
{
  return m_priority;
}

uint32_t PriorityTag::GetSerializedSize (void) const
{
  return 9;
}

void PriorityTag::Serialize (TagBuffer i) const
{
  i.WriteU32(m_flowid);
  i.WriteU8(m_priority);
  i.WriteU32(m_seq);
}

void PriorityTag::Deserialize (TagBuffer i)
{
  m_flowid = i.ReadU32();
  m_priority = i.ReadU8();
  m_seq = i.ReadU32();
}

void PriorityTag::Print(std::ostream &os) const
{
  os << "Id: " << (uint32_t)m_flowid;
  os << ", Priority: " << (uint16_t)m_priority;
}

} //namespace ns3

