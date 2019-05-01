import sys


def count_compressed_bits(path):
    # 打开Beringei运行时生成的文件，统计ts和value压缩后的数据大小（bit为单位）
    bits = 0
    # path = "./compress_value.txt"
    file = open(path)
    line = file.readline()
    count_dict = {}
    while line:
        a = line.split()
        item = int(a[10])
        if item in count_dict.keys():
            count_dict[item] += 1
        else:
            count_dict[item] = 1
        line = file.readline()
    file.close()
    count_list = sorted(count_dict.items(), key=lambda x: x[1], reverse=True)
    num_of_lines = len(["" for line in open(path, "r")])
    print("\n--------------------------------------------------------")
    print("The Number Of Corresponding File Lines is:", num_of_lines)
    print(count_list)
    # 统计压缩后的不同bits大小的占比数，便于后续的结果分析
    for key in count_dict.keys():
        count_dict[key] = float('%.4f' % (count_dict[key] / num_of_lines))
    count_list = sorted(count_dict.items(), key=lambda x: x[1], reverse=True)
    count_dict = dict(count_list)
    print(count_dict)
    # 占比*压缩后bit大小的和，即为ts/value压缩后的整体的数据大小(bits)
    for key in count_dict.keys():
        bits += key * count_dict[key]
    print("The compressed bits:", float('%.4f' % bits))
    return float('%.4f' % bits)


a = count_compressed_bits("./compress_ts.txt")
b = count_compressed_bits("./compress_value.txt")
print("\n--------------------------------------------------------")
NumOfLines = len(["" for line in open("./compress_ts.txt", "r")])
size = int(NumOfLines * 16 / 1024)
print("Input data size: {} KB".format(size))
ratio = float('%.4f' % (128 / (a + b)))
print("The overall compress ratio is:{}\n".format(ratio))
f = open("graph.xlsx", 'a')
f.writelines([str(size), ' ', str(ratio), '\n'])
f.close()