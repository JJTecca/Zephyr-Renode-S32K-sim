import os
import subprocess

renode_start_menu = r"C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Renode"

if not os.path.isdir(renode_start_menu):
    raise FileNotFoundError(f"Folder not found: {renode_start_menu}")

for root, dirs, files in os.walk(renode_start_menu):
    for file_name in files:
        full_path = os.path.join(root, file_name)
        if "Renode.lnk" in full_path:
            subprocess.Popen(
                ["cmd", "/c", "start", "", full_path]
            )
            break
        else:
            continue