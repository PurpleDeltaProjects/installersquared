applist = ["happy", "cookie", "watermelon", "apple-music-windows", "spotify-windows"]

with open("a.exe", "a") as f:
    f.write("applist---start")
    f.write(",".join(applist))
    f.write("applist---end")