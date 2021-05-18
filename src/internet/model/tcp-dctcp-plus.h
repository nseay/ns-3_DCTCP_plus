

namespace ns3 {

/**
 * \ingroup tcp
 *
 * \brief An implementation of DCTCP. This model implements all of the
 * endpoint capabilities mentioned in the DCTCP SIGCOMM paper.
 */

  class TcpDctcpPlus : public TcpDctcp
  {
  public:
    TcpDctcpPlus();

  private:
    enum DctcpPlusState {
      DCTCP_NORMAL,
      DCTCP_TIME_INC,
      DCTCP_TIME_DES
    };

    Time m_backoffTimeUnit;
    Time m_slowTime;
    Time m_backoffTimeUnit;
    Ptr<UniformRandomVariable> m_backoffTimeGenerator;
    DctcpPlusState m_currState;
    bool m_randomizeSendingTime;
    int m_divisorFactor;

    bool isCongested();
    bool isToDCTCPTimeInc();
    bool isToDCTCPTimeDes();
    void regulateSendingTimeInterval();
  }
}