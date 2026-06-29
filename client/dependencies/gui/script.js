let appinfo, applist, appcontainer, mode, success, complete

let appcategories = []

let appselection = document.getElementById("appselection");
let downloadscreen = document.getElementById("downloadscreen");
let messagescreen = document.getElementById("messagescreen");

let downloadbox = document.getElementById("downloadbox");

getMode().then(function(modedata) {
    mode = modedata
    if (mode == "dynamic") {
        appselection.classList.toggle("hidden");
    } else if (mode == "static") {
        downloadscreen.classList.toggle("hidden");
    } else {
        messagescreen.classList.toggle("hidden");
        document.getElementById("messagebox").textContent = mode
        setTimeout(closeApp, 5000);
    }

    getAppInfo()
    .then(function(data) {
        if (mode == "dynamic") {

            appinfo = data

            applist = Object.keys(appinfo);

            appcontainer = document.getElementById("app-container");

            applist.sort((a,b) =>  appinfo[b]["popularity"] - appinfo[a]["popularity"])

            function fillappcontainer() {

                applist.forEach(appid => {

                    let appname = appinfo[appid]["name"];

                    let appdesc = appinfo[appid]["description"];
                
                    let appwebsite = appinfo[appid]["app-website"];

                    let appcategory = appinfo[appid]["category"].toLowerCase().replace(" ", "-")

                    let string = `\n\n                    <div class="app-box curved-border" id="app-box-${appid}">

                                    <div class="app-icon-container">
                                        <img src="https://{{URL}}/data/images/appicons/${appcategory}-icon.webp" class="app-icon">
                                    </div>

                                    <label for="checkbox-${appid}" class="app-info-container ubuntu-regular"></label>

                                    <div class="app-title larger-text">
                                        <input type="checkbox" id="checkbox-${appid}" name="apps" value="${appid}" class="larger-element">
                                        <a target="_blank" href="${appwebsite}">${appname}</a>
                                    </div>

                                    <div class="app-description">
                                        ${appdesc}
                                    </div>
                                    
                                </div>`;

                    appcontainer.insertAdjacentHTML("beforeend", string);

                })
            }
            fillappcontainer()



            //fill the filter by box on the site
            applist.forEach(app => {
                if (!appcategories.includes(appinfo[app]["category"])) {
                    appcategories.push(appinfo[app]["category"])
                }
            })

            let filteroptions = document.getElementById("filter-options");

            appcategories.sort()

            appcategories.forEach(category => {
                let string = `\n                   <input type="radio" name="filter" value="${category}" id="filter-${category}">
                                <label for="filter-${category}"> ${category} </label> <br>`

                filteroptions.insertAdjacentHTML("beforeend", string)
            })



            //make the filter buttons and search bar work
            let searchbar = document.getElementById("searchbar");

            function appfilter() {

                applist.forEach(app => {
                    document.getElementById(`app-box-${app}`).style.display = "block";
                })

                let filter = document.querySelector('[name="filter"]:checked').value;

                let search = searchbar.value.trim().toLowerCase();

                let applisttemp = Array.from(applist) //a list of apps to be removed

                applist.forEach(app => {

                    if (filter == "All" || appinfo[app]["category"] == filter) {
                        let indexofapp = applisttemp.indexOf(app);
                        applisttemp.splice(indexofapp, 1);

                        if (search && !appinfo[app]["name"].toLowerCase().includes(search)) {
                            applisttemp.push(app);
                        }
                    }

                })

                applisttemp.forEach(app => {
                    document.getElementById(`app-box-${app}`).style.display = "none";
                    })


                appcontainer.scrollTop = 0;

            }

            filteroptions.addEventListener("change", appfilter);
            searchbar.addEventListener("input", appfilter);



            //make sort buttons work
            let sortoptions = document.getElementById("sort-options");

            sortoptions.addEventListener("change", () => {

                let checkedboxids = [];
                
                document.querySelectorAll('[name="apps"]:checked').forEach(element => {
                    checkedboxids.push(element.id)
                });

                let sortvalue = document.querySelector('[name="sort"]:checked').value;

                appcontainer.innerHTML = "";

                if (sortvalue == "PopularityH-L") {applist.sort((a,b) =>  appinfo[b]["popularity"] - appinfo[a]["popularity"])}
                else if (sortvalue == "PopularityL-H") {applist.sort((a,b) =>  appinfo[a]["popularity"] - appinfo[b]["popularity"])}
                else if (sortvalue == "AlphabeticalA-Z") {applist.sort()}
                else if (sortvalue == "AlphabeticalZ-A") {applist.sort().reverse()}

                fillappcontainer();

                appfilter();

                checkedboxids.forEach(id => {
                    let checkbox = document.getElementById(id)
                    let label = checkbox.closest(".app-box").querySelector("label")
                    checkbox.checked = true;
                    label.classList.add("no-transition");
                    label.classList.add("green-background");
                    setTimeout(() => {label.classList.remove("no-transition")}, 0)
                })

                appcontainer.scrollTop = 0;

            })



            //make download button not work if no apps are selected and app description green if checked
            let downloadbutton = document.getElementById("download-button")

            appcontainer.addEventListener("change", e => {

                let labelclasses = e.target.closest(".app-box").querySelector("label").classList;

                (e.target.checked) ? labelclasses.add("green-background") : labelclasses.remove("green-background")

                let checkboxes = Array.from(document.querySelectorAll('[name="apps"]'));
                
                let anychecked = checkboxes.some(cb => cb.checked)

                if (anychecked) {
                    downloadbutton.disabled = false
                    downloadbutton.style.cursor = "pointer"
                } else {
                    downloadbutton.disabled = true
                    downloadbutton.style.cursor = "auto"
                }
            })



            //make scrolling activate the animations
            let mouseX = window.innerWidth / 2;
            let mouseY = window.innerHeight / 2;

            function setappboxhover() {
                let elementhovered = document.elementFromPoint(mouseX, mouseY)
                if (!elementhovered) {return}
                
                if (elementhovered.closest(".app-box")) {
                    elementhovered.closest(".app-box").classList.add("hover")
                } else {
                    document.querySelectorAll(".app-box.hover").forEach(element => {element.classList.remove("hover")})
                }
            }

            document.addEventListener("mousemove", e => {
                mouseX = e.clientX;
                mouseY = e.clientY;
                requestAnimationFrame(setappboxhover);
            })

            appcontainer.addEventListener("scroll", function() {
                requestAnimationFrame(setappboxhover);
            })


            //get the form information
            document.getElementById("appform").addEventListener("submit", async e => {

                e.preventDefault();

                formdata = new FormData(e.target)

                applist = formdata.getAll("apps")

                //change to download screen
                appselection.classList.toggle("hidden");
                downloadscreen.classList.toggle("hidden");

                let progress = "(0/" + (applist.length).toString() + ")"

                //download every app in the gathered applist
                for (let i=0; i<applist.length; i++) {

                    let app = applist[i];
                    let appname = appinfo[app]["name"];
                    
                    let text = "Installing: " + appname + progress;
                    downloadbox.textContent = text;

                    progress = " (" + (i+1).toString() + "/" + (applist.length).toString() + ")";
                    await downloadApp(app)

                    complete = false
                    while(!complete) {}

                    if (success) {
                        text = appname + " Installed Successfully!" + progress;
                    } else {
                        text = appname + " Failed to Install." + progress;
                    }

                    downloadbox.textContent = text;

                }

                setTimeout(closeApp, 10000);

            })

        }
    })





})
