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

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <stdlib.h>
#include "ns3/exp-util.h"

namespace ns3 {

///将csv数据转换成向量
void GetData(std::string path, std::vector<std::vector<double>> &incom_data);
///获得乱序统计数据
void GetOutOfOrderStatistic(int loss_pack_num,std::vector<std::vector<double>> incom_data,std::vector<std::vector<double>> outcom_data,int &outoforder_pack_num,std::vector<int> &outoforder_pack_vec,std::vector<int> &outoforder_mark);
/// 获得延迟统计数据
void GetDelayStatistic(int loss_pack_num,int outoforder_pack_num,int out_pack_num,std::vector<std::vector<double>> incom_data,std::vector<std::vector<double>> outcom_data,std::vector<double> &delay_vec,std::vector<int> outoforder_mark,std::vector<double> &delay_shake_vec);
//输出txt文件
void WriteResult(std::vector<double> delay_shake_vec,std::vector<double> delay_vec,
		int loss_pack_num,double loss_ratio,int outoforder_pack_num,
		std::vector<int> outoforder_mark,std::vector<std::vector<double>> incom_data,
		std::vector<std::vector<double>> outcom_data, std::string dir, int id);
//获得整体统计数据
void GetStatistics(std::string incom_path,std::string outcom_path, std::string dir, int id);

}


