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
    .AddConstructor<TcpDctcpPlus> ();
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
    m_cWnd(1448),
    m_divisorFactor(2),
    m_initialPacingRate(DataRate(0)),
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

void TcpDctcpPlus::Init (Ptr<TcpSocketState> tcb)
{
  TcpDctcp::Init(tcb);
  m_initialPacingRate = tcb->m_pacingRate;
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
  TcpDctcp::PktsAcked(tcb, segmentsAcked, rtt);
  if (!rtt.IsZero()) {
    m_retrans = tcb->m_highTxMark >= tcb->m_nextTxSequence;
    m_cWnd = tcb->m_cWnd;
    ndctcpStatusEvolution();
    tcb->m_pacing = true;
    if (m_currState == DCTCP_NORMAL) {
      // switch back to initial rate as we've returned to normal
      tcb->m_pacingRate = m_initialPacingRate;
    }
    // tcb->m_pacing = m_currState != DCTCP_NORMAL;

    if (tcb->m_pacing && m_currState !=DCTCP_NORMAL) {
      // TODO: revisit m_segsize: with vs without header...
      // NS_LOG_DEBUG("NOT! Normal");
      // NS_LOG_DEBUG(rtt + m_slowTime);
      // NS_LOG_DEBUG(rtt);
      uint64_t new_rate = uint64_t(1000000 * (tcb->m_segmentSize * 8.0)/double((rtt + m_slowTime).GetMicroSeconds()));
      new_rate = new_rate == 0 ? 1 : new_rate;
      // NS_LOG_DEBUG(new_rate);
      tcb->m_pacingRate = DataRate(new_rate);
    }
  }
}

// Algorithm 1 from DCTCP+ Paper
void TcpDctcpPlus::ndctcpStatusEvolution() 
{
  switch (m_currState) {
    case DCTCP_NORMAL:
      // NS_LOG_DEBUG("Normal");
      if (isToDCTCPTimeInc()) {
        m_currState = DCTCP_TIME_INC;
        m_slowTime = MicroSeconds(m_randomizeSendingTime ? m_backoffTimeGenerator->GetInteger(1, m_backoffTimeUnit) : m_backoffTimeUnit);
      }
      break;
    case DCTCP_TIME_INC:
      // NS_LOG_DEBUG("Inc");
      if (isToDCTCPTimeInc()) {
        m_slowTime += MicroSeconds(m_randomizeSendingTime ? m_backoffTimeGenerator->GetInteger(1, m_backoffTimeUnit) : m_backoffTimeUnit);
      } else if (isToDCTCPTimeDes()){
        m_currState = DCTCP_TIME_INC;
        m_slowTime = MicroSeconds((m_slowTime.ToInteger(ns3::Time::Unit::US) / m_divisorFactor) + 1);
      }
      break;
    case DCTCP_TIME_DES:
      // NS_LOG_DEBUG("Des");
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