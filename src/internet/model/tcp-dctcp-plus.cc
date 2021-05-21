#include "tcp-dctcp-plus.h"

#include "tcp-dctcp.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/tcp-socket-state.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpDctcpPlus");

NS_OBJECT_ENSURE_REGISTERED (TcpDctcpPlus);

TypeId TcpDctcpPlus::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpDctcpPlus")
    .SetParent<TcpDctcp> ()
    .AddConstructor<TcpDctcpPlus> ()
    //.SetGroupName ("Internet")
  ;
  return tid;
}



std::string TcpDctcpPlus::GetName () const
{
  return "TcpDctcpPlus";
}

TcpDctcpPlus::TcpDctcpPlus ()
  : TcpDctcp (),
    m_backoffTimeUnit(100),
    m_currState(TcpDctcpPlus::DCTCP_NORMAL),
    m_cWnd(1),
    m_divisorFactor(2),
    m_minCwnd(1),
    m_randomizeSendingTime(true),
    m_retrans(false),
    m_slowTime(MicroSeconds(0)),
    // TODO: update as we go
    m_thresholdT(MicroSeconds(2))
{
  NS_LOG_FUNCTION (this);
  m_backoffTimeGenerator = CreateObject<UniformRandomVariable> ();
}


bool TcpDctcpPlus::isCongested() {
  // TODO: figure this out
  return getCeState() || m_retrans; 
}

bool TcpDctcpPlus::isToDCTCPTimeInc() 
{
  switch (m_currState) {
    case DCTCP_NORMAL:
      return (m_cWnd == m_minCwnd && isCongested());
    default:
      return isCongested();
  }
}

bool TcpDctcpPlus::isToDCTCPTimeDes() 
{
  switch (m_currState) {
    case DCTCP_NORMAL:
      return false;
    case DCTCP_TIME_INC:
      return !isCongested();
    case DCTCP_TIME_DES:
      return (!isCongested() && m_slowTime > m_thresholdT);
    default: //Impossible
      return false;
  }
}

void TcpDctcpPlus::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt)
{
  m_retrans = tcb->m_highTxMark >= tcb->m_nextTxSequence;
  m_cWnd = tcb->m_cWnd;
  TcpDctcp::PktsAcked(tcb, segmentsAcked, rtt);
}

// Algorithm 1 from DCTCP+ Paper
// TODO: make changes to DCTCP operations based on these statuses
void TcpDctcpPlus::ndctcpStatusEvolution() 
{
  switch (m_currState) {
    case DCTCP_NORMAL:
      if (isToDCTCPTimeInc()) {
        m_currState = DCTCP_TIME_INC;
        m_slowTime = MicroSeconds(m_randomizeSendingTime ? m_backoffTimeGenerator->GetInteger(0, m_backoffTimeUnit) : m_backoffTimeUnit);
      }
      break;
    case DCTCP_TIME_INC:
      if (isToDCTCPTimeInc()) {
        m_slowTime += MicroSeconds(m_randomizeSendingTime ? m_backoffTimeGenerator->GetInteger(0, m_backoffTimeUnit) : m_backoffTimeUnit);
      } else if (isToDCTCPTimeDes()){
        m_currState = DCTCP_TIME_INC;
        m_slowTime = MicroSeconds(m_slowTime.ToInteger(ns3::Time::Unit::US) / m_divisorFactor);
      }
      break;
    case DCTCP_TIME_DES:
      if (isToDCTCPTimeInc()) {
        m_currState = DCTCP_TIME_DES;
        m_slowTime += MicroSeconds(m_randomizeSendingTime ? m_backoffTimeGenerator->GetInteger(0, m_backoffTimeUnit) : m_backoffTimeUnit);
      } else if (m_slowTime > m_thresholdT) {
        m_slowTime = MicroSeconds(m_slowTime.ToInteger(ns3::Time::Unit::US) / m_divisorFactor);
      } else {
        m_currState = DCTCP_NORMAL;
      }
      break;
  }
}
}