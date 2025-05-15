import xml.etree.ElementTree as ET
import json
import re
import os
import sys
import shutil

def add_rte_path(root, component_path):
    """
    获取指定component下package元素的version属性
    
    Args:
        root: XML根节点
        component_path: 目标component的XPath路径
    """
    try:        
        # 查找指定的component
        component = root.find(component_path)
        if component is None:
            print("Component not found")
            return None
            
        # 获取component下的package元素
        package = component.find('.//package')
        if package is None:
            print("No package element found in this component")
            return None
            
        # 获取version属性
        version = package.get('version')
        if version:
            arm_compiler_path = os.path.join(package_dir, "Keil", "ARM_Compiler", version, "Include")
            return arm_compiler_path
        else:
            print("Package has no version attribute")
            return None
            
    except Exception as e:
        print(f"Error occurred: {e}")
        return None

# 获取UV4的路径

command = "UV4"
uv4_path = shutil.which(command)
uv4_dir = os.path.dirname(uv4_path)

# 获取Keil根目录

keil_dir = os.path.dirname(uv4_dir)

# 获取Keil安装目录下的Pack目录

package_dir = os.path.join(keil_dir, "ARM", "Packs")

# 定义文件路径

settings_json_path = '.vscode/settings.json'
mdk_prj_path = 'project.uvprojx'

# 检查是否存在.uvprojx文件
if os.path.isfile(mdk_prj_path) != True:
    print("'project.uvprojx' does not exist.")
    sys.exit(0)

# 解析XML文件
tree = ET.ElementTree(file=mdk_prj_path)
root = tree.getroot()
IncludePath = root.find('.//TargetArmAds/Cads//IncludePath').text
Define = root.find('.//TargetArmAds/Cads//Define').text

# print(IncludePath)
# print(Define)

# 获取 Defines 列表
if Define is not None:
    define_list = re.findall('[^, ]+', Define) # 这样提取出来后是一个list
else:
    define_list = []

# 获取 Include Path
if IncludePath is not None:
    raw_list = IncludePath.split(';')  # 使用分号分割字符串
else:
    raw_list = []

# 添加 RTE 路径(Event Recorder)
ARM_Compiler_path = add_rte_path(root, ".//components/component[@Cbundle='ARM Compiler']")
if ARM_Compiler_path is not None:
    raw_list.append(ARM_Compiler_path)

# print(raw_list)

compare_flag = False
path_list = []

# 排除重复的路径
for path in raw_list:
    for path_temp in path_list:
        if path == path_temp:
            compare_flag = True
    if compare_flag == True:
        compare_flag = False
    else:
        path_list.append(path)

# print(path_list)

# 检查是否存在settings.json文件
if os.path.isfile(settings_json_path) != True:
    print("'settings.json' does not exist.")
    sys.exit(0)

with open(settings_json_path, "r", encoding='utf-8') as json_file:
    json_raw = json.load(json_file) # 将Json文件读取为Python对象

# 删除原来的 includePath 和 defines
if('C_Cpp.default.includePath' in json_raw):
    del json_raw['C_Cpp.default.includePath']
if('C_Cpp.default.defines') in json_raw:
    del json_raw['C_Cpp.default.defines']

# 重新添加 includePath 和 defines
json_raw['C_Cpp.default.includePath'] = path_list
json_raw['C_Cpp.default.defines'] = define_list

# 将修改保存到文件
with open(settings_json_path, "w", encoding='utf-8') as json_file:
    json.dump(json_raw, json_file, indent=4)

print("Done.")
