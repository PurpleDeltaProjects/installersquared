import subprocess

from urllib.parse import quote

import os

command = "g++ -o InstallerSquared windowsinstaller.cpp dependencies/json11.cpp dependencies/icon.o -static -static-libgcc -static-libstdc++ -municode -lwinhttp -ladvapi32 -lole32 -lshell32 -lshlwapi -luser32 -lversion"

url = "installersquared.netlify.app"

#open all files

with open("./dependencies/gui/index.html", "r") as f:
    html = f.read()

with open("./dependencies/gui/script.js", "r") as f:
    js = f.read()

with open("./dependencies/gui/style.css", "r") as f:
    css = f.read()

with open("./windowsinstallertemplate.cpp", "r") as f:
    cpp = f.read()


#add the javascript and css to the html
html = html.replace("<style>", "<style>"+css).replace("<script>", "<script>"+js).replace("{{URL}}", url)

#add the html to the c++
cpp = cpp.replace("{{HTML}}", quote(html)).replace("{{URL}}", url)

#make the combined file
with open("./windowsinstaller.cpp", "w") as f:
    f.write(cpp)

#compile it
sub = subprocess.run(command, capture_output=True, text=True)

if sub.stdout: print(sub.stdout)

if sub.stderr: print(sub.stderr)

#delete the temp source code
os.remove("./windowsinstaller.cpp")