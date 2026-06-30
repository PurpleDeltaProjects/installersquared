applist = ["happy", "cookie", "watermelon", "chrome-windows", "firefox-windows"]

with open("InstallerSquared.exe", "a") as f:
    f.write("applist---start")
    f.write(",".join(applist))
    f.write("applist---end")