#ifndef SCPSTP_OPTIMIZER_H
#define SCPSTP_OPTIMIZER_H

#include "ns3/basic-simulation.h"
#include "ns3/ipv4-routing-helper.h"

namespace ns3 {

    class ScpsTpOptimizer
    {
    public:
        static void OptimizeBasic(Ptr<BasicSimulation> basicSimulation);
    private:
        static void Generic(std::string filename);
        Ptr<BasicSimulation> m_basicSimulation;
    };

} // namespace ns3

#endif /* SCPSTP_OPTIMIZER_H */
