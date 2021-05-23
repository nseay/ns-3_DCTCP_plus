#ifndef TCP_DCTCP_PLUS_H
#define TCP_DCTCP_PLUS_H

#include "ns3/tcp-dctcp.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

/**
 * \ingroup tcp
 *
 * \brief An implementation of DCTCP Plus. This model implements all of the
 * endpoint capabilities mentioned in the slowing little quickens more paper.
 */

  class TcpDctcpPlus : public TcpDctcp
  {
  public:
    TcpDctcpPlus();
    static TypeId GetTypeId (void);
    virtual void Init (Ptr<TcpSocketState> tcb);
    
  private:
    enum DctcpPlusState {
      DCTCP_NORMAL,
      DCTCP_TIME_INC,
      DCTCP_TIME_DES
    };

    uint32_t m_backoffTimeUnit;
    Ptr<UniformRandomVariable> m_backoffTimeGenerator;
    DctcpPlusState m_currState;
    uint32_t m_cWnd;
    uint32_t m_divisorFactor;
    DataRate m_initialPacingRate;
    uint32_t m_minCwnd;
    bool m_randomizeSendingTime;
    bool m_retrans;
    Time m_slowTime;
    Time m_thresholdT;

    std::string GetName () const;
    bool isCongested();
    bool isToDCTCPTimeInc();
    bool isToDCTCPTimeDes();
    void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt);
    void ndctcpStatusEvolution();
    void regulateSendingTimeInterval();
  };
}

#endif