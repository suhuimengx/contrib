/*
 * Copyright (c) 2023 NJU
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Xiaoyu Liu <xyliu0119@163.com>
 */

#ifndef SAG_BURST_INFO_H
#define SAG_BURST_INFO_H

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <cstring>
#include <fstream>
#include <cinttypes>
#include <algorithm>
#include <regex>

namespace ns3 {

class SAGBurstInfoUdp
{
public:
    SAGBurstInfoUdp(
            int64_t burst_id,
            int64_t from_node_id,
            int64_t to_node_id,
            double target_rate_megabit_per_s,
            int64_t start_time_ns,
            int64_t duration_ns,
            std::string additional_parameters,
            std::string metadata
    );
    int64_t GetBurstId() const;
    int64_t GetFromNodeId();
    int64_t GetToNodeId();
    double GetTargetRateMegabitPerSec();
    int64_t GetStartTimeNs();
    int64_t GetDurationNs();
    std::string GetAdditionalParameters();
    std::string GetMetadata();
private:
    int64_t m_burst_id;
    int64_t m_from_node_id;
    int64_t m_to_node_id;
    double m_target_rate_megabit_per_s;
    int64_t m_start_time_ns;
    int64_t m_duration_ns;
    std::string m_additional_parameters;
    std::string m_metadata;
};

class SAGBurstInfoTcp
{
public:
	SAGBurstInfoTcp();
    SAGBurstInfoTcp(
            int64_t tcp_flow_id,
            int64_t from_node_id,
            int64_t to_node_id,
			double target_rate_megabit_per_s,
            int64_t start_time_ns,
			int64_t duration_ns,
            std::string additional_parameters,
            std::string metadata
    );
    int64_t GetTcpFlowId();
    int64_t GetFromNodeId();
    int64_t GetToNodeId();
    //int64_t GetSizeByte();
    double GetTargetRateMegabitPerSec();
    int64_t GetStartTimeNs();
    int64_t GetDurationNs();
    std::string GetAdditionalParameters();
    std::string GetMetadata();
private:
    int64_t m_tcp_flow_id;
    int64_t m_from_node_id;
    int64_t m_to_node_id;
    //int64_t m_size_byte;
    double m_target_rate_megabit_per_s;
    int64_t m_start_time_ns;
    int64_t m_duration_ns;
    std::string m_additional_parameters;
    std::string m_metadata;
};

class SAGBurstInfoScpsTp
{
public:
	SAGBurstInfoScpsTp();
    SAGBurstInfoScpsTp(
            int64_t scps_tp_flow_id,
            int64_t from_node_id,
            int64_t to_node_id,
			double target_rate_megabit_per_s,
            int64_t start_time_ns,
			int64_t duration_ns,
            std::string additional_parameters,
            std::string metadata
    );
    int64_t GetScpsTpFlowId();
    int64_t GetFromNodeId();
    int64_t GetToNodeId();
    //int64_t GetSizeByte();
    double GetTargetRateMegabitPerSec();
    int64_t GetStartTimeNs();
    int64_t GetDurationNs();
    std::string GetAdditionalParameters();
    std::string GetMetadata();
private:
    int64_t m_scps_tp_flow_id;
    int64_t m_from_node_id;
    int64_t m_to_node_id;
    //int64_t m_size_byte;
    double m_target_rate_megabit_per_s;
    int64_t m_start_time_ns;
    int64_t m_duration_ns;
    std::string m_additional_parameters;
    std::string m_metadata;
};

class SAGBurstInfoRtp
{
public:
    SAGBurstInfoRtp(
            int64_t burst_id,
            int64_t from_node_id,
            int64_t to_node_id,
            double target_rate_megabit_per_s,
            int64_t start_time_ns,
            int64_t duration_ns,
            std::string additional_parameters,
            std::string metadata
    );
    int64_t GetBurstId() const;
    int64_t GetFromNodeId();
    int64_t GetToNodeId();
    double GetTargetRateMegabitPerSec();
    int64_t GetStartTimeNs();
    int64_t GetDurationNs();
    std::string GetAdditionalParameters();
    std::string GetMetadata();
private:
    int64_t m_burst_id;
    int64_t m_from_node_id;
    int64_t m_to_node_id;
    double m_target_rate_megabit_per_s;
    int64_t m_start_time_ns;
    int64_t m_duration_ns;
    std::string m_additional_parameters;
    std::string m_metadata;
};


class SAGBurstInfo3GppHttp
{
public:
	SAGBurstInfo3GppHttp(
            int64_t burst_id,
            int64_t from_node_id,
            int64_t to_node_id,
            int64_t start_time_ns,
            int64_t duration_ns
    );
    int64_t GetBurstId() const;
    int64_t GetFromNodeId();
    int64_t GetToNodeId();
    int64_t GetStartTimeNs();
    int64_t GetDurationNs();
private:
    int64_t m_burst_id;
    int64_t m_from_node_id;
    int64_t m_to_node_id;
    int64_t m_start_time_ns;
    int64_t m_duration_ns;
};


class SAGBurstInfoFTP
{
public:
	SAGBurstInfoFTP(
            int64_t ftp_flow_id,
            int64_t from_node_id,
            int64_t to_node_id,
            int64_t size_byte,
            int64_t start_time_ns
    );
    int64_t GetFtpFlowId();
    int64_t GetFromNodeId();
    int64_t GetToNodeId();
    int64_t GetSizeByte();
    int64_t GetStartTimeNs();

private:
    int64_t m_ftp_flow_id;
    int64_t m_from_node_id;
    int64_t m_to_node_id;
    int64_t m_size_byte;
    int64_t m_start_time_ns;

};





}

#endif /* SAG_BURST_INFO_H */
