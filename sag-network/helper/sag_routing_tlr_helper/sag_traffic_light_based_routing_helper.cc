
#include "ns3/sag_traffic_light_based_routing_helper.h"
namespace ns3 {
    NS_OBJECT_ENSURE_REGISTERED( Sag_Traffic_Light_Based_Routing_Helper);
    TypeId Sag_Traffic_Light_Based_Routing_Helper::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::Sag_Traffic_Light_Based_Routing_Helper")
            .SetParent<SAGRoutingHelper>()
            .SetGroupName("SagRouting")
            .AddConstructor<Sag_Traffic_Light_Based_Routing_Helper>()
            ;
        return tid;
    }

    Sag_Traffic_Light_Based_Routing_Helper::Sag_Traffic_Light_Based_Routing_Helper()
    {
        m_factory.SetTypeId("ns3::tlr::Traffic_Light_Based_Routing");
        std::cout << "Set up Sag_Traffic_Light_Based_Routing_Helper" << std::endl;
    }

    Sag_Traffic_Light_Based_Routing_Helper::Sag_Traffic_Light_Based_Routing_Helper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes)
    {
        m_factory.SetTypeId("ns3::tlr::Traffic_Light_Based_Routing");
        std::cout << "Set up Sag_Traffic_Light_Based_Routing_Helper" << std::endl;

    }




}
