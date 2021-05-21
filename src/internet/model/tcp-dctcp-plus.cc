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
    m_backoffTimeUnit(100), // Microseconds
    m_currState(TcpDctcpPlus::DCTCP_NORMAL),
    m_cWnd(1),
    m_divisorFactor(2),
    m_minCwnd(1448),
    m_randomizeSendingTime(true),
    m_retrans(false),
    m_slowTime(MicroSeconds(1)),
    // TODO: update as we go
    m_thresholdT(MicroSeconds(2))
{
  NS_LOG_FUNCTION (this);
  LogComponentEnable("TcpDctcpPlus", LOG_LEVEL_DEBUG);
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
      NS_LOG_DEBUG("min cwnd vs actual");
      NS_LOG_DEBUG(m_minCwnd);
      NS_LOG_DEBUG(m_cWnd);
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
  ndctcpStatusEvolution();
  tcb->m_pacing = m_currState != DCTCP_NORMAL;

  if (tcb->m_pacing) {
    // TODO: revisit m_segsize: with vs without header...
    NS_LOG_DEBUG("NOT! Normal");
    NS_LOG_DEBUG(rtt + m_slowTime);
    NS_LOG_DEBUG(rtt);
    tcb->m_pacingRate = DataRate((tcb->m_segmentSize * 8)/(rtt + m_slowTime).GetSeconds());
  }
}

// Algorithm 1 from DCTCP+ Paper
// TODO: make changes to DCTCP operations based on these statuses
void TcpDctcpPlus::ndctcpStatusEvolution() 
{
  switch (m_currState) {
    case DCTCP_NORMAL:
      NS_LOG_DEBUG("Normal");
      if (isToDCTCPTimeInc()) {
        m_currState = DCTCP_TIME_INC;
        m_slowTime = MicroSeconds(m_randomizeSendingTime ? m_backoffTimeGenerator->GetInteger(1, m_backoffTimeUnit) : m_backoffTimeUnit);
      }
      break;
    case DCTCP_TIME_INC:
      NS_LOG_DEBUG("Inc");
      if (isToDCTCPTimeInc()) {
        m_slowTime += MicroSeconds(m_randomizeSendingTime ? m_backoffTimeGenerator->GetInteger(1, m_backoffTimeUnit) : m_backoffTimeUnit);
      } else if (isToDCTCPTimeDes()){
        m_currState = DCTCP_TIME_INC;
        m_slowTime = MicroSeconds((m_slowTime.ToInteger(ns3::Time::Unit::US) / m_divisorFactor) + 1);
      }
      break;
    case DCTCP_TIME_DES:
      NS_LOG_DEBUG("Des");
      if (isToDCTCPTimeInc()) {
        m_currState = DCTCP_TIME_DES;
        m_slowTime += MicroSeconds(m_randomizeSendingTime ? m_backoffTimeGenerator->GetInteger(1, m_backoffTimeUnit) : m_backoffTimeUnit);
      } else if (m_slowTime > m_thresholdT) {
        m_slowTime = MicroSeconds((m_slowTime.ToInteger(ns3::Time::Unit::US) / m_divisorFactor) + 1);
      } else {
        m_currState = DCTCP_NORMAL;
      }
      break;
  }
}
}