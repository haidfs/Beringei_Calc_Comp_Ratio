/**
* Copyright (c) 2016-present, Facebook, Inc.
* All rights reserved.
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "beringei/client/BeringeiClient.h"
#include "beringei/plugins/BeringeiConfigurationAdapter.h"

#include <chrono>
#include <memory>
#include <set>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>

#include <stdio.h>
#include <string.h>

#include <folly/Conv.h>
#include <folly/init/Init.h>

using namespace facebook;

DECLARE_string(beringei_configuration_path);
typedef void (*func)(void);

std::vector <float> getColFromFile(std::string path, int colNumber){
        std::ifstream myfile(path);

        if (!myfile.is_open()){
                std::cout << "Unable to open myfile" << std::endl;
                system("pause");
                exit(1);

        }

        std::vector<std::string> vec;
        std::string temp;


        while (getline(myfile, temp))                    //利用getline（）读取每一行，并按照行为单位放入到std::vector
        {
                vec.push_back(temp);
        }

        std::vector <float> radius;


        for (auto it = vec.begin(); it != vec.end(); it++)
        {
                std::istringstream is(*it);                    //用每一行的数据初始化一个字符串输入流；
                std::string s;
                int pam = 0;

                while (is >> s)                          //以空格为界，把istd::stringstream中数据取出放入到依次s中
                {
                        if (pam == colNumber)                       //获取第六列的数据

                        {
                                float r = atof(s.c_str());     //做数据类型转换，将std::string类型转换成float
                                radius.push_back(r);
                                //std::cout << r << std::endl;
                        }

                        pam++;

                }
        }
        return radius;

}

void perforMainFork(int iterNumber, std::string keyName, auto colNumber){
  auto beringeiConfig =
      std::make_shared<gorilla::BeringeiConfigurationAdapter>(true);
    auto beringeiClient =
      std::make_shared<gorilla::BeringeiClient>(beringeiConfig, 10, 2, true);
    int shardCount = beringeiClient->getNumShardsFromWriteClient();
    LOG(INFO) << "Config knows about these write services: ";
    for (const auto& wservice : beringeiConfig->getWriteServices()) {
    LOG(INFO) << "  " << wservice;
    }
    LOG(INFO) << "Beringei has " << shardCount << " shards";

    if (shardCount == 0) {
      LOG(FATAL) << "Shard count can't be zero, though.";
    } 
  //  for (auto item=iterNumber; item >= 1; item--){
    std::vector <float> radius = getColFromFile("/opt/run/2/wlan_kpi_2_43fields.csv", colNumber);
    for (auto item = radius.begin(); item != radius.end(); item++){
	std::vector<gorilla::DataPoint> dps;
    dps.emplace_back();
    dps.back().key.key = keyName;
    dps.back().key.shardId =
    std::hash<std::string>()(dps.back().key.key) % shardCount;
    dps.back().value.unixTime =
        std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
    dps.back().value.value = folly::to<double>(*item);

    auto pushedPoints = beringeiClient->putDataPoints(dps);

    if (!pushedPoints) {
      LOG(ERROR) << "Failed to perform the put!";
    }

    beringeiClient->flushQueue();
	}
	//return 0;
}

void f1(void) { perforMainFork(100000, "key1", 10); }
void f2(void) { perforMainFork(100000, "key2", 6); }
//void f3(void) { perforMainFork(100000, "key3", 10); }

//__attribute__((section(".init_array")))  int (* p_perforMainFork1)(int) = &perforMainFork1;

int main(int argc, char** argv) {
  gflags::SetUsageMessage("[<options>] <key> <value> [<timestamp>]");
  folly::init(&argc, &argv, true);
  if (argc == 1){
	//perforMainFork(100000, "key1", 10);
	
  //  std::vector<func> func_vec = { &f1, &f2, &f3};
    std::vector<func> func_vec = { &f1, &f2};
    std::vector<std::thread> thread_vec;
	
    for (func& f : func_vec)
     thread_vec.emplace_back(std::thread(f));
    for (std::thread& t : thread_vec)
      t.join();
    return 0;
  }

  auto beringeiConfig =
      std::make_shared<gorilla::BeringeiConfigurationAdapter>(true);
  auto beringeiClient =
      std::make_shared<gorilla::BeringeiClient>(beringeiConfig, 10, 2, true);

  if (argc < 3) {
    gflags::ShowUsageWithFlagsRestrict(argv[0], "BeringeiPut.cpp");
    return 1;
  }

  int shardCount = beringeiClient->getNumShardsFromWriteClient();
  LOG(INFO) << "Config knows about these write services: ";
  for (const auto& wservice : beringeiConfig->getWriteServices()) {
    LOG(INFO) << "  " << wservice;
  }
  LOG(INFO) << "Beringei has " << shardCount << " shards";

  if (shardCount == 0) {
    LOG(FATAL) << "Shard count can't be zero, though.";
  }

  std::vector<gorilla::DataPoint> dps;
  dps.emplace_back();
  dps.back().key.key = std::string(argv[1]);
  dps.back().key.shardId =
      std::hash<std::string>()(dps.back().key.key) % shardCount;
  dps.back().value.unixTime =
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  dps.back().value.value = folly::to<double>(argv[2]);

  LOG(INFO) << "Num pts: " << dps.size();
  //LOG(INFO) << "BeringeiPut argc: " << argc;

  if (argc > 3) {
    dps.back().value.unixTime = folly::to<int64_t>(argv[3]);
  }

  auto pushedPoints = beringeiClient->putDataPoints(dps);

  if (!pushedPoints) {
    LOG(ERROR) << "Failed to perform the put!";
  }

  beringeiClient->flushQueue();
  return 0;
}

