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

#include "statistic.h"

namespace ns3 {

///将csv数据转换成向量
void GetData(std::string path, std::vector<std::vector<double>> &incom_data)
{
	std::ifstream incom_bur(path);
    std::vector<double> temp_line;
    std::string csv_line;
    while (getline(incom_bur, csv_line)) {
    	std::stringstream ss(csv_line); //声明一个输入流,将一行数据存入ss
        std::string temp_str;
        while(getline(ss,temp_str,','))
        {
/*            cout<<temp_str<<",";*/
            double temp_dob = std::stold(temp_str);
            //std::cout<<temp_dob<<std::endl;
            temp_line.push_back(temp_dob);
        }
        incom_data.push_back(temp_line);
        temp_line.clear();

    }
}

///获得乱序统计数据
void GetOutOfOrderStatistic(int loss_pack_num,std::vector<std::vector<double>> incom_data,std::vector<std::vector<double>> outcom_data,int &outoforder_pack_num,std::vector<int> &outoforder_pack_vec,std::vector<int> &outoforder_mark)
{
    outoforder_pack_num = 0;
    int out_pack_num = outcom_data.size();
    int in_pack_num = incom_data.size();

    if(loss_pack_num == 0)
    {
        // 两个向量第二行相减，得到一个列向量
        for(int i = 0; i< out_pack_num ; i++)
        {
            int temp = double(incom_data[i][1] - outcom_data[i][1]);
            outoforder_pack_vec.push_back(temp);
            if(temp != 0)
            {
                outoforder_mark.push_back(i);
            }
        }

        //统计乱序数据包个数
        for(int i = 0; i<out_pack_num ; i++)
        {
            if(outoforder_pack_vec[i]!= 0)
            {
                outoforder_pack_num += 1;
            }
        }

    }
    else
    {
    	std::vector<int> temp;
        for(int i = 0; i< out_pack_num ; i++)
        {
            bool loss = true;
            for(int a = 0; a<in_pack_num ;a++)
            {
                if(outcom_data[i][1] == incom_data[a][1])
                {
                    loss = false;
                }
            }
            if(loss == true)
            {
            	temp.push_back(i);
            }

        }
        for(int e = (int)temp.size()-1; e>= 0;e--)
         {
         	outcom_data.erase(outcom_data.begin()+temp[e]);
         }

        out_pack_num = (int)outcom_data.size();

        // 两个向量第二行相减，得到一个列向量
        for(int i = 0; i< out_pack_num ; i++)
        {
            int temp = double(incom_data[i][1] - outcom_data[i][1]);
            outoforder_pack_vec.push_back(temp);
            if(temp != 0)
            {
                outoforder_mark.push_back(i);
            }
        }

        //统计乱序数据包个数
        for(int i = 0; i<out_pack_num ; i++)
        {
            if(outoforder_pack_vec[i]!= 0)
            {
                outoforder_pack_num += 1;
            }
        }

    }
}

/// 获得延迟统计数据
void GetDelayStatistic(int loss_pack_num,int outoforder_pack_num,int out_pack_num,std::vector<std::vector<double>> incom_data,std::vector<std::vector<double>> outcom_data,std::vector<double> &delay_vec,std::vector<int> outoforder_mark,std::vector<double> &delay_shake_vec)
{
    int in_pack_num = incom_data.size();
    if (loss_pack_num == 0)
    {
        if (outoforder_pack_num == 0)//如果无乱序
        {
            for (int i = 0; i < out_pack_num ; i++)
            {
                double delay = double(incom_data[i][2] - outcom_data[i][2]);
                //std::cout << delay << std::endl;
                delay_vec.push_back(delay);
            }
        }
        else//如果有乱序
        {
            for (int i = 0; i < out_pack_num ; i++)
            {
                bool exist = false;
                for (int c = 0; c < (int)outoforder_mark.size(); c++)
                {
                    if (i == outoforder_mark[c])
                    {
                        exist = true;
                    }
                }
                if (exist == false)
                {
                    double delay = double(incom_data[i][2] - outcom_data[i][2]);
                    //std::cout << delay << std::endl;
                    delay_vec.push_back(delay);
                }
                else
                {
                    for (int d = 0; d < out_pack_num ; d++)
                    {
                        if (incom_data[d][1] == outcom_data[i][1])
                        {
                            double delay = double(incom_data[d][2] - outcom_data[i][2]);
                            //std::cout << delay << std::endl;
                            delay_vec.push_back(delay);
                        }
                    }
                }
            }
        }
    }
    else
    {
    	std::vector<int> temp;
    	/*std::cout<<outcom_data.size()<<std::endl;*/
        for(int i = 0; i< out_pack_num ; i++)
        {
            bool loss = true;
            for(int a = 0; a<in_pack_num ;a++)
            {
                if(outcom_data[i][1] == incom_data[a][1])
                {
                    loss = false;
                }
            }
            if(loss == true)
            {
            	temp.push_back(i);
            }
        }

        for(int e = (int)temp.size()-1; e>=0 ;e--)
         {
         	outcom_data.erase(outcom_data.begin()+temp[e]);
         }

        out_pack_num = (int)outcom_data.size();

        if (outoforder_pack_num == 0)//如果无乱序
        {
            for (int i = 0; i < out_pack_num ; i++)
            {
                double delay = double(incom_data[i][2] - outcom_data[i][2]);
                delay_vec.push_back(delay);
            }
        }
        else//如果有乱序
        {
            for (int i = 0; i < out_pack_num ; i++)
            {
                bool exist = false;
                for (int c = 0; c < (int)outoforder_mark.size(); c++)
                {
                    if (i == outoforder_mark[c])
                    {
                        exist = true;
                    }
                }
                if (exist == false)
                {
                    double delay = double(incom_data[i][2] - outcom_data[i][2]);
                    //std::cout << delay << std::endl;
                    delay_vec.push_back(delay);
                }
                else
                {
                    for (int d = 0; d < out_pack_num ; d++)
                    {
                        if (incom_data[d][1] == outcom_data[i][1])
                        {
                            double delay = double(incom_data[d][2] - outcom_data[i][2]);
                          //  std::cout << delay << std::endl;
                            delay_vec.push_back(delay);
                        }
                    }
                }
            }
        }
    }

    //时延抖动
    for(int i = 0; i< (int)delay_vec.size()-1;i++)
    {
        double shake = delay_vec[i+1] - delay_vec[i];
        delay_shake_vec.push_back(shake);
    }
}

void WriteResult(std::vector<double> delay_shake_vec,std::vector<double> delay_vec,
		int loss_pack_num,double loss_ratio,int outoforder_pack_num,
		std::vector<int> outoforder_mark,std::vector<std::vector<double>> incom_data,
		std::vector<std::vector<double>> outcom_data, std::string dir, int id)
{
    int out_pack_num = outcom_data.size();
    int in_pack_num = incom_data.size();

    std::ofstream outFile;
    outFile.open(dir +"/burst_"+std::to_string(id)+"_packetLoss.csv", std::ios::out);
    outFile.setf(std::ios::fixed);
    outFile.precision(0);

    ///丢包情况
    if(loss_pack_num !=0 )
    {
/*        outFile<<"The number of packet loss is "<<loss_pack_num<<", the loss ratio is "<<loss_ratio<<"."<<endl;
        outFile<<"The lossed packets are ";*/
        for(int i = 0; i< out_pack_num ; i++)
        {
            bool loss = true;
            for(int a = 0; a< in_pack_num ;a++)
            {
                if(outcom_data[i][1] == incom_data[a][1])
                {
                    loss = false;
                    //std::cout<<outcom_data[i][1]<<std::endl;
                    break;
                }
            }
            if(loss == true)
            {
                outFile<<outcom_data[i][0]<<", "<<outcom_data[i][1]<<", "<<outcom_data[i][2];
                if(i != out_pack_num)
                {
                    outFile<<std::endl;
                }
                else
                {

                }
            }
        }
    }

    else
    {
        outFile<<"The number of packet loss is "<<loss_pack_num<<"."<<std::endl;
    }
    /*std::cout<<loss_ratio;*/

    ///乱序情况
    outFile.close();
    outFile.open(dir +"/burst_"+std::to_string(id)+"_outOfOrder.csv", std::ios::out);
    outFile.setf(std::ios::fixed);
    outFile.precision(0);
    if(outoforder_pack_num == 0)
    {
        outFile<<"The number of packets which are out of order is "<<outoforder_pack_num<<"."<<std::endl;
    }

    else
    {
/*        outFile<<"The number of packets which are out of order is "<<outoforder_pack_num<<", the ratio is "<<outoforder_ratio<<"."<<endl;
        outFile<<"The packets which are out of order are ";*/
        for (int i = 0; i< (int)outoforder_mark.size(); i++)
        {
        	auto j = outoforder_mark[i];
            outFile<<outcom_data[j][0]<<", "<<outcom_data[j][1]<<", "<<outcom_data[j][2];;
            if(i != (int)outoforder_mark.size())
            {
                outFile<<std::endl;
            }
            else
            {

            }
        }
    }

    ///延迟情况
    outFile.close();
    outFile.open(dir +"/burst_"+std::to_string(id)+"_delay.csv", std::ios::out);
    outFile.setf(std::ios::fixed);
    outFile.precision(0);

	std::vector<int> temp;
    for(int i = 0; i< out_pack_num ; i++)
    {
        bool loss = true;
        for(int a = 0; a<in_pack_num ;a++)
        {
            if(outcom_data[i][1] == incom_data[a][1])
            {
                loss = false;
            }
        }
        if(loss == true)
        {
        	temp.push_back(i);
        }
    }

    for(int e = (int)temp.size()-1; e>=0 ;e--)
     {
     	outcom_data.erase(outcom_data.begin()+temp[e]);
     }
    out_pack_num = (int)outcom_data.size();

    for(int i = 0; i<(int)delay_vec.size();i++)
    {
        outFile<<outcom_data[i][0]<<", "<<outcom_data[i][2]<<", "<<delay_vec[i];
        if(i != (int)delay_vec.size()-1)
        {
            outFile<<std::endl;
        }
        else
        {
        }
    }

    outFile.close();
    outFile.open(dir +"/burst_"+std::to_string(id)+"_delayJitter.csv", std::ios::out);
    outFile.setf(std::ios::fixed);
    outFile.precision(0);
    for(int i = 0; i<(int)delay_shake_vec.size();i++)
    {
        outFile<<outcom_data[i+1][0]<<", "<<outcom_data[i+1][2]<<", "<<delay_shake_vec[i];
        if(i != (int)delay_vec.size())
        {
            outFile<<std::endl;
        }
        else
        {
        }
    }




}

void GetStatistics(std::string incom_path,std::string outcom_path, std::string dir, int id)
{
	std::string newDir = dir + "/burst_" + std::to_string(id) + "_qos";
	dir = newDir;
	remove_dir_if_exists(dir);
	mkdir_if_not_exists(dir);

	std::vector<std::vector<double>> incom_data;// 二位向量，横向量是行
	std::vector<std::vector<double>> outcom_data;//

    GetData(incom_path,incom_data);
    GetData(outcom_path,outcom_data);

    int out_pack_num = (int)outcom_data.size(); //发包个数
    int in_pack_num = (int)incom_data.size(); //收包个数

    ///统计丢包
    int loss_pack_num = out_pack_num - in_pack_num; //丢包个数
    double loss_ratio = loss_pack_num / out_pack_num; //丢包率

    ///统计乱序
    std::vector<int> outoforder_pack_vec; //matlab中的diff
    int outoforder_pack_num = 0;//乱序的数目
    std::vector<int> outoforder_mark;//乱序数据包的角标

    GetOutOfOrderStatistic(loss_pack_num,incom_data,outcom_data,outoforder_pack_num,outoforder_pack_vec,outoforder_mark);

    ///统计时延及抖动
    std::vector<double> delay_vec;//各数据包到达时延
    std::vector<double> delay_shake_vec;//数据包时延抖动
    GetDelayStatistic(loss_pack_num,outoforder_pack_num,out_pack_num, incom_data, outcom_data,delay_vec, outoforder_mark,delay_shake_vec);

    WriteResult(delay_shake_vec,delay_vec,loss_pack_num,loss_ratio,outoforder_pack_num,outoforder_mark, incom_data,outcom_data, dir, id);


}
}


