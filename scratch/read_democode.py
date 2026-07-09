import os

filepath = r"d:\Software\antigravity\water_sensor_mdc04\doc\offical\MDC04_IIC_Samplecode-51-MY202207\DRIVE\src\MDC04_IIC.c"

try:
    with open(filepath, 'r', encoding='gbk') as file:
        lines = file.readlines()
except Exception:
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as file:
        lines = file.readlines()

print("Lines of ReadCap:")
for i in range(136, 180):
    if i < len(lines):
        print(f"{i+1}: {lines[i].strip()}")
