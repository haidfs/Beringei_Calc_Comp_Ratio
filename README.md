# Beringei_Calc_Comp_Ratio
Beringei存在二级压缩，第一级压缩是在内存中使用Gorilla为原型的压缩方法，二级压缩是用通用zlib压缩后持久化到磁盘。
该文件夹下代码用于计算Beringei的一级压缩比：

1.按照Github上所说的编译好Beringei，在Ubuntu(Beringei以BSD协议开源，官方在Ubuntu16.04做测试可运行，我的环境是18.04也可以)虚拟机上，需要存在/opt/run/2/wlan_kpi_2_43fields.csv（入库文件）

2.相对build路径， cd ../beringei/lib/ 使用文件夹中的TimeSeriesStream.cpp替换原本工程下的TimeSeriesStream.cpp。

3.相对build路径，cd ../beringei/tools/ 使用文件夹中的BeringeiPut.cpp替换原本工程下的BeringeiPut.cpp。工程中的BeringeiPut_2thread.cpp与BeringeiPut.cpp功能一样，只是采用了多线程实现。

4.重新编译Beringei。

5.将beringei_start.sh、calc_compress.sh、overall_count.py放到build路径下

6.在build路径下执行nohup sh beringei_start.sh

7.新开一个终端，在build路径下执行time ./beringei/tools/beringei_put \
        -beringei_configuration_path /tmp/beringei.json 12（这里的12代表想压缩的第一步的文件的第几列）
        
8.等待7执行完成， 在build路径下执行sh calc_compress.sh即可获得对应的（入库文件的某一列的）压缩比
