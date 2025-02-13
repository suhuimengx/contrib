#ifndef SIMULATOR_CONTRIB_STATISTIC_MODEL_TRAJECTORY_H_
#define SIMULATOR_CONTRIB_STATISTIC_MODEL_TRAJECTORY_H_

#include <vector>
#include <cfloat>
#include "ns3/node-container.h"
#include "ns3/mobility-model.h"
#include "ns3/basic-simulation.h"
#include "ns3/topology-satellite-network.h"

namespace ns3 {

struct sat_position {
    int id;
	int time;
	float positionx;
	float positiony;
	float positionz;
	float latitude;
	float longitude;
	float altitude;
};


struct gs_position {
    int id;
    std::string name;
	float positionx;
	float positiony;
	float positionz;
};

//struct Adj {
//    int sat1;
//    int sat2;
//};

struct duration_time
{
	JulianDate start;
	JulianDate end;
};

struct Handover{
	int time;
	int sat1;
	int sat2;
	bool overlap;
};

struct RealHandover
{
	int sat1;
	int sat2;
	duration_time duration[1000];
	int size;
};


/**
 * \ingroup Statistics
 *
 * \brief Trajectory
 */
class Trajectory
{
public:
	// constructor
	Trajectory (Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology);
	virtual ~Trajectory ();

	void SetBasicSimHandle(Ptr<BasicSimulation> basicSimulation);

	void ReadTimeAxleJson(Ptr<BasicSimulation> basicSimulation);

	void ReadSatPosition(int i);

	void WriteSatPositionJson();

	void WriteGSPositionJson();

	void WriteAirCraftsPositionJson();

	void WriteTimeAxleJson();

	void WriteAdjJson();

	void WriteHandoverJson();



protected:
	Ptr<BasicSimulation> m_basicSimulation;
	Ptr<TopologySatelliteNetwork> m_topology;

	std::vector<sat_position> m_satp;
	std::vector<gs_position> m_gsp;
	//std::vector<Adj> m_adj_matrix;
	std::vector<Handover> m_ho_matrix;
	double m_time_IntervalNs;
	double m_time_end;


};


}
#endif /* SIMULATOR_CONTRIB_STATISTIC_MODEL_ACCESS_H_ */
