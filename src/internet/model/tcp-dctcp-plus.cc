#include "tcp-dctcp-plus.h"

#include "tcp-dctcp.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/tcp-socket-state.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpDctcpPlus");

NS_OBJECT_ENSURE_REGISTERED (TcpDctcpPlus);

TypeId TcpDctcp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpDctcpPlus")
    .SetParent<TcpDctcp> ()
    .AddConstructor<TcpDctcpPlus> ()
    .SetGroupName ("Internet")
    .AddAttribute ("DctcpShiftG",
                   "Parameter G for updating dctcp_alpha",
                   DoubleValue (0.0625),
                   MakeDoubleAccessor (&TcpDctcpPlus::m_g),
                   MakeDoubleChecker<double> (0, 1))
    .AddAttribute ("DctcpAlphaOnInit",
                   "Initial alpha value",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&TcpDctcpPlus::InitializeDctcpAlpha),
                   MakeDoubleChecker<double> (0, 1))
    .AddAttribute ("UseEct0",
                   "Use ECT(0) for ECN codepoint, if false use ECT(1)",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TcpDctcpPlus::m_useEct0),
                   MakeBooleanChecker ())
    .AddTraceSource ("CongestionEstimate",
                     "Update sender-side congestion estimate state",
                     MakeTraceSourceAccessor (&TcpDctcpPlus::m_traceCongestionEstimate),
                     "ns3::TcpDctcp::CongestionEstimateTracedCallback")
  ;
  return tid;
}



std::string TcpDctcp::GetName () const
{
  return "TcpDctcpPlus";
}

TcpDctcpPlus::TcpDctcpPlus ()
  : TcpDctcp (),
    m_backoffTimeUnit(MicroSeconds(100)),
    m_slowTime(MicroSeconds(0)),
    m_backoffTimeUnit(MicroSeconds(1)),
    m_randomizeSendingTime(true),
    m_currState(TcpDctcpPlus::DCTCP_NORMAL),
    m_divisorFactor(2),
    // TODO: define this
    m_thresholdT()
{
  NS_LOG_FUNCTION (this);
  m_backoffTimeGenerator = CreateObject<UniformRandomVariable> ();
}


bool TcpDctcpPlus::isCongested() {
  // TODO: figure this out
  return ECE == 1 || retrans; 
}

bool TcpDctcpPlus::isToDCTCPTimeInc() 
{
  switch m_currState {
    case DCTCP_NORMAL:
      return (cwnd == minCwnd && isCongested())
    case DCTCP_TIME_INC:
    case DCTCP_TIME_DES:
      return isCongested()
  }
}

bool TcpDctcpPlus::isToDCTCPTimeDes() 
{
  switch m_currState {
    case DCTCP_NORMAL:
      return false;
    case DCTCP_TIME_INC:
      return !isCongested();
    case DCTCP_TIME_DES:
      return (!isCongested() && m_slowTime > m_thresholdT);
  }
}

// Algorithm 1 from DCTCP+ Paper
// TODO: make changes to DCTCP operations based on these statuses
void TcpDctcpPlus::ndctcpStatusEvolution() 
{
  switch m_currState {
    case DCTCP_NORMAL:
      if (isToDCTCPTimeInc()) {
        m_currState = DCTCP_TIME_INC
        m_slowTime = m_randomizeSendingTime ? m_backoffTimeGenerator.getInteger(0, m_backoffTimeUnit) : m_backoffTimeUnit;
      }
      break;
    case DCTCP_TIME_INC:
      if (isToDCTCPTimeInc()) {
        m_slowTime += (m_randomizeSendingTime ? m_backoffTimeGenerator.getInteger(0, m_backoffTimeUnit), m_backoffTimeUnit);
      } else if (isToDCTCPTimeDes()){
        m_currState = DCTCP_TIME_INC;
        m_slowTime /= m_divisorFactor;
      }
      break;
    case DCTCP_TIME_DES:
      if (isToDCTCPTimeInc()) {
        m_currState = DCTCP_TIME_DES;
        m_slowTime += (m_randomizeSendingTime ? m_backoffTimeGenerator.getInteger(0, m_backoffTimeUnit), m_backoffTimeUnit);
      } else if (m_slowTime > m_thresholdT) {
        m_slowTime /= m_divisorFactor;
      } else {
        m_currState = DCTCP_NORMAL
      }
      break;
  }
}