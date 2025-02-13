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

#include "ns3/sag_burst_info.h"

namespace ns3 {

// SAG UDP BURST
SAGBurstInfoUdp::SAGBurstInfoUdp(
        int64_t burst_id,
        int64_t from_node_id,
        int64_t to_node_id,
        double target_rate_megabit_per_s,
        int64_t start_time_ns,
        int64_t duration_ns,
        std::string additional_parameters,
        std::string metadata
) 
{
    m_burst_id = burst_id;
    m_from_node_id = from_node_id;
    m_to_node_id = to_node_id;
    m_target_rate_megabit_per_s = target_rate_megabit_per_s;
    m_start_time_ns = start_time_ns;
    m_duration_ns = duration_ns;
    m_additional_parameters = additional_parameters;
    m_metadata = metadata;
}

int64_t SAGBurstInfoUdp::GetBurstId() const 
{
    return m_burst_id;
}

int64_t SAGBurstInfoUdp::GetFromNodeId() 
{
    return m_from_node_id;
}

int64_t SAGBurstInfoUdp::GetToNodeId() 
{
    return m_to_node_id;
}

double SAGBurstInfoUdp::GetTargetRateMegabitPerSec() 
{
    return m_target_rate_megabit_per_s;
}

int64_t SAGBurstInfoUdp::GetStartTimeNs() 
{
    return m_start_time_ns;
}

int64_t SAGBurstInfoUdp::GetDurationNs() 
{
    return m_duration_ns;
}

std::string SAGBurstInfoUdp::GetAdditionalParameters() 
{
    return m_additional_parameters;
}

std::string SAGBurstInfoUdp::GetMetadata() 
{
    return m_metadata;
}





//SAG TCP BURST
SAGBurstInfoTcp::SAGBurstInfoTcp(){
    m_tcp_flow_id = 0;
    m_from_node_id = 0;
    m_to_node_id = 0;
    //m_size_byte = size_byte;
    m_target_rate_megabit_per_s = 0;
    m_start_time_ns = 0;
    m_duration_ns = 0;
    m_additional_parameters = "";
    m_metadata = "";
}
SAGBurstInfoTcp::SAGBurstInfoTcp(
        int64_t tcp_flow_id,
        int64_t from_node_id,
        int64_t to_node_id,
		double target_rate_megabit_per_s,
        int64_t start_time_ns,
		int64_t duration_ns,
        std::string additional_parameters,
        std::string metadata) {
    m_tcp_flow_id = tcp_flow_id;
    m_from_node_id = from_node_id;
    m_to_node_id = to_node_id;
    //m_size_byte = size_byte;
    m_target_rate_megabit_per_s = target_rate_megabit_per_s;
    m_start_time_ns = start_time_ns;
    m_duration_ns = duration_ns;
    m_additional_parameters = additional_parameters;
    m_metadata = metadata;
}

int64_t SAGBurstInfoTcp::GetTcpFlowId() {
    return m_tcp_flow_id;
}

int64_t SAGBurstInfoTcp::GetFromNodeId() {
    return m_from_node_id;
}

int64_t SAGBurstInfoTcp::GetToNodeId() {
    return m_to_node_id;
}

//int64_t SAGBurstInfoTcp::GetSizeByte() {
//    return m_size_byte;
//}
double SAGBurstInfoTcp::GetTargetRateMegabitPerSec()
{
    return m_target_rate_megabit_per_s;
}

int64_t SAGBurstInfoTcp::GetStartTimeNs() {
    return m_start_time_ns;
}

int64_t SAGBurstInfoTcp::GetDurationNs(){
	return m_duration_ns;
}

std::string SAGBurstInfoTcp::GetAdditionalParameters() {
    return m_additional_parameters;
}

std::string SAGBurstInfoTcp::GetMetadata() {
    return m_metadata;
}

//SAG SCPSTP BURST
SAGBurstInfoScpsTp::SAGBurstInfoScpsTp(){
    m_scps_tp_flow_id = 0;
    m_from_node_id = 0;
    m_to_node_id = 0;
    //m_size_byte = size_byte;
    m_target_rate_megabit_per_s = 0;
    m_start_time_ns = 0;
    m_duration_ns = 0;
    m_additional_parameters = "";
    m_metadata = "";
}
SAGBurstInfoScpsTp::SAGBurstInfoScpsTp(
        int64_t scps_tp_flow_id,
        int64_t from_node_id,
        int64_t to_node_id,
		double target_rate_megabit_per_s,
        int64_t start_time_ns,
		int64_t duration_ns,
        std::string additional_parameters,
        std::string metadata) {
    m_scps_tp_flow_id = scps_tp_flow_id;
    m_from_node_id = from_node_id;
    m_to_node_id = to_node_id;
    //m_size_byte = size_byte;
    m_target_rate_megabit_per_s = target_rate_megabit_per_s;
    m_start_time_ns = start_time_ns;
    m_duration_ns = duration_ns;
    m_additional_parameters = additional_parameters;
    m_metadata = metadata;
}

int64_t SAGBurstInfoScpsTp::GetScpsTpFlowId() {
    return m_scps_tp_flow_id;
}

int64_t SAGBurstInfoScpsTp::GetFromNodeId() {
    return m_from_node_id;
}

int64_t SAGBurstInfoScpsTp::GetToNodeId() {
    return m_to_node_id;
}

//int64_t SAGBurstInfoScpsTp::GetSizeByte() {
//    return m_size_byte;
//}
double SAGBurstInfoScpsTp::GetTargetRateMegabitPerSec()
{
    return m_target_rate_megabit_per_s;
}

int64_t SAGBurstInfoScpsTp::GetStartTimeNs() {
    return m_start_time_ns;
}

int64_t SAGBurstInfoScpsTp::GetDurationNs(){
	return m_duration_ns;
}

std::string SAGBurstInfoScpsTp::GetAdditionalParameters() {
    return m_additional_parameters;
}

std::string SAGBurstInfoScpsTp::GetMetadata() {
    return m_metadata;
}

// SAG RTP BURST
SAGBurstInfoRtp::SAGBurstInfoRtp(
        int64_t burst_id,
        int64_t from_node_id,
        int64_t to_node_id,
        double target_rate_megabit_per_s,
        int64_t start_time_ns,
        int64_t duration_ns,
        std::string additional_parameters,
        std::string metadata
)
{
    m_burst_id = burst_id;
    m_from_node_id = from_node_id;
    m_to_node_id = to_node_id;
    m_target_rate_megabit_per_s = target_rate_megabit_per_s;
    m_start_time_ns = start_time_ns;
    m_duration_ns = duration_ns;
    m_additional_parameters = additional_parameters;
    m_metadata = metadata;
}

int64_t SAGBurstInfoRtp::GetBurstId() const
{
    return m_burst_id;
}

int64_t SAGBurstInfoRtp::GetFromNodeId()
{
    return m_from_node_id;
}

int64_t SAGBurstInfoRtp::GetToNodeId()
{
    return m_to_node_id;
}

double SAGBurstInfoRtp::GetTargetRateMegabitPerSec()
{
    return m_target_rate_megabit_per_s;
}

int64_t SAGBurstInfoRtp::GetStartTimeNs()
{
    return m_start_time_ns;
}

int64_t SAGBurstInfoRtp::GetDurationNs()
{
    return m_duration_ns;
}

std::string SAGBurstInfoRtp::GetAdditionalParameters()
{
    return m_additional_parameters;
}

std::string SAGBurstInfoRtp::GetMetadata()
{
    return m_metadata;
}


// SAG 3Gpp Http BURST
SAGBurstInfo3GppHttp::SAGBurstInfo3GppHttp(
        int64_t burst_id,
        int64_t from_node_id,
        int64_t to_node_id,
        int64_t start_time_ns,
        int64_t duration_ns
)
{
    m_burst_id = burst_id;
    m_from_node_id = from_node_id;
    m_to_node_id = to_node_id;
    m_start_time_ns = start_time_ns;
    m_duration_ns = duration_ns;
}

int64_t SAGBurstInfo3GppHttp::GetBurstId() const
{
    return m_burst_id;
}

int64_t SAGBurstInfo3GppHttp::GetFromNodeId()
{
    return m_from_node_id;
}

int64_t SAGBurstInfo3GppHttp::GetToNodeId()
{
    return m_to_node_id;
}

int64_t SAGBurstInfo3GppHttp::GetStartTimeNs()
{
    return m_start_time_ns;
}

int64_t SAGBurstInfo3GppHttp::GetDurationNs()
{
    return m_duration_ns;
}


//SAG TCP BURST
SAGBurstInfoFTP::SAGBurstInfoFTP(
        int64_t ftp_flow_id,
        int64_t from_node_id,
        int64_t to_node_id,
        int64_t size_byte,
        int64_t start_time_ns
) {
    m_ftp_flow_id = ftp_flow_id;
    m_from_node_id = from_node_id;
    m_to_node_id = to_node_id;
    m_size_byte = size_byte;
    m_start_time_ns = start_time_ns;
}

int64_t SAGBurstInfoFTP::GetFtpFlowId() {
    return m_ftp_flow_id;
}

int64_t SAGBurstInfoFTP::GetFromNodeId() {
    return m_from_node_id;
}

int64_t SAGBurstInfoFTP::GetToNodeId() {
    return m_to_node_id;
}

int64_t SAGBurstInfoFTP::GetSizeByte() {
    return m_size_byte;
}

int64_t SAGBurstInfoFTP::GetStartTimeNs() {
    return m_start_time_ns;
}





}
